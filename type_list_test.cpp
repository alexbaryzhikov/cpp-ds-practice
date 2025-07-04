#include <gtest/gtest.h>
#include "type_list.hpp"

TEST(TypeListTest, Size) {
    using MyList = TypeList<int, double, char, float>;
    EXPECT_EQ(MyList::size, 4);
}

TEST(TypeListTest, TypeAt) {
    using MyList = TypeList<int, double, char, float>;
    EXPECT_TRUE((std::is_same_v<MyList::type_at_t<0>, int>));
    EXPECT_TRUE((std::is_same_v<MyList::type_at_t<1>, double>));
}

TEST(TypeListTest, Contains) {
    using MyList = TypeList<int, double, char, float>;
    EXPECT_TRUE(MyList::contains_v<int>);
    EXPECT_FALSE(MyList::contains_v<bool>);
}

TEST(TypeListTest, IndexOf) {
    using MyList = TypeList<int, double, char, float>;
    EXPECT_EQ(MyList::index_of_v<double>, 1);
    EXPECT_EQ(MyList::index_of_v<bool>, -1);  // Not found
}

TEST(TypeListTest, PushFront) {
    using MyList = TypeList<int, double, char, float>;
    using ListWithBool = MyList::push_front_t<bool>;
    
    EXPECT_EQ(ListWithBool::size, MyList::size + 1);
    EXPECT_TRUE((std::is_same_v<ListWithBool::type_at_t<0>, bool>));
}

TEST(TypeListTest, PushBack) {
    using MyList = TypeList<int, double, char, float>;
    using ListWithBool = MyList::push_back_t<bool>;
    
    EXPECT_EQ(ListWithBool::size, MyList::size + 1);
    EXPECT_TRUE((std::is_same_v<ListWithBool::type_at_t<ListWithBool::size - 1>, bool>));
}

TEST(TypeListTest, Remove) {
    using MyList = TypeList<int, double, char, float>;
    using ListWithoutDouble = MyList::remove_t<double>;
    
    EXPECT_EQ(ListWithoutDouble::size, MyList::size - 1);
    EXPECT_TRUE((std::is_same_v<ListWithoutDouble::type_at_t<1>, char>));
    EXPECT_FALSE(ListWithoutDouble::contains_v<double>);
}

TEST(TypeListTest, PopFront) {
    using MyList = TypeList<int, double, char, float>;
    using ListWithoutFirst = MyList::pop_front_t;
    
    EXPECT_EQ(ListWithoutFirst::size, MyList::size - 1);
    EXPECT_TRUE((std::is_same_v<ListWithoutFirst::type_at_t<0>, double>));
}

TEST(TypeListTest, PopBack) {
    using MyList = TypeList<int, double, char, float>;
    using ListWithoutLast = MyList::pop_back_t;
    
    EXPECT_EQ(ListWithoutLast::size, MyList::size - 1);
    EXPECT_TRUE((std::is_same_v<ListWithoutLast::type_at_t<ListWithoutLast::size - 1>, char>));
    EXPECT_FALSE(ListWithoutLast::contains_v<float>);
}

TEST(TypeListTest, Equality) {
    using MyList = TypeList<int, double, char, float>;
    using SameAsList = TypeList<int, double, char, float>;
    using DifferentList = TypeList<double, int, char, float>;
    
    EXPECT_TRUE((MyList{} == SameAsList{}));
    EXPECT_FALSE((MyList{} == DifferentList{}));
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
} 