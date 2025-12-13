//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// strexpr.cpp


#include "strexpr.h"


const StrExprSystem *StrExpr::s_system = nullptr;


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// StrExprClipboard
class StrExprClipboard : public StrExpr
{
public:
    StrExprClipboard(const std::string &i_symbol) : StrExpr(i_symbol) {};

    ~StrExprClipboard() {};

    std::unique_ptr<StrExpr> clone() const override {
        return std::make_unique<StrExprClipboard>(*this);
    }

    std::string eval() const override {
        if (s_system) {
            return s_system->getClipboardText();
        }
        return "";
    }
};


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// StrExprWindowClassName
class StrExprWindowClassName : public StrExpr
{
public:
    StrExprWindowClassName(const std::string &i_symbol) : StrExpr(i_symbol) {};

    ~StrExprWindowClassName() {};

    std::unique_ptr<StrExpr> clone() const override {
        return std::make_unique<StrExprWindowClassName>(*this);
    }

    std::string eval() const override {
        if (s_system) {
            return s_system->getStrExprWindowClassName();
        }
        return "";
    }
};


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// StrExprWindowTitleName
class StrExprWindowTitleName : public StrExpr
{
public:
    StrExprWindowTitleName(const std::string &i_symbol) : StrExpr(i_symbol) {};

    ~StrExprWindowTitleName() {};

    std::unique_ptr<StrExpr> clone() const override {
        return std::make_unique<StrExprWindowTitleName>(*this);
    }

    std::string eval() const override {
        if (s_system) {
            return s_system->getStrExprWindowTitleName();
        }
        return "";
    }
};


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// StrExprArg


// default constructor
StrExprArg::StrExprArg()
{
    m_expr = std::make_unique<StrExpr>("");
}


// copy contructor
StrExprArg::StrExprArg(const StrExprArg &i_data)
{
    if (i_data.m_expr) {
        m_expr = i_data.m_expr->clone();
    } else {
        m_expr = std::make_unique<StrExpr>("");
    }
}


StrExprArg &StrExprArg::operator=(const StrExprArg &i_data)
{
    if (this == &i_data)
        return *this;

    if (i_data.m_expr) {
        m_expr = i_data.m_expr->clone();
    } else {
        m_expr = std::make_unique<StrExpr>("");
    }

    return *this;
}


// initializer
StrExprArg::StrExprArg(const std::string &i_symbol, Type i_type)
{
    switch (i_type) {
    case Literal:
        m_expr = std::make_unique<StrExpr>(i_symbol);
        break;
    case Builtin:
        if (i_symbol == "Clipboard")
            m_expr = std::make_unique<StrExprClipboard>(i_symbol);
        else if (i_symbol == "WindowClassName")
            m_expr = std::make_unique<StrExprWindowClassName>(i_symbol);
        else if (i_symbol == "WindowTitleName")
            m_expr = std::make_unique<StrExprWindowTitleName>(i_symbol);
        else
            m_expr = std::make_unique<StrExpr>(i_symbol); // Fallback
        break;
    default:
        m_expr = std::make_unique<StrExpr>(i_symbol);
        break;
    }
}


StrExprArg::~StrExprArg()
{
    // unique_ptr handles deletion
}


std::string StrExprArg::eval() const
{
    return m_expr->eval();
}

void StrExprArg::setSystem(const StrExprSystem *i_system)
{
    StrExpr::setSystem(i_system);
}

// stream output (narrow)
std::ostream &operator<<(std::ostream &i_ost, const StrExprArg &i_data)
{
    i_ost << i_data.eval();
    return i_ost;
}

#ifdef _WIN32
// stream output (wide, for Windows UI)
tostream &operator<<(tostream &i_ost, const StrExprArg &i_data)
{
    i_ost << to_tstring(i_data.eval());
    return i_ost;
}
#endif
