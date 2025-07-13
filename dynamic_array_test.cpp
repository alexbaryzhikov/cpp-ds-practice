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
    static bool verbose;

    void* allocate(size_t count, std::align_val_t alignment) const {
        if (allocationCount + 1 == allocationThrowsAt) {
            if (verbose) {
                std::println("{:2}) Failed to allocate", allocationCount + 1);
            }
            allocationThrowsAt = -1;
            throw std::bad_alloc();
        }
        if (verbose) {
            std::println("{:2}) Allocated", allocationCount + 1);
        }
        ++allocationCount;
        return ::operator new(count, alignment);
    }

    void deallocate(void* pointer, std::align_val_t alignment) const noexcept {
        if (verbose) {
            std::println("{:2}) Deallocated", deallocationCount + 1);
        }
        ++deallocationCount;
        ::operator delete(pointer, alignment, std::nothrow);
    }

    static void reset() {
        allocationCount = 0;
        deallocationCount = 0;
        allocationThrowsAt = -1;
        verbose = false;
    }
};

int TestAllocator::allocationCount = 0;
int TestAllocator::deallocationCount = 0;
int TestAllocator::allocationThrowsAt = -1;
bool TestAllocator::verbose = false;

struct Dummy {};
Dummy dummy;

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
            constructorThrowsAt = -1;
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

    Probe(int id, Dummy _)
        : id(id) {
        if (constructionCount + 1 == constructorThrowsAt) {
            if (verbose) {
                std::println("{:2}) Failed to construct, id: {}", constructionCount + 1, id);
            }
            constructorThrowsAt = -1;
            throw std::runtime_error("Construction failed");
        }
        ++constructionCount;
        if (verbose) {
            std::println("{:2}) Constructed", constructionCount);
        }
    }

    Probe(const Probe& other)
        : id(other.id) {
        if (constructionCount + 1 == constructorThrowsAt) {
            if (verbose) {
                std::println("{:2}) Failed to copy construct, id: {}", constructionCount + 1, id);
            }
            constructorThrowsAt = -1;
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

// =============================================================================
// Constructors
// =============================================================================

// Test that default constructor works correctly
TEST_F(DArrayTest, DefaultConstructor) {
    DArray<int> arr;

    EXPECT_EQ(arr.size(), 0);
    EXPECT_EQ(arr.capacity(), 0);
    EXPECT_TRUE(arr.empty());
}

// Test that size constructor success works correctly
TEST_F(DArrayTest, SizeConstructor) {
    DArray<int> arr1(5);

    EXPECT_EQ(arr1.size(), 5);
    EXPECT_EQ(arr1.capacity(), 5);
}

// Test size constructor with allocation failure
TEST_F(DArrayTest, SizeConstructorAllocFailure) {
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

// Test that fill constructor works correctly
TEST_F(DArrayTest, FillConstructor) {
    DArray<int> arr2(42, 3);

    EXPECT_EQ(arr2.size(), 3);
    EXPECT_EQ(arr2.capacity(), 3);
    EXPECT_EQ(arr2[0], 42);
    EXPECT_EQ(arr2[1], 42);
    EXPECT_EQ(arr2[2], 42);
}

// Test fill constructor with allocation failure
TEST_F(DArrayTest, FillConstructorAllocFailure) {
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

// Test that range constructor success works correctly
TEST_F(DArrayTest, RangeConstructor) {
    int data[] = {1, 2, 3};

    DArray<int> arr3(data, data + 3);

    EXPECT_EQ(arr3.size(), 3);
    EXPECT_EQ(arr3.capacity(), 3);
    EXPECT_EQ(arr3[0], 1);
    EXPECT_EQ(arr3[1], 2);
    EXPECT_EQ(arr3[2], 3);
}

// Test range constructor with allocation failure
TEST_F(DArrayTest, RangeConstructorAllocFailure) {
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

// Test that initializer list constructor works correctly
TEST_F(DArrayTest, InitializerListConstructor) {
    DArray<int> arr4 = {10, 20, 30};

    EXPECT_EQ(arr4.size(), 3);
    EXPECT_EQ(arr4.capacity(), 3);
    EXPECT_EQ(arr4[0], 10);
    EXPECT_EQ(arr4[1], 20);
    EXPECT_EQ(arr4[2], 30);
}

// Test initializer list constructor with allocation failure
TEST_F(DArrayTest, InitializerListConstructorAllocFailure) {
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

// Test that copy constructor works correctly
TEST_F(DArrayTest, CopyConstructor) {
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
TEST_F(DArrayTest, CopyConstructorAllocFailure) {
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

// Test move constructor success
TEST_F(DArrayTest, MoveConstructor) {
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

// Test construction with zero size
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

// =============================================================================
// Destructors
// =============================================================================

// Test that destructor properly cleans up
TEST_F(DArrayTest, Destructor) {
    {
        DArray<Probe> arr(5);

        EXPECT_EQ(Probe::constructionCount, 5);
        EXPECT_EQ(Probe::destructionCount, 0);
    }
    EXPECT_EQ(Probe::destructionCount, 5);
}

// =============================================================================
// Assignments
// =============================================================================

// Test that fill assignment works correctly
TEST_F(DArrayTest, FillAssignment) {
    std::initializer_list<Probe> elements = {1, 2, 3};
    const Probe element = 255;
    {
        DArrayType arr = elements;

        arr.assign(element, 2);

        EXPECT_EQ(arr.size(), 2);
        EXPECT_EQ(arr.capacity(), 2);
        EXPECT_EQ(arr[0].id, 255);
        EXPECT_EQ(arr[1].id, 255);
    }
    EXPECT_EQ(TestAllocator::allocationCount, 2);
    EXPECT_EQ(TestAllocator::deallocationCount, 2);
    EXPECT_EQ(Probe::constructionCount, 5);
    EXPECT_EQ(Probe::destructionCount, 5);
}

// Test fill assignment of array element
TEST_F(DArrayTest, FillAssignmentArrayElement) {
    std::initializer_list<Probe> elements = {1, 2, 3};
    {
        DArrayType arr = elements;

        arr.assign(arr[2], 2);

        EXPECT_EQ(arr.size(), 2);
        EXPECT_EQ(arr.capacity(), 2);
        EXPECT_EQ(arr[0].id, 3);
        EXPECT_EQ(arr[1].id, 3);
    }
    EXPECT_EQ(TestAllocator::allocationCount, 2);
    EXPECT_EQ(TestAllocator::deallocationCount, 2);
    EXPECT_EQ(Probe::constructionCount, 5);
    EXPECT_EQ(Probe::destructionCount, 5);
}

// Test fill assignment with allocation failure
TEST_F(DArrayTest, FillAssignmentAllocFailure) {
    std::initializer_list<Probe> elements = {1, 2, 3};
    const Probe element = 255;
    TestAllocator::allocationThrowsAt = 2;
    {
        DArrayType arr = elements;

        EXPECT_THROW({ arr.assign(element, 2); }, std::bad_alloc);

        EXPECT_EQ(arr.size(), 3);
        EXPECT_EQ(arr.capacity(), 3);
        EXPECT_EQ(arr[0].id, 1);
        EXPECT_EQ(arr[1].id, 2);
        EXPECT_EQ(arr[2].id, 3);
    }
    EXPECT_EQ(TestAllocator::allocationCount, 1);
    EXPECT_EQ(TestAllocator::deallocationCount, 1);
    EXPECT_EQ(Probe::constructionCount, 3);
    EXPECT_EQ(Probe::destructionCount, 3);
}

// Test fill assignment with element construction failure
TEST_F(DArrayTest, FillAssignmentElementFailure) {
    std::initializer_list<Probe> elements = {1, 2, 3};
    const Probe element = 255;
    Probe::constructorThrowsAt = 5;
    {
        DArrayType arr = elements;

        EXPECT_THROW({ arr.assign(element, 2); }, std::runtime_error);

        EXPECT_EQ(arr.size(), 3);
        EXPECT_EQ(arr.capacity(), 3);
        EXPECT_EQ(arr[0].id, 1);
        EXPECT_EQ(arr[1].id, 2);
        EXPECT_EQ(arr[2].id, 3);
    }
    EXPECT_EQ(TestAllocator::allocationCount, 2);
    EXPECT_EQ(TestAllocator::deallocationCount, 2);
    EXPECT_EQ(Probe::constructionCount, 4);
    EXPECT_EQ(Probe::destructionCount, 4);
}

// Test that range assignment works correctly
TEST_F(DArrayTest, RangeAssignment) {
    std::initializer_list<Probe> elements = {1, 2, 3, 4, 5};
    const Probe range[] = {11, 22, 33};
    {
        DArrayType arr = elements;

        arr.assign(range, range + 3);

        EXPECT_EQ(arr.size(), 3);
        EXPECT_EQ(arr.capacity(), 3);
        EXPECT_EQ(arr[0].id, 11);
        EXPECT_EQ(arr[1].id, 22);
        EXPECT_EQ(arr[2].id, 33);
    }
    EXPECT_EQ(TestAllocator::allocationCount, 2);
    EXPECT_EQ(TestAllocator::deallocationCount, 2);
    EXPECT_EQ(Probe::constructionCount, 8);
    EXPECT_EQ(Probe::destructionCount, 8);
}

// Test range assignment of sub-array
TEST_F(DArrayTest, RangeAssignmentSubArray) {
    std::initializer_list<Probe> elements = {1, 2, 3, 4, 5};
    {
        DArrayType arr = elements;

        arr.assign(arr.begin() + 2, arr.end());

        EXPECT_EQ(arr.size(), 3);
        EXPECT_EQ(arr.capacity(), 3);
        EXPECT_EQ(arr[0].id, 3);
        EXPECT_EQ(arr[1].id, 4);
        EXPECT_EQ(arr[2].id, 5);
    }
    EXPECT_EQ(TestAllocator::allocationCount, 2);
    EXPECT_EQ(TestAllocator::deallocationCount, 2);
    EXPECT_EQ(Probe::constructionCount, 8);
    EXPECT_EQ(Probe::destructionCount, 8);
}

// Test range assignment with allocation failure
TEST_F(DArrayTest, RangeAssignmentAllocFailure) {
    std::initializer_list<Probe> elements = {1, 2, 3, 4, 5};
    const Probe range[] = {11, 22, 33};
    TestAllocator::allocationThrowsAt = 2;
    {
        DArrayType arr = elements;

        EXPECT_THROW({ arr.assign(range, range + 3); }, std::bad_alloc);

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

// Test range assignment with element construction failure
TEST_F(DArrayTest, RangeAssignmentElementFailure) {
    std::initializer_list<Probe> elements = {1, 2, 3, 4, 5};
    const Probe range[] = {11, 22, 33};
    Probe::constructorThrowsAt = 8;
    {
        DArrayType arr = elements;

        EXPECT_THROW({ arr.assign(range, range + 3); }, std::runtime_error);

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

// Test that initializer list assignment works correctly
TEST_F(DArrayTest, InitializerListAssignment) {
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
TEST_F(DArrayTest, InitializerListAssignmentEmptyArray) {
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
TEST_F(DArrayTest, InitializerListAssignmentAllocFailure) {
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

// Test that copy assignment works correctly
TEST_F(DArrayTest, CopyAssignment) {
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
TEST_F(DArrayTest, CopyAssignmentEmptyArray) {
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

// Test copy assignment of self
TEST_F(DArrayTest, CopyAssignmentSelf) {
    std::initializer_list<Probe> elements = {1, 2, 3};
    {
        DArrayType arr = elements;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wself-assign"
        arr = arr;
#pragma clang diagnostic pop

        EXPECT_EQ(arr.size(), 3);
        EXPECT_EQ(arr.capacity(), 3);
        EXPECT_EQ(arr[0].id, 1);
        EXPECT_EQ(arr[1].id, 2);
        EXPECT_EQ(arr[2].id, 3);
    }
    EXPECT_EQ(TestAllocator::allocationCount, 1);
    EXPECT_EQ(TestAllocator::deallocationCount, 1);
    EXPECT_EQ(Probe::constructionCount, 3);
    EXPECT_EQ(Probe::destructionCount, 3);
}

// Test copy assignment with allocation failure
TEST_F(DArrayTest, CopyAssignmentAllocFailure) {
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

// Test that move assignment works correctly
TEST_F(DArrayTest, MoveAssignment) {
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
TEST_F(DArrayTest, MoveAssignmentEmptyArray) {
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

// Test move assignment of self
TEST_F(DArrayTest, MoveAssignmentSelf) {
    std::initializer_list<Probe> elements = {1, 2, 3};
    {
        DArrayType arr = elements;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wself-move"
        arr = std::move(arr);
#pragma clang diagnostic pop

        EXPECT_EQ(arr.size(), 3);
        EXPECT_EQ(arr.capacity(), 3);
        EXPECT_EQ(arr[0].id, 1);
        EXPECT_EQ(arr[1].id, 2);
        EXPECT_EQ(arr[2].id, 3);
    }
    EXPECT_EQ(TestAllocator::allocationCount, 1);
    EXPECT_EQ(TestAllocator::deallocationCount, 1);
    EXPECT_EQ(Probe::constructionCount, 3);
    EXPECT_EQ(Probe::destructionCount, 3);
}

// =============================================================================
// Element Access
// =============================================================================

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

// Test that at() works correctly
TEST_F(DArrayTest, AtAccessor) {
    DArray<int> arr = {1, 2, 3, 4, 5};
    EXPECT_EQ(arr.at(0), 1);
    EXPECT_EQ(arr.at(2), 3);
    EXPECT_EQ(arr.at(4), 5);
    EXPECT_THROW({ arr.at(6); }, std::out_of_range);

    arr.at(2) = 42;
    EXPECT_EQ(arr.at(2), 42);

    const DArray<int>& const_arr = arr;
    EXPECT_EQ(const_arr.at(0), 1);
    EXPECT_EQ(const_arr.at(2), 42);
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

// =============================================================================
// Iterators
// =============================================================================

// Test that iterators work correctly
TEST_F(DArrayTest, Iterators) {
    DArray<int> arr = {1, 2, 3, 4, 5};

    int sum = 0;
    int i = 0;
    for (auto it = arr.begin(); it != arr.end(); ++it) {
        sum += *it;
        EXPECT_EQ(*it, arr[i++]);
    }
    EXPECT_EQ(sum, 15);

    *arr.begin() = 42;
    EXPECT_EQ(arr[0], 42);
}

// Test that constant iterators work correctly
TEST_F(DArrayTest, ConstIterators) {
    const DArray<int> arr = {1, 2, 3, 4, 5};

    int sum = 0;
    int i = 0;
    for (auto it = arr.begin(); it != arr.end(); ++it) {
        sum += *it;
        EXPECT_EQ(*it, arr[i++]);
    }
    EXPECT_EQ(sum, 15);
}

// Test that reverse iterators work correctly
TEST_F(DArrayTest, RevereseIterators) {
    DArray<int> arr = {1, 2, 3, 4, 5};

    int sum = 0;
    int i = 5;
    for (auto it = arr.rbegin(); it != arr.rend(); ++it) {
        sum += *it;
        EXPECT_EQ(*it, arr[--i]);
    }
    EXPECT_EQ(sum, 15);

    *arr.rbegin() = 42;
    EXPECT_EQ(arr[4], 42);
}

// Test that constant reverse iterators work correctly
TEST_F(DArrayTest, ConstRevereseIterators) {
    const DArray<int> arr = {1, 2, 3, 4, 5};

    int sum = 0;
    int i = 5;
    for (auto it = arr.rbegin(); it != arr.rend(); ++it) {
        sum += *it;
        EXPECT_EQ(*it, arr[--i]);
    }
    EXPECT_EQ(sum, 15);
}

// =============================================================================
// Capacity
// =============================================================================

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

    DArray<int> arr3;
    arr3.push(1);
    arr3.push(2);
    arr3.push(3);
    EXPECT_EQ(arr3.size(), 3);
    EXPECT_EQ(arr3.capacity(), 4);
}

// Test that reserve() works correctly
TEST_F(DArrayTest, Reserve) {
    std::initializer_list<Probe> elements = {1, 2, 3};
    {
        DArrayType arr = elements;

        arr.reserve(10);

        EXPECT_EQ(arr.size(), 3);
        EXPECT_EQ(arr.capacity(), 10);
        EXPECT_EQ(arr[0].id, 1);
        EXPECT_EQ(arr[1].id, 2);
        EXPECT_EQ(arr[2].id, 3);
    }
    EXPECT_EQ(TestAllocator::allocationCount, 2);
    EXPECT_EQ(TestAllocator::deallocationCount, 2);
    EXPECT_EQ(Probe::constructionCount, 6);
    EXPECT_EQ(Probe::destructionCount, 3);
}

// Test reserve() with allocation failure
TEST_F(DArrayTest, ReserveAllocFailure) {
    std::initializer_list<Probe> elements = {1, 2, 3};
    TestAllocator::allocationThrowsAt = 2;
    {
        DArrayType arr = elements;

        EXPECT_THROW({ arr.reserve(10); }, std::bad_alloc);

        EXPECT_EQ(arr.size(), 3);
        EXPECT_EQ(arr.capacity(), 3);
        EXPECT_EQ(arr[0].id, 1);
        EXPECT_EQ(arr[1].id, 2);
        EXPECT_EQ(arr[2].id, 3);
    }
    EXPECT_EQ(TestAllocator::allocationCount, 1);
    EXPECT_EQ(TestAllocator::deallocationCount, 1);
    EXPECT_EQ(Probe::constructionCount, 3);
    EXPECT_EQ(Probe::destructionCount, 3);
}

// Test that shrinkToFit() works correctly
TEST_F(DArrayTest, ShrinkToFit) {
    std::initializer_list<Probe> elements = {1, 2, 3};
    {
        DArrayType arr;
        for (const Probe& element : elements) {
            arr.push(element);
        }

        arr.shrinkToFit();

        EXPECT_EQ(arr.size(), 3);
        EXPECT_EQ(arr.capacity(), 3);
        EXPECT_EQ(arr[0].id, 1);
        EXPECT_EQ(arr[1].id, 2);
        EXPECT_EQ(arr[2].id, 3);
    }
    EXPECT_EQ(TestAllocator::allocationCount, 4);
    EXPECT_EQ(TestAllocator::deallocationCount, 4);
    EXPECT_EQ(Probe::constructionCount, 9);
    EXPECT_EQ(Probe::destructionCount, 6);
}

// Test shrinkToFit() with allocation failure
TEST_F(DArrayTest, ShrinkToFitAllocFailure) {
    std::initializer_list<Probe> elements = {1, 2, 3};
    TestAllocator::allocationThrowsAt = 4;
    {
        DArrayType arr;
        for (const Probe& element : elements) {
            arr.push(element);
        }

        arr.shrinkToFit(); // Fails silently

        EXPECT_EQ(arr.size(), 3);
        EXPECT_EQ(arr.capacity(), 4);
        EXPECT_EQ(arr[0].id, 1);
        EXPECT_EQ(arr[1].id, 2);
        EXPECT_EQ(arr[2].id, 3);
    }
    EXPECT_EQ(TestAllocator::allocationCount, 3);
    EXPECT_EQ(TestAllocator::deallocationCount, 3);
    EXPECT_EQ(Probe::constructionCount, 6);
    EXPECT_EQ(Probe::destructionCount, 6);
}

// Test shrinkToFit() of zero size array
TEST_F(DArrayTest, ShrinkToFitZeroSize) {
    {
        DArrayType arr(3);
        arr.clear();

        arr.shrinkToFit();

        EXPECT_EQ(arr.size(), 0);
        EXPECT_EQ(arr.capacity(), 0);
        EXPECT_EQ(arr.data(), nullptr);
    }
    EXPECT_EQ(TestAllocator::allocationCount, 1);
    EXPECT_EQ(TestAllocator::deallocationCount, 1);
    EXPECT_EQ(Probe::constructionCount, 3);
    EXPECT_EQ(Probe::destructionCount, 3);
}

// =============================================================================
// Modifiers
// =============================================================================

// Test that clear() works correctly
TEST_F(DArrayTest, Clear) {
    DArray<int> arr = {1, 2, 3, 4, 5};

    arr.clear();

    EXPECT_EQ(arr.size(), 0);
    EXPECT_EQ(arr.capacity(), 5); // Capacity should remain
}

// Test that copying insert() works correctly
TEST_F(DArrayTest, InsertCopy) {
    std::initializer_list<Probe> elements = {1, 2, 3, 4, 5};
    const Probe element = 255;
    {
        DArrayType arr;
        for (const Probe& element : elements) {
            arr.push(element);
        }

        arr.insert(arr.begin() + 3, element);

        EXPECT_EQ(arr.size(), 6);
        EXPECT_EQ(arr.capacity(), 8);
        EXPECT_EQ(arr[0].id, 1);
        EXPECT_EQ(arr[1].id, 2);
        EXPECT_EQ(arr[2].id, 3);
        EXPECT_EQ(arr[3].id, 255);
        EXPECT_EQ(arr[4].id, 4);
        EXPECT_EQ(arr[5].id, 5);
    }
    EXPECT_EQ(TestAllocator::allocationCount, TestAllocator::deallocationCount);
    EXPECT_EQ(Probe::constructionCount, Probe::destructionCount);
}

// Test copying insert() at the end of the array
TEST_F(DArrayTest, InsertCopyEnd) {
    std::initializer_list<Probe> elements = {1, 2, 3, 4, 5};
    const Probe element = 255;
    {
        DArrayType arr;
        for (const Probe& element : elements) {
            arr.push(element);
        }

        arr.insert(arr.end(), element);

        EXPECT_EQ(arr.size(), 6);
        EXPECT_EQ(arr.capacity(), 8);
        EXPECT_EQ(arr[0].id, 1);
        EXPECT_EQ(arr[1].id, 2);
        EXPECT_EQ(arr[2].id, 3);
        EXPECT_EQ(arr[3].id, 4);
        EXPECT_EQ(arr[4].id, 5);
        EXPECT_EQ(arr[5].id, 255);
    }
    EXPECT_EQ(TestAllocator::allocationCount, TestAllocator::deallocationCount);
    EXPECT_EQ(Probe::constructionCount, Probe::destructionCount);
}

// Test copying insert() of array element
TEST_F(DArrayTest, InsertCopyArrayElement) {
    std::initializer_list<Probe> elements = {1, 2, 3, 4, 5};
    {
        DArrayType arr;
        for (const Probe& element : elements) {
            arr.push(element);
        }

        arr.insert(arr.begin(), arr.back());

        EXPECT_EQ(arr.size(), 6);
        EXPECT_EQ(arr.capacity(), 8);
        EXPECT_EQ(arr[0].id, 5);
        EXPECT_EQ(arr[1].id, 1);
        EXPECT_EQ(arr[2].id, 2);
        EXPECT_EQ(arr[3].id, 3);
        EXPECT_EQ(arr[4].id, 4);
        EXPECT_EQ(arr[5].id, 5);
    }
    EXPECT_EQ(TestAllocator::allocationCount, TestAllocator::deallocationCount);
    EXPECT_EQ(Probe::constructionCount, Probe::destructionCount);
}

// Test copying insert() with reallocation
TEST_F(DArrayTest, InsertCopyRealloc) {
    std::initializer_list<Probe> elements = {1, 2, 3, 4, 5};
    const Probe element = 255;
    {
        DArrayType arr = elements;

        arr.insert(arr.begin() + 3, element);

        EXPECT_EQ(arr.size(), 6);
        EXPECT_EQ(arr.capacity(), 10);
        EXPECT_EQ(arr[0].id, 1);
        EXPECT_EQ(arr[1].id, 2);
        EXPECT_EQ(arr[2].id, 3);
        EXPECT_EQ(arr[3].id, 255);
        EXPECT_EQ(arr[4].id, 4);
        EXPECT_EQ(arr[5].id, 5);
    }
    EXPECT_EQ(TestAllocator::allocationCount, TestAllocator::deallocationCount);
    EXPECT_EQ(Probe::constructionCount, Probe::destructionCount);
}

// Test copying insert() with reallocation failure
TEST_F(DArrayTest, InsertCopyReallocFailure) {
    std::initializer_list<Probe> elements = {1, 2, 3, 4, 5};
    const Probe element = 255;
    TestAllocator::allocationThrowsAt = 2;
    {
        DArrayType arr = elements;

        EXPECT_THROW({ arr.insert(arr.begin() + 3, element); }, std::bad_alloc);

        EXPECT_EQ(arr.size(), 5);
        EXPECT_EQ(arr.capacity(), 5);
        EXPECT_EQ(arr[0].id, 1);
        EXPECT_EQ(arr[1].id, 2);
        EXPECT_EQ(arr[2].id, 3);
        EXPECT_EQ(arr[3].id, 4);
        EXPECT_EQ(arr[4].id, 5);
    }
    EXPECT_EQ(TestAllocator::allocationCount, TestAllocator::deallocationCount);
    EXPECT_EQ(Probe::constructionCount, Probe::destructionCount);
}

// Test copying insert() with element construction failure
TEST_F(DArrayTest, InsertCopyElementFailure) {
    std::initializer_list<Probe> elements = {1, 2, 3, 4, 5};
    const Probe element = 255;
    Probe::constructorThrowsAt = 15;
    {
        DArrayType arr;
        for (const Probe& element : elements) {
            arr.push(element);
        }

        EXPECT_THROW({ arr.insert(arr.begin() + 3, element); }, std::runtime_error);

        EXPECT_EQ(arr.size(), 5);
        EXPECT_EQ(arr.capacity(), 8);
        EXPECT_EQ(arr[0].id, 1);
        EXPECT_EQ(arr[1].id, 2);
        EXPECT_EQ(arr[2].id, 3);
        EXPECT_EQ(arr[3].id, 4);
        EXPECT_EQ(arr[4].id, 5);
    }
    EXPECT_EQ(TestAllocator::allocationCount, TestAllocator::deallocationCount);
    EXPECT_EQ(Probe::constructionCount, Probe::destructionCount);
}

// Test copying insert() with reallocation and element construction failure
TEST_F(DArrayTest, InsertCopyReallocAndElementFailure) {
    std::initializer_list<Probe> elements = {1, 2, 3, 4, 5};
    const Probe element = 255;
    Probe::constructorThrowsAt = 6;
    {
        DArrayType arr = elements;

        EXPECT_THROW({ arr.insert(arr.begin() + 3, element); }, std::runtime_error);

        EXPECT_EQ(arr.size(), 5);
        EXPECT_EQ(arr.capacity(), 5);
        EXPECT_EQ(arr[0].id, 1);
        EXPECT_EQ(arr[1].id, 2);
        EXPECT_EQ(arr[2].id, 3);
        EXPECT_EQ(arr[3].id, 4);
        EXPECT_EQ(arr[4].id, 5);
    }
    EXPECT_EQ(TestAllocator::allocationCount, TestAllocator::deallocationCount);
    EXPECT_EQ(Probe::constructionCount, Probe::destructionCount);
}

// Test moving insert()
TEST_F(DArrayTest, InsertMove) {
    std::initializer_list<Probe> elements = {1, 2, 3, 4, 5};
    const Probe element = 255;
    {
        DArrayType arr;
        for (const Probe& element : elements) {
            arr.push(element);
        }

        arr.insert(arr.begin() + 3, std::move(element));

        EXPECT_EQ(arr.size(), 6);
        EXPECT_EQ(arr.capacity(), 8);
        EXPECT_EQ(arr[0].id, 1);
        EXPECT_EQ(arr[1].id, 2);
        EXPECT_EQ(arr[2].id, 3);
        EXPECT_EQ(arr[3].id, 255);
        EXPECT_EQ(arr[4].id, 4);
        EXPECT_EQ(arr[5].id, 5);
    }
    EXPECT_EQ(TestAllocator::allocationCount, TestAllocator::deallocationCount);
    EXPECT_EQ(Probe::constructionCount, Probe::destructionCount);
}

// Test moving insert() at the end of the array
TEST_F(DArrayTest, InsertMoveEnd) {
    std::initializer_list<Probe> elements = {1, 2, 3, 4, 5};
    const Probe element = 255;
    {
        DArrayType arr;
        for (const Probe& element : elements) {
            arr.push(element);
        }

        arr.insert(arr.end(), std::move(element));

        EXPECT_EQ(arr.size(), 6);
        EXPECT_EQ(arr.capacity(), 8);
        EXPECT_EQ(arr[0].id, 1);
        EXPECT_EQ(arr[1].id, 2);
        EXPECT_EQ(arr[2].id, 3);
        EXPECT_EQ(arr[3].id, 4);
        EXPECT_EQ(arr[4].id, 5);
        EXPECT_EQ(arr[5].id, 255);
    }
    EXPECT_EQ(TestAllocator::allocationCount, TestAllocator::deallocationCount);
    EXPECT_EQ(Probe::constructionCount, Probe::destructionCount);
}

// Test moving insert() with array element
TEST_F(DArrayTest, InsertMoveArrayElement) {
    std::initializer_list<Probe> elements = {1, 2, 3, 4, 5};
    {
        DArrayType arr;
        for (const Probe& element : elements) {
            arr.push(element);
        }

        arr.insert(arr.begin(), std::move(arr.back()));

        EXPECT_EQ(arr.size(), 6);
        EXPECT_EQ(arr.capacity(), 8);
        EXPECT_EQ(arr[0].id, 5);
        EXPECT_EQ(arr[1].id, 1);
        EXPECT_EQ(arr[2].id, 2);
        EXPECT_EQ(arr[3].id, 3);
        EXPECT_EQ(arr[4].id, 4);
        EXPECT_EQ(arr[5].id, -1);
    }
    EXPECT_EQ(TestAllocator::allocationCount, TestAllocator::deallocationCount);
    EXPECT_EQ(Probe::constructionCount, Probe::destructionCount);
}

// Test moving insert() with reallocation
TEST_F(DArrayTest, InsertMoveRealloc) {
    std::initializer_list<Probe> elements = {1, 2, 3, 4, 5};
    const Probe element = 255;
    {
        DArrayType arr = elements;

        arr.insert(arr.begin() + 3, std::move(element));

        EXPECT_EQ(arr.size(), 6);
        EXPECT_EQ(arr.capacity(), 10);
        EXPECT_EQ(arr[0].id, 1);
        EXPECT_EQ(arr[1].id, 2);
        EXPECT_EQ(arr[2].id, 3);
        EXPECT_EQ(arr[3].id, 255);
        EXPECT_EQ(arr[4].id, 4);
        EXPECT_EQ(arr[5].id, 5);
    }
    EXPECT_EQ(TestAllocator::allocationCount, TestAllocator::deallocationCount);
    EXPECT_EQ(Probe::constructionCount, Probe::destructionCount);
}

// Test moving insert() with reallocation failure
TEST_F(DArrayTest, InsertMoveReallocFailure) {
    std::initializer_list<Probe> elements = {1, 2, 3, 4, 5};
    const Probe element = 255;
    TestAllocator::allocationThrowsAt = 2;
    {
        DArrayType arr = elements;

        EXPECT_THROW({ arr.insert(arr.begin() + 3, std::move(element)); }, std::bad_alloc);

        EXPECT_EQ(arr.size(), 5);
        EXPECT_EQ(arr.capacity(), 5);
        EXPECT_EQ(arr[0].id, 1);
        EXPECT_EQ(arr[1].id, 2);
        EXPECT_EQ(arr[2].id, 3);
        EXPECT_EQ(arr[3].id, 4);
        EXPECT_EQ(arr[4].id, 5);
        EXPECT_EQ(element.id, 255);
    }
    EXPECT_EQ(TestAllocator::allocationCount, TestAllocator::deallocationCount);
    EXPECT_EQ(Probe::constructionCount, Probe::destructionCount);
}

// Test fill copying insert()
TEST_F(DArrayTest, InsertCopyFill) {
    std::initializer_list<Probe> elements = {1, 2, 3, 4, 5};
    const Probe element = 255;
    {
        DArrayType arr;
        for (const Probe& element : elements) {
            arr.push(element);
        }

        arr.insert(arr.begin() + 3, element, 3);

        EXPECT_EQ(arr.size(), 8);
        EXPECT_EQ(arr.capacity(), 8);
        EXPECT_EQ(arr[0].id, 1);
        EXPECT_EQ(arr[1].id, 2);
        EXPECT_EQ(arr[2].id, 3);
        EXPECT_EQ(arr[3].id, 255);
        EXPECT_EQ(arr[4].id, 255);
        EXPECT_EQ(arr[5].id, 255);
        EXPECT_EQ(arr[6].id, 4);
        EXPECT_EQ(arr[7].id, 5);
    }
    EXPECT_EQ(TestAllocator::allocationCount, TestAllocator::deallocationCount);
    EXPECT_EQ(Probe::constructionCount, Probe::destructionCount);
}

// Test fill copying insert() at the end of array
TEST_F(DArrayTest, InsertCopyFillEnd) {
    std::initializer_list<Probe> elements = {1, 2, 3, 4, 5};
    const Probe element = 255;
    {
        DArrayType arr;
        for (const Probe& element : elements) {
            arr.push(element);
        }

        arr.insert(arr.end(), element, 3);

        EXPECT_EQ(arr.size(), 8);
        EXPECT_EQ(arr.capacity(), 8);
        EXPECT_EQ(arr[0].id, 1);
        EXPECT_EQ(arr[1].id, 2);
        EXPECT_EQ(arr[2].id, 3);
        EXPECT_EQ(arr[3].id, 4);
        EXPECT_EQ(arr[4].id, 5);
        EXPECT_EQ(arr[5].id, 255);
        EXPECT_EQ(arr[6].id, 255);
        EXPECT_EQ(arr[7].id, 255);
    }
    EXPECT_EQ(TestAllocator::allocationCount, TestAllocator::deallocationCount);
    EXPECT_EQ(Probe::constructionCount, Probe::destructionCount);
}

// Test fill copying insert() with array element
TEST_F(DArrayTest, InsertCopyFillArrayElement) {
    std::initializer_list<Probe> elements = {1, 2, 3, 4, 5};
    {
        DArrayType arr;
        for (const Probe& element : elements) {
            arr.push(element);
        }

        arr.insert(arr.begin(), arr.back(), 3);

        EXPECT_EQ(arr.size(), 8);
        EXPECT_EQ(arr.capacity(), 8);
        EXPECT_EQ(arr[0].id, 5);
        EXPECT_EQ(arr[1].id, 5);
        EXPECT_EQ(arr[2].id, 5);
        EXPECT_EQ(arr[3].id, 1);
        EXPECT_EQ(arr[4].id, 2);
        EXPECT_EQ(arr[5].id, 3);
        EXPECT_EQ(arr[6].id, 4);
        EXPECT_EQ(arr[7].id, 5);
    }
    EXPECT_EQ(TestAllocator::allocationCount, TestAllocator::deallocationCount);
    EXPECT_EQ(Probe::constructionCount, Probe::destructionCount);
}

// Test fill copying insert() with reallocation
TEST_F(DArrayTest, InsertCopyFillRealloc) {
    std::initializer_list<Probe> elements = {1, 2, 3, 4, 5};
    const Probe element = 255;
    {
        DArrayType arr = elements;

        arr.insert(arr.begin() + 3, element, 3);

        EXPECT_EQ(arr.size(), 8);
        EXPECT_EQ(arr.capacity(), 10);
        EXPECT_EQ(arr[0].id, 1);
        EXPECT_EQ(arr[1].id, 2);
        EXPECT_EQ(arr[2].id, 3);
        EXPECT_EQ(arr[3].id, 255);
        EXPECT_EQ(arr[4].id, 255);
        EXPECT_EQ(arr[5].id, 255);
        EXPECT_EQ(arr[6].id, 4);
        EXPECT_EQ(arr[7].id, 5);
    }
    EXPECT_EQ(TestAllocator::allocationCount, TestAllocator::deallocationCount);
    EXPECT_EQ(Probe::constructionCount, Probe::destructionCount);
}

// Test fill copying insert() with reallocation failure
TEST_F(DArrayTest, InsertCopyFillReallocFailure) {
    std::initializer_list<Probe> elements = {1, 2, 3, 4, 5};
    const Probe element = 255;
    TestAllocator::allocationThrowsAt = 2;
    {
        DArrayType arr = elements;

        EXPECT_THROW({ arr.insert(arr.begin() + 3, element, 3); }, std::bad_alloc);

        EXPECT_EQ(arr.size(), 5);
        EXPECT_EQ(arr.capacity(), 5);
        EXPECT_EQ(arr[0].id, 1);
        EXPECT_EQ(arr[1].id, 2);
        EXPECT_EQ(arr[2].id, 3);
        EXPECT_EQ(arr[3].id, 4);
        EXPECT_EQ(arr[4].id, 5);
    }
    EXPECT_EQ(TestAllocator::allocationCount, TestAllocator::deallocationCount);
    EXPECT_EQ(Probe::constructionCount, Probe::destructionCount);
}

// Test fill copying insert() with element construction failure
TEST_F(DArrayTest, InsertCopyFillElementFailure) {
    std::initializer_list<Probe> elements = {1, 2, 3, 4, 5};
    const Probe element = 255;
    Probe::constructorThrowsAt = 17;
    {
        DArrayType arr;
        for (const Probe& element : elements) {
            arr.push(element);
        }

        EXPECT_THROW({ arr.insert(arr.begin() + 3, element, 3); }, std::runtime_error);

        EXPECT_EQ(arr.size(), 5);
        EXPECT_EQ(arr.capacity(), 8);
        EXPECT_EQ(arr[0].id, 1);
        EXPECT_EQ(arr[1].id, 2);
        EXPECT_EQ(arr[2].id, 3);
        EXPECT_EQ(arr[3].id, 4);
        EXPECT_EQ(arr[4].id, 5);
    }
    EXPECT_EQ(TestAllocator::allocationCount, TestAllocator::deallocationCount);
    EXPECT_EQ(Probe::constructionCount, Probe::destructionCount);
}

// Test fill copying insert()  with reallocation and element construction failure
TEST_F(DArrayTest, InsertCopyFillReallocAndElementFailure) {
    std::initializer_list<Probe> elements = {1, 2, 3, 4, 5};
    const Probe element = 255;
    Probe::constructorThrowsAt = 8;
    {
        DArrayType arr = elements;

        EXPECT_THROW({ arr.insert(arr.begin() + 3, element, 3); }, std::runtime_error);

        EXPECT_EQ(arr.size(), 5);
        EXPECT_EQ(arr.capacity(), 5);
        EXPECT_EQ(arr[0].id, 1);
        EXPECT_EQ(arr[1].id, 2);
        EXPECT_EQ(arr[2].id, 3);
        EXPECT_EQ(arr[3].id, 4);
        EXPECT_EQ(arr[4].id, 5);
    }
    EXPECT_EQ(TestAllocator::allocationCount, TestAllocator::deallocationCount);
    EXPECT_EQ(Probe::constructionCount, Probe::destructionCount);
}

// Test that range copying insert() works correctly
TEST_F(DArrayTest, InsertCopyRange) {
    std::initializer_list<Probe> elements = {1, 2, 3, 4, 5};
    const Probe range[] = {11, 22, 33};
    {
        DArrayType arr;
        for (const Probe& element : elements) {
            arr.push(element);
        }

        arr.insert(arr.begin() + 3, range, range + 3);

        EXPECT_EQ(arr.size(), 8);
        EXPECT_EQ(arr.capacity(), 8);
        EXPECT_EQ(arr[0].id, 1);
        EXPECT_EQ(arr[1].id, 2);
        EXPECT_EQ(arr[2].id, 3);
        EXPECT_EQ(arr[3].id, 11);
        EXPECT_EQ(arr[4].id, 22);
        EXPECT_EQ(arr[5].id, 33);
        EXPECT_EQ(arr[6].id, 4);
        EXPECT_EQ(arr[7].id, 5);
    }
    EXPECT_EQ(TestAllocator::allocationCount, TestAllocator::deallocationCount);
    EXPECT_EQ(Probe::constructionCount, Probe::destructionCount);
}

// Test range copying insert() at the end of the array
TEST_F(DArrayTest, InsertCopyRangeEnd) {
    std::initializer_list<Probe> elements = {1, 2, 3, 4, 5};
    const Probe range[] = {11, 22, 33};
    {
        DArrayType arr;
        for (const Probe& element : elements) {
            arr.push(element);
        }

        arr.insert(arr.end(), range, range + 3);

        EXPECT_EQ(arr.size(), 8);
        EXPECT_EQ(arr.capacity(), 8);
        EXPECT_EQ(arr[0].id, 1);
        EXPECT_EQ(arr[1].id, 2);
        EXPECT_EQ(arr[2].id, 3);
        EXPECT_EQ(arr[3].id, 4);
        EXPECT_EQ(arr[4].id, 5);
        EXPECT_EQ(arr[5].id, 11);
        EXPECT_EQ(arr[6].id, 22);
        EXPECT_EQ(arr[7].id, 33);
    }
    EXPECT_EQ(TestAllocator::allocationCount, TestAllocator::deallocationCount);
    EXPECT_EQ(Probe::constructionCount, Probe::destructionCount);
}

// Test range copying insert() with sub-array
TEST_F(DArrayTest, InsertCopyRangeSubArray) {
    std::initializer_list<Probe> elements = {1, 2, 3, 4, 5};
    {
        DArrayType arr;
        for (const Probe& element : elements) {
            arr.push(element);
        }

        arr.insert(arr.begin(), arr.begin() + 2, arr.end());

        EXPECT_EQ(arr.size(), 8);
        EXPECT_EQ(arr.capacity(), 8);
        EXPECT_EQ(arr[0].id, 3);
        EXPECT_EQ(arr[1].id, 4);
        EXPECT_EQ(arr[2].id, 5);
        EXPECT_EQ(arr[3].id, 1);
        EXPECT_EQ(arr[4].id, 2);
        EXPECT_EQ(arr[5].id, 3);
        EXPECT_EQ(arr[6].id, 4);
        EXPECT_EQ(arr[7].id, 5);
    }
    EXPECT_EQ(TestAllocator::allocationCount, TestAllocator::deallocationCount);
    EXPECT_EQ(Probe::constructionCount, Probe::destructionCount);
}

// Test range copying insert() with reallocation
TEST_F(DArrayTest, InsertCopyRangeRealloc) {
    std::initializer_list<Probe> elements = {1, 2, 3, 4, 5};
    const Probe range[] = {11, 22, 33};
    {
        DArrayType arr = elements;

        arr.insert(arr.begin() + 3, range, range + 3);

        EXPECT_EQ(arr.size(), 8);
        EXPECT_EQ(arr.capacity(), 10);
        EXPECT_EQ(arr[0].id, 1);
        EXPECT_EQ(arr[1].id, 2);
        EXPECT_EQ(arr[2].id, 3);
        EXPECT_EQ(arr[3].id, 11);
        EXPECT_EQ(arr[4].id, 22);
        EXPECT_EQ(arr[5].id, 33);
        EXPECT_EQ(arr[6].id, 4);
        EXPECT_EQ(arr[7].id, 5);
    }
    EXPECT_EQ(TestAllocator::allocationCount, TestAllocator::deallocationCount);
    EXPECT_EQ(Probe::constructionCount, Probe::destructionCount);
}

// Test range copying insert() with reallocation failure
TEST_F(DArrayTest, InsertCopyRangeReallocFailure) {
    std::initializer_list<Probe> elements = {1, 2, 3, 4, 5};
    const Probe range[] = {11, 22, 33};
    TestAllocator::allocationThrowsAt = 2;
    {
        DArrayType arr = elements;

        EXPECT_THROW({ arr.insert(arr.begin() + 3, range, range + 3); }, std::bad_alloc);

        EXPECT_EQ(arr.size(), 5);
        EXPECT_EQ(arr.capacity(), 5);
        EXPECT_EQ(arr[0].id, 1);
        EXPECT_EQ(arr[1].id, 2);
        EXPECT_EQ(arr[2].id, 3);
        EXPECT_EQ(arr[3].id, 4);
        EXPECT_EQ(arr[4].id, 5);
    }
    EXPECT_EQ(TestAllocator::allocationCount, TestAllocator::deallocationCount);
    EXPECT_EQ(Probe::constructionCount, Probe::destructionCount);
}

// Test range copying insert() with element construction failure
TEST_F(DArrayTest, InsertCopyRangeElementFailure) {
    std::initializer_list<Probe> elements = {1, 2, 3, 4, 5};
    const Probe range[] = {11, 22, 33};
    Probe::constructorThrowsAt = 17;
    {
        DArrayType arr;
        for (const Probe& element : elements) {
            arr.push(element);
        }

        EXPECT_THROW({ arr.insert(arr.begin() + 3, range, range + 3); }, std::runtime_error);

        EXPECT_EQ(arr.size(), 5);
        EXPECT_EQ(arr.capacity(), 8);
        EXPECT_EQ(arr[0].id, 1);
        EXPECT_EQ(arr[1].id, 2);
        EXPECT_EQ(arr[2].id, 3);
        EXPECT_EQ(arr[3].id, 4);
        EXPECT_EQ(arr[4].id, 5);
    }
    EXPECT_EQ(TestAllocator::allocationCount, TestAllocator::deallocationCount);
    EXPECT_EQ(Probe::constructionCount, Probe::destructionCount);
}

// Test range copying insert() with reallocation and element construction failure
TEST_F(DArrayTest, InsertCopyRangeReallocAndElementFailure) {
    std::initializer_list<Probe> elements = {1, 2, 3, 4, 5};
    const Probe range[] = {11, 22, 33};
    Probe::constructorThrowsAt = 8;
    {
        DArrayType arr = elements;

        EXPECT_THROW({ arr.insert(arr.begin() + 3, range, range + 3); }, std::runtime_error);

        EXPECT_EQ(arr.size(), 5);
        EXPECT_EQ(arr.capacity(), 5);
        EXPECT_EQ(arr[0].id, 1);
        EXPECT_EQ(arr[1].id, 2);
        EXPECT_EQ(arr[2].id, 3);
        EXPECT_EQ(arr[3].id, 4);
        EXPECT_EQ(arr[4].id, 5);
    }
    EXPECT_EQ(TestAllocator::allocationCount, TestAllocator::deallocationCount);
    EXPECT_EQ(Probe::constructionCount, Probe::destructionCount);
}

// Test that erase() works correctly
TEST_F(DArrayTest, Erase) {
    std::initializer_list<Probe> elements = {1, 2, 3, 4, 5};
    {
        DArrayType arr = elements;

        arr.erase(arr.begin() + 1);

        EXPECT_EQ(arr.size(), 4);
        EXPECT_EQ(arr.capacity(), 5);
        EXPECT_EQ(arr[0].id, 1);
        EXPECT_EQ(arr[1].id, 3);
        EXPECT_EQ(arr[2].id, 4);
        EXPECT_EQ(arr[3].id, 5);
    }
    EXPECT_EQ(TestAllocator::allocationCount, TestAllocator::deallocationCount);
    EXPECT_EQ(Probe::constructionCount, Probe::destructionCount);
}

// Test erase() with the last element of the array
TEST_F(DArrayTest, EraseLast) {
    std::initializer_list<Probe> elements = {1, 2, 3, 4, 5};
    {
        DArrayType arr = elements;

        arr.erase(arr.end() - 1);

        EXPECT_EQ(arr.size(), 4);
        EXPECT_EQ(arr.capacity(), 5);
        EXPECT_EQ(arr[0].id, 1);
        EXPECT_EQ(arr[1].id, 2);
        EXPECT_EQ(arr[2].id, 3);
        EXPECT_EQ(arr[3].id, 4);
    }
    EXPECT_EQ(TestAllocator::allocationCount, TestAllocator::deallocationCount);
    EXPECT_EQ(Probe::constructionCount, Probe::destructionCount);
}

// Test that range erase() workds correctly
TEST_F(DArrayTest, EraseRange) {
    std::initializer_list<Probe> elements = {1, 2, 3, 4, 5};
    {
        DArrayType arr = elements;

        arr.erase(arr.begin(), arr.begin() + 3);

        EXPECT_EQ(arr.size(), 2);
        EXPECT_EQ(arr.capacity(), 5);
        EXPECT_EQ(arr[0].id, 4);
        EXPECT_EQ(arr[1].id, 5);
    }
    EXPECT_EQ(TestAllocator::allocationCount, TestAllocator::deallocationCount);
    EXPECT_EQ(Probe::constructionCount, Probe::destructionCount);
}

// Test range erase() at the end of the array
TEST_F(DArrayTest, EraseRangeEnd) {
    std::initializer_list<Probe> elements = {1, 2, 3, 4, 5};
    {
        DArrayType arr = elements;

        arr.erase(arr.begin() + 2, arr.end());

        EXPECT_EQ(arr.size(), 2);
        EXPECT_EQ(arr.capacity(), 5);
        EXPECT_EQ(arr[0].id, 1);
        EXPECT_EQ(arr[1].id, 2);
    }
    EXPECT_EQ(TestAllocator::allocationCount, TestAllocator::deallocationCount);
    EXPECT_EQ(Probe::constructionCount, Probe::destructionCount);
}

// Test that copying push() works correctly
TEST_F(DArrayTest, PushCopy) {
    const Probe elements[] = {1, 2, 3, 4, 5};
    int expected_size[] = {1, 2, 3, 4, 5};
    int expected_capacity[] = {1, 2, 4, 4, 8};
    {
        DArrayType arr;

        for (int i = 0; i < std::size(elements); ++i) {
            arr.push(elements[i]);
            EXPECT_EQ(arr.size(), expected_size[i]);
            EXPECT_EQ(arr.capacity(), expected_capacity[i]);
            EXPECT_EQ(arr[i].id, elements[i].id);
        }
    }
    EXPECT_EQ(TestAllocator::allocationCount, TestAllocator::deallocationCount);
    EXPECT_EQ(Probe::constructionCount, Probe::destructionCount);
}

// Test copying push() with allocation failure
TEST_F(DArrayTest, PushCopyAllocFailure) {
    const Probe elements[] = {1, 2, 3, 4, 5};
    TestAllocator::allocationThrowsAt = 3;
    {
        DArrayType arr;

        EXPECT_THROW(
            {
                for (const Probe& element : elements) {
                    arr.push(element);
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

// Test copying push() with element construction failure
TEST_F(DArrayTest, PushCopyElementFailure) {
    const Probe elements[] = {1, 2, 3, 4, 5};
    Probe::constructorThrowsAt = 8;
    {
        DArrayType arr;

        EXPECT_THROW(
            {
                for (const Probe& element : elements) {
                    arr.push(element);
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

// Test that moving push() works correctly
TEST_F(DArrayTest, PushMove) {
    Probe elements[] = {1, 2, 3, 4, 5};
    int expected_size[] = {1, 2, 3, 4, 5};
    int expected_capacity[] = {1, 2, 4, 4, 8};
    {
        DArrayType arr;

        for (int i = 0; i < std::size(elements); ++i) {
            int id = elements[i].id;
            arr.push(std::move(elements[i]));
            EXPECT_EQ(arr.size(), expected_size[i]);
            EXPECT_EQ(arr.capacity(), expected_capacity[i]);
            EXPECT_EQ(arr[i].id, id);
            EXPECT_EQ(elements[i].id, -1);
        }
    }
    EXPECT_EQ(TestAllocator::allocationCount, TestAllocator::deallocationCount);
    EXPECT_EQ(Probe::constructionCount, Probe::destructionCount);
}

// Test moving push() with allocation failure
TEST_F(DArrayTest, PushMoveAllocFailure) {
    Probe elements[] = {1, 2, 3, 4, 5};
    TestAllocator::allocationThrowsAt = 3;
    {
        DArrayType arr;

        EXPECT_THROW(
            {
                for (Probe& element : elements) {
                    arr.push(std::move(element));
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

// Test that pop() works correctly
TEST_F(DArrayTest, Pop) {
    std::initializer_list<Probe> elements = {1, 2, 3, 4, 5};
    {
        DArrayType arr = elements;

        arr.pop();
        EXPECT_EQ(arr.size(), 4);
        EXPECT_EQ(arr.capacity(), 5);
        EXPECT_EQ(arr.back().id, 4);

        arr.pop();
        EXPECT_EQ(arr.size(), 3);
        EXPECT_EQ(arr.capacity(), 5);
        EXPECT_EQ(arr.back().id, 3);

        arr.pop();
        arr.pop();
        arr.pop();
        EXPECT_EQ(arr.size(), 0);
        EXPECT_EQ(arr.capacity(), 5);
        EXPECT_TRUE(arr.empty());
    }
    EXPECT_EQ(TestAllocator::allocationCount, TestAllocator::deallocationCount);
    EXPECT_EQ(Probe::constructionCount, Probe::destructionCount);
}

// Test that emplaceAtEnd() works correctly
TEST_F(DArrayTest, EmplaceAtEnd) {
    int ids[] = {1, 2, 3, 4, 5};
    int expected_size[] = {1, 2, 3, 4, 5};
    int expected_capacity[] = {1, 2, 4, 4, 8};
    {
        DArrayType arr;

        for (int i = 0; i < std::size(ids); ++i) {
            arr.emplaceAtEnd(ids[i], dummy);
            EXPECT_EQ(arr.size(), expected_size[i]);
            EXPECT_EQ(arr.capacity(), expected_capacity[i]);
            EXPECT_EQ(arr[i].id, ids[i]);
        }
    }
    EXPECT_EQ(TestAllocator::allocationCount, TestAllocator::deallocationCount);
    EXPECT_EQ(Probe::constructionCount, Probe::destructionCount);
}

// Test emplaceAtEnd() with allocation failure
TEST_F(DArrayTest, EmplaceAtEndAllocFailure) {
    int ids[] = {1, 2, 3, 4, 5};
    TestAllocator::allocationThrowsAt = 3;
    {
        DArrayType arr;

        EXPECT_THROW(
            {
                for (int id : ids) {
                    arr.emplaceAtEnd(id, dummy);
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

// Test emplaceAtEnd() with element construction failure
TEST_F(DArrayTest, EmplaceAtEndElementFailure) {
    int ids[] = {1, 2, 3, 4, 5};
    Probe::constructorThrowsAt = 8;
    {
        DArrayType arr;

        EXPECT_THROW(
            {
                for (int id : ids) {
                    arr.emplaceAtEnd(id, dummy);
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

// Test that emplace() works correctly
TEST_F(DArrayTest, Emplace) {
    std::initializer_list<Probe> elements = {1, 2, 3, 4, 5};
    {
        DArrayType arr;
        for (const Probe& element : elements) {
            arr.push(element);
        }

        arr.emplace(arr.begin() + 3, 255, dummy);

        EXPECT_EQ(arr.size(), 6);
        EXPECT_EQ(arr.capacity(), 8);
        EXPECT_EQ(arr[0].id, 1);
        EXPECT_EQ(arr[1].id, 2);
        EXPECT_EQ(arr[2].id, 3);
        EXPECT_EQ(arr[3].id, 255);
        EXPECT_EQ(arr[4].id, 4);
        EXPECT_EQ(arr[5].id, 5);
    }
    EXPECT_EQ(TestAllocator::allocationCount, TestAllocator::deallocationCount);
    EXPECT_EQ(Probe::constructionCount, Probe::destructionCount);
}

// Test that emplace() at the end of array
TEST_F(DArrayTest, EmplaceEnd) {
    std::initializer_list<Probe> elements = {1, 2, 3, 4, 5};
    {
        DArrayType arr;
        for (const Probe& element : elements) {
            arr.push(element);
        }

        arr.emplace(arr.end(), 255, dummy);

        EXPECT_EQ(arr.size(), 6);
        EXPECT_EQ(arr.capacity(), 8);
        EXPECT_EQ(arr[0].id, 1);
        EXPECT_EQ(arr[1].id, 2);
        EXPECT_EQ(arr[2].id, 3);
        EXPECT_EQ(arr[3].id, 4);
        EXPECT_EQ(arr[4].id, 5);
        EXPECT_EQ(arr[5].id, 255);
    }
    EXPECT_EQ(TestAllocator::allocationCount, TestAllocator::deallocationCount);
    EXPECT_EQ(Probe::constructionCount, Probe::destructionCount);
}

// Test emplace with reallocation
TEST_F(DArrayTest, EmplaceRealloc) {
    std::initializer_list<Probe> elements = {1, 2, 3, 4, 5};
    {
        DArrayType arr = elements;

        arr.emplace(arr.begin() + 3, 255, dummy);

        EXPECT_EQ(arr.size(), 6);
        EXPECT_EQ(arr.capacity(), 10);
        EXPECT_EQ(arr[0].id, 1);
        EXPECT_EQ(arr[1].id, 2);
        EXPECT_EQ(arr[2].id, 3);
        EXPECT_EQ(arr[3].id, 255);
        EXPECT_EQ(arr[4].id, 4);
        EXPECT_EQ(arr[5].id, 5);
    }
    EXPECT_EQ(TestAllocator::allocationCount, TestAllocator::deallocationCount);
    EXPECT_EQ(Probe::constructionCount, Probe::destructionCount);
}

// Test emplace with reallocation failure
TEST_F(DArrayTest, EmplaceReallocFailure) {
    std::initializer_list<Probe> elements = {1, 2, 3, 4, 5};
    TestAllocator::allocationThrowsAt = 2;
    {
        DArrayType arr = elements;

        EXPECT_THROW({ arr.emplace(arr.begin() + 3, 255, dummy); }, std::bad_alloc);

        EXPECT_EQ(arr.size(), 5);
        EXPECT_EQ(arr.capacity(), 5);
        EXPECT_EQ(arr[0].id, 1);
        EXPECT_EQ(arr[1].id, 2);
        EXPECT_EQ(arr[2].id, 3);
        EXPECT_EQ(arr[3].id, 4);
        EXPECT_EQ(arr[4].id, 5);
    }
    EXPECT_EQ(TestAllocator::allocationCount, TestAllocator::deallocationCount);
    EXPECT_EQ(Probe::constructionCount, Probe::destructionCount);
}

// Test emplace with element construction failure
TEST_F(DArrayTest, EmplaceElementFailure) {
    std::initializer_list<Probe> elements = {1, 2, 3, 4, 5};
    Probe::constructorThrowsAt = 15;
    {
        DArrayType arr;
        for (const Probe& element : elements) {
            arr.push(element);
        }

        EXPECT_THROW({ arr.emplace(arr.begin() + 3, 255, dummy); }, std::runtime_error);

        EXPECT_EQ(arr.size(), 5);
        EXPECT_EQ(arr.capacity(), 8);
        EXPECT_EQ(arr[0].id, 1);
        EXPECT_EQ(arr[1].id, 2);
        EXPECT_EQ(arr[2].id, 3);
        EXPECT_EQ(arr[3].id, 4);
        EXPECT_EQ(arr[4].id, 5);
    }
    EXPECT_EQ(TestAllocator::allocationCount, TestAllocator::deallocationCount);
    EXPECT_EQ(Probe::constructionCount, Probe::destructionCount);
}

// Test emplace with reallocation and element construction failure
TEST_F(DArrayTest, EmplaceReallocElementFailure) {
    std::initializer_list<Probe> elements = {1, 2, 3, 4, 5};
    Probe::constructorThrowsAt = 6;
    {
        DArrayType arr = elements;

        EXPECT_THROW({ arr.emplace(arr.begin() + 3, 255, dummy); }, std::runtime_error);

        EXPECT_EQ(arr.size(), 5);
        EXPECT_EQ(arr.capacity(), 5);
        EXPECT_EQ(arr[0].id, 1);
        EXPECT_EQ(arr[1].id, 2);
        EXPECT_EQ(arr[2].id, 3);
        EXPECT_EQ(arr[3].id, 4);
        EXPECT_EQ(arr[4].id, 5);
    }
    EXPECT_EQ(TestAllocator::allocationCount, TestAllocator::deallocationCount);
    EXPECT_EQ(Probe::constructionCount, Probe::destructionCount);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}