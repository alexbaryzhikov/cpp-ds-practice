#include "unique_pointer.hpp"
#include <print>

int main() {
    UniquePointer<int> p = new int(42);
    std::println("Pointer value: {}", *p);
}