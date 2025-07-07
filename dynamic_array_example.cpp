#include "dynamic_array.hpp"
#include <print>

template <typename T>
void print_info(const DArray<T>& a) {
    std::println("size: {:2}, capacity: {:2}, data: {}", a.size(), a.capacity(), a);
}

int main() {
    DArray<int> a;
    std::print("A) ");
    print_info(a);

    DArray<int> b(2);
    std::print("B) ");
    print_info(b);

    DArray<int> c(7, 3);
    std::print("C) ");
    print_info(c);

    int nums[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    DArray<int> d(nums + 3, nums + 8);
    std::print("D) ");
    print_info(d);

    DArray<int> e = {1, 2, 3, 4, 5, 6, 7};
    std::print("E) ");
    print_info(e);

    DArray<int> e2;
    for (auto x : e) {
        e2.push_back(x * 2);
    }
    std::print("Ex2) ");
    print_info(e2);

    DArray<int> f(c);
    f = e;
    std::print("F) ");
    print_info(f);

    DArray<int> g;
    std::println("G)");
    print_info(g);
    for (int i = 1; i < 10; ++i) {
        g.push_back(i);
        print_info(g);
    }
}