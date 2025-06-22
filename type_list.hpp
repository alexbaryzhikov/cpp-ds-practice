#ifndef TYPE_LIST_HPP
#define TYPE_LIST_HPP

#include <type_traits>
#include <cstddef>
#include <tuple>

template<typename... Types>
class TypeList {
public:
    static constexpr std::size_t size = sizeof...(Types);

private:
    using tuple_type = std::tuple<Types...>;

public:
    // Get type at index N
    template<std::size_t N>
    using type_at_t = std::tuple_element_t<N, tuple_type>;

    // Check if list contains type T
    template<typename T>
    struct contains {
        static constexpr bool value = (std::is_same_v<T, Types> || ...);
    };

    template<typename T>
    static constexpr bool contains_v = contains<T>::value;

    // Get index of type T
    template<typename T>
    struct index_of {
        template<typename First, typename... Rest>
        static constexpr std::size_t get_index(std::size_t current = 0) {
            if constexpr (std::is_same_v<T, First>)
                return current;
            else if constexpr (sizeof...(Rest) > 0)
                return get_index<Rest...>(current + 1);
            else
                return -1;
        }

        static constexpr std::size_t value = get_index<Types...>();
    };

    template<typename T>
    static constexpr std::size_t index_of_v = index_of<T>::value;

    // Add type to front
    template<typename T>
    using push_front_t = TypeList<T, Types...>;

    // Add type to back
    template<typename T>
    using push_back_t = TypeList<Types..., T>;

    // Remove first occurrence of type T
    template<typename T>
    struct remove {
    private:
        template<typename... Accumulated>
        struct remove_impl {
            template<typename First, typename... Rest>
            static auto apply() {
                if constexpr (std::is_same_v<T, First>) {
                    if constexpr (sizeof...(Rest) > 0)
                        return std::type_identity<TypeList<Accumulated..., Rest...>>{};
                    else
                        return std::type_identity<TypeList<Accumulated...>>{};
                }
                else if constexpr (sizeof...(Rest) > 0)
                    return remove_impl<Accumulated..., First>::template apply<Rest...>();
                else
                    return std::type_identity<TypeList<Accumulated..., First>>{};
            }
        };

    public:
        using type = typename decltype(remove_impl<>::template apply<Types...>())::type;
    };

    template<typename T>
    using remove_t = typename remove<T>::type;

    // Remove first type from the list
    struct pop_front {
    private:
        template<typename First, typename... Rest>
        static auto apply() {
            return std::type_identity<TypeList<Rest...>>{};
        }

    public:
        using type = typename decltype(apply<Types...>())::type;
    };

    using pop_front_t = typename pop_front::type;

    // Remove last type from the list
    struct pop_back {
    private:
        template<typename... Accumulated>
        struct pop_back_impl {
            template<typename First, typename... Rest>
            static auto apply() {
                if constexpr (sizeof...(Rest) == 0)
                    return std::type_identity<TypeList<Accumulated...>>{};
                else
                    return pop_back_impl<Accumulated..., First>::template apply<Rest...>();
            }
        };

    public:
        using type = typename decltype(pop_back_impl<>::template apply<Types...>())::type;
    };

    using pop_back_t = typename pop_back::type;

    // Compare two type lists for equality
    template<typename... OtherTypes>
    friend constexpr bool operator==(const TypeList&, const TypeList<OtherTypes...>&) {
        if constexpr (sizeof...(Types) != sizeof...(OtherTypes)) {
            return false;
        } else if constexpr (sizeof...(Types) == 0) {
            return true;
        } else {
            return std::is_same_v<TypeList<Types...>, TypeList<OtherTypes...>>;
        }
    }
};

#endif // TYPE_LIST_HPP 