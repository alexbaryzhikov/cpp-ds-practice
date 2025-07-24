#include "unique_pointer.hpp"
#include <gtest/gtest.h>
#include <initializer_list>

// =============================================================================
// Concepts
// =============================================================================

struct SafeDereference {
    int& operator*() noexcept;
};

struct UnsafeDereference {
    int& operator*();
};

TEST(ConceptsTest, NoExceptDeref) {
    ASSERT_TRUE(NoExceptDeref<int*>);
    ASSERT_TRUE(NoExceptDeref<SafeDereference>);

    ASSERT_FALSE(NoExceptDeref<int>);
    ASSERT_FALSE(NoExceptDeref<void*>);
    ASSERT_FALSE(NoExceptDeref<UnsafeDereference>);
}

struct UnsafeSwap {
    int id;
};

void swap(UnsafeSwap&, UnsafeSwap&) {}

TEST(ConceptsTest, NoExceptSwap) {
    ASSERT_TRUE(NoExceptSwap<int>);

    ASSERT_FALSE(NoExceptSwap<UnsafeSwap>);
}

struct Base {};

struct Derived : public Base {};

TEST(ConceptsTest, ConvertiblePointer) {
    ASSERT_TRUE((ConvertiblePointer<int, int>));
    ASSERT_TRUE((ConvertiblePointer<int, void>));
    ASSERT_TRUE((ConvertiblePointer<int, const int>));
    ASSERT_TRUE((ConvertiblePointer<Derived, Base>));

    ASSERT_FALSE((ConvertiblePointer<void, int>));
    ASSERT_FALSE((ConvertiblePointer<short, int>));
    ASSERT_FALSE((ConvertiblePointer<double, int>));
    ASSERT_FALSE((ConvertiblePointer<const int, int>));
    ASSERT_FALSE((ConvertiblePointer<Base, Derived>));
}

struct Complete {};

struct Incomplete;

TEST(ConceptsTest, CompleteType) {
    ASSERT_TRUE(CompleteType<int>);
    ASSERT_TRUE(CompleteType<int[5]>);
    ASSERT_TRUE(CompleteType<Complete>);

    ASSERT_FALSE(CompleteType<void>);
    ASSERT_FALSE(CompleteType<int[]>);
    ASSERT_FALSE(CompleteType<Incomplete>);
}

// =============================================================================
// UniquePointer
// =============================================================================

struct TestDeleter {
    static int copyConstructedCount;
    static int copyAssignedCount;
    static int callCount;
    static bool verbose;

    static void reset() {
        copyConstructedCount = 0;
        copyAssignedCount = 0;
        callCount = 0;
        verbose = false;
    }

    TestDeleter() {
        if (verbose) {
            std::println("Constructed");
        }
    }

    TestDeleter(const TestDeleter&) {
        if (verbose) {
            std::println("{:2}) Copy constructed", copyConstructedCount + 1);
        }
        ++copyConstructedCount;
    }

    TestDeleter& operator=(const TestDeleter&) {
        if (verbose) {
            std::println("{:2}) Copy assigned", copyAssignedCount + 1);
        }
        ++copyAssignedCount;
        return *this;
    }

    template <typename T>
    void operator()(T* ptr) const noexcept {
        if (verbose) {
            std::println("{:2}) Called", callCount + 1);
        }
        delete ptr;
        ++callCount;
    }
};

int TestDeleter::copyConstructedCount = 0;
int TestDeleter::copyAssignedCount = 0;
int TestDeleter::callCount = 0;
bool TestDeleter::verbose = false;

class UniquePointerTest : public ::testing::Test {
protected:
    void SetUp() override {
        TestDeleter::reset();
    }
};

TEST_F(UniquePointerTest, DefaultConstructor) {
    UniquePointer<int> ptr;

    EXPECT_EQ(ptr.get(), nullptr);
}

TEST_F(UniquePointerTest, NullPtrConstructor) {
    UniquePointer<int> ptr = nullptr;

    EXPECT_EQ(ptr.get(), nullptr);
}

TEST_F(UniquePointerTest, ConstructorWithValue) {
    int* rawPtr = new int(42);

    UniquePointer<int> ptr = rawPtr;

    EXPECT_EQ(ptr.get(), rawPtr);
    EXPECT_EQ(*ptr, 42);
}

TEST_F(UniquePointerTest, MoveConstructor) {
    int* rawPtr = new int(42);

    UniquePointer<int, TestDeleter> ptr1 = rawPtr;
    UniquePointer<int, TestDeleter> ptr2 = std::move(ptr1);

    EXPECT_EQ(ptr1.get(), nullptr);
    EXPECT_EQ(ptr2.get(), rawPtr);
    EXPECT_EQ(*ptr2, 42);
    EXPECT_EQ(TestDeleter::copyConstructedCount, 1);
    EXPECT_EQ(TestDeleter::copyAssignedCount, 0);
    EXPECT_EQ(TestDeleter::callCount, 0);
}

TEST_F(UniquePointerTest, MoveConstructorConvertiblePointer) {
    int* rawPtr = new int(42);

    UniquePointer<int, TestDeleter> ptr1 = rawPtr;
    UniquePointer<const int, TestDeleter> ptr2 = std::move(ptr1);

    EXPECT_EQ(ptr1.get(), nullptr);
    EXPECT_EQ(ptr2.get(), rawPtr);
    EXPECT_EQ(*ptr2, 42);
    EXPECT_EQ(TestDeleter::copyConstructedCount, 1);
    EXPECT_EQ(TestDeleter::copyAssignedCount, 0);
    EXPECT_EQ(TestDeleter::callCount, 0);
}

TEST_F(UniquePointerTest, Destructor) {
    int* rawPtr = new int(42);
    {
        UniquePointer<int, TestDeleter> ptr = rawPtr;
    }
    EXPECT_EQ(TestDeleter::callCount, 1);
}

TEST_F(UniquePointerTest, MoveAssignment) {
    int* rawPtr1 = new int(42);
    int* rawPtr2 = new int(100);

    UniquePointer<int, TestDeleter> ptr1 = rawPtr1;
    UniquePointer<int, TestDeleter> ptr2 = rawPtr2;

    ptr2 = std::move(ptr1);

    EXPECT_EQ(ptr1.get(), nullptr);
    EXPECT_EQ(ptr2.get(), rawPtr1);
    EXPECT_EQ(*ptr2, 42);
    EXPECT_EQ(TestDeleter::copyConstructedCount, 0);
    EXPECT_EQ(TestDeleter::copyAssignedCount, 1);
    EXPECT_EQ(TestDeleter::callCount, 1);
}

TEST_F(UniquePointerTest, MoveAssignmentConvertiblePointer) {
    int* rawPtr1 = new int(42);
    const int* rawPtr2 = new int(100);

    UniquePointer<int, TestDeleter> ptr1 = rawPtr1;
    UniquePointer<const int, TestDeleter> ptr2 = rawPtr2;

    ptr2 = std::move(ptr1);

    EXPECT_EQ(ptr1.get(), nullptr);
    EXPECT_EQ(ptr2.get(), rawPtr1);
    EXPECT_EQ(*ptr2, 42);
    EXPECT_EQ(TestDeleter::copyConstructedCount, 0);
    EXPECT_EQ(TestDeleter::copyAssignedCount, 1);
    EXPECT_EQ(TestDeleter::callCount, 1);
}

TEST_F(UniquePointerTest, NullPtrAssignment) {
    UniquePointer<int, TestDeleter> ptr = new int(42);

    ptr = nullptr;

    EXPECT_EQ(ptr.get(), nullptr);
    EXPECT_EQ(TestDeleter::callCount, 1);
}

TEST_F(UniquePointerTest, Release) {
    UniquePointer<int, TestDeleter> ptr = new int(42);

    int* rawPtr = ptr.release();

    EXPECT_NE(rawPtr, nullptr);
    EXPECT_EQ(*rawPtr, 42);
    EXPECT_EQ(ptr.get(), nullptr);
    EXPECT_EQ(TestDeleter::callCount, 0);
    delete rawPtr;
}

TEST_F(UniquePointerTest, Reset) {
    UniquePointer<int, TestDeleter> ptr = new int(42);

    ptr.reset();

    EXPECT_EQ(ptr.get(), nullptr);
    EXPECT_EQ(TestDeleter::callCount, 1);
}

TEST_F(UniquePointerTest, ResetWithPointer) {
    UniquePointer<int, TestDeleter> ptr = new int(42);
    int* rawPtr = new int(100);

    ptr.reset(rawPtr);

    EXPECT_EQ(ptr.get(), rawPtr);
    EXPECT_EQ(*ptr, 100);
    EXPECT_EQ(TestDeleter::callCount, 1);
}

TEST_F(UniquePointerTest, Swap) {
    int* rawPtr1 = new int(42);
    int* rawPtr2 = new int(100);
    UniquePointer<int> ptr1 = rawPtr1;
    UniquePointer<int> ptr2 = rawPtr2;

    ptr1.swap(ptr2);

    EXPECT_EQ(ptr1.get(), rawPtr2);
    EXPECT_EQ(*ptr1, 100);
    EXPECT_EQ(ptr2.get(), rawPtr1);
    EXPECT_EQ(*ptr2, 42);
}

TEST_F(UniquePointerTest, Get) {
    UniquePointer<int> ptr = new int(42);

    EXPECT_NE(ptr.get(), nullptr);
    EXPECT_EQ(*ptr.get(), 42);
}

TEST_F(UniquePointerTest, BoolOperator) {
    UniquePointer<int> ptr1;
    EXPECT_FALSE(ptr1);

    UniquePointer<int> ptr2 = new int(42);
    EXPECT_TRUE(ptr2);
}

TEST_F(UniquePointerTest, DereferenceOperator) {
    UniquePointer<int> ptr = new int(42);
    EXPECT_EQ(*ptr, 42);

    *ptr = 100;
    EXPECT_EQ(*ptr, 100);
}

TEST_F(UniquePointerTest, ArrowOperator) {
    struct MyStruct {
        int value;
        int getValue() { return value; }
    };

    UniquePointer<MyStruct> ptr = new MyStruct(42);
    EXPECT_EQ(ptr->value, 42);
    EXPECT_EQ(ptr->getValue(), 42);

    ptr->value = 100;
    EXPECT_EQ(ptr->value, 100);
    EXPECT_EQ(ptr->getValue(), 100);
}
// =============================================================================
// UniquePointer (array version)
// =============================================================================

struct TestArrayDeleter {
    static int copyConstructedCount;
    static int copyAssignedCount;
    static int callCount;
    static bool verbose;

    static void reset() {
        copyConstructedCount = 0;
        copyAssignedCount = 0;
        callCount = 0;
        verbose = false;
    }

    TestArrayDeleter() {
        if (verbose) {
            std::println("Constructed");
        }
    }

    TestArrayDeleter(const TestArrayDeleter&) {
        if (verbose) {
            std::println("{:2}) Copy constructed", copyConstructedCount + 1);
        }
        ++copyConstructedCount;
    }

    TestArrayDeleter& operator=(const TestArrayDeleter&) {
        if (verbose) {
            std::println("{:2}) Copy assigned", copyAssignedCount + 1);
        }
        ++copyAssignedCount;
        return *this;
    }

    template <typename T>
    void operator()(T* ptr) const noexcept {
        if (verbose) {
            std::println("{:2}) Called", callCount + 1);
        }
        delete[] ptr;
        ++callCount;
    }
};

int TestArrayDeleter::copyConstructedCount = 0;
int TestArrayDeleter::copyAssignedCount = 0;
int TestArrayDeleter::callCount = 0;
bool TestArrayDeleter::verbose = false;

int* makeArray(std::initializer_list<int> nums) {
    int* ptr = new int[nums.size()];
    size_t i = 0;
    for (int num : nums)
        ptr[i++] = num;
    return ptr;
}

class UniquePointerArrayTest : public ::testing::Test {
protected:
    void SetUp() override {
        TestArrayDeleter::reset();
    }
};

TEST_F(UniquePointerArrayTest, DefaultConstructor) {
    UniquePointer<int[]> ptr;

    EXPECT_EQ(ptr.get(), nullptr);
}

TEST_F(UniquePointerArrayTest, NullPtrConstructor) {
    UniquePointer<int[]> ptr = nullptr;

    EXPECT_EQ(ptr.get(), nullptr);
}

TEST_F(UniquePointerArrayTest, ConstructorWithValue) {
    int* rawPtr = makeArray({42, 43, 44});

    UniquePointer<int[]> ptr = rawPtr;

    EXPECT_EQ(ptr.get(), rawPtr);
    EXPECT_EQ(ptr[0], 42);
    EXPECT_EQ(ptr[1], 43);
    EXPECT_EQ(ptr[2], 44);
}

TEST_F(UniquePointerArrayTest, MoveConstructor) {
    int* rawPtr = makeArray({42, 43, 44});

    UniquePointer<int[], TestArrayDeleter> ptr1 = rawPtr;
    UniquePointer<int[], TestArrayDeleter> ptr2 = std::move(ptr1);

    EXPECT_EQ(ptr1.get(), nullptr);
    EXPECT_EQ(ptr2.get(), rawPtr);
    EXPECT_EQ(ptr2[0], 42);
    EXPECT_EQ(ptr2[1], 43);
    EXPECT_EQ(ptr2[2], 44);
    EXPECT_EQ(TestArrayDeleter::copyConstructedCount, 1);
    EXPECT_EQ(TestArrayDeleter::copyAssignedCount, 0);
    EXPECT_EQ(TestArrayDeleter::callCount, 0);
}

TEST_F(UniquePointerArrayTest, MoveConstructorConvertiblePointer) {
    int* rawPtr = makeArray({42, 43, 44});

    UniquePointer<int[], TestArrayDeleter> ptr1 = rawPtr;
    UniquePointer<const int[], TestArrayDeleter> ptr2 = std::move(ptr1);

    EXPECT_EQ(ptr1.get(), nullptr);
    EXPECT_EQ(ptr2.get(), rawPtr);
    EXPECT_EQ(ptr2[0], 42);
    EXPECT_EQ(ptr2[1], 43);
    EXPECT_EQ(ptr2[2], 44);
    EXPECT_EQ(TestArrayDeleter::copyConstructedCount, 1);
    EXPECT_EQ(TestArrayDeleter::copyAssignedCount, 0);
    EXPECT_EQ(TestArrayDeleter::callCount, 0);
}

TEST_F(UniquePointerArrayTest, Destructor) {
    int* rawPtr = makeArray({42, 43, 44});
    {
        UniquePointer<int[], TestArrayDeleter> ptr = rawPtr;
    }
    EXPECT_EQ(TestArrayDeleter::callCount, 1);
}

TEST_F(UniquePointerArrayTest, MoveAssignment) {
    int* rawPtr1 = makeArray({42, 43, 44});
    int* rawPtr2 = makeArray({100, 200, 300});

    UniquePointer<int[], TestArrayDeleter> ptr1 = rawPtr1;
    UniquePointer<int[], TestArrayDeleter> ptr2 = rawPtr2;

    ptr2 = std::move(ptr1);

    EXPECT_EQ(ptr1.get(), nullptr);
    EXPECT_EQ(ptr2.get(), rawPtr1);
    EXPECT_EQ(ptr2[0], 42);
    EXPECT_EQ(ptr2[1], 43);
    EXPECT_EQ(ptr2[2], 44);
    EXPECT_EQ(TestArrayDeleter::copyConstructedCount, 0);
    EXPECT_EQ(TestArrayDeleter::copyAssignedCount, 1);
    EXPECT_EQ(TestArrayDeleter::callCount, 1);
}

TEST_F(UniquePointerArrayTest, MoveAssignmentConvertiblePointer) {
    int* rawPtr1 = makeArray({42, 43, 44});
    const int* rawPtr2 = makeArray({100, 200, 300});

    UniquePointer<int[], TestArrayDeleter> ptr1 = rawPtr1;
    UniquePointer<const int[], TestArrayDeleter> ptr2 = rawPtr2;

    ptr2 = std::move(ptr1);

    EXPECT_EQ(ptr1.get(), nullptr);
    EXPECT_EQ(ptr2.get(), rawPtr1);
    EXPECT_EQ(ptr2[0], 42);
    EXPECT_EQ(ptr2[1], 43);
    EXPECT_EQ(ptr2[2], 44);
    EXPECT_EQ(TestArrayDeleter::copyConstructedCount, 0);
    EXPECT_EQ(TestArrayDeleter::copyAssignedCount, 1);
    EXPECT_EQ(TestArrayDeleter::callCount, 1);
}

TEST_F(UniquePointerArrayTest, NullPtrAssignment) {
    UniquePointer<int[], TestArrayDeleter> ptr = makeArray({42, 43, 44});

    ptr = nullptr;

    EXPECT_EQ(ptr.get(), nullptr);
    EXPECT_EQ(TestArrayDeleter::callCount, 1);
}

TEST_F(UniquePointerArrayTest, Release) {
    UniquePointer<int[], TestArrayDeleter> ptr = makeArray({42, 43, 44});

    int* rawPtr = ptr.release();

    EXPECT_NE(rawPtr, nullptr);
    EXPECT_EQ(rawPtr[0], 42);
    EXPECT_EQ(rawPtr[1], 43);
    EXPECT_EQ(rawPtr[2], 44);
    EXPECT_EQ(ptr.get(), nullptr);
    EXPECT_EQ(TestArrayDeleter::callCount, 0);
    delete[] rawPtr;
}

TEST_F(UniquePointerArrayTest, Reset) {
    UniquePointer<int[], TestArrayDeleter> ptr = makeArray({42, 43, 44});

    ptr.reset();

    EXPECT_EQ(ptr.get(), nullptr);
    EXPECT_EQ(TestArrayDeleter::callCount, 1);
}

TEST_F(UniquePointerArrayTest, ResetWithPointer) {
    UniquePointer<int[], TestArrayDeleter> ptr = makeArray({42, 43, 44});
    int* rawPtr = makeArray({100, 200, 300});

    ptr.reset(rawPtr);

    EXPECT_EQ(ptr.get(), rawPtr);
    EXPECT_EQ(ptr[0], 100);
    EXPECT_EQ(ptr[1], 200);
    EXPECT_EQ(ptr[2], 300);
    EXPECT_EQ(TestArrayDeleter::callCount, 1);
}

TEST_F(UniquePointerArrayTest, Swap) {
    int* rawPtr1 = makeArray({42, 43, 44});
    int* rawPtr2 = makeArray({100, 200, 300});
    UniquePointer<int[]> ptr1 = rawPtr1;
    UniquePointer<int[]> ptr2 = rawPtr2;

    ptr1.swap(ptr2);

    EXPECT_EQ(ptr1.get(), rawPtr2);
    EXPECT_EQ(ptr1[0], 100);
    EXPECT_EQ(ptr1[1], 200);
    EXPECT_EQ(ptr1[2], 300);
    EXPECT_EQ(ptr2.get(), rawPtr1);
    EXPECT_EQ(ptr2[0], 42);
    EXPECT_EQ(ptr2[1], 43);
    EXPECT_EQ(ptr2[2], 44);
}

TEST_F(UniquePointerArrayTest, Get) {
    UniquePointer<int[]> ptr = makeArray({42, 43, 44});

    EXPECT_NE(ptr.get(), nullptr);
    EXPECT_EQ(*ptr.get(), 42);
    EXPECT_EQ(*(ptr.get() + 1), 43);
    EXPECT_EQ(*(ptr.get() + 2), 44);
}

TEST_F(UniquePointerArrayTest, BoolOperator) {
    UniquePointer<int[]> ptr1;
    EXPECT_FALSE(ptr1);

    UniquePointer<int[]> ptr2 = makeArray({42, 43, 44});
    EXPECT_TRUE(ptr2);
}

TEST_F(UniquePointerArrayTest, SubscriptOperator) {
    UniquePointer<int[]> ptr = makeArray({42, 43, 44});
    EXPECT_EQ(ptr[0], 42);
    EXPECT_EQ(ptr[1], 43);
    EXPECT_EQ(ptr[2], 44);

    ptr[0] = 100;
    ptr[1] = 200;
    ptr[2] = 300;

    EXPECT_EQ(ptr[0], 100);
    EXPECT_EQ(ptr[1], 200);
    EXPECT_EQ(ptr[2], 300);
}

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
