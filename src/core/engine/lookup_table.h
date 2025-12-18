#pragma once
#include <vector>
#include <unordered_map>
#include <iostream>
#include "compiled_rule.h"

namespace yamy::engine {

class RuleLookupTable {
public:
    // Add a rule to the table
    void addRule(uint16_t inputScanCode, const CompiledRule& rule) {
        m_buckets[inputScanCode].push_back(rule);
    }

    // clear table
    void clear() { m_buckets.clear(); }

    // Find the first matching rule
    const CompiledRule* findMatch(uint16_t scanCode, const std::bitset<yamy::input::ModifierState::TOTAL_BITS>& state) const {
        auto it = m_buckets.find(scanCode);
        if (it == m_buckets.end()) {
            // DEBUG: No rules for this scan code
            if (scanCode == 0x23 || scanCode == 0x24 || scanCode == 0x25 || scanCode == 0x26) {
                std::cerr << "[LOOKUP-DEBUG] No rules in table for scan 0x" << std::hex << scanCode << std::dec << std::endl;
            }
            return nullptr;
        }

        const auto& rules = it->second;
        if (scanCode == 0x23 || scanCode == 0x24 || scanCode == 0x25 || scanCode == 0x26) {
            std::cerr << "[LOOKUP-DEBUG] Checking " << rules.size() << " rules for scan 0x" << std::hex << scanCode << std::dec << std::endl;
            // Check virtual modifiers in state
            for (int i = 0; i < 16; i++) {
                if (state.test(yamy::input::ModifierState::VIRTUAL_OFFSET + i)) {
                    std::cerr << "[LOOKUP-DEBUG] State has M" << std::hex << i << std::dec << " active" << std::endl;
                }
            }
        }

        for (const auto& rule : rules) {
            if (rule.matches(state)) {
                return &rule;
            } else if (scanCode == 0x23 || scanCode == 0x24 || scanCode == 0x25 || scanCode == 0x26) {
                std::cerr << "[LOOKUP-DEBUG] Rule didn't match. Required ON bits: ";
                for (size_t i = 0; i < rule.requiredOn.size(); i++) {
                    if (rule.requiredOn.test(i)) std::cerr << i << " ";
                }
                std::cerr << std::endl;
            }
        }
        return nullptr;
    }

private:
    // Map ScanCode -> Vector of Rules (Ordered by priority)
    std::unordered_map<uint16_t, std::vector<CompiledRule>> m_buckets;
};

} // namespace yamy::engine