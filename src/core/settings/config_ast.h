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
    enum class Type {
        KEYMAP,
        WINDOW_AND,
        WINDOW_OR,
    };
    Type type = Type::KEYMAP;
    std::string name;
    std::string parentName; // Optional inheritance
    std::string windowClassRegex;
    std::string windowTitleRegex;
    std::vector<KeyAssignment> assignments;
};

struct ScanCodeDefinition {
    uint8_t scan;
    std::vector<std::string> flags; // "E0-", "E1-"
};

struct KeyDefinition {
    std::vector<std::string> names;
    std::vector<ScanCodeDefinition> scanCodes;
};

struct ModifierDefinition {
    std::string type; // "shift", "alt", etc.
    std::vector<std::string> keyNames;
};

struct AliasDefinition {
    std::string aliasName;
    std::string keyName;
};

struct SubstituteDefinition {
    std::vector<std::string> from_mods; // Simplified for now
    std::string from_key;
    std::string to_mods; // Simplified for now
    std::string to_key;
};

struct NumberModifierDefinition {
    std::string numberKeyName;
    std::string modifierKeyName;
};

struct OptionDefinition {
    std::string name;
    std::string value;
};

struct ConfigAST {
    std::vector<std::string> includedFiles;
    std::vector<KeymapDefinition> keymaps;
    std::vector<KeyDefinition> keyDefinitions;
    std::vector<ModifierDefinition> modifierDefinitions;
    std::vector<AliasDefinition> aliasDefinitions;
    std::vector<SubstituteDefinition> substituteDefinitions;
    std::vector<NumberModifierDefinition> numberModifierDefinitions;
    std::vector<OptionDefinition> optionDefinitions;
    // ... other definitions like 'def key', 'def mod'
};

} // namespace yamy::ast
