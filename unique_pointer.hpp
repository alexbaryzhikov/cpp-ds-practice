#ifndef POINTERS_HPP
#define POINTERS_HPP

#include <concepts>
#include <cstddef>
#include <print>
#include <type_traits>
#include <utility>

template <typename T>
concept CompleteType = requires {
    { sizeof(T) } -> std::same_as<std::size_t>;
};

template <typename T>
concept IncompleteType = !CompleteType<T>;

template <typename T>
concept DefaultInit = std::default_initializable<T>;

template <typename P>
concept NoExceptDeref = requires(P p) {
    { *p } noexcept;
};

template <typename T>
concept NoExceptSwap = std::is_nothrow_swappable_v<T>;

template <typename T>
concept NotPointer = !std::is_pointer_v<T>;

template <typename T>
concept NotRvalueReference = !std::is_rvalue_reference_v<T>;

template <typename T>
concept NotFunction = !std::is_function_v<T>;

template <typename T>
concept NotArray = !std::is_array_v<T>;

template <typename T>
concept ArrayUnknownBound = std::is_array_v<T> && IncompleteType<T>;

template <typename From, typename To>
concept ConvertiblePointer = std::convertible_to<From*, To*>;

template <typename From, typename To>
concept ConvertiblePointerToArray = std::convertible_to<From (*)[], To (*)[]>;

template <typename D, typename V>
concept Deleter = requires(D deleter, std::remove_extent_t<V>* pointer) {
    { deleter(pointer) } noexcept;
};

template <typename From, typename To>
concept ConvertibleDeleter = (std::is_reference_v<To> && std::same_as<From, To>) ||
                             (!std::is_reference_v<To> && std::convertible_to<From, To>);

template <typename L, typename R>
concept AssignableDeleter = std::is_assignable_v<L, R>;

template <typename T>
    requires NotFunction<T>
struct DefaultDeleter {

    DefaultDeleter() noexcept = default;

    template <typename U>
        requires ConvertiblePointer<U, T>
    DefaultDeleter(const DefaultDeleter<U>&) noexcept {}

    void operator()(T* pointer) const noexcept
        requires CompleteType<T>
    {
        delete pointer;
    }
};

template <typename T>
struct DefaultDeleter<T[]> {
    DefaultDeleter() noexcept = default;

    template <typename U>
        requires ConvertiblePointerToArray<U, T>
    DefaultDeleter(const DefaultDeleter<U[]>&) noexcept {}

    template <typename U>
        requires ConvertiblePointerToArray<U, T> && CompleteType<U>
    void operator()(U* pointer) const noexcept {
        delete[] pointer;
    }
};

template <typename ValueT, typename DeleterT = DefaultDeleter<ValueT>>
    requires Deleter<DeleterT, ValueT> &&
             DefaultInit<DeleterT> &&
             NotRvalueReference<DeleterT> &&
             NotPointer<DeleterT>
class UniquePointer {
public:
    using ValueType = ValueT;
    using DeleterType = DeleterT;

private:
    ValueT* _pointer = nullptr;
    DeleterT _deleter;

public:
    UniquePointer() noexcept {}

    UniquePointer(nullptr_t) noexcept {}

    UniquePointer(ValueT* pointer) noexcept
        : _pointer(pointer) {}

    UniquePointer(const UniquePointer&) = delete;

    UniquePointer(UniquePointer&& other) noexcept
        : _pointer(other.release())
        , _deleter(std::forward<DeleterT>(other.getDeleter())) {}

    template <typename ValueU, typename DeleterU>
        requires Deleter<DeleterU, ValueU> &&
                     ConvertiblePointer<ValueU, ValueT> &&
                     ConvertibleDeleter<DeleterU, DeleterT>
    UniquePointer(UniquePointer<ValueU, DeleterU>&& other) noexcept
        : _pointer(other.release())
        , _deleter(std::forward<DeleterU>(other.getDeleter())) {}

    ~UniquePointer() noexcept {
        reset();
    }

    UniquePointer& operator=(nullptr_t) noexcept {
        reset();
        return *this;
    }

    UniquePointer& operator=(const UniquePointer&) = delete;

    UniquePointer& operator=(UniquePointer&& other) noexcept {
        reset(other.release());
        _deleter = std::forward<DeleterT>(other.getDeleter());
        return *this;
    }

    template <typename ValueU, typename DeleterU>
        requires Deleter<DeleterU, ValueU> &&
                 ConvertiblePointer<ValueU, ValueT> &&
                 AssignableDeleter<DeleterT, DeleterU>
    UniquePointer& operator=(UniquePointer<ValueU, DeleterU>&& other) noexcept {
        reset(other.release());
        _deleter = std::forward<DeleterU>(other.getDeleter());
        return *this;
    }

    ValueT* release() noexcept {
        ValueT* pointer = _pointer;
        _pointer = nullptr;
        return pointer;
    }

    void reset(ValueT* pointer = nullptr) noexcept {
        ValueT* oldPointer = _pointer;
        _pointer = pointer;
        if (oldPointer != nullptr) {
            _deleter(oldPointer);
        }
    }

    void swap(UniquePointer& other) noexcept(NoExceptSwap<ValueT*> && NoExceptSwap<DeleterT>) {
        std::swap(_pointer, other._pointer);
        std::swap(_deleter, other._deleter);
    }

    ValueT* get() const noexcept {
        return _pointer;
    }

    DeleterT& getDeleter() noexcept {
        return _deleter;
    }

    const DeleterT& getDeleter() const noexcept {
        return _deleter;
    }

    explicit operator bool() const noexcept {
        return _pointer != nullptr;
    }

    std::add_lvalue_reference_t<ValueT> operator*() const noexcept(NoExceptDeref<ValueT*> || std::is_void_v<ValueT>) {
        return *_pointer;
    }

    ValueT* operator->() const noexcept {
        return _pointer;
    }
};

template <typename ValueT, typename DeleterT>
class UniquePointer<ValueT[], DeleterT> {
public:
    using ValueType = ValueT;
    using DeleterType = DeleterT;

private:
    ValueT* _pointer = nullptr;
    DeleterT _deleter;

public:
    UniquePointer() noexcept {}

    UniquePointer(nullptr_t) noexcept {}

    template <typename ValueU>
        requires ConvertiblePointerToArray<ValueU, ValueT>
    UniquePointer(ValueU* pointer) noexcept
        : _pointer(pointer) {}

    UniquePointer(const UniquePointer&) = delete;

    UniquePointer(UniquePointer&& other) noexcept
        : _pointer(other.release())
        , _deleter(std::forward<DeleterT>(other.getDeleter())) {}

    template <typename ValueU, typename DeleterU>
        requires ConvertiblePointerToArray<ValueU, ValueT> &&
                     Deleter<DeleterU, ValueU[]> &&
                     ConvertibleDeleter<DeleterU, DeleterT>
    UniquePointer(UniquePointer<ValueU[], DeleterU>&& other) noexcept
        : _pointer(other.release())
        , _deleter(std::forward<DeleterU>(other.getDeleter())) {}

    ~UniquePointer() noexcept {
        reset();
    }

    UniquePointer& operator=(nullptr_t) noexcept {
        reset();
        return *this;
    }

    UniquePointer& operator=(const UniquePointer&) = delete;

    UniquePointer& operator=(UniquePointer&& other) noexcept {
        reset(other.release());
        _deleter = std::forward<DeleterT>(other.getDeleter());
        return *this;
    }

    template <typename ValueU, typename DeleterU>
        requires ConvertiblePointerToArray<ValueU, ValueT> &&
                 Deleter<DeleterU, ValueU[]> &&
                 AssignableDeleter<DeleterT, DeleterU>
    UniquePointer& operator=(UniquePointer<ValueU[], DeleterU>&& other) noexcept {
        reset(other.release());
        _deleter = std::forward<DeleterU>(other.getDeleter());
        return *this;
    }

    ValueT* release() noexcept {
        ValueT* pointer = _pointer;
        _pointer = nullptr;
        return pointer;
    }

    template <typename ValueU>
        requires ConvertiblePointerToArray<ValueU, ValueT>
    void reset(ValueU* pointer) noexcept {
        ValueT* oldPointer = _pointer;
        _pointer = pointer;
        if (oldPointer != nullptr) {
            _deleter(oldPointer);
        }
    }

    void reset(nullptr_t = nullptr) noexcept {
        ValueT* oldPointer = _pointer;
        _pointer = nullptr;
        if (oldPointer != nullptr) {
            _deleter(oldPointer);
        }
    }

    void swap(UniquePointer& other) noexcept(NoExceptSwap<ValueT*> && NoExceptSwap<DeleterT>) {
        std::swap(_pointer, other._pointer);
        std::swap(_deleter, other._deleter);
    }

    ValueT* get() const noexcept {
        return _pointer;
    }

    DeleterT& getDeleter() noexcept {
        return _deleter;
    }

    const DeleterT& getDeleter() const noexcept {
        return _deleter;
    }

    explicit operator bool() const noexcept {
        return _pointer != nullptr;
    }

    std::add_lvalue_reference_t<ValueT> operator[](size_t index) const {
        return _pointer[index];
    }
};

template <typename T, typename... Args>
    requires NotArray<T>
inline UniquePointer<T> makeUnique(Args&&... args) {
    return new T(std::forward<Args>(args)...);
}

template <typename T>
    requires ArrayUnknownBound<T>
inline UniquePointer<T> makeUnique(size_t n) {
    return new std::remove_extent_t<T>[n]();
}

#endif // POINTERS_HPP