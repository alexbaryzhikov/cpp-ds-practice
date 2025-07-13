#include "dynamic_array.hpp"
#include <print>

template <typename T>
void print_info(const DArray<T>& a) {
    std::println("size: {:2}, capacity: {:2}, data: {}", a.size(), a.capacity(), a);
}

int main() {
    std::println("Default constructor");
    DArray<int> arr1;
    print_info(arr1);

    std::println("Size constructor");
    DArray<int> arr2(2);
    print_info(arr2);

    std::println("Fill constructor");
    DArray<int> arr3(7, 3);
    print_info(arr3);

    std::println("Range constructor");
    const int nums[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    DArray<int> arr4(nums + 3, nums + 8);
    print_info(arr4);

    std::println("Initializer list constructor");
    DArray<int> arr5 = {1, 2, 3, 4, 5};
    print_info(arr5);

    std::println("Copy constructor");
    DArray<int> arr6 = arr5;
    print_info(arr5);
    print_info(arr6);

    std::println("Move constructor");
    DArray<int> arr7 = std::move(arr6);
    print_info(arr6);
    print_info(arr7);

    std::println("Copy assignment");
    DArray<int> arr8;
    arr8 = arr7;
    print_info(arr7);
    print_info(arr8);

    std::println("Move assignment");
    DArray<int> arr9;
    arr9 = std::move(arr8);
    print_info(arr8);
    print_info(arr9);

    std::println("Subscript operator");
    DArray<int> arr10 = {1, 2, 3, 4, 5};
    arr10[0] = 255;
    print_info(arr10);

    std::println("Iterator");
    DArray<int> arr11 = {1, 2, 3, 4, 5};
    DArray<int> arr12;
    for (auto x : arr11) {
        arr12.push(x * 2);
    }
    print_info(arr12);

    std::println("Reverse iterator");
    DArray<int> arr13 = {1, 2, 3, 4, 5};
    DArray<int> arr14;
    for (auto it = arr13.rbegin(); it != arr13.rend(); ++it) {
        arr14.push(*it);
    }
    print_info(arr14);

    std::println("Capacity");
    DArray<int> arr15;
    print_info(arr15);
    for (int i = 0; i < 5; ++i) {
        arr15.push(i + 1);
        print_info(arr15);
    }

    std::println("Shrink to fit");
    print_info(arr15);
    arr15.shrinkToFit();
    print_info(arr15);

    std::println("Insert");
    DArray<int> arr16 = {1, 2, 3, 4, 5};
    print_info(arr16);
    arr16.insert(arr16.begin() + 2, 99);
    print_info(arr16);

    std::println("Erase");
    print_info(arr16);
    arr16.erase(arr16.end() - 1);
    print_info(arr16);
    arr16.erase(arr16.begin() + 2);
    print_info(arr16);

    std::println("Fill and range insert");
    const int nums2[] = {1, 2, 3};
    DArray<int> arr17 = {4, 5, 6, 7, 8};
    print_info(arr17);
    arr17.insert(arr17.end(), 9, 3);
    print_info(arr17);
    arr17.insert(arr17.begin(), nums2, nums2 + 3);
    print_info(arr17);

    std::println("Range erase");
    DArray<int> arr18 = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    print_info(arr18);
    arr18.erase(arr18.begin() + 7, arr18.end());
    print_info(arr18);
    arr18.erase(arr18.begin(), arr18.begin() + 4);
    print_info(arr18);

    std::println("Swap");
    DArray<int> arr19 = {1, 2, 3, 4, 5};
    DArray<int> arr20 = {6, 7, 8, 9};
    print_info(arr19);
    print_info(arr20);
    arr19.swap(arr20);
    print_info(arr19);
    print_info(arr20);
}