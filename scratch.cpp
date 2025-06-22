#include "dynamic_array.hpp"
#include <print>
#include <vector>

int main() {
    std::vector<int> v(5);
    std::println("size: {}, capacity: {}", v.size(), v.capacity());
    std::println("{}", v);

    DArray<int> a(5);
    std::println("size: {}, capacity: {}", a.size(), a.capacity());
}
