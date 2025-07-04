#ifndef DYNAMIC_ARRAY_HPP
#define DYNAMIC_ARRAY_HPP

#include <__utility/exception_guard.h>
#include <cassert>
#include <format>
#include <initializer_list>
#include <iterator>
#include <limits>
#include <new>
#include <string_view>

struct DefaultAlloc {
    void* allocate(size_t count, std::align_val_t alignment) {
        return ::operator new(count, alignment);
    }

    void deallocate(void* pointer, std::align_val_t alignment) noexcept {
        ::operator delete(pointer, alignment, std::nothrow);
    }
};

template <typename Val, typename Alloc = DefaultAlloc> 
class DArray {
private:
    Val* _data = nullptr;
    size_t _size = 0;
    size_t _capacity = 0;
    Alloc alloc;

public:
    DArray() noexcept {}

    DArray(size_t n) {
        auto guard = std::__make_exception_guard(DestructArray(*this));
        if (n > 0) {
            allocate(n);
            constructAtEnd(n);
        }
        guard.__complete();
    }

    DArray(const Val& x, size_t n) {
        auto guard = std::__make_exception_guard(DestructArray(*this));
        if (n > 0) {
            allocate(n);
            constructAtEnd(x, n);
        }
        guard.__complete();
    }

    DArray(Val* begin, Val* end) {
        auto guard = std::__make_exception_guard(DestructArray(*this));
        size_t n = static_cast<size_t>(end - begin);
        if (n > 0) {
            allocate(n);
            constructAtEnd(begin, end, n);
        }
        guard.__complete();
    }

    DArray(std::initializer_list<Val> il) {
        auto guard = std::__make_exception_guard(DestructArray(*this));
        if (il.size() > 0) {
            allocate(il.size());
            constructAtEnd(il.begin(), il.end(), il.size());
        }
        guard.__complete();
    }

    ~DArray() noexcept {
        DestructArray(*this)();
    }

    size_t size() const noexcept {
        return static_cast<size_t>(_size);
    }

    size_t maxSize() const noexcept {
        return std::numeric_limits<size_t>::max() / sizeof(Val);
    }

    size_t capacity() const noexcept {
        return static_cast<size_t>(_capacity);
    }

    bool empty() const noexcept {
        return _size == 0;
    }

    void clear() noexcept {
        destructAtEnd(_size);
    }

    Val& front() noexcept {
        return _data[0];
    }

    const Val& front() const noexcept {
        return _data[0];
    }

    Val& back() noexcept {
        return _data[_size - 1];
    }

    const Val& back() const noexcept {
        return _data[_size - 1];
    }

    Val& operator[](size_t i) noexcept {
        return _data[i];
    }

    const Val& operator[](size_t i) const noexcept {
        return _data[i];
    }

    Val* begin() noexcept {
        return _data;
    }

    const Val* begin() const noexcept {
        return _data;
    }

    Val* end() noexcept {
        return _data + _size;
    }

    const Val* end() const noexcept {
        return _data + _size;
    }

private:
    void allocate(size_t n) {
        if (n > maxSize()) {
            throw std::bad_alloc();
        }
        _data = static_cast<Val*>(alloc.allocate(n * sizeof(Val), alignment()));
        _size = 0;
        _capacity = n;
    }

    void deallocate() noexcept {
        alloc.deallocate(_data, alignment());
        _data = nullptr;
        _size = 0;
        _capacity = 0;
    }

    // Default constructs n objects starting at the end of the array.
    // Precondition: n > 0
    // Precondition: size + n <= capacity
    // Postcondition: size == size + n
    void constructAtEnd(size_t n) {
        assert(n > 0);
        assert(_size + n <= _capacity);
        ConstructTransaction transaction(*this, n);
        for (; transaction.dst != transaction.end; ++transaction.dst) {
            new (transaction.dst) Val();
        }
    }

    // Copy constructs n objects from x starting at the end of the array.
    // Precondition: n > 0
    // Precondition: size + n <= capacity
    // Postcondition: size == size + n
    // Postcondition: array[i] == x for all i in [size - n, size)
    void constructAtEnd(const Val& x, size_t n) {
        assert(n > 0);
        assert(_size + n <= _capacity);
        ConstructTransaction transaction(*this, n);
        transaction.dst = copyElement(x, n, transaction.dst);
    }

    Val* copyElement(const Val& x, size_t n, Val* dst) {
        auto dstCopy = dst;
        auto guard = std::__make_exception_guard(DestructRangeInReverse(dstCopy, dst));
        for (Val* end = dst + n; dst != end; ++dst) {
            new (dst) Val(x);
        }
        guard.__complete();
        return dst;
    }

    // Copy constructs objects from range [begin, end) starting at the end of the array.
    // Precondition: n > 0
    // Precondition: size + n <= capacity
    // Postcondition: size == size + n
    // Postcondition: array[i] == range[j] for all i in [size - n, size) and j in [0, n)
    void constructAtEnd(const Val* begin, const Val* end, size_t n) {
        assert(n > 0);
        assert(_size + n <= _capacity);
        ConstructTransaction transaction(*this, n);
        transaction.dst = copyRange(begin, end, transaction.dst);
    }

    Val* copyRange(const Val* begin, const Val* end, Val* dst) {
        auto dstCopy = dst;
        auto guard = std::__make_exception_guard(DestructRangeInReverse(dstCopy, dst));
        for (const Val* src = begin; src != end;) {
            new (dst) Val(*src);
            ++src;
            ++dst;
        }
        guard.__complete();
        return dst;
    }

    void destructAtEnd(size_t n) noexcept {
        assert(n <= _size);
        Val* begin = _data + _size - 1;
        Val* end = begin - n;
        for (; begin != end; --begin) {
            begin->~Val();
        }
        _size -= n;
    }

    std::align_val_t alignment() const noexcept {
        return static_cast<std::align_val_t>(alignof(Val));
    }

    struct ConstructTransaction {
        DArray& array;
        Val* dst;
        const Val* end;

        ConstructTransaction(DArray& array, size_t n)
            : array(array)
            , dst(array._data + array._size)
            , end(array._data + array._size + n) {};

        ~ConstructTransaction() {
            array._size = static_cast<size_t>(dst - array._data);
        }
    };

    struct DestructRangeInReverse {
        Val*& begin;
        Val*& end;

        DestructRangeInReverse(Val*& begin, Val*& end): begin(begin), end(end) {}

        void operator()() const {
            auto reverseBegin = std::reverse_iterator<Val*>(end);
            auto reverseEnd = std::reverse_iterator<Val*>(begin);
            for (; reverseBegin != reverseEnd; ++reverseBegin) {
                reverseBegin->~Val();
            }
        }
    };

    struct DestructArray {
        DArray& array;

        DestructArray(DArray& array) : array(array) {}
        
        void operator()() {
            if (array._data != nullptr) {
                array.destructAtEnd(array._size);
                array.deallocate();
            }
        }
    };

    friend std::formatter<DArray<Val>>;
};

template<typename V>
struct std::formatter<DArray<V>> : std::formatter<std::string_view> {
    auto format(const DArray<V>& array, auto& context) const {
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