#pragma once
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// strexpr.h


#ifndef _STREXPR_H
#  define _STREXPR_H

#  include "stringtool.h"
#  include <memory>
#  include <string>


/// Abstract interface for system-dependent operations
class StrExprSystem
{
public:
    virtual ~StrExprSystem() {}
    
    /// Get text from clipboard
    virtual std::string getClipboardText() const = 0;
    
    /// Get current window class name
    virtual std::string getStrExprWindowClassName() const = 0;
    
    /// Get current window title name
    virtual std::string getStrExprWindowTitleName() const = 0;
};


/// string type expression
class StrExpr
{
private:
    std::string m_symbol;
protected:
    static const StrExprSystem *s_system;
public:
    StrExpr(const std::string &i_symbol) : m_symbol(i_symbol) {};

    virtual ~StrExpr() {};

    virtual std::unique_ptr<StrExpr> clone() const {
        return std::make_unique<StrExpr>(*this);
    }

    virtual std::string eval() const {
        return m_symbol;
    }

    static void setSystem(const StrExprSystem *i_system) {
        s_system = i_system;
    }
};


/// string type expression for function arguments
class StrExprArg
{
private:
    std::unique_ptr<StrExpr> m_expr;
public:
    enum Type {
        Literal,
        Builtin,
    };
    StrExprArg();
    StrExprArg(const StrExprArg &i_data);
    StrExprArg(const std::string &i_symbol, Type i_type);
    ~StrExprArg();
    StrExprArg &operator=(const StrExprArg &i_data);
    std::string eval() const;
    static void setSystem(const StrExprSystem *i_system);
};


/// stream output
std::ostream &operator<<(std::ostream &i_ost, const StrExprArg &i_data);


#endif // !_STREXPR_H
