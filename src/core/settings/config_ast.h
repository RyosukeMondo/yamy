#pragma once
#include <string>
#include <vector>
#include <optional>
#include <variant>

namespace yamy::ast {

// Represents a raw key token (e.g., "*A", "S-", "M00-")
struct ModifierToken {
    std::string text;
    // For M00-MFF, L00-LFF, or standard names
};

struct KeyToken {
    std::string name;
    // e.g., "A", "Space", "V_MyKey"
};

struct KeyAssignment {
    // Left side: The modified key triggering the action
    std::vector<ModifierToken> modifiers;
    KeyToken key;

    // Right side: The sequence of actions
    // Note: In AST, we keep this as raw tokens or a lightweight ActionAST
    // rather than full Runtime Actions.
    // For Phase 1, we might wrap the existing Action* or KeySeq* 
    // if we haven't fully decoupled Action parsing yet.
    // Ideally:
    std::string actionSource; // Raw string for now, or parsed Action nodes
};

struct KeymapDefinition {
    std::string name;
    std::string parentName; // Optional inheritance
    std::string windowClassRegex;
    std::string windowTitleRegex;
    std::vector<KeyAssignment> assignments;
};

struct ConfigAST {
    std::vector<std::string> includedFiles;
    std::vector<KeymapDefinition> keymaps;
    // ... other definitions like 'def key', 'def mod'
};

} // namespace yamy::ast
