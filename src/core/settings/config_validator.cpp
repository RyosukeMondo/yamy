//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// config_validator.cpp
// Implementation of configuration file validator

#include "config_validator.h"
#include "parser.h"
#include "errormessage.h"
#include "stringtool.h"

#include <fstream>
#include <sstream>
#include <chrono>
#include <set>
#include <algorithm>
#include <sys/stat.h>

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ValidationError implementation
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

std::string ValidationError::format() const
{
    std::ostringstream oss;

    // Format: filename(line): severity code: message
    if (lineNumber > 0) {
        oss << "(" << lineNumber;
        if (columnNumber > 0) {
            oss << ":" << columnNumber;
        }
        oss << ")";
    }
    oss << " : ";

    if (severity == ValidationSeverity::Error) {
        oss << "error";
    } else {
        oss << "warning";
    }

    if (!code.empty()) {
        oss << " " << code;
    }

    oss << ": " << message;

    if (!context.empty()) {
        oss << "\n    " << context;
    }

    return oss.str();
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ValidationResult implementation
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

size_t ValidationResult::errorCount() const
{
    size_t count = 0;
    for (const auto& err : errors) {
        if (err.isError()) {
            ++count;
        }
    }
    return count;
}

size_t ValidationResult::warningCount() const
{
    size_t count = 0;
    for (const auto& err : errors) {
        if (!err.isError()) {
            ++count;
        }
    }
    return count;
}

std::string ValidationResult::formatAll() const
{
    std::ostringstream oss;
    for (const auto& err : errors) {
        oss << err.format() << "\n";
    }
    return oss.str();
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ConfigValidator::ValidationContext - Internal state during validation
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

struct ConfigValidator::ValidationContext {
    std::set<std::string> definedKeymaps;
    std::set<std::string> definedKeyseqs;
    std::set<std::string> definedKeys;
    std::set<std::string> definedSymbols;
    std::set<std::string> referencedKeymaps;
    std::set<std::string> referencedKeyseqs;
    std::set<std::string> includedFiles;
    std::string currentFile;
    size_t includeDepth;
    std::vector<bool> conditionStack;

    ValidationContext()
        : includeDepth(0) {}
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ConfigValidator implementation
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

ConfigValidator::ConfigValidator()
    : m_options()
{
}

ConfigValidator::ConfigValidator(const Options& options)
    : m_options(options)
{
}

ConfigValidator::~ConfigValidator() = default;

void ConfigValidator::addIncludePath(const std::string& path)
{
    m_includePaths.push_back(path);
}

void ConfigValidator::clearIncludePaths()
{
    m_includePaths.clear();
}

bool ConfigValidator::readFile(const std::string& path, std::string& content)
{
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    std::ostringstream oss;
    oss << file.rdbuf();
    content = oss.str();

    // Handle UTF-8 BOM
    if (content.size() >= 3 &&
        static_cast<unsigned char>(content[0]) == 0xEF &&
        static_cast<unsigned char>(content[1]) == 0xBB &&
        static_cast<unsigned char>(content[2]) == 0xBF) {
        content = content.substr(3);
    }

    return true;
}

bool ConfigValidator::resolveIncludePath(const std::string& includeName,
                                         const std::string& currentFile,
                                         std::string& resolvedPath)
{
    // First try relative to current file
    if (!currentFile.empty()) {
        // Use std::filesystem to handle path separators cross-platform
        std::filesystem::path currentPath(currentFile);
        std::filesystem::path dirPath = currentPath.parent_path();
        if (!dirPath.empty()) {
            resolvedPath = (dirPath / includeName).string();
            struct stat st;
            if (stat(resolvedPath.c_str(), &st) == 0) {
                return true;
            }
        }
    }

    // Try include paths
    for (const auto& path : m_includePaths) {
        resolvedPath = path;
        if (!resolvedPath.empty() && resolvedPath.back() != '/' &&
            resolvedPath.back() != '\\') {
            resolvedPath += '/';
        }
        resolvedPath += includeName;
        struct stat st;
        if (stat(resolvedPath.c_str(), &st) == 0) {
            return true;
        }
    }

    // Try as absolute path
    resolvedPath = includeName;
    struct stat st;
    if (stat(resolvedPath.c_str(), &st) == 0) {
        return true;
    }

    return false;
}

ValidationResult ConfigValidator::validate(const std::string& configPath)
{
    ValidationResult result;

    auto startTime = std::chrono::high_resolution_clock::now();

    std::string content;
    if (!readFile(configPath, content)) {
        result.hasErrors = true;
        result.errors.push_back(ValidationError(
            0, ValidationSeverity::Error, "E001",
            "Cannot open file: " + configPath));
        return result;
    }

    ValidationContext ctx;
    ctx.currentFile = configPath;
    ctx.includedFiles.insert(configPath);

    validateSyntax(content, configPath, result);
    validateSemantics(content, configPath, result, ctx);

    auto endTime = std::chrono::high_resolution_clock::now();
    result.validationTimeMs =
        std::chrono::duration<double, std::milli>(endTime - startTime).count();

    return result;
}

ValidationResult ConfigValidator::validateString(const std::string& data,
                                                  const std::string& filename)
{
    ValidationResult result;

    auto startTime = std::chrono::high_resolution_clock::now();

    ValidationContext ctx;
    ctx.currentFile = filename;

    validateSyntax(data, filename, result);
    validateSemantics(data, filename, result, ctx);

    auto endTime = std::chrono::high_resolution_clock::now();
    result.validationTimeMs =
        std::chrono::duration<double, std::milli>(endTime - startTime).count();

    return result;
}

void ConfigValidator::validateSyntax(const std::string& data,
                                     const std::string& filename,
                                     ValidationResult& result)
{
    // Use existing parser to check syntax
    Parser parser(data.c_str(), data.size());

    // Set up prefixes for proper tokenization
    static const std::vector<std::string> prefixes = {
        "=", "=>", "&&", "||", ":", "$", "&",
        "-=", "+=", "!!!", "!!", "!",
        "E0-", "E1-",
        "S-", "A-", "M-", "C-", "W-", "*", "~",
        "U-", "D-",
        "R-", "IL-", "IC-", "I-",
        "NL-", "CL-", "SL-", "KL-",
        "MAX-", "MIN-", "MMAX-", "MMIN-",
        "T-", "TS-",
        "M0-", "M1-", "M2-", "M3-", "M4-",
        "M5-", "M6-", "M7-", "M8-", "M9-",
        "M10-", "M11-", "M12-", "M13-", "M14-",
        "M15-", "M16-", "M17-", "M18-", "M19-",
        "L0-", "L1-", "L2-", "L3-", "L4-",
        "L5-", "L6-", "L7-", "L8-", "L9-",
    };
    parser.setPrefixes(&prefixes);

    std::vector<Token> tokens;
    while (true) {
        try {
            if (!parser.getLine(&tokens)) {
                break;
            }
        } catch (const ErrorMessage& e) {
            result.hasErrors = true;
            result.errors.push_back(ValidationError(
                parser.getLineNumber(),
                ValidationSeverity::Error,
                "E100",
                e.getMessage()));
        } catch (const std::exception& e) {
            result.hasErrors = true;
            result.errors.push_back(ValidationError(
                parser.getLineNumber(),
                ValidationSeverity::Error,
                "E101",
                std::string("Parser exception: ") + e.what()));
        }
    }
}

void ConfigValidator::validateSemantics(const std::string& data,
                                        const std::string& filename,
                                        ValidationResult& result,
                                        ValidationContext& ctx)
{
    Parser parser(data.c_str(), data.size());

    static const std::vector<std::string> prefixes = {
        "=", "=>", "&&", "||", ":", "$", "&",
        "-=", "+=", "!!!", "!!", "!",
        "E0-", "E1-",
        "S-", "A-", "M-", "C-", "W-", "*", "~",
        "U-", "D-",
        "R-", "IL-", "IC-", "I-",
        "NL-", "CL-", "SL-", "KL-",
        "MAX-", "MIN-", "MMAX-", "MMIN-",
        "T-", "TS-",
        "M0-", "M1-", "M2-", "M3-", "M4-",
        "M5-", "M6-", "M7-", "M8-", "M9-",
        "M10-", "M11-", "M12-", "M13-", "M14-",
        "M15-", "M16-", "M17-", "M18-", "M19-",
        "L0-", "L1-", "L2-", "L3-", "L4-",
        "L5-", "L6-", "L7-", "L8-", "L9-",
    };
    parser.setPrefixes(&prefixes);

    std::vector<Token> tokens;
    while (true) {
        try {
            if (!parser.getLine(&tokens)) {
                break;
            }
            validateLine(tokens, parser.getLineNumber(), result, ctx);
        } catch (const ErrorMessage& e) {
            // Syntax errors already caught above
        } catch (const std::exception& e) {
            // Already handled
        }
    }

    // Check for unbalanced if/endif
    if (!ctx.conditionStack.empty()) {
        result.hasErrors = true;
        result.errors.push_back(ValidationError(
            0, ValidationSeverity::Error, "E200",
            "Unbalanced if/endif: missing " +
            std::to_string(ctx.conditionStack.size()) + " endif(s)"));
    }

    // Check for undefined keymap references
    if (m_options.checkKeymapRefs) {
        for (const auto& ref : ctx.referencedKeymaps) {
            if (ctx.definedKeymaps.find(ref) == ctx.definedKeymaps.end() &&
                ref != "Global") {  // Global is always defined implicitly
                result.hasWarnings = true;
                result.errors.push_back(ValidationError(
                    0, ValidationSeverity::Warning, "W200",
                    "Undefined keymap reference: " + ref));
            }
        }
    }

    // Check for undefined keyseq references
    if (m_options.checkKeyseqRefs) {
        for (const auto& ref : ctx.referencedKeyseqs) {
            if (ctx.definedKeyseqs.find(ref) == ctx.definedKeyseqs.end()) {
                result.hasWarnings = true;
                result.errors.push_back(ValidationError(
                    0, ValidationSeverity::Warning, "W201",
                    "Undefined keyseq reference: $" + ref));
            }
        }
    }
}

void ConfigValidator::validateLine(const std::vector<Token>& tokens,
                                   size_t lineNumber,
                                   ValidationResult& result,
                                   ValidationContext& ctx)
{
    if (tokens.empty()) {
        return;
    }

    auto it = tokens.begin();
    const Token& first = *it;

    // Handle condition directives
    if (first == "if" || first == "and") {
        ctx.conditionStack.push_back(true);
        return;
    }

    if (first == "else" || first == "elseif" || first == "elsif" ||
        first == "elif" || first == "or") {
        if (ctx.conditionStack.empty()) {
            result.hasErrors = true;
            result.errors.push_back(ValidationError(
                lineNumber, ValidationSeverity::Error, "E201",
                "'" + first.getString() + "' without matching 'if'"));
        }
        return;
    }

    if (first == "endif") {
        if (ctx.conditionStack.empty()) {
            result.hasErrors = true;
            result.errors.push_back(ValidationError(
                lineNumber, ValidationSeverity::Error, "E202",
                "'endif' without matching 'if'"));
        } else {
            ctx.conditionStack.pop_back();
        }
        return;
    }

    // Handle define
    if (first == "define") {
        if (tokens.size() >= 2) {
            ctx.definedSymbols.insert((++it)->getString());
        } else {
            result.hasErrors = true;
            result.errors.push_back(ValidationError(
                lineNumber, ValidationSeverity::Error, "E210",
                "'define' requires a symbol name"));
        }
        return;
    }

    // Handle include
    if (first == "include") {
        if (tokens.size() >= 2) {
            ++it;
            validateInclude(it->getString(), lineNumber, result, ctx);
        } else {
            result.hasErrors = true;
            result.errors.push_back(ValidationError(
                lineNumber, ValidationSeverity::Error, "E211",
                "'include' requires a filename"));
        }
        return;
    }

    // Handle keymap/window definitions
    if (first == "keymap" || first == "keymap2" || first == "window") {
        if (tokens.size() >= 2) {
            ++it;
            std::string keymapName = it->getString();
            ctx.definedKeymaps.insert(keymapName);

            // Check for parent keymap reference
            while (it != tokens.end()) {
                if (*it == ":") {
                    ++it;
                    if (it != tokens.end()) {
                        ctx.referencedKeymaps.insert(it->getString());
                    }
                    break;
                }
                ++it;
            }
        } else {
            result.hasErrors = true;
            result.errors.push_back(ValidationError(
                lineNumber, ValidationSeverity::Error, "E220",
                "'" + first.getString() + "' requires a name"));
        }
        return;
    }

    // Handle keyseq definitions
    if (first == "keyseq") {
        ++it;
        if (it != tokens.end() && *it == "$") {
            ++it;
            if (it != tokens.end()) {
                std::string definedName = it->getString();
                ctx.definedKeyseqs.insert(definedName);
                ++it;

                // Skip past '=' if present
                if (it != tokens.end() && (*it == "=" || *it == "=>")) {
                    ++it;
                }

                // Check for keyseq references in the right-hand side
                while (it != tokens.end()) {
                    if (*it == "$") {
                        ++it;
                        if (it != tokens.end() && it->isString()) {
                            ctx.referencedKeyseqs.insert(it->getString());
                        }
                    }
                    if (it != tokens.end()) {
                        ++it;
                    }
                }
            }
        } else {
            result.hasErrors = true;
            result.errors.push_back(ValidationError(
                lineNumber, ValidationSeverity::Error, "E221",
                "'keyseq' requires '$name' format"));
        }
        return;
    }

    // Handle def (keyboard definitions)
    if (first == "def") {
        ++it;
        if (it == tokens.end()) {
            result.hasErrors = true;
            result.errors.push_back(ValidationError(
                lineNumber, ValidationSeverity::Error, "E230",
                "'def' requires a definition type"));
            return;
        }

        if (*it == "key") {
            // def key - key definition
            ++it;
            if (it != tokens.end()) {
                // Extract key name(s)
                if (*it == '(') {
                    ++it;
                    while (it != tokens.end() && !(*it == ')')) {
                        if (it->isString()) {
                            ctx.definedKeys.insert(it->getString());
                        }
                        ++it;
                    }
                } else {
                    ctx.definedKeys.insert(it->getString());
                }
            }
        }
        return;
    }

    // Check for keyseq references ($name)
    for (auto token = tokens.begin(); token != tokens.end(); ++token) {
        if (*token == "$") {
            ++token;
            if (token != tokens.end() && token->isString()) {
                ctx.referencedKeyseqs.insert(token->getString());
            }
        }
    }

    // Handle key assignments (key SomeKey => ...)
    if (first == "key") {
        // Key assignment - check references in the assignment
        return;
    }

    // Handle mod assignments
    if (first == "mod") {
        return;
    }

    // Handle event assignments
    if (first == "event") {
        return;
    }
}

void ConfigValidator::validateInclude(const std::string& includePath,
                                      size_t lineNumber,
                                      ValidationResult& result,
                                      ValidationContext& ctx)
{
    if (!m_options.checkIncludes) {
        return;
    }

    if (ctx.includeDepth >= m_options.maxIncludeDepth) {
        result.hasErrors = true;
        result.errors.push_back(ValidationError(
            lineNumber, ValidationSeverity::Error, "E300",
            "Include depth limit exceeded (max: " +
            std::to_string(m_options.maxIncludeDepth) + ")"));
        return;
    }

    std::string resolvedPath;
    if (!resolveIncludePath(includePath, ctx.currentFile, resolvedPath)) {
        result.hasErrors = true;
        result.errors.push_back(ValidationError(
            lineNumber, ValidationSeverity::Error, "E301",
            "Cannot find include file: " + includePath));
        return;
    }

    // Check for circular includes
    if (ctx.includedFiles.find(resolvedPath) != ctx.includedFiles.end()) {
        result.hasErrors = true;
        result.errors.push_back(ValidationError(
            lineNumber, ValidationSeverity::Error, "E302",
            "Circular include detected: " + resolvedPath));
        return;
    }

    // Read and validate included file
    std::string content;
    if (!readFile(resolvedPath, content)) {
        result.hasErrors = true;
        result.errors.push_back(ValidationError(
            lineNumber, ValidationSeverity::Error, "E303",
            "Cannot read include file: " + resolvedPath));
        return;
    }

    // Save current state
    std::string savedFile = ctx.currentFile;
    ctx.currentFile = resolvedPath;
    ctx.includedFiles.insert(resolvedPath);
    ++ctx.includeDepth;

    // Validate included file
    validateSyntax(content, resolvedPath, result);
    validateSemantics(content, resolvedPath, result, ctx);

    // Restore state
    ctx.currentFile = savedFile;
    --ctx.includeDepth;
}

void ConfigValidator::checkKeyReference(const std::string& keyName,
                                        size_t lineNumber,
                                        ValidationResult& result,
                                        ValidationContext& ctx)
{
    if (!m_options.checkKeyNames) {
        return;
    }

    // Skip if key is defined locally
    if (ctx.definedKeys.find(keyName) != ctx.definedKeys.end()) {
        return;
    }

    // Note: Full key name validation would require loading the keyboard
    // definition, which is beyond scope of simple validation.
    // This is left as a placeholder for future enhancement.
}

void ConfigValidator::checkKeymapReference(const std::string& keymapName,
                                           size_t lineNumber,
                                           ValidationResult& result,
                                           ValidationContext& ctx)
{
    if (!m_options.checkKeymapRefs) {
        return;
    }

    ctx.referencedKeymaps.insert(keymapName);
}

void ConfigValidator::checkKeyseqReference(const std::string& keyseqName,
                                           size_t lineNumber,
                                           ValidationResult& result,
                                           ValidationContext& ctx)
{
    if (!m_options.checkKeyseqRefs) {
        return;
    }

    ctx.referencedKeyseqs.insert(keyseqName);
}
