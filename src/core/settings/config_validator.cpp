//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// config_validator.cpp
// Implementation of configuration file validator (JSON-based)

#include "config_validator.h"
#include "stringtool.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <chrono>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

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
    for (const auto& error : errors) {
        if (error.isError()) {
            count++;
        }
    }
    return count;
}

size_t ValidationResult::warningCount() const
{
    size_t count = 0;
    for (const auto& error : errors) {
        if (!error.isError()) {
            count++;
        }
    }
    return count;
}

std::string ValidationResult::formatAll() const
{
    std::ostringstream oss;
    for (const auto& error : errors) {
        oss << error.format() << "\n";
    }
    return oss.str();
}

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

ConfigValidator::~ConfigValidator()
{
}

ValidationResult ConfigValidator::validate(const std::string& configPath)
{
    ValidationResult result;
    auto start = std::chrono::high_resolution_clock::now();

    // Check if file exists
    if (!std::filesystem::exists(configPath)) {
        ValidationError error(0, ValidationSeverity::Error, "E001",
                            "Configuration file not found: " + configPath);
        result.errors.push_back(error);
        result.hasErrors = true;
        return result;
    }

    // Read file content
    std::string content;
    if (!readFile(configPath, content)) {
        ValidationError error(0, ValidationSeverity::Error, "E002",
                            "Failed to read configuration file: " + configPath);
        result.errors.push_back(error);
        result.hasErrors = true;
        return result;
    }

    // Validate content
    result = validateString(content, configPath);

    auto end = std::chrono::high_resolution_clock::now();
    result.validationTimeMs = std::chrono::duration<double, std::milli>(end - start).count();

    return result;
}

ValidationResult ConfigValidator::validateString(const std::string& data,
                                                 const std::string& filename)
{
    ValidationResult result;

    // Check if this is a JSON file
    bool isJson = filename.find(".json") != std::string::npos;

    if (isJson) {
        // Validate JSON syntax
        try {
            auto j = json::parse(data);

            // Basic JSON config schema validation
            if (!j.contains("keyboard")) {
                ValidationError error(0, ValidationSeverity::Warning, "W001",
                                    "Missing 'keyboard' section - config may not work correctly");
                result.errors.push_back(error);
                result.hasWarnings = true;
            }

            if (!j.contains("mappings") && !j.contains("virtualModifiers")) {
                ValidationError error(0, ValidationSeverity::Warning, "W002",
                                    "No 'mappings' or 'virtualModifiers' defined - config has no effect");
                result.errors.push_back(error);
                result.hasWarnings = true;
            }

            // Check for version field
            if (!j.contains("version")) {
                ValidationError error(0, ValidationSeverity::Warning, "W003",
                                    "Missing 'version' field - assuming latest version");
                result.errors.push_back(error);
                result.hasWarnings = true;
            }

        } catch (const json::parse_error& e) {
            ValidationError error(0, ValidationSeverity::Error, "E003",
                                std::string("JSON parse error: ") + e.what());
            result.errors.push_back(error);
            result.hasErrors = true;
        }
    } else {
        // Legacy .mayu file - not supported anymore
        ValidationError error(0, ValidationSeverity::Error, "E004",
                            ".mayu files are no longer supported - please convert to JSON format");
        result.errors.push_back(error);
        result.hasErrors = true;
    }

    return result;
}

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
    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }

    std::ostringstream oss;
    oss << file.rdbuf();
    content = oss.str();
    return true;
}
