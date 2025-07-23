cmake_minimum_required(VERSION 3.14)
project(cpp-ds-practice)

# Set C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include FetchContent for downloading Google Test
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.zip
    DOWNLOAD_EXTRACT_TIMESTAMP true
)
FetchContent_MakeAvailable(googletest)

add_executable(scratch scratch.cpp)

add_executable(dynamic_array_example dynamic_array_example.cpp)

add_executable(dynamic_array_test dynamic_array_test.cpp)
target_link_libraries(dynamic_array_test GTest::gtest_main)

add_executable(unique_pointer_example unique_pointer_example.cpp)

add_executable(type_list_example type_list_example.cpp)

add_executable(type_list_test type_list_test.cpp)
target_link_libraries(type_list_test GTest::gtest_main)

enable_testing()
add_test(NAME type_list_test COMMAND type_list_test)
add_test(NAME dynamic_array_test COMMAND dynamic_array_test) 