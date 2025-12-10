#pragma once
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// config_validator.h
// Validates .mayu configuration files for syntax and semantic errors

#ifndef _CONFIG_VALIDATOR_H
#define _CONFIG_VALIDATOR_H

#include <string>
#include <vector>

/// Severity level for validation errors
enum class ValidationSeverity {
    Error,      /// Prevents config from loading
    Warning     /// Config may load but behavior is undefined
};

/// Represents a validation error with location and message
struct ValidationError {
    size_t lineNumber;          /// Line number where error occurred (1-based)
    size_t columnNumber;        /// Column number (1-based, 0 if unknown)
    ValidationSeverity severity;/// Error or warning
    std::string code;           /// Error code (e.g., "E001", "W001")
    std::string message;        /// Human-readable error message
    std::string context;        /// The problematic line or snippet

    ValidationError()
        : lineNumber(0), columnNumber(0), severity(ValidationSeverity::Error) {}

    ValidationError(size_t line, ValidationSeverity sev,
                   const std::string& errCode, const std::string& msg)
        : lineNumber(line), columnNumber(0), severity(sev),
          code(errCode), message(msg) {}

    ValidationError(size_t line, size_t col, ValidationSeverity sev,
                   const std::string& errCode, const std::string& msg,
                   const std::string& ctx = "")
        : lineNumber(line), columnNumber(col), severity(sev),
          code(errCode), message(msg), context(ctx) {}

    /// Format error for display
    std::string format() const;

    /// Check if this is an error (vs warning)
    bool isError() const { return severity == ValidationSeverity::Error; }
};

/// Result of validation containing all errors found
struct ValidationResult {
    std::vector<ValidationError> errors;
    bool hasErrors;             /// True if any errors (not warnings) found
    bool hasWarnings;           /// True if any warnings found
    double validationTimeMs;    /// Time taken for validation in milliseconds

    ValidationResult() : hasErrors(false), hasWarnings(false), validationTimeMs(0) {}

    /// Get count of errors only (excluding warnings)
    size_t errorCount() const;

    /// Get count of warnings only
    size_t warningCount() const;

    /// Check if validation passed (no errors)
    bool isValid() const { return !hasErrors; }

    /// Get all errors formatted as string
    std::string formatAll() const;
};

/// Validates .mayu configuration files without loading into engine
/// Performs syntax checking and semantic validation
class ConfigValidator {
public:
    /// Validation options
    struct Options {
        bool checkIncludes;     /// Follow and validate include directives
        bool checkKeyNames;     /// Validate key names against known keys
        bool checkKeymapRefs;   /// Check keymap references exist
        bool checkKeyseqRefs;   /// Check keyseq references exist
        size_t maxIncludeDepth; /// Maximum depth for include recursion

        Options()
            : checkIncludes(true),
              checkKeyNames(true),
              checkKeymapRefs(true),
              checkKeyseqRefs(true),
              maxIncludeDepth(10) {}
    };

    ConfigValidator();
    explicit ConfigValidator(const Options& options);
    ~ConfigValidator();

    /// Validate a configuration file by path
    /// @param configPath Path to .mayu file
    /// @return ValidationResult containing all errors/warnings
    ValidationResult validate(const std::string& configPath);

    /// Validate configuration from string data
    /// @param data Configuration content as string
    /// @param filename Optional filename for error messages
    /// @return ValidationResult containing all errors/warnings
    ValidationResult validateString(const std::string& data,
                                   const std::string& filename = "<string>");

    /// Get/set validation options
    const Options& options() const { return m_options; }
    void setOptions(const Options& options) { m_options = options; }

    /// Add a search path for include directives
    void addIncludePath(const std::string& path);

    /// Clear all include search paths
    void clearIncludePaths();

private:
    /// Internal validation state
    struct ValidationContext;

    /// Perform syntax validation using parser
    void validateSyntax(const std::string& data,
                       const std::string& filename,
                       ValidationResult& result);

    /// Perform semantic validation (references, definitions)
    void validateSemantics(const std::string& data,
                          const std::string& filename,
                          ValidationResult& result,
                          ValidationContext& ctx);

    /// Validate a single line's tokens
    void validateLine(const std::vector<class Token>& tokens,
                     size_t lineNumber,
                     ValidationResult& result,
                     ValidationContext& ctx);

    /// Check for undefined key references
    void checkKeyReference(const std::string& keyName,
                          size_t lineNumber,
                          ValidationResult& result,
                          ValidationContext& ctx);

    /// Check for undefined keymap references
    void checkKeymapReference(const std::string& keymapName,
                             size_t lineNumber,
                             ValidationResult& result,
                             ValidationContext& ctx);

    /// Check for undefined keyseq references
    void checkKeyseqReference(const std::string& keyseqName,
                             size_t lineNumber,
                             ValidationResult& result,
                             ValidationContext& ctx);

    /// Validate include directive
    void validateInclude(const std::string& includePath,
                        size_t lineNumber,
                        ValidationResult& result,
                        ValidationContext& ctx);

    /// Read file contents
    bool readFile(const std::string& path, std::string& content);

    /// Resolve include path
    bool resolveIncludePath(const std::string& includeName,
                           const std::string& currentFile,
                           std::string& resolvedPath);

    Options m_options;
    std::vector<std::string> m_includePaths;
};

#endif // !_CONFIG_VALIDATOR_H
