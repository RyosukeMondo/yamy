#include "config_compiler.h"

namespace yamy {

ConfigCompiler::ConfigCompiler(const yamy::ast::ConfigAST& ast, Setting& setting)
    : m_ast(ast), m_setting(setting)
{
}

bool ConfigCompiler::compile()
{
    // TODO: Implement compilation logic
    // - Process key definitions
    // - Process keymap definitions
    // - etc.
    return true;
}

} // namespace yamy
