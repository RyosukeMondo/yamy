# Property-Based Testing Guide

## Introduction

Property-based testing (PBT) is a testing methodology that verifies code properties hold for a wide range of random inputs. Instead of writing individual test cases with specific values, you define general properties that must always be true, and the testing framework (RapidCheck) generates hundreds or thousands of random test cases to verify them.

**Why property-based testing?**
- **Broader coverage**: Tests edge cases you wouldn't think to write manually
- **Automatic shrinking**: When a test fails, the framework minimizes the failing input to the simplest case
- **Specification by invariants**: Properties document what your code *must* do, not just what it *does* do

**When to use PBT in YAMY:**
- State machines (modifier tracking, layer switching)
- Data structures with invariants (keymaps, prefix history)
- Algorithms with mathematical properties (idempotence, commutativity)
- Stress testing (handling large event sequences)

## Property-Based Testing vs Unit Testing

| Aspect | Unit Testing | Property-Based Testing |
|--------|--------------|------------------------|
| **Test input** | Hand-written specific values | Random generated values |
| **Test count** | One per test case | Hundreds per property (configurable) |
| **Coverage** | Tests known cases | Explores unknown edge cases |
| **Failure debugging** | Immediate (you wrote the input) | Requires shrinking to minimal case |
| **Best for** | Regression tests, specific bugs | Invariants, state machines, algorithms |

**In YAMY, we use both:**
- Unit tests for specific behavior and regressions
- Property tests for state machine invariants and stress testing

## Writing Good Properties

### Good Properties are Universal

❌ **Bad**: Specific case disguised as a property
```cpp
rc::check("lookup returns correct value", []() {
    SimpleKeymap km("TestMap");
    km.addAssignment(SimpleKey{"A"}, SimpleAction{"Action1"});
    const auto* result = km.searchAssignment(SimpleKey{"A"});
    RC_ASSERT(result != nullptr);
    RC_ASSERT(*result == SimpleAction{"Action1"});
});
```

✅ **Good**: Universal property about behavior
```cpp
rc::check("searching for the same key twice returns same result", []() {
    SimpleKeymap km("TestMap");

    // Generate random assignments
    const auto numAssignments = *rc::gen::inRange(1, 10);
    for (int i = 0; i < numAssignments; ++i) {
        auto key = *rc::gen::arbitrary<SimpleKey>();
        auto action = *rc::gen::arbitrary<SimpleAction>();
        km.addAssignment(key, action);
    }

    // Pick random key and verify idempotence
    auto searchKey = *rc::gen::arbitrary<SimpleKey>();
    const auto* result1 = km.searchAssignment(searchKey);
    const auto* result2 = km.searchAssignment(searchKey);

    RC_ASSERT(result1 == result2);  // Idempotence
});
```

### Good Properties Test Invariants

**Invariant**: A condition that must always be true, regardless of inputs or state.

**Common invariants in YAMY:**

1. **Data structure invariants**: `tests/property_keymap.cpp:203-232`
   - Keymap parent chains are acyclic
   - History depth never exceeds `MAX_KEYMAP_PREFIX_HISTORY`

2. **State machine invariants**: `tests/property_modifier.cpp:254-293`
   - No stuck keys after event sequence
   - Standard modifiers (Shift, Ctrl, Alt, Win) can always be released

3. **Operation properties**: `tests/property_keymap.cpp:164-196`
   - Assigning the same key twice overwrites (no duplicates)
   - Assignment count matches number of unique keys

### Good Properties are Composable

Break complex properties into smaller testable pieces:

```cpp
// Property 1: Keys can be pressed
TEST_CASE("ModifierState: Key events update state correctly")

// Property 2: Keys can be released
TEST_CASE("ModifierState: No stuck keys after event sequence")

// Property 3: Multiple keys work together
TEST_CASE("ModifierState: Multiple simultaneous modifiers")
```

## RapidCheck Basics

### Generators

Generators produce random test data. RapidCheck provides built-in generators:

```cpp
// Generate random integers in range [0, 26)
auto value = *rc::gen::inRange(0, 26);

// Generate random boolean
auto flag = *rc::gen::arbitrary<bool>();

// Generate from a list of values
auto key = *rc::gen::elementOf(ALL_MODIFIER_KEYS);

// Generate containers (vectors, sets, etc.)
auto values = *rc::gen::container<std::vector<int>>(rc::gen::inRange(0, 100));
```

### Custom Generators

Define generators for your own types using `Arbitrary<T>`:

```cpp
namespace rc {
template<>
struct Arbitrary<SimpleKey> {
    static Gen<SimpleKey> arbitrary() {
        return gen::map(gen::inRange(0, 26), [](int i) {
            return SimpleKey{std::string(1, 'A' + i)};
        });
    }
};
}
```

Now you can use `*rc::gen::arbitrary<SimpleKey>()` anywhere. See `tests/property_keymap.cpp:94-114`

### Assertions

Use `RC_ASSERT` for property checks (not `REQUIRE` or `assert`):

```cpp
RC_ASSERT(stack.getHistorySize() <= MAX_KEYMAP_PREFIX_HISTORY);
RC_ASSERT(result1 == result2);
RC_ASSERT(!hasCycle());
```

`RC_ASSERT` enables shrinking - when a property fails, RapidCheck records the assertion failure and minimizes the input.

### Running Properties

Properties integrate with Catch2:

```cpp
TEST_CASE("Description", "[property][tag]") {
    rc::check("property description", []() {
        // Generate inputs
        auto input = *rc::gen::arbitrary<Input>();

        // Test property
        auto result = processInput(input);

        RC_ASSERT(result.isValid());
    });
}
```

**Control iterations via environment variable:**
```bash
# Default: 100 iterations
./yamy_property_tests

# Run 1000 iterations (CI standard)
RC_PARAMS="max_success=1000" ./yamy_property_tests

# Verbose output
RC_PARAMS="verbose_progress=1" ./yamy_property_tests

# Combine parameters
RC_PARAMS="max_success=1000 verbose_progress=1" ./yamy_property_tests
```

## Understanding Shrinking

**Shrinking** is RapidCheck's killer feature: when a property fails, it automatically simplifies the failing input to the minimal example that still fails.

### How Shrinking Works

1. **Property fails** with a random input (e.g., 50-event sequence)
2. **RapidCheck tries simpler inputs**: Remove events, reduce values, simplify structure
3. **Find minimal failing case**: Shrink until further simplification passes
4. **Report minimal failure**: Show the simplest input that reproduces the bug

### Real YAMY Example

**Scenario**: Bug where pressing LShift twice in a row causes stuck modifier state.

**Initial failure** (random 100-event sequence):
```
Falsifiable after 42 tests:
events = [
  {LShift, down}, {A, down}, {A, up}, {RCtrl, down},
  {LShift, down},  // <-- Bug: double press
  {B, down}, {B, up}, {LShift, up}, {RCtrl, up},
  ... (91 more events)
]
```

**After shrinking** (minimal 2-event sequence):
```
Shrunk to minimal failing case:
events = [
  {LShift, down},
  {LShift, down}   // <-- Minimal reproduction
]
```

Now debugging is trivial - just look at what happens when LShift is pressed twice.

### Shrinking in YAMY Property Tests

See the shrinking documentation example in `tests/property_modifier.cpp:476-502`:

```cpp
TEST_CASE("ModifierState: Shrinking example for documentation") {
    // This test demonstrates shrinking by documenting what happens when
    // a property fails. RapidCheck will minimize the failing input.
    //
    // Example: If we had a bug where pressing LShift twice causes issues,
    // RapidCheck would shrink a 100-event sequence down to just:
    // [LShift down, LShift down]
    //
    // This makes debugging much easier than analyzing the full sequence.
}
```

### Interpreting Shrunk Failures

When a property fails, you'll see output like:

```
Failed after 156 tests and 12 shrinks

Shrunk to:
  numEvents = 3
  events = [
    {scanCode: 0x2A, isExtended: false, isKeyDown: true},   // LShift down
    {scanCode: 0x1D, isExtended: false, isKeyDown: true},   // LCtrl down
    {scanCode: 0x2A, isExtended: false, isKeyDown: false}   // LShift up
  ]

Assertion failed: RC_ASSERT(state.getFlags() == expectedFlags)
  Expected: 0x02 (LCTRL)
  Actual:   0x00 (NONE)
```

**How to debug:**
1. Copy the shrunk input into a unit test
2. Run with a debugger on that specific sequence
3. Fix the bug
4. Re-run the property test to verify fix

## YAMY Property Test Examples

### Example 1: Keymap Lookup Idempotence

**Property**: Searching for the same key twice always returns the same result.

**Why it matters**: Ensures lookup has no side effects.

```cpp
TEST_CASE("SimpleKeymap: Lookup is idempotent") {
    rc::check("searchAssignment returns same result on repeated calls", []() {
        SimpleKeymap km("TestMap");

        // Add random assignments
        const auto numAssignments = *rc::gen::inRange(1, 10);
        for (int i = 0; i < numAssignments; ++i) {
            auto key = *rc::gen::arbitrary<SimpleKey>();
            auto action = *rc::gen::arbitrary<SimpleAction>();
            km.addAssignment(key, action);
        }

        // Search same key twice
        auto searchKey = *rc::gen::arbitrary<SimpleKey>();
        const auto* result1 = km.searchAssignment(searchKey);
        const auto* result2 = km.searchAssignment(searchKey);

        RC_ASSERT(result1 == result2);  // Same pointer
    });
}
```

See `tests/property_keymap.cpp:121-157`

### Example 2: Modifier State Consistency

**Property**: After processing a sequence of key events, modifier state matches the last event for each key.

**Why it matters**: Ensures state machine tracks key state correctly.

```cpp
TEST_CASE("ModifierState: State is consistent with event history") {
    rc::check("final state matches last event for each key", []() {
        ModifierState state;

        // Generate random event sequence
        const auto numEvents = *rc::gen::inRange(0, 30);
        std::vector<TestEvent> events;
        for (int i = 0; i < numEvents; ++i) {
            events.push_back(*rc::gen::arbitrary<TestEvent>());
        }

        // Apply events
        applyEvents(state, events);

        // Verify each key's final state
        for (const auto& modKey : ALL_MODIFIER_KEYS) {
            bool expectedPressed = getFinalKeyState(events, modKey);
            bool actualPressed = (state.getFlags() & modKey.flag) != 0;
            RC_ASSERT(actualPressed == expectedPressed);
        }
    });
}
```

See `tests/property_modifier.cpp:202-225`

### Example 3: Layer Stack Depth Limit

**Property**: Layer history never exceeds `MAX_KEYMAP_PREFIX_HISTORY` (64 entries).

**Why it matters**: Prevents unbounded memory growth.

```cpp
TEST_CASE("LayerStack: Depth never exceeds maximum") {
    rc::check("history size stays within MAX_KEYMAP_PREFIX_HISTORY", []() {
        Layer baseLayer{"Base"};
        LayerStack stack(baseLayer);

        // Generate 100 random layer activations
        const auto numActivations = *rc::gen::inRange(0, 100);
        for (int i = 0; i < numActivations; ++i) {
            Layer newLayer = *rc::gen::arbitrary<Layer>();
            stack.activatePrefix(newLayer);

            // Invariant: never exceeds limit
            RC_ASSERT(stack.getHistorySize() <= MAX_KEYMAP_PREFIX_HISTORY);
        }
    });
}
```

See `tests/property_layer.cpp:156-173`

### Example 4: No Stuck Keys

**Property**: After any sequence of key events, all modifier keys can be released.

**Why it matters**: Ensures no impossible states that leave modifiers stuck.

```cpp
TEST_CASE("ModifierState: No stuck keys after event sequence") {
    rc::check("keys can be released after arbitrary event sequence", []() {
        ModifierState state;

        // Apply random event sequence
        const auto numEvents = *rc::gen::inRange(0, 30);
        std::vector<TestEvent> events;
        for (int i = 0; i < numEvents; ++i) {
            events.push_back(*rc::gen::arbitrary<TestEvent>());
        }
        applyEvents(state, events);

        // Release all modifier keys
        for (const auto& modKey : ALL_MODIFIER_KEYS) {
            TestEvent release{modKey, false};
            applyEvents(state, {release});
        }

        // All non-lock modifiers should be released
        uint32_t flags = state.getFlags();
        RC_ASSERT((flags & MODFLAG_SHIFT) == 0);
        RC_ASSERT((flags & MODFLAG_CTRL) == 0);
        RC_ASSERT((flags & MODFLAG_ALT) == 0);
        RC_ASSERT((flags & MODFLAG_WIN) == 0);
    });
}
```

See `tests/property_modifier.cpp:254-293`

## Common Patterns

### Pattern 1: State Machine Testing

**Goal**: Verify state transitions are valid for all input sequences.

```cpp
// 1. Define state model
class StateMachine { /* ... */ };

// 2. Define operations
enum Op { PRESS_KEY, RELEASE_KEY, SWITCH_LAYER };

// 3. Generate operation sequence
const auto numOps = *rc::gen::inRange(0, 50);
for (int i = 0; i < numOps; ++i) {
    Op op = *rc::gen::elementOf({PRESS_KEY, RELEASE_KEY, SWITCH_LAYER});
    applyOperation(state, op);

    // 4. Assert invariants after each operation
    RC_ASSERT(state.isValid());
}
```

See `tests/property_layer.cpp:468-509` for a complete example.

### Pattern 2: Round-Trip Properties

**Goal**: Encoding then decoding returns original value.

```cpp
rc::check("serialize and deserialize preserves value", []() {
    auto original = *rc::gen::arbitrary<KeyEvent>();
    auto serialized = serialize(original);
    auto deserialized = deserialize(serialized);

    RC_ASSERT(deserialized == original);
});
```

### Pattern 3: Metamorphic Properties

**Goal**: Changing inputs in a known way produces known output changes.

```cpp
rc::check("adding assignment increases count by 1", []() {
    SimpleKeymap km("TestMap");
    auto initialCount = km.getAssignmentCount();

    auto key = *rc::gen::arbitrary<SimpleKey>();
    auto action = *rc::gen::arbitrary<SimpleAction>();
    km.addAssignment(key, action);

    // Count should increase (unless key was already assigned)
    RC_ASSERT(km.getAssignmentCount() >= initialCount);
    RC_ASSERT(km.getAssignmentCount() <= initialCount + 1);
});
```

### Pattern 4: Boundary Conditions

**Goal**: Test behavior at limits.

```cpp
rc::check("maximum depth is enforced", []() {
    LayerStack stack(baseLayer);

    // Activate MAX + 10 layers
    for (int i = 0; i < MAX_KEYMAP_PREFIX_HISTORY + 10; ++i) {
        stack.activatePrefix(Layer{"L" + std::to_string(i)});
        RC_ASSERT(stack.getHistorySize() <= MAX_KEYMAP_PREFIX_HISTORY);
    }

    RC_ASSERT(stack.getHistorySize() == MAX_KEYMAP_PREFIX_HISTORY);
});
```

See `tests/property_layer.cpp:441-461`

## Integration with CI

Property tests run in CI with different iteration counts:

**Pull Request Checks** (fast feedback):
```yaml
- name: Run property tests
  run: |
    RC_PARAMS="max_success=1000" ctest -R property
  timeout: 10m
```

**Nightly Builds** (thorough exploration):
```yaml
- name: Run extensive property tests
  run: |
    RC_PARAMS="max_success=10000" ctest -R property
  timeout: 60m
```

See `.github/workflows/ci.yml` for YAMY's CI configuration (task 5.4).

## Best Practices

### DO:
- ✅ Test invariants, not implementation details
- ✅ Use descriptive property names ("lookup is idempotent")
- ✅ Start with simple properties, add complex ones later
- ✅ Use shrinking to find minimal failing cases
- ✅ Combine property tests with unit tests
- ✅ Document discovered edge cases as unit test regressions

### DON'T:
- ❌ Test specific values (that's unit testing)
- ❌ Write properties that just reimplement the code
- ❌ Ignore shrunk failures - they're your minimal bug reproduction
- ❌ Run property tests with too few iterations (<100)
- ❌ Make properties too complex to understand
- ❌ Forget to test boundary conditions

## Troubleshooting

### Property Test Takes Too Long

**Problem**: Test runs for minutes without completing.

**Solutions**:
- Reduce number of iterations: `RC_PARAMS="max_success=100"`
- Reduce input size in generators: `gen::inRange(1, 10)` instead of `gen::inRange(1, 1000)`
- Profile to find slow operations and optimize
- Consider if property is testing too much - split into smaller properties

### Property Fails But Shrinking Doesn't Help

**Problem**: Shrunk case is still complex or doesn't make sense.

**Solutions**:
- Add custom shrinking for your types (see RapidCheck docs)
- Add intermediate assertions to pinpoint exact failure
- Convert shrunk case to unit test and debug with GDB
- Verify generators produce valid inputs (invalid inputs can cause confusing failures)

### Property Always Passes (Too Weak)

**Problem**: Property never fails, even when you introduce bugs.

**Solutions**:
- Strengthen the property - add more assertions
- Verify generators cover edge cases (empty containers, boundary values)
- Temporarily introduce a known bug to verify property catches it
- Check if `RC_ASSERT` is actually reachable (add logging)

## Further Reading

- **RapidCheck GitHub**: https://github.com/emil-e/rapidcheck
- **QuickCheck Paper** (original PBT framework): "QuickCheck: A Lightweight Tool for Random Testing of Haskell Programs"
- **Property-Based Testing Tutorial**: John Hughes, "Don't Write Tests" (YouTube)
- **YAMY Property Tests**: `tests/property_*.cpp` - comprehensive examples

## Summary

Property-based testing complements unit testing by:
1. **Exploring edge cases** you wouldn't think to write manually
2. **Verifying invariants** hold for all inputs, not just examples
3. **Shrinking failures** to minimal bug reproductions
4. **Stress testing** with thousands of random inputs

**In YAMY, we use PBT for:**
- Keymap invariants (idempotence, uniqueness, parent chains)
- Modifier state machines (key pairing, consistency, no stuck keys)
- Layer switching (stack depth, prefix isolation, valid transitions)

**Start simple, iterate:**
1. Write a simple property (e.g., "function doesn't crash")
2. Run with 100 iterations
3. Strengthen property (e.g., "function returns valid result")
4. Increase iterations in CI (1000+)
5. Add more properties as you discover invariants

When a property fails, **trust the shrinking** - it's showing you the minimal bug reproduction. Turn it into a unit test and fix the bug.
