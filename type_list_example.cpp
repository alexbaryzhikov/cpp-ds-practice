#include <iostream>
#include <string>
#include "type_list.hpp"

// Helper to print type names
template<typename T>
constexpr const char* type_name() {
    if constexpr (std::is_same_v<T, int>)
        return "int";
    else if constexpr (std::is_same_v<T, double>)
        return "double";
    else if constexpr (std::is_same_v<T, char>)
        return "char";
    else if constexpr (std::is_same_v<T, float>)
        return "float";
    else if constexpr (std::is_same_v<T, bool>)
        return "bool";
    else
        return "unknown";
}

int main() {
    // Create a type list with some types
    using MyList = TypeList<int, double, char, float>;
    
    std::cout << "Original list:\n";
    std::cout << "Size: " << MyList::size << '\n';
    
    // Demonstrate type_at
    using FirstType = MyList::type_at_t<0>;
    using SecondType = MyList::type_at_t<1>;
    std::cout << "\nAccessing types by index:\n";
    std::cout << "Type at index 0: " << type_name<FirstType>() << '\n';
    std::cout << "Type at index 1: " << type_name<SecondType>() << '\n';
    
    // Demonstrate contains
    std::cout << "\nType checking:\n";
    std::cout << "Contains int: " << std::boolalpha << MyList::contains_v<int> << '\n';
    std::cout << "Contains bool: " << MyList::contains_v<bool> << '\n';
    
    // Demonstrate index_of
    std::cout << "\nFinding type indices:\n";
    std::cout << "Index of double: " << MyList::index_of_v<double> << '\n';
    std::cout << "Index of bool (not found): " << MyList::index_of_v<bool> << '\n';
    
    // Demonstrate push_front
    using ListWithBool = MyList::push_front_t<bool>;
    std::cout << "\nAfter push_front<bool>:\n";
    std::cout << "New size: " << ListWithBool::size << '\n';
    std::cout << "New first type: " << type_name<ListWithBool::type_at_t<0>>() << '\n';
    
    // Demonstrate push_back
    using ListWithBoolAtEnd = MyList::push_back_t<bool>;
    std::cout << "\nAfter push_back<bool>:\n";
    std::cout << "New size: " << ListWithBoolAtEnd::size << '\n';
    std::cout << "Last type: " << type_name<ListWithBoolAtEnd::type_at_t<ListWithBoolAtEnd::size - 1>>() << '\n';
    
    // Demonstrate remove
    using ListWithoutDouble = MyList::remove_t<double>;
    std::cout << "\nAfter removing double:\n";
    std::cout << "New size: " << ListWithoutDouble::size << '\n';
    std::cout << "Type at index 1 (was double): " << type_name<ListWithoutDouble::type_at_t<1>>() << '\n';
    
    // Demonstrate pop_front
    using ListWithoutFirst = MyList::pop_front_t;
    std::cout << "\nAfter pop_front:\n";
    std::cout << "New size: " << ListWithoutFirst::size << '\n';
    std::cout << "New first type: " << type_name<ListWithoutFirst::type_at_t<0>>() << '\n';
    
    // Demonstrate pop_back
    using ListWithoutLast = MyList::pop_back_t;
    std::cout << "\nAfter pop_back:\n";
    std::cout << "New size: " << ListWithoutLast::size << '\n';
    std::cout << "New last type: " << type_name<ListWithoutLast::type_at_t<ListWithoutLast::size - 1>>() << '\n';
    
    // Demonstrate operator==
    using SameAsList = TypeList<int, double, char, float>;
    using DifferentList = TypeList<double, int, char, float>;
    
    std::cout << "\nComparing type lists:\n";
    std::cout << "MyList == SameAsList: " << std::boolalpha 
              << (MyList{} == SameAsList{}) << '\n';
    std::cout << "MyList == DifferentList: " << 
              (MyList{} == DifferentList{}) << '\n';
    
    return 0;
} 