#ifndef POINTERS_HPP
#define POINTERS_HPP

#include <utility>

template <typename D, typename V>
concept Deleter = requires(D deleter, V* pointer) {
    { deleter(pointer) } noexcept;
};

template <typename V>
struct DefaultDeleter {

    constexpr DefaultDeleter() noexcept = default;

    void operator()(V* pointer) const noexcept {
        delete pointer;
    }
};

template <typename V>
struct DefaultDeleter<V[]> {

    constexpr DefaultDeleter() noexcept = default;

    void operator()(V* pointer) const noexcept {
        delete[] pointer;
    }
};

template <typename ValueT, Deleter<ValueT> DeleterT = DefaultDeleter<ValueT>>
class UniquePointer {
private:
    ValueT* _pointer = nullptr;
    DeleterT _deleter;

public:
    UniquePointer() noexcept {}

    UniquePointer(ValueT* pointer) noexcept
        : _pointer(pointer) {}

    UniquePointer(UniquePointer&& other) noexcept {
        clear();
        swap(other);
    }

    ~UniquePointer() noexcept {
        clear();
    }

    UniquePointer& operator=(UniquePointer&& other) noexcept {
        clear();
        swap(other);
        return *this;
    }

    ValueT* release() noexcept {
        ValueT* p = _pointer;
        _pointer = nullptr;
        return p;
    }

    void clear() noexcept {
        if (_pointer != nullptr) {
            _deleter(_pointer);
            _pointer = nullptr;
        }
    }

    void swap(UniquePointer& other) noexcept {
        std::swap(_pointer, other._pointer);
        std::swap(_deleter, other._deleter);
    }

    ValueT* get() const noexcept {
        return _pointer;
    }

    DeleterT getDeleter() const noexcept {
        return _deleter;
    }

    explicit operator bool() const noexcept {
        return _pointer != nullptr;
    }

    ValueT& operator*() const noexcept(IsNoexeptDeref::value) {
        return *_pointer;
    }

    ValueT* operator->() const noexcept {
        return _pointer;
    }

private:
    struct IsNoexeptDeref {
        static constexpr bool value = noexcept(*std::declval<ValueT*>());
    };
};

#endif // POINTERS_HPP