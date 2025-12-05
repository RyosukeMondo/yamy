#include <gtest/gtest.h>
#include "../core/layoutmanager.h"

TEST(LayoutManagerTest, CalculateRect_LeftTopOrigin) {
    RECT originalParent = {0, 0, 100, 100}; // 100x100
    RECT originalChild = {10, 10, 50, 50};  // at 10,10 size 40x40
    RECT currentParent = {0, 0, 200, 200};  // 200x200 (doubled)
    
    LayoutManager::Origin origins[4] = {
        LayoutManager::ORIGIN_LEFT_EDGE,
        LayoutManager::ORIGIN_TOP_EDGE,
        LayoutManager::ORIGIN_LEFT_EDGE,
        LayoutManager::ORIGIN_TOP_EDGE
    };
    
    RECT newRect = LayoutManager::calculateRect(originalParent, originalChild, currentParent, origins);
    
    // Left/Top anchored: should stay at 10,10.
    // Right/Bottom also anchored to Left/Top (width/height logic depends on what we passed for right/bottom)
    // Wait, m_rc.right is passed as input. 
    // For ORIGIN_LEFT_EDGE on right edge: *outPtrs[2] = originalChildPos[2]
    // So right stays same relative to left edge.
    
    EXPECT_EQ(newRect.left, 10);
    EXPECT_EQ(newRect.top, 10);
    EXPECT_EQ(newRect.right, 50);
    EXPECT_EQ(newRect.bottom, 50);
}

TEST(LayoutManagerTest, CalculateRect_RightBottomOrigin) {
    RECT originalParent = {0, 0, 100, 100}; 
    RECT originalChild = {10, 10, 50, 50};  
    RECT currentParent = {0, 0, 200, 200};  
    
    LayoutManager::Origin origins[4] = {
        LayoutManager::ORIGIN_RIGHT_EDGE,
        LayoutManager::ORIGIN_BOTTOM_EDGE,
        LayoutManager::ORIGIN_RIGHT_EDGE,
        LayoutManager::ORIGIN_BOTTOM_EDGE
    };
    
    RECT newRect = LayoutManager::calculateRect(originalParent, originalChild, currentParent, origins);
    
    // Right/Bottom anchored.
    // Original dist from right: 100 - 10 = 90 (for left), 100 - 50 = 50 (for right)
    // New width 200.
    // New left = 200 - 90 = 110.
    // New right = 200 - 50 = 150.
    // Original dist from bottom: 100 - 10 = 90 (top), 100 - 50 = 50 (bottom)
    // New top = 200 - 90 = 110.
    // New bottom = 200 - 50 = 150.
    
    EXPECT_EQ(newRect.left, 110);
    EXPECT_EQ(newRect.top, 110);
    EXPECT_EQ(newRect.right, 150);
    EXPECT_EQ(newRect.bottom, 150);
}

TEST(LayoutManagerTest, CalculateRect_ResizeBehavior) {
    // Left anchored left/top, Right anchored right/bottom (Resize)
    RECT originalParent = {0, 0, 100, 100}; 
    RECT originalChild = {10, 10, 90, 90};  // 10px margin all around
    RECT currentParent = {0, 0, 200, 200};  
    
    LayoutManager::Origin origins[4] = {
        LayoutManager::ORIGIN_LEFT_EDGE,
        LayoutManager::ORIGIN_TOP_EDGE,
        LayoutManager::ORIGIN_RIGHT_EDGE,
        LayoutManager::ORIGIN_BOTTOM_EDGE
    };
    
    RECT newRect = LayoutManager::calculateRect(originalParent, originalChild, currentParent, origins);
    
    EXPECT_EQ(newRect.left, 10);
    EXPECT_EQ(newRect.top, 10);
    
    // Right: 200 - (100 - 90) = 200 - 10 = 190.
    EXPECT_EQ(newRect.right, 190);
    EXPECT_EQ(newRect.bottom, 190);
}

TEST(LayoutManagerTest, CalculateRect_CenterOrigin) {
    RECT originalParent = {0, 0, 100, 100}; 
    RECT originalChild = {40, 40, 60, 60};  // 20x20 rect centered at 50,50
    RECT currentParent = {0, 0, 200, 200};  
    
    LayoutManager::Origin origins[4] = {
        LayoutManager::ORIGIN_CENTER,
        LayoutManager::ORIGIN_CENTER,
        LayoutManager::ORIGIN_CENTER,
        LayoutManager::ORIGIN_CENTER
    };
    
    RECT newRect = LayoutManager::calculateRect(originalParent, originalChild, currentParent, origins);
    
    // Center logic:
    // NewPos = NewDim/2 - (OldDim/2 - OldPos)
    // Left: 200/2 - (100/2 - 40) = 100 - 10 = 90.
    // Right: 200/2 - (100/2 - 60) = 100 - (-10) = 110.
    // Width 20.
    // Centered at 100.
    
    EXPECT_EQ(newRect.left, 90);
    EXPECT_EQ(newRect.top, 90);
    EXPECT_EQ(newRect.right, 110);
    EXPECT_EQ(newRect.bottom, 110);
}
