#include "dynamic_array.hpp"
#include <gtest/gtest.h>
#include <initializer_list>
#include <new>
#include <stdexcept>
#include <limits>

// Test allocator that can throw during allocation
struct TestAlloc {
    static int allocationCount;
    static int deallocationCount;
    static int allocationFails;
    
    void* allocate(size_t count, std::align_val_t alignment) {
        if (allocationFails) {
            throw std::bad_alloc();
        }
        ++allocationCount;
        return ::operator new(count, alignment);
    }

    void deallocate(void* pointer, std::align_val_t alignment) noexcept {
        ++deallocationCount;
        ::operator delete(pointer, alignment, std::nothrow);
    }

    static void reset() {
        allocationCount = 0;
        deallocationCount = 0;
        allocationFails = false;
    }
};

int TestAlloc::allocationCount = 0;
int TestAlloc::deallocationCount = 0;
int TestAlloc::allocationFails = false;

// Test class that can throw during construction
struct Probe {
    static int constructionCount;
    static int destructionCount;
    static int constructorThrowsAt;

    int id;

    Probe(): id(0)  {
        if (constructionCount + 1 == constructorThrowsAt) {
            throw std::runtime_error("Construction failed");
        }
        ++constructionCount;
    }

    Probe(int id): id(id) {
        // Used for test values initialization
    }
    
    Probe(const Probe& other): id(other.id) {
        if (constructionCount + 1 == constructorThrowsAt) {
            throw std::runtime_error("Copy construction failed");
        }
        ++constructionCount;
    }
    
    Probe& operator=(const Probe& other) {
        id = other.id;
        if (constructionCount + 1 == constructorThrowsAt) {
            throw std::runtime_error("Copy assignment failed");
        }
        ++constructionCount;
        return *this;
    }
    
    ~Probe() {
        ++destructionCount;
    }
    
    static void reset() {
        constructionCount = 0;
        destructionCount = 0;
        constructorThrowsAt = -1;
    }
};

int Probe::constructionCount = 0;
int Probe::destructionCount = 0;
int Probe::constructorThrowsAt = -1;

class DArrayTest : public ::testing::Test {
protected:
    void SetUp() override {
        TestAlloc::reset();
        Probe::reset();
    }
};

using DArrayType = DArray<Probe, TestAlloc>;

// Test size constructor with allocation failure
TEST_F(DArrayTest, SizeConstructorAllocationFailure) {
    TestAlloc::allocationFails = true;

    EXPECT_THROW({
        DArrayType arr(10);
    }, std::bad_alloc);

    EXPECT_EQ(TestAlloc::allocationCount, 0);
    EXPECT_EQ(TestAlloc::deallocationCount, 0);
    EXPECT_EQ(Probe::constructionCount, 0);
    EXPECT_EQ(Probe::destructionCount, 0);
}

// Test size constructor with element construction failure
TEST_F(DArrayTest, SizeConstructorElementFailure) {
    Probe::constructorThrowsAt = 5;
    
    EXPECT_THROW({
        DArrayType arr(10);
    }, std::runtime_error);
    
    EXPECT_EQ(TestAlloc::allocationCount, 1);
    EXPECT_EQ(TestAlloc::deallocationCount, 1);
    EXPECT_EQ(Probe::constructionCount, 4);
    EXPECT_EQ(Probe::destructionCount, 4);
}

// Test fill constructor with allocation failure
TEST_F(DArrayTest, FillConstructorAllocationFailure) {
    TestAlloc::allocationFails = true;
    Probe value = 42;

    EXPECT_THROW({
        DArrayType arr(value, 10);
    }, std::bad_alloc);

    EXPECT_EQ(TestAlloc::allocationCount, 0);
    EXPECT_EQ(TestAlloc::deallocationCount, 0);
    EXPECT_EQ(Probe::constructionCount, 0);
    EXPECT_EQ(Probe::destructionCount, 0);
}

// Test fill constructor with element construction failure
TEST_F(DArrayTest, FillConstructorElementFailure) {
    Probe::constructorThrowsAt = 5;
    Probe value = 42;

    EXPECT_THROW({
        DArrayType arr(value, 10);
    }, std::runtime_error);
    
    EXPECT_EQ(TestAlloc::allocationCount, 1);
    EXPECT_EQ(TestAlloc::deallocationCount, 1);
    EXPECT_EQ(Probe::constructionCount, 4);
    EXPECT_EQ(Probe::destructionCount, 4);
}

// Test range constructor with allocation failure
TEST_F(DArrayTest, RangeConstructorAllocationFailure) {
    TestAlloc::allocationFails = true;
    
    Probe data[] = {1, 2, 3, 4, 5};
    EXPECT_THROW({
        DArrayType arr(data, data + 5);
    }, std::bad_alloc);
    
    EXPECT_EQ(TestAlloc::allocationCount, 0);
    EXPECT_EQ(TestAlloc::deallocationCount, 0);
    EXPECT_EQ(Probe::constructionCount, 0);
    EXPECT_EQ(Probe::destructionCount, 0);
}

// Test range constructor with element construction failure
TEST_F(DArrayTest, RangeConstructorElementFailure) {
    Probe::constructorThrowsAt = 5;
    Probe data[] = {1, 2, 3, 4, 5};

    EXPECT_THROW({
        DArrayType arr(data, data + 5);
    }, std::runtime_error);
    
    EXPECT_EQ(TestAlloc::allocationCount, 1);
    EXPECT_EQ(TestAlloc::deallocationCount, 1);
    EXPECT_EQ(Probe::constructionCount, 4);
    EXPECT_EQ(Probe::destructionCount, 4);
}

// Test initializer list constructor with allocation failure
TEST_F(DArrayTest, InitializerListConstructorAllocationFailure) {
    TestAlloc::allocationFails = true;
    std::initializer_list<Probe> il = {1, 2, 3, 4, 5};

    EXPECT_THROW({
        DArrayType arr(il);
    }, std::bad_alloc);
    
    EXPECT_EQ(TestAlloc::allocationCount, 0);
    EXPECT_EQ(TestAlloc::deallocationCount, 0);
    EXPECT_EQ(Probe::constructionCount, 0);
    EXPECT_EQ(Probe::destructionCount, 0);
}

// Test initializer list constructor with element construction failure
TEST_F(DArrayTest, InitializerListConstructorElementFailure) {
    Probe::constructorThrowsAt = 5;
    std::initializer_list<Probe> il = {1, 2, 3, 4, 5};

    EXPECT_THROW({
        DArrayType arr(il);
    }, std::runtime_error);
    
    EXPECT_EQ(TestAlloc::allocationCount, 1);
    EXPECT_EQ(TestAlloc::deallocationCount, 1);
    EXPECT_EQ(Probe::constructionCount, 4);
    EXPECT_EQ(Probe::destructionCount, 4);
}

// Test that successful construction works correctly
TEST_F(DArrayTest, SuccessfulConstruction) {
    // Test default constructor
    DArray<int> arr;
    EXPECT_EQ(arr.size(), 0);
    EXPECT_EQ(arr.capacity(), 0);
    EXPECT_TRUE(arr.empty());

    // Test size constructor
    DArray<int> arr1(5);
    EXPECT_EQ(arr1.size(), 5);
    EXPECT_EQ(arr1.capacity(), 5);
    
    // Test fill constructor
    DArray<int> arr2(42, 3);
    EXPECT_EQ(arr2.size(), 3);
    EXPECT_EQ(arr2.capacity(), 3);
    EXPECT_EQ(arr2[0], 42);
    EXPECT_EQ(arr2[1], 42);
    EXPECT_EQ(arr2[2], 42);
    
    // Test range constructor
    int data[] = {1, 2, 3};
    DArray<int> arr3(data, data + 3);
    EXPECT_EQ(arr3.size(), 3);
    EXPECT_EQ(arr3.capacity(), 3);
    EXPECT_EQ(arr3[0], 1);
    EXPECT_EQ(arr3[1], 2);
    EXPECT_EQ(arr3[2], 3);
    
    // Test initializer list constructor
    DArray<int> arr4 = {10, 20, 30};
    EXPECT_EQ(arr4.size(), 3);
    EXPECT_EQ(arr4.capacity(), 3);
    EXPECT_EQ(arr4[0], 10);
    EXPECT_EQ(arr4[1], 20);
    EXPECT_EQ(arr4[2], 30);
}

// Test that destructor properly cleans up
TEST_F(DArrayTest, DestructorCleanup) {
    {
        DArray<Probe> arr(5);

        EXPECT_EQ(Probe::constructionCount, 5);
        EXPECT_EQ(Probe::destructionCount, 0);
    }
    
    EXPECT_EQ(Probe::destructionCount, 5);
}

// Test with zero size (edge case)
TEST_F(DArrayTest, ZeroSizeConstruction) {
    DArray<int> arr1(0);
    EXPECT_EQ(arr1.size(), 0);
    EXPECT_EQ(arr1.capacity(), 0);
    
    DArray<int> arr2(42, 0);
    EXPECT_EQ(arr2.size(), 0);
    EXPECT_EQ(arr2.capacity(), 0);
    
    int data[] = {1, 2, 3};
    DArray<int> arr3(data, data); // Empty range
    EXPECT_EQ(arr3.size(), 0);
    EXPECT_EQ(arr3.capacity(), 0);
    
    DArray<int> arr4 = {}; // Empty initializer list
    EXPECT_EQ(arr4.size(), 0);
    EXPECT_EQ(arr4.capacity(), 0);
}

// Test with very large size
TEST_F(DArrayTest, LargeSizeConstruction) {
    size_t large_size = std::numeric_limits<size_t>::max() / sizeof(int) + 1;
    
    EXPECT_THROW({
        DArray<int> arr(large_size);
    }, std::bad_alloc);
}

// Test that clear() works correctly
TEST_F(DArrayTest, Clear) {
    DArray<int> arr = {1, 2, 3, 4, 5};
    EXPECT_EQ(arr.size(), 5);
    
    arr.clear();
    EXPECT_EQ(arr.size(), 0);
    EXPECT_EQ(arr.capacity(), 5); // Capacity should remain
}

// Test that iterators work correctly
TEST_F(DArrayTest, Iterators) {
    DArray<int> arr = {1, 2, 3, 4, 5};
    
    int sum = 0;
    for (auto it = arr.begin(); it != arr.end(); ++it) {
        sum += *it;
    }
    EXPECT_EQ(sum, 15);
    
    // Test const iterators
    const DArray<int>& const_arr = arr;
    sum = 0;
    for (auto it = const_arr.begin(); it != const_arr.end(); ++it) {
        sum += *it;
    }
    EXPECT_EQ(sum, 15);
}

// Test that front() and back() work correctly
TEST_F(DArrayTest, FrontAndBack) {
    DArray<int> arr = {1, 2, 3, 4, 5};
    
    EXPECT_EQ(arr.front(), 1);
    EXPECT_EQ(arr.back(), 5);
    
    const DArray<int>& const_arr = arr;
    EXPECT_EQ(const_arr.front(), 1);
    EXPECT_EQ(const_arr.back(), 5);
}

// Test that operator[] works correctly
TEST_F(DArrayTest, SubscriptOperator) {
    DArray<int> arr = {1, 2, 3, 4, 5};
    
    EXPECT_EQ(arr[0], 1);
    EXPECT_EQ(arr[2], 3);
    EXPECT_EQ(arr[4], 5);
    
    arr[2] = 42;
    EXPECT_EQ(arr[2], 42);
    
    const DArray<int>& const_arr = arr;
    EXPECT_EQ(const_arr[0], 1);
    EXPECT_EQ(const_arr[2], 42);
}

// Test that empty() works correctly
TEST_F(DArrayTest, Empty) {
    DArray<int> arr1;
    EXPECT_TRUE(arr1.empty());
    
    DArray<int> arr2 = {1, 2, 3};
    EXPECT_FALSE(arr2.empty());
    
    arr2.clear();
    EXPECT_TRUE(arr2.empty());
}

// Test that size() and capacity() work correctly
TEST_F(DArrayTest, SizeAndCapacity) {
    DArray<int> arr1;
    EXPECT_EQ(arr1.size(), 0);
    EXPECT_EQ(arr1.capacity(), 0);
    
    DArray<int> arr2(10);
    EXPECT_EQ(arr2.size(), 10);
    EXPECT_EQ(arr2.capacity(), 10);
    
    DArray<int> arr3 = {1, 2, 3};
    EXPECT_EQ(arr3.size(), 3);
    EXPECT_EQ(arr3.capacity(), 3);
}

// Test that maxSize() works correctly
TEST_F(DArrayTest, MaxSize) {
    DArray<int> arr;
    EXPECT_EQ(arr.maxSize(), std::numeric_limits<size_t>::max() / sizeof(int));
    
    DArray<double> arr_double;
    EXPECT_EQ(arr_double.maxSize(), std::numeric_limits<size_t>::max() / sizeof(double));
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
} 