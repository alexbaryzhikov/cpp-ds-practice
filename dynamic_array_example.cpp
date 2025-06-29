#include <print>
#include "dynamic_array.hpp"

template <typename T>
void info(const DArray<T>& a) {
    std::println("size: {}, capacity: {}", a.size(), a.capacity());
    std::println("{}", a);
}

int main() {
    DArray<int> a;
    info(a);

    DArray<int> b(2);
    info(b);

    DArray<int> c(7, 3);
    info(c);

    int nums[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    DArray<int> d(nums + 3, nums + 8);
    info(d);

    DArray<int> e = {1, 2, 3, 4, 5, 6, 7};
    info(e);

    std::print("x2: ");
    for (auto x : e) {
        std::print("{} ", x * 2);
    }
    std::println();
}