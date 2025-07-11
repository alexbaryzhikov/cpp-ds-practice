#include "dynamic_array.hpp"
#include <gtest/gtest.h>
#include <initializer_list>
#include <limits>
#include <new>
#include <print>
#include <stdexcept>

// Test allocator that can throw during allocation
struct TestAllocator {
    static int allocationCount;
    static int deallocationCount;
    static int allocationThrowsAt;

    void* allocate(size_t count, std::align_val_t alignment) const {
        if (allocationCount + 1 == allocationThrowsAt) {
            throw std::bad_alloc();
        }
        ++allocationCount;
        return ::operator new(count, alignment);
    }

    void deallocate(void* pointer, std::align_val_t alignment) const noexcept {
        ++deallocationCount;
        ::operator delete(pointer, alignment, std::nothrow);
    }

    static void reset() {
        allocationCount = 0;
        deallocationCount = 0;
        allocationThrowsAt = -1;
    }
};

int TestAllocator::allocationCount = 0;
int TestAllocator::deallocationCount = 0;
int TestAllocator::allocationThrowsAt = -1;

// Test class that can throw during construction
struct Probe {
    static int constructionCount;
    static int destructionCount;
    static int constructorThrowsAt;
    static bool verbose;

    int id;

    Probe()
        : id(0) {
        if (constructionCount + 1 == constructorThrowsAt) {
            if (verbose) {
                std::println("{:2}) Failed to default construct, id: {}", constructionCount + 1, id);
            }
            throw std::runtime_error("Construction failed");
        }
        ++constructionCount;
        if (verbose) {
            std::println("{:2}) Default constructed", constructionCount);
        }
    }

    Probe(int id)
        : id(id) {
        // Used for test values initialization
    }

    Probe(const Probe& other)
        : id(other.id) {
        if (constructionCount + 1 == constructorThrowsAt) {
            if (verbose) {
                std::println("{:2}) Failed to copy construct, id: {}", constructionCount + 1, id);
            }
            throw std::runtime_error("Copy construction failed");
        }
        ++constructionCount;
        if (verbose) {
            std::println("{:2}) Copy constructed, id: {}", constructionCount, id);
        }
    }

    Probe(Probe&& other) noexcept
        : id(other.id) {
        ++constructionCount;
        other.id = -1;
        if (verbose) {
            std::println("{:2}) Move constructed, id: {}", constructionCount, id);
        }
    }

    ~Probe() {
        ++destructionCount;
        if (verbose) {
            std::println("{:2}) Destructed, id: {}", destructionCount, id);
        }
    }

    Probe& operator=(const Probe& other) = delete;

    Probe& operator=(Probe&& other) noexcept = delete;

    bool operator==(const Probe& other) const = default;

    static void reset() {
        constructionCount = 0;
        destructionCount = 0;
        constructorThrowsAt = -1;
        verbose = false;
    }
};

int Probe::constructionCount = 0;
int Probe::destructionCount = 0;
int Probe::constructorThrowsAt = -1;
bool Probe::verbose = false;

class DArrayTest : public ::testing::Test {
protected:
    void SetUp() override {
        TestAllocator::reset();
        Probe::reset();
    }
};

using DArrayType = DArray<Probe, TestAllocator>;

// Test default constructor success
TEST_F(DArrayTest, DefaultConstructorSuccess) {
    DArray<int> arr;

    EXPECT_EQ(arr.size(), 0);
    EXPECT_EQ(arr.capacity(), 0);
    EXPECT_TRUE(arr.empty());
}

// Test size constructor success
TEST_F(DArrayTest, SizeConstructorSuccess) {
    DArray<int> arr1(5);

    EXPECT_EQ(arr1.size(), 5);
    EXPECT_EQ(arr1.capacity(), 5);
}

// Test size constructor with allocation failure
TEST_F(DArrayTest, SizeConstructorAllocationFailure) {
    TestAllocator::allocationThrowsAt = 1;

    EXPECT_THROW({ DArrayType arr(10); }, std::bad_alloc);

    EXPECT_EQ(TestAllocator::allocationCount, 0);
    EXPECT_EQ(TestAllocator::deallocationCount, 0);
    EXPECT_EQ(Probe::constructionCount, 0);
    EXPECT_EQ(Probe::destructionCount, 0);
}

// Test size constructor with element construction failure
TEST_F(DArrayTest, SizeConstructorElementFailure) {
    Probe::constructorThrowsAt = 5;

    EXPECT_THROW({ DArrayType arr(10); }, std::runtime_error);

    EXPECT_EQ(TestAllocator::allocationCount, 1);
    EXPECT_EQ(TestAllocator::deallocationCount, 1);
    EXPECT_EQ(Probe::constructionCount, 4);
    EXPECT_EQ(Probe::destructionCount, 4);
}

// Test fill constructor success
TEST_F(DArrayTest, FillConstructorSuccess) {
    DArray<int> arr2(42, 3);

    EXPECT_EQ(arr2.size(), 3);
    EXPECT_EQ(arr2.capacity(), 3);
    EXPECT_EQ(arr2[0], 42);
    EXPECT_EQ(arr2[1], 42);
    EXPECT_EQ(arr2[2], 42);
}

// Test fill constructor with allocation failure
TEST_F(DArrayTest, FillConstructorAllocationFailure) {
    TestAllocator::allocationThrowsAt = 1;
    Probe value = 42;

    EXPECT_THROW({ DArrayType arr(value, 10); }, std::bad_alloc);

    EXPECT_EQ(TestAllocator::allocationCount, 0);
    EXPECT_EQ(TestAllocator::deallocationCount, 0);
    EXPECT_EQ(Probe::constructionCount, 0);
    EXPECT_EQ(Probe::destructionCount, 0);
}

// Test fill constructor with element construction failure
TEST_F(DArrayTest, FillConstructorElementFailure) {
    Probe::constructorThrowsAt = 5;
    Probe value = 42;

    EXPECT_THROW({ DArrayType arr(value, 10); }, std::runtime_error);

    EXPECT_EQ(TestAllocator::allocationCount, 1);
    EXPECT_EQ(TestAllocator::deallocationCount, 1);
    EXPECT_EQ(Probe::constructionCount, 4);
    EXPECT_EQ(Probe::destructionCount, 4);
}

// Test range constructor success
TEST_F(DArrayTest, RangeConstructorSuccess) {
    int data[] = {1, 2, 3};

    DArray<int> arr3(data, data + 3);

    EXPECT_EQ(arr3.size(), 3);
    EXPECT_EQ(arr3.capacity(), 3);
    EXPECT_EQ(arr3[0], 1);
    EXPECT_EQ(arr3[1], 2);
    EXPECT_EQ(arr3[2], 3);
}

// Test range constructor with allocation failure
TEST_F(DArrayTest, RangeConstructorAllocationFailure) {
    Probe data[] = {1, 2, 3, 4, 5};
    TestAllocator::allocationThrowsAt = 1;

    EXPECT_THROW({ DArrayType arr(data, data + 5); }, std::bad_alloc);

    EXPECT_EQ(TestAllocator::allocationCount, 0);
    EXPECT_EQ(TestAllocator::deallocationCount, 0);
    EXPECT_EQ(Probe::constructionCount, 0);
    EXPECT_EQ(Probe::destructionCount, 0);
}

// Test range constructor with element construction failure
TEST_F(DArrayTest, RangeConstructorElementFailure) {
    Probe data[] = {1, 2, 3, 4, 5};
    Probe::constructorThrowsAt = 5;

    EXPECT_THROW({ DArrayType arr(data, data + 5); }, std::runtime_error);

    EXPECT_EQ(TestAllocator::allocationCount, 1);
    EXPECT_EQ(TestAllocator::deallocationCount, 1);
    EXPECT_EQ(Probe::constructionCount, 4);
    EXPECT_EQ(Probe::destructionCount, 4);
}

// Test initializer list constructor success
TEST_F(DArrayTest, InitializerListConstructorSuccess) {
    DArray<int> arr4 = {10, 20, 30};

    EXPECT_EQ(arr4.size(), 3);
    EXPECT_EQ(arr4.capacity(), 3);
    EXPECT_EQ(arr4[0], 10);
    EXPECT_EQ(arr4[1], 20);
    EXPECT_EQ(arr4[2], 30);
}

// Test initializer list constructor with allocation failure
TEST_F(DArrayTest, InitializerListConstructorAllocationFailure) {
    std::initializer_list<Probe> elements = {1, 2, 3, 4, 5};
    TestAllocator::allocationThrowsAt = 1;

    EXPECT_THROW({ DArrayType arr = elements; }, std::bad_alloc);

    EXPECT_EQ(TestAllocator::allocationCount, 0);
    EXPECT_EQ(TestAllocator::deallocationCount, 0);
    EXPECT_EQ(Probe::constructionCount, 0);
    EXPECT_EQ(Probe::destructionCount, 0);
}

// Test initializer list constructor with element construction failure
TEST_F(DArrayTest, InitializerListConstructorElementFailure) {
    std::initializer_list<Probe> elements = {1, 2, 3, 4, 5};
    Probe::constructorThrowsAt = 5;

    EXPECT_THROW({ DArrayType arr = elements; }, std::runtime_error);

    EXPECT_EQ(TestAllocator::allocationCount, 1);
    EXPECT_EQ(TestAllocator::deallocationCount, 1);
    EXPECT_EQ(Probe::constructionCount, 4);
    EXPECT_EQ(Probe::destructionCount, 4);
}

// Test initializer list assignment success
TEST_F(DArrayTest, InitializerListAssignmentSuccess) {
    std::initializer_list<Probe> elements1 = {1, 2, 3, 4, 5};
    std::initializer_list<Probe> elements2 = {6, 7, 8};
    {
        DArrayType arr = elements1;

        arr = elements2;

        EXPECT_EQ(arr.size(), 3);
        EXPECT_EQ(arr.capacity(), 3);
        EXPECT_EQ(arr[0].id, 6);
        EXPECT_EQ(arr[1].id, 7);
        EXPECT_EQ(arr[2].id, 8);
    }
    EXPECT_EQ(TestAllocator::allocationCount, 2);
    EXPECT_EQ(TestAllocator::deallocationCount, 2);
    EXPECT_EQ(Probe::constructionCount, 8);
    EXPECT_EQ(Probe::destructionCount, 8);
}

// Test initializer list assignment of empty array
TEST_F(DArrayTest, InitializerListAssignmentOfEmptyArray) {
    std::initializer_list<Probe> elements = {1, 2, 3, 4, 5};
    {
        DArrayType arr = elements;

        arr = {};

        EXPECT_EQ(arr.size(), 0);
        EXPECT_EQ(arr.capacity(), 5);
    }
    EXPECT_EQ(TestAllocator::allocationCount, 1);
    EXPECT_EQ(TestAllocator::deallocationCount, 1);
    EXPECT_EQ(Probe::constructionCount, 5);
    EXPECT_EQ(Probe::destructionCount, 5);
}

// Test initializer list assignment with allocation failure
TEST_F(DArrayTest, InitializerListAssignmentAllocationFailure) {
    std::initializer_list<Probe> elements1 = {1, 2, 3, 4, 5};
    std::initializer_list<Probe> elements2 = {6, 7, 8};
    TestAllocator::allocationThrowsAt = 2;
    {
        DArrayType arr = elements1;

        EXPECT_THROW({ arr = elements2; }, std::bad_alloc);

        EXPECT_EQ(arr.size(), 5);
        EXPECT_EQ(arr.capacity(), 5);
        EXPECT_EQ(arr[0].id, 1);
        EXPECT_EQ(arr[1].id, 2);
        EXPECT_EQ(arr[2].id, 3);
        EXPECT_EQ(arr[3].id, 4);
        EXPECT_EQ(arr[4].id, 5);
    }
    EXPECT_EQ(TestAllocator::allocationCount, 1);
    EXPECT_EQ(TestAllocator::deallocationCount, 1);
    EXPECT_EQ(Probe::constructionCount, 5);
    EXPECT_EQ(Probe::destructionCount, 5);
}

// Test initializer list assignment with element construction failure
TEST_F(DArrayTest, InitializerListAssignmentElementFailure) {
    std::initializer_list<Probe> elements1 = {1, 2, 3, 4, 5};
    std::initializer_list<Probe> elements2 = {6, 7, 8};
    Probe::constructorThrowsAt = 8;
    {
        DArrayType arr = elements1;

        EXPECT_THROW({ arr = elements2; }, std::runtime_error);

        EXPECT_EQ(arr.size(), 5);
        EXPECT_EQ(arr.capacity(), 5);
        EXPECT_EQ(arr[0].id, 1);
        EXPECT_EQ(arr[1].id, 2);
        EXPECT_EQ(arr[2].id, 3);
        EXPECT_EQ(arr[3].id, 4);
        EXPECT_EQ(arr[4].id, 5);
    }
    EXPECT_EQ(TestAllocator::allocationCount, 2);
    EXPECT_EQ(TestAllocator::deallocationCount, 2);
    EXPECT_EQ(Probe::constructionCount, 7);
    EXPECT_EQ(Probe::destructionCount, 7);
}

// Test copy constructor success
TEST_F(DArrayTest, CopyConstructorSuccess) {
    std::initializer_list<Probe> elements = {1, 2, 3, 4, 5};
    {
        DArrayType arr = elements;

        DArrayType copy = arr;

        EXPECT_EQ(copy.size(), arr.size());
        EXPECT_EQ(copy.capacity(), arr.size());
        EXPECT_EQ(copy[0], arr[0]);
        EXPECT_EQ(copy[1], arr[1]);
        EXPECT_EQ(copy[2], arr[2]);
        EXPECT_EQ(copy[3], arr[3]);
        EXPECT_EQ(copy[4], arr[4]);
    }
    EXPECT_EQ(TestAllocator::allocationCount, 2);
    EXPECT_EQ(TestAllocator::deallocationCount, 2);
    EXPECT_EQ(Probe::constructionCount, 10);
    EXPECT_EQ(Probe::destructionCount, 10);
}

// Test copy constructor with allocation failure
TEST_F(DArrayTest, CopyConstructorAllocationFailure) {
    std::initializer_list<Probe> elements = {1, 2, 3, 4, 5};
    TestAllocator::allocationThrowsAt = 2;
    {
        DArrayType arr = elements;

        EXPECT_THROW({ DArrayType copy = arr; }, std::bad_alloc);
    }
    EXPECT_EQ(TestAllocator::allocationCount, 1);
    EXPECT_EQ(TestAllocator::deallocationCount, 1);
    EXPECT_EQ(Probe::constructionCount, 5);
    EXPECT_EQ(Probe::destructionCount, 5);
}

// Test copy constructor with element construction failure
TEST_F(DArrayTest, CopyConstructorElementFailure) {
    std::initializer_list<Probe> elements = {1, 2, 3, 4, 5};
    Probe::constructorThrowsAt = 10;
    {
        DArrayType arr = elements;

        EXPECT_THROW({ DArrayType copy = arr; }, std::runtime_error);
    }
    EXPECT_EQ(TestAllocator::allocationCount, 2);
    EXPECT_EQ(TestAllocator::deallocationCount, 2);
    EXPECT_EQ(Probe::constructionCount, 9);
    EXPECT_EQ(Probe::destructionCount, 9);
}

// Test copy assignment success
TEST_F(DArrayTest, CopyAssignmentSuccess) {
    std::initializer_list<Probe> elements1 = {1, 2, 3, 4, 5};
    std::initializer_list<Probe> elements2 = {6, 7, 8};
    {
        DArrayType arr1 = elements1;
        DArrayType arr2 = elements2;

        arr2 = arr1;

        EXPECT_EQ(arr2.size(), arr1.size());
        EXPECT_EQ(arr2.capacity(), arr1.size());
        EXPECT_EQ(arr2[0], arr1[0]);
        EXPECT_EQ(arr2[1], arr1[1]);
        EXPECT_EQ(arr2[2], arr1[2]);
        EXPECT_EQ(arr2[3], arr1[3]);
        EXPECT_EQ(arr2[4], arr1[4]);
    }
    EXPECT_EQ(TestAllocator::allocationCount, 3);
    EXPECT_EQ(TestAllocator::deallocationCount, 3);
    EXPECT_EQ(Probe::constructionCount, 13);
    EXPECT_EQ(Probe::destructionCount, 13);
}

// Test copy assignment of empty array
TEST_F(DArrayTest, CopyAssignmentOfEmptyArray) {
    std::initializer_list<Probe> elements = {1, 2, 3, 4, 5};
    {
        DArrayType arr1;
        DArrayType arr2 = elements;

        arr2 = arr1;

        EXPECT_EQ(arr2.size(), 0);
        EXPECT_EQ(arr2.capacity(), 5);
    }
    EXPECT_EQ(TestAllocator::allocationCount, 1);
    EXPECT_EQ(TestAllocator::deallocationCount, 1);
    EXPECT_EQ(Probe::constructionCount, 5);
    EXPECT_EQ(Probe::destructionCount, 5);
}

// Test copy assignment with allocation failure
TEST_F(DArrayTest, CopyAssignmentAllocationFailure) {
    std::initializer_list<Probe> elements1 = {1, 2, 3, 4, 5};
    std::initializer_list<Probe> elements2 = {6, 7, 8};
    TestAllocator::allocationThrowsAt = 3;
    {
        DArrayType arr1 = elements1;
        DArrayType arr2 = elements2;

        EXPECT_THROW({ arr2 = arr1; }, std::bad_alloc);

        EXPECT_EQ(arr2.size(), 3);
        EXPECT_EQ(arr2.capacity(), 3);
        EXPECT_EQ(arr2[0].id, 6);
        EXPECT_EQ(arr2[1].id, 7);
        EXPECT_EQ(arr2[2].id, 8);
    }
    EXPECT_EQ(TestAllocator::allocationCount, 2);
    EXPECT_EQ(TestAllocator::deallocationCount, 2);
    EXPECT_EQ(Probe::constructionCount, 8);
    EXPECT_EQ(Probe::destructionCount, 8);
}

// Test copy assignment with element construction failure
TEST_F(DArrayTest, CopyAssignmentElementFailure) {
    std::initializer_list<Probe> elements1 = {1, 2, 3, 4, 5};
    std::initializer_list<Probe> elements2 = {6, 7, 8};
    Probe::constructorThrowsAt = 13;
    {
        DArrayType arr1 = elements1;
        DArrayType arr2 = elements2;

        EXPECT_THROW({ arr2 = arr1; }, std::runtime_error);

        EXPECT_EQ(arr2.size(), 3);
        EXPECT_EQ(arr2.capacity(), 3);
        EXPECT_EQ(arr2[0].id, 6);
        EXPECT_EQ(arr2[1].id, 7);
        EXPECT_EQ(arr2[2].id, 8);
    }
    EXPECT_EQ(TestAllocator::allocationCount, 3);
    EXPECT_EQ(TestAllocator::deallocationCount, 3);
    EXPECT_EQ(Probe::constructionCount, 12);
    EXPECT_EQ(Probe::destructionCount, 12);
}

// Test move constructor success
TEST_F(DArrayTest, MoveConstructorSuccess) {
    DArray<int> src = {1, 2, 3};

    DArray<int> dst = std::move(src);

    EXPECT_EQ(src.size(), 0);
    EXPECT_EQ(src.capacity(), 0);
    EXPECT_EQ(dst.size(), 3);
    EXPECT_EQ(dst.capacity(), 3);
    EXPECT_EQ(dst[0], 1);
    EXPECT_EQ(dst[1], 2);
    EXPECT_EQ(dst[2], 3);
}

// Test move assignment success
TEST_F(DArrayTest, MoveAssignmentSuccess) {
    std::initializer_list<Probe> elements1 = {1, 2, 3, 4, 5};
    std::initializer_list<Probe> elements2 = {6, 7, 8};
    {
        DArrayType src = elements1;
        DArrayType dst = elements2;

        dst = std::move(src);

        EXPECT_EQ(src.size(), 0);
        EXPECT_EQ(src.capacity(), 0);
        EXPECT_EQ(dst.size(), 5);
        EXPECT_EQ(dst.capacity(), 5);
        EXPECT_EQ(dst[0].id, 1);
        EXPECT_EQ(dst[1].id, 2);
        EXPECT_EQ(dst[2].id, 3);
        EXPECT_EQ(dst[3].id, 4);
        EXPECT_EQ(dst[4].id, 5);
    }
    EXPECT_EQ(TestAllocator::allocationCount, 2);
    EXPECT_EQ(TestAllocator::deallocationCount, 2);
    EXPECT_EQ(Probe::constructionCount, 8);
    EXPECT_EQ(Probe::destructionCount, 8);
}

// Test move assignment of empty array
TEST_F(DArrayTest, MoveAssignmentOfEmptyArray) {
    {
        DArrayType src(5);
        src.clear();
        DArrayType dst;

        dst = std::move(src);

        EXPECT_EQ(src.size(), 0);
        EXPECT_EQ(src.capacity(), 0);
        EXPECT_EQ(dst.size(), 0);
        EXPECT_EQ(dst.capacity(), 5);
    }
    EXPECT_EQ(TestAllocator::allocationCount, 1);
    EXPECT_EQ(TestAllocator::deallocationCount, 1);
    EXPECT_EQ(Probe::constructionCount, 5);
    EXPECT_EQ(Probe::destructionCount, 5);
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

// Test construction with zero size (edge case)
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

// Test construction with very large size
TEST_F(DArrayTest, LargeSizeConstruction) {
    size_t large_size = std::numeric_limits<size_t>::max() / sizeof(int) + 1;

    EXPECT_THROW({ DArray<int> arr(large_size); }, std::bad_alloc);
}

// Test that clear() works correctly
TEST_F(DArrayTest, Clear) {
    DArray<int> arr = {1, 2, 3, 4, 5};

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

// Test that copying push_back works correctly
TEST_F(DArrayTest, PushBackCopy) {
    const Probe elements[] = {1, 2, 3, 4, 5};
    int expected_size[] = {1, 2, 3, 4, 5};
    int expected_capacity[] = {1, 2, 4, 4, 8};
    {
        DArrayType arr;

        for (int i = 0; i < std::size(elements); ++i) {
            arr.push_back(elements[i]);
            EXPECT_EQ(arr.size(), expected_size[i]);
            EXPECT_EQ(arr.capacity(), expected_capacity[i]);
            EXPECT_EQ(arr[i].id, elements[i].id);
        }
    }
    EXPECT_EQ(TestAllocator::allocationCount, TestAllocator::deallocationCount);
    EXPECT_EQ(Probe::constructionCount, Probe::destructionCount);
}

// Test copying push_back with allocation failure
TEST_F(DArrayTest, PushBackCopyAllocationFailure) {
    const Probe elements[] = {1, 2, 3, 4, 5};
    TestAllocator::allocationThrowsAt = 3;
    {
        DArrayType arr;

        EXPECT_THROW(
            {
                for (const Probe& element : elements) {
                    arr.push_back(element);
                }
            },
            std::bad_alloc);

        EXPECT_EQ(arr.size(), 2);
        EXPECT_EQ(arr.capacity(), 2);
        EXPECT_EQ(arr[0].id, 1);
        EXPECT_EQ(arr[1].id, 2);
    }
    EXPECT_EQ(TestAllocator::allocationCount, TestAllocator::deallocationCount);
    EXPECT_EQ(Probe::constructionCount, Probe::destructionCount);
}

// Test copying push_back with element construction failure
TEST_F(DArrayTest, PushBackCopyElementFailure) {
    const Probe elements[] = {1, 2, 3, 4, 5};
    Probe::constructorThrowsAt = 8;
    {
        DArrayType arr;

        EXPECT_THROW(
            {
                for (const Probe& element : elements) {
                    arr.push_back(element);
                }
            },
            std::runtime_error);

        EXPECT_EQ(arr.size(), 4);
        EXPECT_EQ(arr.capacity(), 4);
        EXPECT_EQ(arr[0].id, 1);
        EXPECT_EQ(arr[1].id, 2);
        EXPECT_EQ(arr[2].id, 3);
        EXPECT_EQ(arr[3].id, 4);
    }
    EXPECT_EQ(TestAllocator::allocationCount, TestAllocator::deallocationCount);
    EXPECT_EQ(Probe::constructionCount, Probe::destructionCount);
}

// Test that moving push_back works correctly
TEST_F(DArrayTest, PushBackMove) {
    Probe elements[] = {1, 2, 3, 4, 5};
    int expected_size[] = {1, 2, 3, 4, 5};
    int expected_capacity[] = {1, 2, 4, 4, 8};
    {
        DArrayType arr;

        for (int i = 0; i < std::size(elements); ++i) {
            int id = elements[i].id;
            arr.push_back(std::move(elements[i]));
            EXPECT_EQ(arr.size(), expected_size[i]);
            EXPECT_EQ(arr.capacity(), expected_capacity[i]);
            EXPECT_EQ(arr[i].id, id);
            EXPECT_EQ(elements[i].id, -1);
        }
    }
    EXPECT_EQ(TestAllocator::allocationCount, TestAllocator::deallocationCount);
    EXPECT_EQ(Probe::constructionCount, Probe::destructionCount);
}

// Test moving push_back with allocation failure
TEST_F(DArrayTest, PushBackMoveAllocationFailure) {
    Probe elements[] = {1, 2, 3, 4, 5};
    TestAllocator::allocationThrowsAt = 3;
    {
        DArrayType arr;

        EXPECT_THROW(
            {
                for (Probe& element : elements) {
                    arr.push_back(std::move(element));
                }
            },
            std::bad_alloc);

        EXPECT_EQ(arr.size(), 2);
        EXPECT_EQ(arr.capacity(), 2);
        EXPECT_EQ(arr[0].id, 1);
        EXPECT_EQ(arr[1].id, 2);
        EXPECT_EQ(elements[0].id, -1);
        EXPECT_EQ(elements[1].id, -1);
        EXPECT_EQ(elements[2].id, 3);
        EXPECT_EQ(elements[3].id, 4);
        EXPECT_EQ(elements[4].id, 5);
    }
    EXPECT_EQ(TestAllocator::allocationCount, TestAllocator::deallocationCount);
    EXPECT_EQ(Probe::constructionCount, Probe::destructionCount);
}

// Test that pop_back works correctly
TEST_F(DArrayTest, PopBack) {
    std::initializer_list<Probe> elements = {1, 2, 3, 4, 5};
    {
        DArrayType arr = elements;

        arr.pop_back();
        EXPECT_EQ(arr.size(), 4);
        EXPECT_EQ(arr.capacity(), 5);
        EXPECT_EQ(arr.back().id, 4);

        arr.pop_back();
        EXPECT_EQ(arr.size(), 3);
        EXPECT_EQ(arr.capacity(), 5);
        EXPECT_EQ(arr.back().id, 3);

        arr.pop_back();
        arr.pop_back();
        arr.pop_back();
        EXPECT_EQ(arr.size(), 0);
        EXPECT_EQ(arr.capacity(), 5);
        EXPECT_TRUE(arr.empty());
    }
    EXPECT_EQ(TestAllocator::allocationCount, TestAllocator::deallocationCount);
    EXPECT_EQ(Probe::constructionCount, Probe::destructionCount);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}