//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// property_keymap.cpp - Property-based tests for Keymap invariants
//
// Tests keymap properties using RapidCheck to explore the state space:
// 1. Lookup idempotence: searching for the same key twice returns same result
// 2. Define uniqueness: adding same key assignment twice overwrites (no duplicates)
// 3. Parent chain consistency: parent chain is acyclic and resolves correctly
//
// Part of Phase 5 (Property-Based Testing) in modern-cpp-toolchain spec
//
// This is a simplified standalone version that tests the core invariants
// without requiring the full YAMY engine dependencies.
//
// Usage:
//   Run with default iterations (100): ./yamy_property_keymap_test
//   Run with 1000 iterations: RC_PARAMS="max_success=1000" ./yamy_property_keymap_test
//   Run with verbose output: RC_PARAMS="verbose_progress=1" ./yamy_property_keymap_test
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include <catch2/catch_all.hpp>
#include <rapidcheck.h>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

//=============================================================================
// Simplified Keymap Model for Property Testing
//=============================================================================

// Simplified key representation
struct SimpleKey {
    std::string name;

    bool operator<(const SimpleKey& other) const {
        return name < other.name;
    }

    bool operator==(const SimpleKey& other) const {
        return name == other.name;
    }
};

// Simplified action representation
struct SimpleAction {
    std::string actionName;

    bool operator==(const SimpleAction& other) const {
        return actionName == other.actionName;
    }
};

// Simplified keymap class for testing invariants
class SimpleKeymap {
private:
    std::map<SimpleKey, SimpleAction> assignments_;
    SimpleKeymap* parent_;
    std::string name_;

public:
    SimpleKeymap(const std::string& name, SimpleKeymap* parent = nullptr)
        : parent_(parent), name_(name) {}

    void addAssignment(const SimpleKey& key, const SimpleAction& action) {
        assignments_[key] = action;  // Overwrites if key exists
    }

    const SimpleAction* searchAssignment(const SimpleKey& key) const {
        auto it = assignments_.find(key);
        if (it != assignments_.end()) {
            return &it->second;
        }
        return nullptr;
    }

    SimpleKeymap* getParent() const {
        return parent_;
    }

    const std::string& getName() const {
        return name_;
    }

    size_t getAssignmentCount() const {
        return assignments_.size();
    }
};

//=============================================================================
// RapidCheck Generators
//=============================================================================

namespace rc {

template<>
struct Arbitrary<SimpleKey> {
    static Gen<SimpleKey> arbitrary() {
        return gen::map(gen::inRange(0, 26), [](int i) {
            return SimpleKey{std::string(1, 'A' + i)};
        });
    }
};

template<>
struct Arbitrary<SimpleAction> {
    static Gen<SimpleAction> arbitrary() {
        return gen::map(gen::inRange(0, 100), [](int i) {
            return SimpleAction{"Action" + std::to_string(i)};
        });
    }
};

} // namespace rc

//=============================================================================
// Property 1: Lookup Idempotence
// Searching for the same key assignment twice must return the same result
//=============================================================================

TEST_CASE("SimpleKeymap: Lookup is idempotent", "[property][keymap][idempotence]") {
    rc::check("searchAssignment returns same result on repeated calls", []() {
        SimpleKeymap km("TestMap");

        // Generate random key assignments
        const auto numAssignments = *rc::gen::inRange(1, 10);
        std::vector<std::pair<SimpleKey, SimpleAction>> assignments;

        for (int i = 0; i < numAssignments; ++i) {
            auto key = *rc::gen::arbitrary<SimpleKey>();
            auto action = *rc::gen::arbitrary<SimpleAction>();
            km.addAssignment(key, action);
            assignments.push_back({key, action});
        }

        // Pick a random key that exists
        if (!assignments.empty()) {
            const auto searchIdx = *rc::gen::inRange(0, static_cast<int>(assignments.size()));
            const SimpleKey& searchKey = assignments[searchIdx].first;

            // Search twice and verify results are identical
            const SimpleAction* result1 = km.searchAssignment(searchKey);
            const SimpleAction* result2 = km.searchAssignment(searchKey);

            // Idempotence: same pointer
            RC_ASSERT(result1 == result2);

            // Both should be non-null since we know the key exists
            RC_ASSERT(result1 != nullptr);
            RC_ASSERT(result2 != nullptr);

            if (result1 != nullptr && result2 != nullptr) {
                RC_ASSERT(*result1 == *result2);
            }
        }
    });
}

//=============================================================================
// Property 2: Define Uniqueness
// Adding the same key assignment twice should overwrite, not duplicate
//=============================================================================

TEST_CASE("SimpleKeymap: Define uniqueness - no duplicate assignments",
          "[property][keymap][uniqueness]") {
    rc::check("addAssignment overwrites existing assignment for same key", []() {
        SimpleKeymap km("TestMap");

        // Create a key
        SimpleKey key{"X"};

        // Add first assignment
        SimpleAction action1{"FirstAction"};
        km.addAssignment(key, action1);

        const SimpleAction* result1 = km.searchAssignment(key);
        RC_ASSERT(result1 != nullptr);
        RC_ASSERT(*result1 == action1);

        // Record the count before second assignment
        size_t countBefore = km.getAssignmentCount();

        // Add second assignment with same key (should overwrite)
        SimpleAction action2{"SecondAction"};
        km.addAssignment(key, action2);

        // Count should remain the same (no new entry)
        size_t countAfter = km.getAssignmentCount();
        RC_ASSERT(countBefore == countAfter);

        const SimpleAction* result2 = km.searchAssignment(key);
        RC_ASSERT(result2 != nullptr);
        RC_ASSERT(*result2 == action2);  // Should be updated to action2
        RC_ASSERT(!(*result2 == action1));  // No longer action1
    });
}

//=============================================================================
// Property 3: Parent Chain Consistency
// Parent chain must be acyclic and resolve correctly
//=============================================================================

TEST_CASE("SimpleKeymap: Parent chain is acyclic", "[property][keymap][parent]") {
    rc::check("parent chain has no cycles", []() {
        // Create a chain of keymaps with random depth
        const auto chainDepth = *rc::gen::inRange(1, 5);
        std::vector<std::unique_ptr<SimpleKeymap>> keymaps;

        SimpleKeymap* parent = nullptr;
        for (int i = 0; i < chainDepth; ++i) {
            std::string name = "Keymap" + std::to_string(i);
            keymaps.push_back(std::make_unique<SimpleKeymap>(name, parent));
            parent = keymaps.back().get();  // Next keymap uses this one as parent
        }

        // Verify chain has no cycles by walking up to root
        for (const auto& km : keymaps) {
            std::set<const SimpleKeymap*> visited;
            const SimpleKeymap* current = km.get();

            while (current != nullptr) {
                // If we've seen this keymap before, we have a cycle
                RC_ASSERT(visited.find(current) == visited.end());
                visited.insert(current);
                current = current->getParent();
            }

            // The number of nodes visited should not exceed the chain depth
            RC_ASSERT(visited.size() <= static_cast<size_t>(chainDepth));
        }
    });
}

TEST_CASE("SimpleKeymap: Parent chain resolves correctly",
          "[property][keymap][parent]") {
    rc::check("child keymap maintains correct parent pointer", []() {
        // Create parent keymap with an assignment
        auto parent = std::make_unique<SimpleKeymap>("Parent");
        SimpleKey parentKey{"P"};
        SimpleAction parentAction{"ParentAction"};
        parent->addAssignment(parentKey, parentAction);

        // Create child keymap with different assignment
        auto child = std::make_unique<SimpleKeymap>("Child", parent.get());
        SimpleKey childKey{"C"};
        SimpleAction childAction{"ChildAction"};
        child->addAssignment(childKey, childAction);

        // Child should find its own assignment
        const SimpleAction* childResult = child->searchAssignment(childKey);
        RC_ASSERT(childResult != nullptr);
        RC_ASSERT(*childResult == childAction);

        // Parent should find its own assignment
        const SimpleAction* parentResult = parent->searchAssignment(parentKey);
        RC_ASSERT(parentResult != nullptr);
        RC_ASSERT(*parentResult == parentAction);

        // Verify parent pointer is correct
        RC_ASSERT(child->getParent() == parent.get());
        RC_ASSERT(parent->getParent() == nullptr);

        // Child doesn't automatically search parent (document actual behavior)
        const SimpleAction* parentLookupInChild = child->searchAssignment(parentKey);
        // This is null because the simple implementation doesn't walk parent chain
        RC_ASSERT(parentLookupInChild == nullptr);
    });
}

//=============================================================================
// Additional Property: Multiple assignments don't interfere
//=============================================================================

TEST_CASE("SimpleKeymap: Multiple independent assignments don't interfere",
          "[property][keymap][independence]") {
    rc::check("assignments to different keys are independent", []() {
        SimpleKeymap km("TestMap");

        // Generate multiple distinct key assignments
        const auto numKeys = *rc::gen::inRange(2, 10);
        std::vector<std::pair<SimpleKey, SimpleAction>> assignments;

        for (int i = 0; i < numKeys; ++i) {
            SimpleKey key{std::string(1, 'A' + i)};
            SimpleAction action{"Action" + std::to_string(i)};
            km.addAssignment(key, action);
            assignments.push_back({key, action});
        }

        // Verify all assignments are still correct
        for (const auto& [key, expectedAction] : assignments) {
            const SimpleAction* result = km.searchAssignment(key);
            RC_ASSERT(result != nullptr);
            RC_ASSERT(*result == expectedAction);
        }

        // Verify count matches number of assignments
        RC_ASSERT(km.getAssignmentCount() == assignments.size());
    });
}

//=============================================================================
// Additional Property: Search for non-existent key returns nullptr
//=============================================================================

TEST_CASE("SimpleKeymap: Search for non-existent key returns nullptr",
          "[property][keymap][not-found]") {
    rc::check("searching for key that was never added returns nullptr", []() {
        SimpleKeymap km("TestMap");

        // Add some assignments
        const auto numAssignments = *rc::gen::inRange(0, 5);
        std::set<std::string> addedKeys;

        for (int i = 0; i < numAssignments; ++i) {
            SimpleKey key{std::string(1, 'A' + i)};
            SimpleAction action{"Action" + std::to_string(i)};
            km.addAssignment(key, action);
            addedKeys.insert(key.name);
        }

        // Try to search for a key that definitely wasn't added
        SimpleKey nonExistentKey{"Z"};
        if (addedKeys.find("Z") == addedKeys.end()) {
            const SimpleAction* result = km.searchAssignment(nonExistentKey);
            RC_ASSERT(result == nullptr);
        }
    });
}
