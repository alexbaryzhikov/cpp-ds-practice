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

template <typename ElementT, Allocator AllocatorT = DefaultAllocator>
class DArray {
private:
    ElementT* _data = nullptr;
    size_t _size = 0;
    size_t _capacity = 0;
    AllocatorT _allocator;

public:
    DArray() noexcept {}

    explicit DArray(size_t n) {
        if (n > 0) {
            auto guard = std::__make_exception_guard(DestroyArray(*this));
            allocate(n);
            constructAtEnd(n);
            guard.__complete();
        }
    }

    DArray(const ElementT& x, size_t n) {
        if (n > 0) {
            auto guard = std::__make_exception_guard(DestroyArray(*this));
            allocate(n);
            constructAtEnd(x, n);
            guard.__complete();
        }
    }

    DArray(const ElementT* first, const ElementT* last) {
        size_t n = static_cast<size_t>(last - first);
        if (n > 0) {
            auto guard = std::__make_exception_guard(DestroyArray(*this));
            allocate(n);
            constructAtEnd(first, last, n);
            guard.__complete();
        }
    }

    DArray(std::initializer_list<ElementT> elements) {
        if (elements.size() > 0) {
            auto guard = std::__make_exception_guard(DestroyArray(*this));
            allocate(elements.size());
            constructAtEnd(elements.begin(), elements.end(), elements.size());
            guard.__complete();
        }
    }

    DArray(const DArray& other) {
        if (other.size() > 0) {
            auto guard = std::__make_exception_guard(DestroyArray(*this));
            allocate(other.size());
            constructAtEnd(other.begin(), other.end(), other.size());
            guard.__complete();
        }
    }

    DArray(DArray&& other) noexcept {
        swap(other);
    }

    ~DArray() noexcept {
        destroy();
    }

    DArray& assign(const ElementT& element, size_t n) {
        copy(element, n);
        return *this;
    }

    DArray& assign(const ElementT* first, const ElementT* last) {
        copy(first, last, last - first);
        return *this;
    }

    DArray& operator=(std::initializer_list<ElementT> elements) {
        copy(elements.begin(), elements.end(), elements.size());
        return *this;
    }

    DArray& operator=(const DArray& other) {
        if (this != std::addressof(other)) {
            copy(other.begin(), other.end(), other.size());
        }
        return *this;
    }

    DArray& operator=(DArray&& other) noexcept {
        if (this != std::addressof(other)) {
            destroy();
            swap(other);
        }
        return *this;
    }

    ElementT& operator[](size_t index) noexcept {
        return _data[index];
    }

    const ElementT& operator[](size_t index) const noexcept {
        return _data[index];
    }

    ElementT& at(size_t index) {
        if (index < _size) {
            return _data[index];
        }
        throw std::out_of_range("Index is out of range");
    }

    const ElementT& at(size_t index) const {
        if (index < _size) {
            return _data[index];
        }
        throw std::out_of_range("Index is out of range");
    }

    ElementT& front() noexcept {
        return _data[0];
    }

    const ElementT& front() const noexcept {
        return _data[0];
    }

    ElementT& back() noexcept {
        return _data[_size - 1];
    }

    const ElementT& back() const noexcept {
        return _data[_size - 1];
    }

    ElementT* data() noexcept {
        return _data;
    }

    const ElementT* data() const noexcept {
        return _data;
    }

    ElementT* begin() noexcept {
        return _data;
    }

    const ElementT* begin() const noexcept {
        return _data;
    }

    ElementT* end() noexcept {
        return _data + _size;
    }

    const ElementT* end() const noexcept {
        return _data + _size;
    }

    std::reverse_iterator<ElementT*> rbegin() noexcept {
        return std::reverse_iterator<ElementT*>(end());
    }

    std::reverse_iterator<const ElementT*> rbegin() const noexcept {
        return std::reverse_iterator<const ElementT*>(end());
    }

    std::reverse_iterator<ElementT*> rend() noexcept {
        return std::reverse_iterator<ElementT*>(begin());
    }

    std::reverse_iterator<const ElementT*> rend() const noexcept {
        return std::reverse_iterator<const ElementT*>(begin());
    }

    bool empty() const noexcept {
        return _size == 0;
    }

    size_t size() const noexcept {
        return _size;
    }

    size_t maxSize() const noexcept {
        return std::numeric_limits<size_t>::max() / sizeof(ElementT);
    }

    size_t capacity() const noexcept {
        return _capacity;
    }

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

    void clear() noexcept {
        if (_data != nullptr) {
            destructAtEnd(_size);
        }
    }

    void destroy() noexcept {
        if (_data != nullptr) {
            destructAtEnd(_size);
            deallocate();
        }
    }

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

    ElementT* erase(ElementT* position) noexcept {
        size_t index = position - begin();
        if (position == end() - 1) {
            destructAtEnd(1);
        } else {
            destructAt(position, 1);
        }
        return begin() + index;
    }

    ElementT* erase(ElementT* first, ElementT* last) {
        size_t index = first - begin();
        if (last == end()) {
            destructAtEnd(last - first);
        } else {
            destructAt(first, last - first);
        }
        return begin() + index;
    }

    void push(const ElementT& element) {
        if (_size < _capacity) {
            constructOneAtEnd(element);
        } else {
            growAndConstructOneAt(end(), element);
        }
    }

    void push(ElementT&& element) {
        if (_size < _capacity) {
            constructOneAtEnd(std::move(element));
        } else {
            growAndConstructOneAt(end(), std::move(element));
        }
    }

    void pop() {
        destructAtEnd(1);
    }

    template <typename... Args>
    ElementT& emplaceAtEnd(Args&&... args) {
        if (_size < _capacity) {
            constructOneAtEnd(std::forward<Args>(args)...);
        } else {
            growAndConstructOneAt(end(), std::forward<Args>(args)...);
        }
        return back();
    }

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

    void swap(DArray& other) noexcept {
        std::swap(_data, other._data);
        std::swap(_size, other._size);
        std::swap(_capacity, other._capacity);
    }

private:
    // Allocates array memory for n elements.
    // Precondition: n <= max size
    // Postcondition: size == 0
    // Postcondition: capacity == n
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

    // Deallocates array memory.
    // Postcondition: size == 0
    // Postcondition: capacity == 0
    void deallocate() noexcept {
        deallocateData(_data);
        _data = nullptr;
        _size = 0;
        _capacity = 0;
    }

    void deallocateData(ElementT* ptr) const noexcept {
        _allocator.deallocate(static_cast<void*>(ptr), alignment());
    }

    // Default constructs n objects starting at the end of the array.
    // Precondition: n > 0
    // Precondition: size + n <= capacity
    // Postcondition: size == size + n
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

    // Copy constructs n objects from x starting at the end of the array.
    // Precondition: n > 0
    // Precondition: size + n <= capacity
    // Postcondition: size == size + n
    // Postcondition: array[i] == x for all i in [size - n, size)
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

    // Copy constructs objects from range [first, last) starting at the end of the array.
    // Precondition: n > 0
    // Precondition: size + n <= capacity
    // Postcondition: size == size + n
    // Postcondition: array[i] == range[j] for all i in [size - n, size) and j in [0, n)
    void constructAtEnd(const ElementT* first, const ElementT* last, size_t n) {
        assert(n > 0);
        assert(_size + n <= _capacity);
        copyRange(first, last, end());
        _size += n;
    }

    // Copy a range of elements to a new place.
    // Precondition: [first, last) and [dst, dst + distance(first, last)) do not overlap
    // Postcondition: a == b for all a in [first, last) and b in [dst, dst + distance(first, last))
    static void copyRange(const ElementT* first, const ElementT* last, ElementT* dst) {
        auto dstCopy = dst;
        auto guard = std::__make_exception_guard(DestructRangeInReverse(dstCopy, dst));
        for (; first != last; ++first, ++dst) {
            new (dst) ElementT(*first);
        }
        guard.__complete();
    }

    // Move a range of elements to a new place.
    // Precondition: [first, last) and [dst, dst + distance(first, last)) do not overlap
    // Postcondition: a -> b for all a in [first, last) and b in [dst, dst + distance(first, last))
    static void moveRange(ElementT* first, ElementT* last, ElementT* dst) {
        for (; first != last; ++first, ++dst) {
            new (dst) ElementT(std::move(*first));
        }
    }

    // Destructs n object at the end of the array.
    // Precondition: n <= size
    // Postcondition: size == size - n
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

    // Moves the array tail right.
    // Precondition: position in [begin, end)
    // Precondition: n > 0
    // Precondition: size + n <= capacity
    // Postcondition: a -> b for all a in [position, end) and b in [position + n, end + n)
    // Postcondition: [position, position + n) is empty
    // Postcondition: size = size + n
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

    // Moves the array tail left.
    // Precondition: position in [begin, end)
    // Precondition: n > 0
    // Precondition: position - n >= begin
    // Precondition: [position - n, position) is empty
    // Postcondition: a -> b for all a in [position, end) and b in [position - n, end - n)
    // Postcondition: [end - n, end) is empty
    // Postcondition: size = size - n
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