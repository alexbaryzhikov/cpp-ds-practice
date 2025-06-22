#ifndef DYNAMIC_ARRAY_HPP
#define DYNAMIC_ARRAY_HPP

#include <__utility/exception_guard.h>
#include <cassert>
#include <limits>
#include <new>

template <typename V> 
class DArray {
private:
    V* _data = nullptr;
    std::size_t _size = 0;
    std::size_t _capacity = 0;

public:
    DArray() noexcept {}

    DArray(size_t n) {
        auto guard = std::__make_exception_guard(DestroyArray(*this));
        if (n > 0) {
            allocate(n);
            construct(n);
        }
        guard.__complete();
    }

    ~DArray() noexcept {
        destruct(_size);
        deallocate();
    }

    size_t size() const noexcept {
        return static_cast<size_t>(_size);
    }

    size_t capacity() const noexcept {
        return static_cast<size_t>(_capacity);
    }

    void clear() noexcept {
        destruct(_size);
    }

private:
    void allocate(size_t n) {
        if (n > maxSize()) {
            throw std::bad_alloc();
        }
        _data = static_cast<V*>(::operator new(n * sizeof(V), align()));
        _size = 0;
        _capacity = n;
    }

    void deallocate() noexcept {
        ::operator delete(_data, align(), std::nothrow_t());
        _data = nullptr;
        _size = 0;
        _capacity = 0;
    }

    void construct(size_t n) {
        assert(n <= _capacity - _size);
        V* cur = _data + _size;
        V* end = cur + n;
        for (; cur != end; ++cur) {
            new (cur) V();
            ++_size;
        }
    }

    void destruct(size_t n) noexcept {
        assert(n <= _size);
        V* end = _data + _size;
        V* cur = end - n;
        for (; cur != end; ++cur) {
            cur->~V();
        }
        _size -= n;
    }

    size_t maxSize() const noexcept {
        return std::numeric_limits<size_t>::max() / sizeof(V);
    }

    std::align_val_t align() const noexcept {
        return static_cast<std::align_val_t>(alignof(V));
    }

    struct DestroyArray {
        DArray& array;

        DestroyArray(DArray& array) : array(array) {}
        
        void operator()() {
            if (array._data != nullptr) {
                array.destruct(array._size);
                array.deallocate();
            }
        }
    };
};

#endif // DYNAMIC_ARRAY_HPP