#pragma once
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// parser.h


#ifndef _PARSER_H
#  define _PARSER_H

#  include "misc.h"
#  include "stringtool.h"
#  include <vector>
#  include <string>


///
class Token
{
public:
    ///
    enum Type {
        Type_string,                ///
        Type_number,                ///
        Type_regexp,                ///
        Type_openParen,                ///
        Type_closeParen,                ///
        Type_comma,                    ///
    };

private:
    u_char m_type;                ///
    bool m_isValueQuoted;                ///
    int m_numericValue;                ///
    std::string m_stringValue;            ///
    long m_data;                    ///

public:
    ///
    Token(const Token &i_token);
    ///
    Token(int i_value, const std::string &i_display);
    ///
    Token(const std::string &i_value, bool i_isValueQuoted,
          bool i_isRegexp = false);
    ///
    Token(Type i_type);

    /// is the value quoted ?
    bool isQuoted() const {
        return m_isValueQuoted;
    }

    /// value type
    Type getType() const {
        return static_cast<Type>(m_type);
    }
    ///
    bool isString() const {
        return m_type == Type_string;
    }
    ///
    bool isNumber() const {
        return m_type == Type_number;
    }
    ///
    bool isRegexp() const {
        return m_type == Type_regexp;
    }
    ///
    bool isOpenParen() const {
        return m_type == Type_openParen;
    }
    ///
    bool isCloseParen() const {
        return m_type == Type_closeParen;
    }
    ///
    bool isComma() const {
        return m_type == Type_comma;
    }

    /// get numeric value
    int getNumber() const;

    /// get string value
    std::string getString() const;

    /// get regexp value
    std::string getRegexp() const;

    /// get data
    long getData() const {
        return m_data;
    }
    ///
    void setData(long i_data) {
        m_data = i_data;
    }

    /// case insensitive equal
    bool operator==(const std::string &i_str) const {
        return *this == i_str.c_str();
    }
    ///
    bool operator==(const char *i_str) const;
    ///
    bool operator!=(const std::string &i_str) const {
        return *this != i_str.c_str();
    }
    ///
    bool operator!=(const char *i_str) const {
        return !(*this == i_str);
    }

    /** paren equal
        @param i_c '<code>(</code>' or '<code>)</code>' */
    bool operator==(const char i_c) const;
    /** paren equal
        @param i_c '<code>(</code>' or '<code>)</code>' */
    bool operator!=(const char i_c) const {
        return !(*this == i_c);
    }

    /// add string
    void add(const std::string &i_str);

    /// stream output
    friend tostream &operator<<(tostream &i_ost, const Token &i_token);
};


///
class Parser
{
public:
    ///
    typedef std::vector<Token> Tokens;

private:
    ///
    typedef std::vector<std::string> Prefixes;

private:
    size_t m_lineNumber;                /// current line number
    const Prefixes *m_prefixes;            /** string that may be prefix
                                                    of a token */

    size_t m_internalLineNumber;            /// next line number
    const char *m_ptr;                /// read pointer
    const char *m_end;                /// end pointer

private:
    /// get a line
    bool getLine(std::string *o_line);

public:
    ///
    Parser(const char *i_str, size_t i_length);

    /** get a parsed line.  if no more lines exist, returns false */
    bool getLine(Tokens *o_tokens);

    /// get current line number
    size_t getLineNumber() const {
        return m_lineNumber;
    }

    /** set string that may be prefix of a token.  prefix_ is not
        copied, so it must be preserved after setPrefix() */
    void setPrefixes(const Prefixes *m_prefixes);
};


#endif // !_PARSER_H
