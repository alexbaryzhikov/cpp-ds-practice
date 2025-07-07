#ifndef DYNAMIC_ARRAY_HPP
#define DYNAMIC_ARRAY_HPP

#include <__utility/exception_guard.h>
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

    DArray(size_t n) {
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

    DArray(ElementT* begin, ElementT* end) {
        size_t n = static_cast<size_t>(end - begin);
        if (n > 0) {
            auto guard = std::__make_exception_guard(DestroyArray(*this));
            allocate(n);
            constructAtEnd(begin, end, n);
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

    DArray& operator=(std::initializer_list<ElementT> elements) {
        copy(elements.begin(), elements.end(), elements.size());
        return *this;
    }

    DArray(const DArray& other) {
        if (other.size() > 0) {
            auto guard = std::__make_exception_guard(DestroyArray(*this));
            allocate(other.size());
            constructAtEnd(other.begin(), other.end(), other.size());
            guard.__complete();
        }
    }

    DArray& operator=(const DArray& other) {
        if (this != std::addressof(other)) {
            copy(other.begin(), other.end(), other.size());
        }
        return *this;
    }

    DArray(DArray&& other) noexcept {
        swap(other);
    }

    DArray& operator=(DArray&& other) noexcept {
        destroy();
        swap(other);
        return *this;
    }

    ~DArray() noexcept {
        destroy();
    }

    size_t size() const noexcept {
        return static_cast<size_t>(_size);
    }

    size_t capacity() const noexcept {
        return static_cast<size_t>(_capacity);
    }

    bool empty() const noexcept {
        return _size == 0;
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

    ElementT& operator[](size_t i) noexcept {
        return _data[i];
    }

    const ElementT& operator[](size_t i) const noexcept {
        return _data[i];
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

    void swap(DArray& other) noexcept {
        std::swap(_data, other._data);
        std::swap(_size, other._size);
        std::swap(_capacity, other._capacity);
    }

    void push_back(const ElementT& element) {
        if (_size < _capacity) {
            constructOneAtEnd(element);
        } else {
            growAndConstructOneAtEnd(element);
        }
    }

    void push_back(ElementT&& element) {
        if (_size < _capacity) {
            constructOneAtEnd(std::move(element));
        } else {
            growAndConstructOneAtEnd(std::move(element));
        }
    }

    void pop_back() {
        destructAtEnd(1);
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
        ConstructTransaction transaction(*this, n);
        transaction.constructElements();
    }

    // Copy constructs n objects from x starting at the end of the array.
    // Precondition: n > 0
    // Precondition: size + n <= capacity
    // Postcondition: size == size + n
    // Postcondition: array[i] == x for all i in [size - n, size)
    void constructAtEnd(const ElementT& x, size_t n) {
        assert(n > 0);
        assert(_size + n <= _capacity);
        ConstructTransaction transaction(*this, n);
        transaction.dst = copyElement(x, n, transaction.dst);
    }

    static ElementT* copyElement(const ElementT& x, size_t n, ElementT* dst) {
        auto dstCopy = dst;
        auto guard = std::__make_exception_guard(DestructRangeInReverse(dstCopy, dst));
        for (ElementT* end = dst + n; dst != end; ++dst) {
            new (dst) ElementT(x);
        }
        guard.__complete();
        return dst;
    }

    // Copy constructs objects from range [begin, end) starting at the end of the array.
    // Precondition: n > 0
    // Precondition: size + n <= capacity
    // Postcondition: size == size + n
    // Postcondition: array[i] == range[j] for all i in [size - n, size) and j in [0, n)
    void constructAtEnd(const ElementT* begin, const ElementT* end, size_t n) {
        assert(n > 0);
        assert(_size + n <= _capacity);
        ConstructTransaction transaction(*this, n);
        transaction.dst = copyRange(begin, end, transaction.dst);
    }

    static ElementT* copyRange(const ElementT* begin, const ElementT* end, ElementT* dst) {
        auto dstCopy = dst;
        auto guard = std::__make_exception_guard(DestructRangeInReverse(dstCopy, dst));
        for (const ElementT* src = begin; src != end; ++src, ++dst) {
            new (dst) ElementT(*src);
        }
        guard.__complete();
        return dst;
    }

    static ElementT* moveRange(ElementT* begin, const ElementT* end, ElementT* dst) {
        for (ElementT* src = begin; src != end; ++src, ++dst) {
            new (dst) ElementT(std::move(*src));
        }
        return dst;
    }

    // Destructs n object at the end of the array.
    // Precondition: n <= size
    // Postcondition: size == size - n
    void destructAtEnd(size_t n) noexcept {
        assert(n <= _size);
        ElementT* begin = _data + _size - n;
        ElementT* end = _data + _size;
        DestructRangeInReverse(begin, end)();
        _size -= n;
    }

    void copy(const ElementT* begin, const ElementT* end, size_t n) {
        if (n > 0) {
            CopyTransaction transaction(*this, begin, end, n);
            transaction.copyElements();
        } else {
            clear();
        }
    }

    template <typename... Args>
    void constructOneAtEnd(Args&&... args) {
        ElementT* pos = _data + _size;
        new (pos) ElementT(std::forward<Args>(args)...);
        ++_size;
    }

    template <typename... Args>
    void growAndConstructOneAtEnd(Args&&... args) {
        size_t newSize = _size + 1;
        size_t newCapacity = extendedCapacity(newSize);
        AllocateTransaction transaction(*this);
        transaction.allocate(newCapacity);
        new (transaction.data + _size) ElementT(std::forward<Args>(args)...);
        moveRange(begin(), end(), transaction.data);
        clear();
        std::swap(_data, transaction.data);
        _size = newSize;
        _capacity = newCapacity;
    }

    std::align_val_t alignment() const noexcept {
        return static_cast<std::align_val_t>(alignof(ElementT));
    }

    size_t maxSize() const noexcept {
        return std::numeric_limits<size_t>::max() / sizeof(ElementT);
    }

    size_t extendedCapacity(size_t newSize) const {
        size_t ms = maxSize();
        if (newSize > ms) {
            throw std::length_error("New array size is too high");
        }
        if (_capacity >= ms / 2) {
            return ms;
        }
        return std::max(_capacity * 2, newSize);
    }

    struct ConstructTransaction {
        DArray& array;
        ElementT* dst;
        const ElementT* end;

        ConstructTransaction(DArray& array, size_t n)
            : array(array)
            , dst(array._data + array._size)
            , end(array._data + array._size + n) {};

        ~ConstructTransaction() {
            array._size = static_cast<size_t>(dst - array._data);
        }

        void constructElements() {
            for (; dst != end; ++dst) {
                new (dst) ElementT();
            }
        }
    };

    struct CopyTransaction {
        DArray& array;
        const ElementT* begin;
        const ElementT* end;
        const size_t size;
        ElementT* data = nullptr;

        CopyTransaction(DArray& array, const ElementT* begin, const ElementT* end, size_t size)
            : array(array)
            , begin(begin)
            , end(end)
            , size(size) {};

        ~CopyTransaction() {
            if (data != nullptr) {
                array.deallocateData(data);
                data = nullptr;
            }
        }

        void copyElements() {
            data = array.allocateData(size);
            copyRange(begin, end, data);
            array.clear();
            std::swap(array._data, data);
            array._size = size;
            array._capacity = size;
        }
    };

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
        ElementT*& begin;
        ElementT*& end;

        DestructRangeInReverse(ElementT*& begin, ElementT*& end)
            : begin(begin)
            , end(end) {}

        void operator()() const {
            auto reverseBegin = std::reverse_iterator<ElementT*>(end);
            auto reverseEnd = std::reverse_iterator<ElementT*>(begin);
            for (; reverseBegin != reverseEnd; ++reverseBegin) {
                reverseBegin->~ElementT();
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