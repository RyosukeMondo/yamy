#pragma once

#include "../settings/config_ast.h"
#include "../settings/setting.h"

namespace yamy {

class ConfigCompiler {
public:
    ConfigCompiler(const yamy::ast::ConfigAST& ast, Setting& setting);

    bool compile();

private:
    const ast::ConfigAST& m_ast;
    Setting& m_setting;
};

} // namespace yamy
