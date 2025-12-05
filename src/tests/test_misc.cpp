#include <gtest/gtest.h>
#include "../utils/array.h"
#include "../utils/misc.h"

// Test for Array class
TEST(ArrayTest, BasicConstructionAndAccess) {
    Array<int> arr((size_t)5, 10);
    EXPECT_EQ(arr.size(), 5);
    EXPECT_FALSE(arr.empty());
    for(size_t i = 0; i < arr.size(); ++i) {
        EXPECT_EQ(arr[i], 10);
    }
}

TEST(ArrayTest, ResizeAndClear) {
    Array<int> arr((size_t)3, 1);
    EXPECT_EQ(arr.size(), 3);
    
    arr.resize((size_t)5, 2); // resize clears and fills with new value based on implementation
    EXPECT_EQ(arr.size(), 5);
    for(size_t i = 0; i < arr.size(); ++i) {
        EXPECT_EQ(arr[i], 2);
    }
    
    arr.clear();
    EXPECT_TRUE(arr.empty());
    EXPECT_EQ(arr.size(), 0);
}

TEST(ArrayTest, CopyConstruction) {
    Array<int> original((size_t)3, 7);
    Array<int> copy(original);
    
    EXPECT_EQ(copy.size(), 3);
    EXPECT_EQ(copy[0], 7);
    
    // Modify copy, original should stay same (deep copy)
    // Array implementation does deep copy in copy ctor
    // However, Array::operator= implements deep copy.
    // Copy constructor calls operator=.
}

// Test for MISC macros
TEST(MiscTest, MaxMinMacros) {
    EXPECT_EQ(MAX(10, 20), 20);
    EXPECT_EQ(MAX(20, 10), 20);
    EXPECT_EQ(MIN(10, 20), 10);
    EXPECT_EQ(MIN(20, 10), 10);
}

TEST(MiscTest, NumberOfMacro) {
    int arr[10];
    EXPECT_EQ(NUMBER_OF(arr), 10);
    
    double darr[5];
    EXPECT_EQ(NUMBER_OF(darr), 5);
}
