//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// parser.cpp


#include "misc.h"

#include "errormessage.h"
#include "parser.h"
#include "core/logging/logger.h"
#include <cassert>


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Token


Token::Token(const Token &i_token)
        : m_type(i_token.m_type),
        m_isValueQuoted(i_token.m_isValueQuoted),
        m_numericValue(i_token.m_numericValue),
        m_stringValue(i_token.m_stringValue),
        m_data(i_token.m_data)
{
}

Token::Token(int i_value, const std::string &i_display)
        : m_type(Type_number),
        m_isValueQuoted(false),
        m_numericValue(i_value),
        m_stringValue(i_display),
        m_data(0)
{
}

Token::Token(const std::string &i_value, bool i_isValueQuoted, bool i_isRegexp)
        : m_type(i_isRegexp ? Type_regexp : Type_string),
        m_isValueQuoted(i_isValueQuoted),
        m_numericValue(0),
        m_stringValue(i_value),
        m_data(0)
{
}

Token::Token(Type i_m_type)
        : m_type(i_m_type),
        m_isValueQuoted(false),
        m_numericValue(0),
        m_stringValue(""),
        m_data(0)
{
    ASSERT(m_type == Type_openParen || m_type == Type_closeParen ||
           m_type == Type_comma);
}

// get numeric value
int Token::getNumber() const
{
    if (m_type == Type_number)
        return m_numericValue;
    if (m_stringValue.empty())
        return 0;
    else
        throw ErrorMessage() << "`" << *this << "' is not a Type_number.";
}

// get string value
std::string Token::getString() const
{
    if (m_type == Type_string)
        return m_stringValue;
    throw ErrorMessage() << "`" << *this << "' is not a string.";
}

// get regexp value
std::string Token::getRegexp() const
{
    if (m_type == Type_regexp)
        return m_stringValue;
    throw ErrorMessage() << "`" << *this << "' is not a regexp.";
}

// case insensitive equal
bool Token::operator==(const char *i_str) const
{
    if (m_type == Type_string)
        return strcasecmp_utf8(m_stringValue.c_str(), i_str) == 0;
    return false;
}

// paren equal
bool Token::operator==(const char i_c) const
{
    if (i_c == '(') return m_type == Type_openParen;
    if (i_c == ')') return m_type == Type_openParen;
    return false;
}

// add string
void Token::add(const std::string &i_str)
{
    m_stringValue += i_str;
}

// stream output
std::ostream &operator<<(std::ostream &i_ost, const Token &i_token)
{
    switch (i_token.m_type) {
    case Token::Type_string:
        i_ost << i_token.m_stringValue;
        break;
    case Token::Type_number:
        i_ost << i_token.m_stringValue;
        break;
    case Token::Type_regexp:
        i_ost << i_token.m_stringValue;
        break;
    case Token::Type_openParen:
        i_ost << "(";
        break;
    case Token::Type_closeParen:
        i_ost << ")";
        break;
    case Token::Type_comma:
        i_ost << ", ";
        break;
    }
    return i_ost;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Parser


Parser::Parser(const char *i_str, size_t i_length)
        : m_lineNumber(1),
        m_prefixes(nullptr),
        m_internalLineNumber(1),
        m_ptr(i_str),
        m_end(i_str + i_length)
{
    yamy::logging::Logger::getInstance().log(yamy::logging::LogLevel::Info, "Parser", "Parser initialized.");
}

// set string that may be prefix of a token.
// prefix_ is not copied, so it must be preserved after setPrefix()
void Parser::setPrefixes(const Prefixes *i_prefixes)
{
    m_prefixes = i_prefixes;
}

// get a line
bool Parser::getLine(std::string *o_line)
{
    o_line->resize(0);

    if (m_ptr == m_end)
        return false;

    const char *begin = m_ptr;
    const char *end = m_end;

    // lines are separated by: "\r\n", "\n"
    while (m_ptr != m_end)
        switch (*m_ptr) {
        case '\n':
            end = m_ptr;
            ++ m_ptr;
            goto got_line_end;
        case '\r':
            if (m_ptr + 1 != m_end && m_ptr[1] == '\n') {
                end = m_ptr;
                m_ptr += 2;
                goto got_line_end;
            }
            // fall through
        default:
            ++ m_ptr;
            break;
        }
got_line_end:
    ++ m_internalLineNumber;
    // o_line->assign(begin, end);        // why bcc cannot link this ?
    o_line->assign(begin, end - begin);        // workarond for bcc
    return true;
}

// Determine the byte length of a UTF-8 character and validate the sequence
// Returns 1-4 for valid UTF-8, 0 for invalid (with is_valid set to false)
static inline int utf8_char_length(const char* str, size_t max_len, bool& is_valid) {
    if (max_len == 0) {
        is_valid = false;
        return 0;
    }

    unsigned char lead = static_cast<unsigned char>(*str);

    // 1-byte ASCII (0x00-0x7F)
    if (lead < 0x80) {
        is_valid = true;
        return 1;
    }

    // Invalid: continuation byte as first byte (0x80-0xBF)
    if (lead < 0xC0) {
        is_valid = false;
        return 0;
    }

    // 2-byte UTF-8 (0xC0-0xDF)
    if (lead < 0xE0) {
        if (max_len < 2) {
            is_valid = false;
            return 0;
        }
        unsigned char cont1 = static_cast<unsigned char>(str[1]);
        if (cont1 < 0x80 || cont1 > 0xBF) {
            is_valid = false;
            return 0;
        }
        is_valid = true;
        return 2;
    }

    // 3-byte UTF-8 (0xE0-0xEF) - Japanese characters use this range
    if (lead < 0xF0) {
        if (max_len < 3) {
            is_valid = false;
            return 0;
        }
        unsigned char cont1 = static_cast<unsigned char>(str[1]);
        unsigned char cont2 = static_cast<unsigned char>(str[2]);
        if ((cont1 < 0x80 || cont1 > 0xBF) || (cont2 < 0x80 || cont2 > 0xBF)) {
            is_valid = false;
            return 0;
        }
        is_valid = true;
        return 3;
    }

    // 4-byte UTF-8 (0xF0-0xF7)
    if (lead < 0xF8) {
        if (max_len < 4) {
            is_valid = false;
            return 0;
        }
        unsigned char cont1 = static_cast<unsigned char>(str[1]);
        unsigned char cont2 = static_cast<unsigned char>(str[2]);
        unsigned char cont3 = static_cast<unsigned char>(str[3]);
        if ((cont1 < 0x80 || cont1 > 0xBF) ||
            (cont2 < 0x80 || cont2 > 0xBF) ||
            (cont3 < 0x80 || cont3 > 0xBF)) {
            is_valid = false;
            return 0;
        }
        is_valid = true;
        return 4;
    }

    // Invalid: lead byte >= 0xF8 (reserved range)
    is_valid = false;
    return 0;
}

// symbol test
static bool isSymbolChar(char i_c)
{
    if (i_c == '\0')
        return false;

    unsigned char uc = static_cast<unsigned char>(i_c);

    // Check for multi-byte UTF-8 lead byte
    if (uc >= 0x80)
        return true;

    if (std::isalpha(uc) || std::isdigit(uc))
        return true;

    if (std::ispunct(uc))
        return !!strchr("-+/?_\\", i_c);

    return std::isgraph(uc);
}


// get a parsed line.
// if no more lines exist, returns false
bool Parser::getLine(std::vector<Token> *o_tokens)
{
    o_tokens->clear();
    m_lineNumber = m_internalLineNumber;

    std::string line;
    bool isTokenExist = false;

    while (getLine(&line)) {
        const char *t = line.c_str();
        bool continueToNextLine = false;

        while (true) {
            // skip white space
            while (*t != '\0' && std::isspace(static_cast<unsigned char>(*t)))
                t ++;
            if (*t == '\0' || *t == '#')
                break; // break inner loop
            if (*t == '\\' && *(t + 1) == '\0') {
                continueToNextLine = true;
                break; // break inner loop, continue outer loop
            }

            const char *tokenStart = t;

            // comma or empty token
            if (*t == ',') {
                if (!isTokenExist)
                    o_tokens->push_back(Token("", false));
                isTokenExist = false;
                o_tokens->push_back(Token(Token::Type_comma));
                t ++;
                continue;
            }

            // paren
            if (*t == '(') {
                o_tokens->push_back(Token(Token::Type_openParen));
                isTokenExist = false;
                t ++;
                continue;
            }
            if (*t == ')') {
                if (!isTokenExist)
                    o_tokens->push_back(Token("", false));
                isTokenExist = true;
                o_tokens->push_back(Token(Token::Type_closeParen));
                t ++;
                continue;
            }

            isTokenExist = true;

            // prefix
            bool matchedPrefix = false;
            if (m_prefixes)
                for (size_t i = 0; i < m_prefixes->size(); i ++)
                    if (strncasecmp(tokenStart, m_prefixes->at(i).c_str(),
                                  m_prefixes->at(i).size()) == 0) {
                        o_tokens->push_back(Token(m_prefixes->at(i), false));
                        t += m_prefixes->at(i).size();
                        matchedPrefix = true;
                        break;
                    }
            if (matchedPrefix) continue;

            // quoted or regexp
            if (*t == '"' || *t == '\'' ||
                    *t == '/' || (*t == '\\' && *(t + 1) == 'm' &&
                                      *(t + 2) != '\0')) {
                bool isRegexp = !(*t == '"' || *t == '\'');
                char q[2] = { *t++, '\0' }; // quote character
                if (q[0] == '\\') {
                    t++;
                    q[0] = *t++;
                }
                tokenStart = t;

                while (*t != '\0' && *t != q[0]) {
                    if (*t == '\\' && *(t + 1))
                        t ++;
                    // Handle UTF-8 multi-byte sequences
                    unsigned char uc = static_cast<unsigned char>(*t);
                    if (uc >= 0x80) {
                        size_t remaining = line.c_str() + line.size() - t;
                        bool is_valid = false;
                        int char_len = utf8_char_length(t, remaining, is_valid);
                        if (!is_valid) {
                            ErrorMessage e;
                            e << "invalid UTF-8 sequence at line " << m_lineNumber;
                            e << ", byte value 0x" << std::hex << static_cast<int>(uc) << std::dec;
                            throw e;
                        }
                        t += char_len;
                    } else {
                        t ++;
                    }
                }

                std::string str =
                    interpretMetaCharacters(tokenStart, t - tokenStart, q, isRegexp);
                // concatinate continuous string
                if (!isRegexp &&
                        0 < o_tokens->size() && o_tokens->back().isString() &&
                        o_tokens->back().isQuoted())
                    o_tokens->back().add(str);
                else
                    o_tokens->push_back(Token(str, true, isRegexp));
                if (*t != '\0')
                    t ++;
                continue;
            }

            // not quoted
            {
                while (isSymbolChar(*t)) {
                    if (*t == '\\') {
                        if (*(t + 1))
                            t ++;
                        else
                            break;
                    }
                    // Handle UTF-8 multi-byte sequences
                    unsigned char uc = static_cast<unsigned char>(*t);
                    if (uc >= 0x80) {
                        size_t remaining = line.c_str() + line.size() - t;
                        bool is_valid = false;
                        int char_len = utf8_char_length(t, remaining, is_valid);
                        if (!is_valid) {
                            ErrorMessage e;
                            e << "invalid UTF-8 sequence at line " << m_lineNumber;
                            e << ", byte value 0x" << std::hex << static_cast<int>(uc) << std::dec;
                            throw e;
                        }
                        t += char_len;
                    } else {
                        t ++;
                    }
                }
                if (t == tokenStart) {
                    ErrorMessage e;
                    e << "invalid character ";
                    e << "\\x";
                    e << std::hex;
                    e << (int)(unsigned char)*t;
                    e << std::dec;
                    if (std::isprint(static_cast<unsigned char>(*t)))
                        e << "(" << *t << ")";
                    throw e;
                }

                char *numEnd = nullptr;
                long value = strtol(tokenStart, &numEnd, 0);
                if (tokenStart == numEnd) {
                    std::string str = interpretMetaCharacters(tokenStart, t - tokenStart);
                    o_tokens->push_back(Token(str, false));
                } else {
                    o_tokens->push_back(
                        Token(value, std::string(tokenStart, numEnd - tokenStart)));
                    t = numEnd;
                }
                continue;
            }
        }
        try {
            if (continueToNextLine)
                continue;

            if (0 < o_tokens->size())
                break;
            m_lineNumber = m_internalLineNumber;
            isTokenExist = false;
        } catch (const ErrorMessage &e) {
            yamy::logging::Logger::getInstance().log(
                yamy::logging::LogLevel::Error, "Parser",
                "Line " + std::to_string(m_lineNumber) + ": " + e.getMessage());
            throw;
        }
    }

    return 0 < o_tokens->size();
}
