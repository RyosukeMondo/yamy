#pragma once
#include <bitset>
#include "../input/modifier_state.h" // For ModifierState::TOTAL_BITS

namespace yamy::engine {

struct CompiledRule {
    // Bitmask of modifiers that MUST be active (1)
    std::bitset<yamy::input::ModifierState::TOTAL_BITS> requiredOn;

    // Bitmask of modifiers that MUST be inactive (0)
    std::bitset<yamy::input::ModifierState::TOTAL_BITS> requiredOff;

    // The output scan code (or action ID)
    uint16_t outputScanCode; 
    
    // Future: Action abstraction (e.g., KeySeq*, FunctionData*)
    // void* actionData; 

    // Helper to check if this rule matches the current state
    bool matches(const std::bitset<yamy::input::ModifierState::TOTAL_BITS>& currentState) const {
        // 1. Check ON requirements: (State & OnMask) == OnMask
        if ((currentState & requiredOn) != requiredOn) return false;

        // 2. Check OFF requirements: (~State & OffMask) == OffMask
        //    Equivalently: (State & OffMask) == 0
        if ((currentState & requiredOff).any()) return false;

        return true;
    }
};

} // namespace yamy::engine
