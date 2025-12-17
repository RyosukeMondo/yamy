#pragma once
#include <vector>
#include <unordered_map>
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
        if (it == m_buckets.end()) return nullptr;

        const auto& rules = it->second;
        for (const auto& rule : rules) {
            if (rule.matches(state)) {
                return &rule;
            }
        }
        return nullptr;
    }

private:
    // Map ScanCode -> Vector of Rules (Ordered by priority)
    std::unordered_map<uint16_t, std::vector<CompiledRule>> m_buckets;
};

} // namespace yamy::engine