#ifndef DYNAMIC_ARRAY_HPP
#define DYNAMIC_ARRAY_HPP

#include <__utility/exception_guard.h>
#include <__utility/is_pointer_in_range.h>
#include <cassert>
#include <concepts>
#include <cstddef>
#include <format>
#include <initializer_list>
#include <iterator>
#include <limits>
#include <memory>
#include <new>
#include <stdexcept>
#include <string_view>

template <typename T>
concept Allocator = requires(T t, size_t count, std::align_val_t alignment, void* pointer) {
    { t.allocate(count, alignment) } -> std::convertible_to<void*>;
    { t.deallocate(pointer, alignment) } noexcept;
};

struct DefaultAllocator {
    void* allocate(size_t count, std::align_val_t alignment) const {
        return ::operator new(count, alignment);
    }

    void deallocate(void* pointer, std::align_val_t alignment) const noexcept {
        ::operator delete(pointer, alignment, std::nothrow);
    }
};

/**
 * @brief A dynamic array (vector-like) implementation.
 * @tparam ElementT The type of elements stored in the array.
 * @tparam AllocatorT The allocator type to use for memory management. Defaults to `DefaultAllocator`.
 */
template <typename ElementT, Allocator AllocatorT = DefaultAllocator>
class DArray {
private:
    ElementT* _data = nullptr;
    size_t _size = 0;
    size_t _capacity = 0;
    AllocatorT _allocator;

public:
    /**
     * @brief Default constructor.
     * Constructs an empty DArray with no allocated memory.
     */
    DArray() noexcept {}

    /**
     * @brief Size constructor.
     * Constructs a DArray with `n` default-constructed elements.
     * @param n The number of elements to construct.
     * @throws std::bad_alloc If memory allocation fails.
     * @throws Any exception thrown by the ElementT default constructor.
     */
    explicit DArray(size_t n) {
        if (n > 0) {
            auto guard = std::__make_exception_guard(DestroyArray(*this));
            allocate(n);
            constructAtEnd(n);
            guard.__complete();
        }
    }

    /**
     * @brief Fill constructor.
     * Constructs a DArray with `n` copies of `x`.
     * @param x The value to fill the array with.
     * @param n The number of elements to construct.
     * @throws std::bad_alloc If memory allocation fails.
     * @throws Any exception thrown by the ElementT copy constructor.
     */
    DArray(const ElementT& x, size_t n) {
        if (n > 0) {
            auto guard = std::__make_exception_guard(DestroyArray(*this));
            allocate(n);
            constructAtEnd(x, n);
            guard.__complete();
        }
    }

    /**
     * @brief Range constructor.
     * Constructs a DArray with elements from the range [first, last).
     * @param first A pointer to the beginning of the range.
     * @param last A pointer to the end of the range (one past the last element).
     * @throws std::bad_alloc If memory allocation fails.
     * @throws Any exception thrown by the ElementT copy constructor.
     */
    DArray(const ElementT* first, const ElementT* last) {
        size_t n = static_cast<size_t>(last - first);
        if (n > 0) {
            auto guard = std::__make_exception_guard(DestroyArray(*this));
            allocate(n);
            constructAtEnd(first, last, n);
            guard.__complete();
        }
    }

    /**
     * @brief Initializer list constructor.
     * Constructs a DArray with elements from an initializer list.
     * @param elements An initializer list of elements.
     * @throws std::bad_alloc If memory allocation fails.
     * @throws Any exception thrown by the ElementT copy constructor.
     */
    DArray(std::initializer_list<ElementT> elements) {
        if (elements.size() > 0) {
            auto guard = std::__make_exception_guard(DestroyArray(*this));
            allocate(elements.size());
            constructAtEnd(elements.begin(), elements.end(), elements.size());
            guard.__complete();
        }
    }

    /**
     * @brief Copy constructor.
     * Constructs a DArray as a copy of another DArray.
     * @param other The DArray to copy.
     * @throws std::bad_alloc If memory allocation fails.
     * @throws Any exception thrown by the ElementT copy constructor.
     */
    DArray(const DArray& other) {
        if (other.size() > 0) {
            auto guard = std::__make_exception_guard(DestroyArray(*this));
            allocate(other.size());
            constructAtEnd(other.begin(), other.end(), other.size());
            guard.__complete();
        }
    }

    /**
     * @brief Move constructor.
     * Constructs a DArray by moving the contents of another DArray.
     * @param other The DArray to move from. After the move `other` is empty.
     */
    DArray(DArray&& other) noexcept {
        swap(other);
    }

    /**
     * @brief Destructor.
     * Destroys all elements and deallocates the memory.
     */
    ~DArray() noexcept {
        destroy();
    }

    /**
     * @brief Assigns `n` copies of `element` to the DArray.
     * Clears existing elements and constructs new ones.
     * @param element The value to assign.
     * @param n The number of times to copy `element`.
     * @return A reference to the DArray.
     * @throws std::bad_alloc If memory allocation fails.
     * @throws Any exception thrown by the ElementT copy constructor.
     */
    DArray& assign(const ElementT& element, size_t n) {
        copy(element, n);
        return *this;
    }

    /**
     * @brief Assigns elements from the range [first, last) to the DArray.
     * Clears existing elements and constructs new ones.
     * @param first A pointer to the beginning of the range.
     * @param last A pointer to the end of the range.
     * @return A reference to the DArray.
     * @throws std::bad_alloc If memory allocation fails.
     * @throws Any exception thrown by the ElementT copy constructor.
     */
    DArray& assign(const ElementT* first, const ElementT* last) {
        copy(first, last, last - first);
        return *this;
    }

    /**
     * @brief Assignment operator from an initializer list.
     * Assigns elements from an initializer list to the DArray.
     * @param elements An initializer list of elements.
     * @return A reference to the DArray.
     * @throws std::bad_alloc If memory allocation fails.
     * @throws Any exception thrown by the ElementT copy constructor.
     */
    DArray& operator=(std::initializer_list<ElementT> elements) {
        copy(elements.begin(), elements.end(), elements.size());
        return *this;
    }

    /**
     * @brief Copy assignment operator.
     * Assigns the contents of another DArray to this DArray.
     * @param other The DArray to copy from.
     * @return A reference to the DArray.
     * @throws std::bad_alloc If memory allocation fails.
     * @throws Any exception thrown by the ElementT copy constructor.
     */
    DArray& operator=(const DArray& other) {
        if (this != std::addressof(other)) {
            copy(other.begin(), other.end(), other.size());
        }
        return *this;
    }

    /**
     * @brief Move assignment operator.
     * Moves the contents of another DArray to this DArray.
     * @param other The DArray to move from. After the move `other` is empty.
     * @return A reference to the DArray.
     */
    DArray& operator=(DArray&& other) noexcept {
        if (this != std::addressof(other)) {
            destroy();
            swap(other);
        }
        return *this;
    }

    /**
     * @brief Accesses the element at the specified index (non-const version).
     * No bounds checking is performed.
     * @param index The index of the element to access.
     * @return A reference to the element at `index`.
     */
    ElementT& operator[](size_t index) noexcept {
        return _data[index];
    }

    /**
     * @brief Accesses the element at the specified index (const version).
     * No bounds checking is performed.
     * @param index The index of the element to access.
     * @return A const reference to the element at `index`.
     */
    const ElementT& operator[](size_t index) const noexcept {
        return _data[index];
    }

    /**
     * @brief Accesses the element at the specified index with bounds checking (non-const version).
     * @param index The index of the element to access.
     * @return A reference to the element at `index`.
     * @throws std::out_of_range If `index` is out of bounds.
     */
    ElementT& at(size_t index) {
        if (index < _size) {
            return _data[index];
        }
        throw std::out_of_range("Index is out of range");
    }

    /**
     * @brief Accesses the element at the specified index with bounds checking (const version).
     * @param index The index of the element to access.
     * @return A const reference to the element at `index`.
     * @throws std::out_of_range If `index` is out of bounds.
     */
    const ElementT& at(size_t index) const {
        if (index < _size) {
            return _data[index];
        }
        throw std::out_of_range("Index is out of range");
    }

    /**
     * @brief Accesses the first element (non-const version).
     * Undefined behavior if the array is empty.
     * @return A reference to the first element.
     */
    ElementT& front() noexcept {
        return _data[0];
    }

    /**
     * @brief Accesses the first element (const version).
     * Undefined behavior if the array is empty.
     * @return A const reference to the first element.
     */
    const ElementT& front() const noexcept {
        return _data[0];
    }

    /**
     * @brief Accesses the last element (non-const version).
     * Undefined behavior if the array is empty.
     * @return A reference to the last element.
     */
    ElementT& back() noexcept {
        return _data[_size - 1];
    }

    /**
     * @brief Accesses the last element (const version).
     * Undefined behavior if the array is empty.
     * @return A const reference to the last element.
     */
    const ElementT& back() const noexcept {
        return _data[_size - 1];
    }

    /**
     * @brief Returns a pointer to the underlying array (non-const version).
     * @return A pointer to the first element of the array. Returns `nullptr` if the array is empty.
     */
    ElementT* data() noexcept {
        return _data;
    }

    /**
     * @brief Returns a pointer to the underlying array (const version).
     * @return A const pointer to the first element of the array. Returns `nullptr` if the array is empty.
     */
    const ElementT* data() const noexcept {
        return _data;
    }

    /**
     * @brief Returns an iterator to the beginning of the array (non-const version).
     * @return An `ElementT*` pointing to the first element.
     */
    ElementT* begin() noexcept {
        return _data;
    }

    /**
     * @brief Returns an iterator to the beginning of the array (const version).
     * @return A `const ElementT*` pointing to the first element.
     */
    const ElementT* begin() const noexcept {
        return _data;
    }

    /**
     * @brief Returns an iterator to the end of the array (non-const version).
     * @return An `ElementT*` pointing one past the last element.
     */
    ElementT* end() noexcept {
        return _data + _size;
    }

    /**
     * @brief Returns an iterator to the end of the array (const version).
     * @return A `const ElementT*` pointing one past the last element.
     */
    const ElementT* end() const noexcept {
        return _data + _size;
    }

    /**
     * @brief Returns a reverse iterator to the reverse beginning of the array (non-const version).
     * @return A `std::reverse_iterator<ElementT*>` pointing to the last element.
     */
    std::reverse_iterator<ElementT*> rbegin() noexcept {
        return std::reverse_iterator<ElementT*>(end());
    }

    /**
     * @brief Returns a reverse iterator to the reverse beginning of the array (const version).
     * @return A `std::reverse_iterator<const ElementT*>` pointing to the last element.
     */
    std::reverse_iterator<const ElementT*> rbegin() const noexcept {
        return std::reverse_iterator<const ElementT*>(end());
    }

    /**
     * @brief Returns a reverse iterator to the reverse end of the array (non-const version).
     * @return A `std::reverse_iterator<ElementT*>` pointing one before the first element.
     */
    std::reverse_iterator<ElementT*> rend() noexcept {
        return std::reverse_iterator<ElementT*>(begin());
    }

    /**
     * @brief Returns a reverse iterator to the reverse end of the array (const version).
     * @return A `std::reverse_iterator<const ElementT*>` pointing one before the first element.
     */
    std::reverse_iterator<const ElementT*> rend() const noexcept {
        return std::reverse_iterator<const ElementT*>(begin());
    }

    /**
     * @brief Checks if the DArray is empty.
     * @return `true` if the DArray contains no elements, `false` otherwise.
     */
    bool empty() const noexcept {
        return _size == 0;
    }

    /**
     * @brief Returns the number of elements in the DArray.
     * @return The current number of elements.
     */
    size_t size() const noexcept {
        return _size;
    }

    /**
     * @brief Returns the maximum possible number of elements the DArray can hold.
     * This is limited by the maximum value of `size_t` and the size of `ElementT`.
     * @return The maximum size.
     */
    size_t maxSize() const noexcept {
        return std::numeric_limits<size_t>::max() / sizeof(ElementT);
    }

    /**
     * @brief Returns the current allocated capacity of the DArray.
     * @return The current capacity.
     */
    size_t capacity() const noexcept {
        return _capacity;
    }

    /**
     * @brief Reserves memory for at least `n` elements.
     * If `n` is greater than the current capacity, a reallocation occurs,
     * and existing elements are moved to the new memory block.
     * If `n` is less than or equal to the current capacity, no action is taken.
     * @param n The new minimum capacity.
     * @throws std::length_error If `n` is greater than `maxSize()`.
     * @throws std::bad_alloc If memory allocation fails during reallocation.
     * @throws Any exception thrown by the ElementT move constructor.
     */
    void reserve(size_t n) {
        if (n > _capacity) {
            if (n > maxSize()) {
                throw std::length_error("Required capacity is too large");
            } else {
                AllocateTransaction transaction(*this);
                transaction.allocate(n);
                moveRange(begin(), end(), transaction.data);
                std::swap(_data, transaction.data);
                _capacity = n;
            }
        }
    }

    /**
     * @brief Reduces the capacity of the DArray to fit its current size.
     * If the current capacity is greater than the size, a reallocation occurs
     * to a new memory block exactly large enough to hold the current elements.
     * If an allocation fails, the operation is silently ignored (no throw).
     */
    void shrinkToFit() noexcept {
        if (_capacity > _size) {
            if (_size > 0) {
                try {
                    AllocateTransaction transaction(*this);
                    transaction.allocate(_size);
                    moveRange(begin(), end(), transaction.data);
                    std::swap(_data, transaction.data);
                    _capacity = _size;
                } catch (...) {
                    // Swallow exception
                }
            } else {
                deallocate();
            }
        }
    }

    /**
     * @brief Clears the contents of the DArray.
     * Destroys all elements, but the allocated capacity remains unchanged.
     * The size becomes 0.
     */
    void clear() noexcept {
        if (_data != nullptr) {
            destructAtEnd(_size);
        }
    }

    /**
     * @brief Destroys all elements and deallocates the memory.
     * The DArray becomes empty with a capacity of 0.
     */
    void destroy() noexcept {
        if (_data != nullptr) {
            destructAtEnd(_size);
            deallocate();
        }
    }

    /**
     * @brief Inserts a copy of `element` at the specified `position`.
     * Elements from `position` onwards are shifted to the right.
     * @param position An iterator pointing to the position where the element should be inserted.
     * @param element The element to insert.
     * @return An iterator pointing to the newly inserted element.
     * @throws std::bad_alloc If memory reallocation fails.
     * @throws Any exception thrown by the ElementT copy/move constructor.
     */
    ElementT* insert(ElementT* position, const ElementT& element) {
        size_t index = position - begin();
        if (_size < _capacity) {
            if (position == end()) {
                constructOneAtEnd(element);
            } else {
                shiftAndConstructOneAt(position, element);
            }
        } else {
            growAndConstructOneAt(position, element);
        }
        return begin() + index;
    }

    /**
     * @brief Inserts `element` (moved) at the specified `position`.
     * Elements from `position` onwards are shifted to the right.
     * @param position An iterator pointing to the position where the element should be inserted.
     * @param element The element to insert (rvalue reference).
     * @return An iterator pointing to the newly inserted element.
     * @throws std::bad_alloc If memory reallocation fails.
     * @throws Any exception thrown by the ElementT move constructor.
     */
    ElementT* insert(ElementT* position, ElementT&& element) {
        size_t index = position - begin();
        if (_size < _capacity) {
            if (position == end()) {
                constructOneAtEnd(std::move(element));
            } else {
                shiftAndConstructOneAt(position, std::move(element));
            }
        } else {
            growAndConstructOneAt(position, std::move(element));
        }
        return begin() + index;
    }

    /**
     * @brief Inserts `n` copies of `element` at the specified `position`.
     * Elements from `position` onwards are shifted to the right.
     * @param position An iterator pointing to the position where elements should be inserted.
     * @param element The element to insert.
     * @param n The number of copies to insert.
     * @return An iterator pointing to the first newly inserted element.
     * @throws std::bad_alloc If memory reallocation fails.
     * @throws Any exception thrown by the ElementT copy/move constructor.
     */
    ElementT* insert(ElementT* position, const ElementT& element, size_t n) {
        size_t index = position - begin();
        if (n > 0) {
            if (_size + n <= _capacity) {
                if (position == end()) {
                    constructAtEnd(element, n);
                } else {
                    shiftAndConstructAt(position, element, n);
                }
            } else {
                growAndConstructAt(position, element, n);
            }
        }
        return begin() + index;
    }

    /**
     * @brief Inserts elements from the range [first, last) at the specified `position`.
     * Elements from `position` onwards are shifted to the right.
     * @param position An iterator pointing to the position where elements should be inserted.
     * @param first A pointer to the beginning of the range.
     * @param last A pointer to the end of the range.
     * @return An iterator pointing to the first newly inserted element.
     * @throws std::bad_alloc If memory reallocation fails.
     * @throws Any exception thrown by the ElementT copy/move constructor.
     */
    ElementT* insert(ElementT* position, const ElementT* first, const ElementT* last) {
        size_t index = position - begin();
        size_t n = last - first;
        if (n > 0) {
            if (_size + n <= _capacity) {
                if (position == end()) {
                    constructAtEnd(first, last, n);
                } else {
                    shiftAndConstructAt(position, first, last, n);
                }
            } else {
                growAndConstructAt(position, first, last, n);
            }
        }
        return begin() + index;
    }

    /**
     * @brief Inserts elements from an initializer list at the specified `position`.
     * Elements from `position` onwards are shifted to the right.
     * @param position An iterator pointing to the position where elements should be inserted.
     * @param elements An initializer list of elements.
     * @return An iterator pointing to the first newly inserted element.
     * @throws std::bad_alloc If memory reallocation fails.
     * @throws Any exception thrown by the ElementT copy/move constructor.
     */
    ElementT* insert(ElementT* position, std::initializer_list<ElementT> elements) {
        size_t index = position - begin();
        if (elements.size() > 0) {
            if (_size + elements.size() <= _capacity) {
                if (position == end()) {
                    constructAtEnd(elements.begin(), elements.end(), elements.size());
                } else {
                    shiftAndConstructAt(position, elements.begin(), elements.end(), elements.size());
                }
            } else {
                growAndConstructAt(position, elements.begin(), elements.end(), elements.size());
            }
        }
        return begin() + index;
    }

    /**
     * @brief Erases the element at the specified `position`.
     * Elements after the erased element are shifted to the left.
     * @param position An iterator pointing to the element to erase.
     * @return An iterator pointing to the element immediately following the erased element, or `end()` if the last element was erased.
     */
    ElementT* erase(ElementT* position) noexcept {
        size_t index = position - begin();
        if (position == end() - 1) {
            destructAtEnd(1);
        } else {
            destructAt(position, 1);
        }
        return begin() + index;
    }

    /**
     * @brief Erases elements in the range [first, last).
     * Elements after the erased range are shifted to the left.
     * @param first An iterator pointing to the first element to erase.
     * @param last An iterator pointing one past the last element to erase.
     * @return An iterator pointing to the element immediately following the erased range, or `end()` if the last elements were erased.
     */
    ElementT* erase(ElementT* first, ElementT* last) {
        size_t index = first - begin();
        if (last == end()) {
            destructAtEnd(last - first);
        } else {
            destructAt(first, last - first);
        }
        return begin() + index;
    }

    /**
     * @brief Appends a copy of `element` to the end of the DArray.
     * If the capacity is insufficient, a reallocation occurs.
     * @param element The element to append.
     * @throws std::bad_alloc If memory reallocation fails.
     * @throws Any exception thrown by the ElementT copy/move constructor.
     */
    void push(const ElementT& element) {
        if (_size < _capacity) {
            constructOneAtEnd(element);
        } else {
            growAndConstructOneAt(end(), element);
        }
    }

    /**
     * @brief Appends `element` (moved) to the end of the DArray.
     * If the capacity is insufficient, a reallocation occurs.
     * @param element The element to append (rvalue reference).
     * @throws std::bad_alloc If memory reallocation fails.
     * @throws Any exception thrown by the ElementT move constructor.
     */
    void push(ElementT&& element) {
        if (_size < _capacity) {
            constructOneAtEnd(std::move(element));
        } else {
            growAndConstructOneAt(end(), std::move(element));
        }
    }

    /**
     * @brief Removes the last element from the DArray.
     * Undefined behavior if the array is empty.
     */
    void pop() {
        destructAtEnd(1);
    }

    /**
     * @brief Constructs an element in-place at the end of the DArray.
     * Arguments are perfectly forwarded to the element's constructor.
     * If the capacity is insufficient, a reallocation occurs.
     * @tparam Args Types of arguments for the element's constructor.
     * @param args Arguments to forward to the element's constructor.
     * @return A reference to the newly constructed element.
     * @throws std::bad_alloc If memory reallocation fails.
     * @throws Any exception thrown by the ElementT constructor.
     */
    template <typename... Args>
    ElementT& emplaceAtEnd(Args&&... args) {
        if (_size < _capacity) {
            constructOneAtEnd(std::forward<Args>(args)...);
        } else {
            growAndConstructOneAt(end(), std::forward<Args>(args)...);
        }
        return back();
    }

    /**
     * @brief Constructs an element in-place at the specified `position`.
     * Arguments are perfectly forwarded to the element's constructor.
     * Elements from `position` onwards are shifted to the right.
     * @tparam Args Types of arguments for the element's constructor.
     * @param position An iterator pointing to the position where the element should be constructed.
     * @param args Arguments to forward to the element's constructor.
     * @return A reference to the newly constructed element.
     * @throws std::bad_alloc If memory reallocation fails.
     * @throws Any exception thrown by the ElementT constructor.
     */
    template <typename... Args>
    ElementT& emplace(ElementT* position, Args&&... args) {
        size_t index = position - begin();
        if (_size < _capacity) {
            if (position == end()) {
                constructOneAtEnd(std::forward<Args>(args)...);
            } else {
                shiftAndEmplaceOneAt(position, std::forward<Args>(args)...);
            }
        } else {
            growAndConstructOneAt(position, std::forward<Args>(args)...);
        }
        return _data[index];
    }

    /**
     * @brief Swaps the contents of this DArray with another DArray.
     * This operation is efficient as it only swaps internal pointers and metadata.
     * @param other The DArray to swap with.
     */
    void swap(DArray& other) noexcept {
        std::swap(_data, other._data);
        std::swap(_size, other._size);
        std::swap(_capacity, other._capacity);
    }

private:
    void allocate(size_t n) {
        _data = allocateData(n);
        _size = 0;
        _capacity = n;
    }

    ElementT* allocateData(size_t n) const {
        if (n > maxSize()) {
            throw std::bad_alloc();
        }
        return static_cast<ElementT*>(_allocator.allocate(n * sizeof(ElementT), alignment()));
    }

    void deallocate() noexcept {
        deallocateData(_data);
        _data = nullptr;
        _size = 0;
        _capacity = 0;
    }

    void deallocateData(ElementT* ptr) const noexcept {
        _allocator.deallocate(static_cast<void*>(ptr), alignment());
    }

    void constructAtEnd(size_t n) {
        assert(n > 0);
        assert(_size + n <= _capacity);
        constructDefaultElements(end(), end() + n);
        _size += n;
    }

    static void constructDefaultElements(ElementT* first, ElementT* last) {
        auto firstCopy = first;
        auto guard = std::__make_exception_guard(DestructRangeInReverse(firstCopy, first));
        for (; first != last; ++first) {
            new (first) ElementT();
        }
        guard.__complete();
    }

    void constructAtEnd(const ElementT& element, size_t n) {
        assert(n > 0);
        assert(_size + n <= _capacity);
        copyElement(element, n, end());
        _size += n;
    }

    static void copyElement(const ElementT& element, size_t n, ElementT* dst) {
        auto dstCopy = dst;
        auto guard = std::__make_exception_guard(DestructRangeInReverse(dstCopy, dst));
        for (ElementT* last = dst + n; dst != last; ++dst) {
            new (dst) ElementT(element);
        }
        guard.__complete();
    }

    void constructAtEnd(const ElementT* first, const ElementT* last, size_t n) {
        assert(n > 0);
        assert(_size + n <= _capacity);
        copyRange(first, last, end());
        _size += n;
    }

    static void copyRange(const ElementT* first, const ElementT* last, ElementT* dst) {
        auto dstCopy = dst;
        auto guard = std::__make_exception_guard(DestructRangeInReverse(dstCopy, dst));
        for (; first != last; ++first, ++dst) {
            new (dst) ElementT(*first);
        }
        guard.__complete();
    }

    static void moveRange(ElementT* first, ElementT* last, ElementT* dst) {
        for (; first != last; ++first, ++dst) {
            new (dst) ElementT(std::move(*first));
        }
    }

    void destructAtEnd(size_t n) noexcept {
        assert(n <= _size);
        ElementT* srcBegin = end() - n;
        ElementT* srcEnd = end();
        DestructRangeInReverse(srcBegin, srcEnd)();
        _size -= n;
    }

    void destructAt(ElementT* position, size_t n) noexcept {
        assert(std::__is_pointer_in_range(begin(), end() - 1, position));
        assert(n > 0);
        assert(position + n <= end());
        ElementT* srcBegin = position;
        ElementT* srcEnd = position + n;
        DestructRangeInReverse(srcBegin, srcEnd)();
        shiftTailLeft(position + n, n);
    }

    void copy(const ElementT& element, size_t n) {
        if (n > 0) {
            AllocateTransaction transaction(*this);
            transaction.allocate(n);
            copyElement(element, n, transaction.data);
            clear();
            std::swap(_data, transaction.data);
            _capacity = _size = n;
        } else {
            clear();
        }
    }

    void copy(const ElementT* first, const ElementT* last, size_t n) {
        if (n > 0) {
            AllocateTransaction transaction(*this);
            transaction.allocate(n);
            copyRange(first, last, transaction.data);
            clear();
            std::swap(_data, transaction.data);
            _capacity = _size = n;
        } else {
            clear();
        }
    }

    template <typename... Args>
    void constructOneAtEnd(Args&&... args) {
        new (end()) ElementT(std::forward<Args>(args)...);
        ++_size;
    }

    template <typename... Args>
    void growAndConstructOneAt(ElementT* position, Args&&... args) {
        size_t newSize = _size + 1;
        size_t newCapacity = extendedCapacity(newSize);
        AllocateTransaction transaction(*this);
        transaction.allocate(newCapacity);
        ElementT* dst = transaction.data + (position - begin());
        new (dst) ElementT(std::forward<Args>(args)...);
        moveRange(begin(), position, transaction.data);
        moveRange(position, end(), dst + 1);
        clear();
        std::swap(_data, transaction.data);
        _size = newSize;
        _capacity = newCapacity;
    }

    void growAndConstructAt(ElementT* position, const ElementT& element, size_t n) {
        size_t newSize = _size + n;
        size_t newCapacity = extendedCapacity(newSize);
        AllocateTransaction transaction(*this);
        transaction.allocate(newCapacity);
        ElementT* dst = transaction.data + (position - begin());
        copyElement(element, n, dst);
        moveRange(begin(), position, transaction.data);
        moveRange(position, end(), dst + n);
        clear();
        std::swap(_data, transaction.data);
        _size = newSize;
        _capacity = newCapacity;
    }

    void growAndConstructAt(ElementT* position, const ElementT* first, const ElementT* last, size_t n) {
        size_t newSize = _size + n;
        size_t newCapacity = extendedCapacity(newSize);
        AllocateTransaction transaction(*this);
        transaction.allocate(newCapacity);
        ElementT* dst = transaction.data + (position - begin());
        copyRange(first, last, dst);
        moveRange(begin(), position, transaction.data);
        moveRange(position, end(), dst + n);
        clear();
        std::swap(_data, transaction.data);
        _size = newSize;
        _capacity = newCapacity;
    }

    template <typename... Args>
    void shiftAndEmplaceOneAt(ElementT* position, Args&&... args) {
        shiftTailRight(position, 1);
        auto guard = std::__make_exception_guard(ShiftArrayTailLeft(*this, position + 1, 1));
        new (position) ElementT(std::forward<Args>(args)...);
        guard.__complete();
    }

    void shiftAndConstructOneAt(ElementT* position, const ElementT& element) {
        shiftTailRight(position, 1);
        const ElementT* src = std::pointer_traits<const ElementT*>::pointer_to(element);
        if (std::__is_pointer_in_range(position, end(), src)) {
            ++src;
        }
        auto guard = std::__make_exception_guard(ShiftArrayTailLeft(*this, position + 1, 1));
        new (position) ElementT(*src);
        guard.__complete();
    }

    void shiftAndConstructOneAt(ElementT* position, ElementT&& element) noexcept {
        shiftTailRight(position, 1);
        ElementT* src = std::pointer_traits<ElementT*>::pointer_to(element);
        if (std::__is_pointer_in_range(position, end(), src)) {
            ++src;
        }
        new (position) ElementT(std::move(*src));
    }

    void shiftAndConstructAt(ElementT* position, const ElementT& element, size_t n) {
        shiftTailRight(position, n);
        const ElementT* src = std::pointer_traits<const ElementT*>::pointer_to(element);
        if (std::__is_pointer_in_range(position, end(), src)) {
            src += n;
        }
        auto guard = std::__make_exception_guard(ShiftArrayTailLeft(*this, position + n, n));
        copyElement(*src, n, position);
        guard.__complete();
    }

    void shiftAndConstructAt(ElementT* position, const ElementT* first, const ElementT* last, size_t n) {
        shiftTailRight(position, n);
        const ElementT* src = first;
        if (std::__is_pointer_in_range(position, end(), src)) {
            src += n;
        }
        auto guard = std::__make_exception_guard(ShiftArrayTailLeft(*this, position + n, n));
        copyRange(src, src + n, position);
        guard.__complete();
    }

    void shiftTailRight(ElementT* position, size_t n) noexcept {
        assert(std::__is_pointer_in_range(begin(), end(), position));
        assert(n > 0);
        assert(_size + n <= _capacity);
        auto srcBegin = std::reverse_iterator<ElementT*>(end());
        auto srcEnd = std::reverse_iterator<ElementT*>(position);
        auto dst = std::reverse_iterator<ElementT*>(end() + n);
        for (; srcBegin != srcEnd; ++srcBegin, ++dst) {
            new (std::to_address(dst)) ElementT(std::move(*srcBegin));
            srcBegin->~ElementT();
        }
        _size += n;
    }

    void shiftTailLeft(ElementT* position, size_t n) noexcept {
        assert(std::__is_pointer_in_range(begin(), end(), position));
        assert(n > 0);
        assert(position - n >= begin());
        ElementT* srcBegin = position;
        ElementT* srcEnd = end();
        ElementT* dst = position - n;
        for (; srcBegin != srcEnd; ++srcBegin, ++dst) {
            new (dst) ElementT(std::move(*srcBegin));
            srcBegin->~ElementT();
        }
        _size -= n;
    }

    std::align_val_t alignment() const noexcept {
        return static_cast<std::align_val_t>(alignof(ElementT));
    }

    size_t extendedCapacity(size_t requiredCapacity) const {
        size_t ms = maxSize();
        if (requiredCapacity > ms) {
            throw std::length_error("Required capacity is too large");
        }
        if (_capacity >= ms / 2) {
            return ms;
        }
        return std::max(_capacity * 2, requiredCapacity);
    }

    struct AllocateTransaction {
        DArray& array;
        ElementT* data = nullptr;

        AllocateTransaction(DArray& array)
            : array(array) {};

        ~AllocateTransaction() {
            if (data != nullptr) {
                array.deallocateData(data);
                data = nullptr;
            }
        }

        void allocate(size_t n) {
            data = array.allocateData(n);
        }
    };

    struct DestructRangeInReverse {
        ElementT*& first;
        ElementT*& last;

        DestructRangeInReverse(ElementT*& first, ElementT*& last)
            : first(first)
            , last(last) {}

        void operator()() const {
            auto reverseFirst = std::reverse_iterator<ElementT*>(last);
            auto reverseLast = std::reverse_iterator<ElementT*>(first);
            for (; reverseFirst != reverseLast; ++reverseFirst) {
                reverseFirst->~ElementT();
            }
        }
    };

    struct DestroyArray {
        DArray& array;

        DestroyArray(DArray& array)
            : array(array) {}

        void operator()() {
            array.destroy();
        }
    };

    struct ShiftArrayTailLeft {
        DArray& array;
        ElementT* position;
        size_t n;

        ShiftArrayTailLeft(DArray& array, ElementT* position, size_t n)
            : array(array)
            , position(position)
            , n(n) {};

        void operator()() const {
            array.shiftTailLeft(position, n);
        }
    };

    friend std::formatter<DArray<ElementT>>;
};

template <typename ElementT>
struct std::formatter<DArray<ElementT>> : std::formatter<std::string_view> {
    auto format(const DArray<ElementT>& array, auto& context) const {
        std::string result = "[";
        for (int i = 0; i < array.size(); ++i) {
            if (i > 0) result += " ";
            result += std::format("{}", *(array._data + i));
        }
        result += "]";
        return std::formatter<std::string_view>::format(result, context);
    }
};

#endif // DYNAMIC_ARRAY_HPP