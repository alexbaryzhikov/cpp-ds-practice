#include "unique_pointer.hpp"
#include <print>

template <typename T, typename ValueT = T::ValueType, typename DeleterT = T::DeleterType>
void inspect(const T&) {
    std::println("{}", __PRETTY_FUNCTION__);
}

struct Point {
    double x;
    double y;
};

int main() {
    UniquePointer<int> intPtr = makeUnique<int>(42);
    inspect(intPtr);

    UniquePointer<int[]> intArrPtr = makeUnique<int[]>(10);
    inspect(intArrPtr);

    UniquePointer<const int[]> constIntArrPtr = std::move(intArrPtr);
    inspect(constIntArrPtr);

    UniquePointer<Point> pointPtr = makeUnique<Point>(0.4, 0.9);
    std::println("Point: ({}, {})", pointPtr->x, pointPtr->y);
}