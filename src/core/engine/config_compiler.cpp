#include "config_compiler.h"

namespace yamy {

ConfigCompiler::ConfigCompiler(const yamy::ast::ConfigAST& ast, Setting& setting)
    : m_ast(ast), m_setting(setting)
{
}

bool ConfigCompiler::compile()
{
    // Process key definitions
    for (const auto& key_def : m_ast.keyDefinitions) {
        Key key;
        for (const auto& name : key_def.names) {
            key.addName(name);
        }
        for (const auto& sc_def : key_def.scanCodes) {
            ScanCode sc;
            sc.m_scan = sc_def.scan;
            sc.m_flags = 0;
            for (const auto& flag : sc_def.flags) {
                if (flag == "E0-") sc.m_flags |= ScanCode::E0;
                else if (flag == "E1-") sc.m_flags |= ScanCode::E1;
            }
            key.addScanCode(sc);
        }
        m_setting.m_keyboard.addKey(key);
    }

    // Process modifier definitions
    for (const auto& mod_def : m_ast.modifierDefinitions) {
        Modifier::Type mt;
        if      (mod_def.type == "shift")   mt = Modifier::Type_Shift;
        else if (mod_def.type == "alt")     mt = Modifier::Type_Alt;
        else if (mod_def.type == "control") mt = Modifier::Type_Control;
        else if (mod_def.type == "windows") mt = Modifier::Type_Windows;
        else continue; // Should not happen if parser is correct

        for (const auto& keyName : mod_def.keyNames) {
            Key *key = m_setting.m_keyboard.searchKeyByNonAliasName(keyName);
            if (key) {
                m_setting.m_keyboard.addModifier(mt, key);
            }
        }
    }

    // Process alias definitions
    for (const auto& alias_def : m_ast.aliasDefinitions) {
        Key *key = m_setting.m_keyboard.searchKeyByNonAliasName(alias_def.keyName);
        if (key) {
            m_setting.m_keyboard.addAlias(alias_def.aliasName, key);
        }
    }

    // Process number modifier definitions
    for (const auto& num_mod_def : m_ast.numberModifierDefinitions) {
        Key *numberKey = m_setting.m_keyboard.searchKeyByNonAliasName(num_mod_def.numberKeyName);
        Key *modifierKey = m_setting.m_keyboard.searchKeyByNonAliasName(num_mod_def.modifierKeyName);
        if (numberKey && modifierKey) {
            m_setting.m_keyboard.addNumberModifier(numberKey, modifierKey);
        }
    }

    // TODO: Implement compilation for other definitions
    // - Process keymap definitions
    // - etc.
    return true;
}

} // namespace yamy
