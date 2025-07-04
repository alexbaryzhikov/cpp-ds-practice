#include <print>
#include "dynamic_array.hpp"

template <typename T>
void print_info(const DArray<T>& a) {
    std::println("size: {}, capacity: {}", a.size(), a.capacity());
    std::println("{}", a);
}

int main() {
    DArray<int> a;
    print_info(a);

    DArray<int> b(2);
    print_info(b);

    DArray<int> c(7, 3);
    print_info(c);

    int nums[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    DArray<int> d(nums + 3, nums + 8);
    print_info(d);

    DArray<int> e = {1, 2, 3, 4, 5, 6, 7};
    print_info(e);

    std::print("x2: ");
    for (auto x : e) {
        std::print("{} ", x * 2);
    }
    std::println();

    DArray<int> f(c);
    f = e;
    print_info(f);
}