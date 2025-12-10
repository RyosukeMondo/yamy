// config_validator_test.cpp
// Unit tests for ConfigValidator class

#include <gtest/gtest.h>
#include "config_validator.h"
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

class ConfigValidatorTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a unique temporary test directory per test
        static int testCounter = 0;
        testDir = fs::temp_directory_path() / ("config_validator_test_" + std::to_string(++testCounter));
        fs::create_directories(testDir);
    }

    void TearDown() override {
        // Clean up test directory
        if (fs::exists(testDir)) {
            fs::remove_all(testDir);
        }
    }

    // Helper to create test config files
    std::string createConfig(const std::string& name, const std::string& content) {
        auto path = testDir / name;
        std::ofstream ofs(path);
        ofs << content;
        ofs.close();
        return path.string();
    }

    fs::path testDir;
};

//=============================================================================
// Basic functionality tests
//=============================================================================

TEST_F(ConfigValidatorTest, ValidateEmptyFile) {
    std::string path = createConfig("empty.mayu", "");
    ConfigValidator validator;
    auto result = validator.validate(path);
    EXPECT_TRUE(result.isValid());
    EXPECT_EQ(result.errorCount(), 0u);
}

TEST_F(ConfigValidatorTest, ValidateCommentOnlyFile) {
    std::string path = createConfig("comments.mayu",
        "# This is a comment\n"
        "# Another comment\n"
    );
    ConfigValidator validator;
    auto result = validator.validate(path);
    EXPECT_TRUE(result.isValid());
    EXPECT_EQ(result.errorCount(), 0u);
}

TEST_F(ConfigValidatorTest, ValidateNonexistentFile) {
    ConfigValidator validator;
    auto result = validator.validate("/nonexistent/path.mayu");
    EXPECT_FALSE(result.isValid());
    EXPECT_GT(result.errorCount(), 0u);
    EXPECT_TRUE(result.errors[0].message.find("Cannot open") != std::string::npos);
}

//=============================================================================
// Basic syntax tests
//=============================================================================

TEST_F(ConfigValidatorTest, ValidateBasicKeymap) {
    std::string path = createConfig("basic.mayu",
        "keymap Global\n"
    );
    ConfigValidator validator;
    auto result = validator.validate(path);
    EXPECT_TRUE(result.isValid());
}

TEST_F(ConfigValidatorTest, ValidateWindowKeymap) {
    std::string path = createConfig("window.mayu",
        "window Terminal /term.*/\n"
        "keymap Editor\n"
    );
    ConfigValidator validator;
    auto result = validator.validate(path);
    EXPECT_TRUE(result.isValid());
}

TEST_F(ConfigValidatorTest, ValidateKeyseqDefinition) {
    std::string path = createConfig("keyseq.mayu",
        "keymap Global\n"
        "keyseq $myseq = A B C\n"
    );
    ConfigValidator validator;
    auto result = validator.validate(path);
    EXPECT_TRUE(result.isValid());
}

TEST_F(ConfigValidatorTest, ValidateDefineSymbol) {
    std::string path = createConfig("define.mayu",
        "define MY_SYMBOL\n"
        "if (MY_SYMBOL)\n"
        "keymap Global\n"
        "endif\n"
    );
    ConfigValidator validator;
    auto result = validator.validate(path);
    EXPECT_TRUE(result.isValid());
}

//=============================================================================
// Conditional directive tests
//=============================================================================

TEST_F(ConfigValidatorTest, ValidateBalancedIfEndif) {
    std::string path = createConfig("balanced.mayu",
        "if (SYMBOL)\n"
        "keymap Global\n"
        "endif\n"
    );
    ConfigValidator validator;
    auto result = validator.validate(path);
    EXPECT_TRUE(result.isValid());
}

TEST_F(ConfigValidatorTest, DetectUnbalancedIf) {
    std::string path = createConfig("unbalanced_if.mayu",
        "if (SYMBOL)\n"
        "keymap Global\n"
        "# missing endif\n"
    );
    ConfigValidator validator;
    auto result = validator.validate(path);
    EXPECT_FALSE(result.isValid());
    EXPECT_GT(result.errorCount(), 0u);
    // Should report unbalanced if/endif
    bool foundError = false;
    for (const auto& err : result.errors) {
        if (err.message.find("Unbalanced") != std::string::npos ||
            err.message.find("unbalanced") != std::string::npos) {
            foundError = true;
            break;
        }
    }
    EXPECT_TRUE(foundError);
}

TEST_F(ConfigValidatorTest, DetectElseWithoutIf) {
    std::string path = createConfig("else_no_if.mayu",
        "else\n"
        "keymap Global\n"
        "endif\n"
    );
    ConfigValidator validator;
    auto result = validator.validate(path);
    EXPECT_FALSE(result.isValid());
}

TEST_F(ConfigValidatorTest, DetectEndifWithoutIf) {
    std::string path = createConfig("endif_no_if.mayu",
        "keymap Global\n"
        "endif\n"
    );
    ConfigValidator validator;
    auto result = validator.validate(path);
    EXPECT_FALSE(result.isValid());
}

TEST_F(ConfigValidatorTest, ValidateNestedIf) {
    std::string path = createConfig("nested.mayu",
        "if (OUTER)\n"
        "  if (INNER)\n"
        "    keymap Global\n"
        "  endif\n"
        "endif\n"
    );
    ConfigValidator validator;
    auto result = validator.validate(path);
    EXPECT_TRUE(result.isValid());
}

//=============================================================================
// Include directive tests
//=============================================================================

TEST_F(ConfigValidatorTest, ValidateIncludeExistingFile) {
    createConfig("included.mayu", "keymap Included\n");
    std::string path = createConfig("main.mayu",
        "keymap Global\n"
        "include \"included.mayu\"\n"
    );
    ConfigValidator validator;
    auto result = validator.validate(path);
    EXPECT_TRUE(result.isValid());
}

TEST_F(ConfigValidatorTest, DetectIncludeMissingFile) {
    std::string path = createConfig("main.mayu",
        "keymap Global\n"
        "include \"nonexistent.mayu\"\n"
    );
    ConfigValidator validator;
    auto result = validator.validate(path);
    EXPECT_FALSE(result.isValid());
    bool foundIncludeError = false;
    for (const auto& err : result.errors) {
        if (err.message.find("Cannot find include") != std::string::npos) {
            foundIncludeError = true;
            break;
        }
    }
    EXPECT_TRUE(foundIncludeError);
}

TEST_F(ConfigValidatorTest, DetectCircularInclude) {
    createConfig("a.mayu", "include \"b.mayu\"\n");
    createConfig("b.mayu", "include \"a.mayu\"\n");
    std::string path = (testDir / "a.mayu").string();

    ConfigValidator validator;
    auto result = validator.validate(path);
    EXPECT_FALSE(result.isValid());
    bool foundCircularError = false;
    for (const auto& err : result.errors) {
        if (err.message.find("Circular") != std::string::npos ||
            err.message.find("circular") != std::string::npos) {
            foundCircularError = true;
            break;
        }
    }
    EXPECT_TRUE(foundCircularError);
}

TEST_F(ConfigValidatorTest, DetectIncludeDepthExceeded) {
    // Create a chain of includes that exceeds default depth (10)
    for (int i = 0; i < 15; ++i) {
        std::string name = "level" + std::to_string(i) + ".mayu";
        std::string nextName = "level" + std::to_string(i + 1) + ".mayu";
        if (i < 14) {
            createConfig(name, "include \"" + nextName + "\"\n");
        } else {
            createConfig(name, "keymap Global\n");
        }
    }

    std::string path = (testDir / "level0.mayu").string();
    ConfigValidator validator;
    auto result = validator.validate(path);
    EXPECT_FALSE(result.isValid());
    bool foundDepthError = false;
    for (const auto& err : result.errors) {
        if (err.message.find("depth") != std::string::npos) {
            foundDepthError = true;
            break;
        }
    }
    EXPECT_TRUE(foundDepthError);
}

TEST_F(ConfigValidatorTest, DisableIncludeChecking) {
    std::string path = createConfig("main.mayu",
        "keymap Global\n"
        "include \"nonexistent.mayu\"\n"
    );
    ConfigValidator::Options opts;
    opts.checkIncludes = false;
    ConfigValidator validator(opts);
    auto result = validator.validate(path);
    // Should not report error for missing include
    EXPECT_TRUE(result.isValid());
}

//=============================================================================
// Keymap and keyseq reference tests
//=============================================================================

TEST_F(ConfigValidatorTest, WarnUndefinedKeymapReference) {
    std::string path = createConfig("ref.mayu",
        "keymap MyKeymap : UndefinedParent\n"
    );
    ConfigValidator validator;
    auto result = validator.validate(path);
    // Should warn (not error) about undefined parent keymap
    EXPECT_TRUE(result.hasWarnings || result.hasErrors);
}

TEST_F(ConfigValidatorTest, ValidateDefinedKeymapReference) {
    std::string path = createConfig("ref.mayu",
        "keymap ParentMap\n"
        "keymap ChildMap : ParentMap\n"
    );
    ConfigValidator validator;
    auto result = validator.validate(path);
    EXPECT_TRUE(result.isValid());
}

TEST_F(ConfigValidatorTest, WarnUndefinedKeyseqReference) {
    // Define a keyseq and then reference an undefined one
    std::string path = createConfig("ref.mayu",
        "keymap Global\n"
        "keyseq $defined_seq = A B\n"
        "keyseq $uses_undefined = $undefined_keyseq\n"
    );
    ConfigValidator validator;
    auto result = validator.validate(path);
    // Should warn about undefined keyseq reference
    EXPECT_TRUE(result.hasWarnings);
}

//=============================================================================
// Validation string input tests
//=============================================================================

TEST_F(ConfigValidatorTest, ValidateString) {
    ConfigValidator validator;
    auto result = validator.validateString(
        "keymap Global\n"
        "if (SYMBOL)\n"
        "endif\n",
        "test.mayu"
    );
    EXPECT_TRUE(result.isValid());
}

TEST_F(ConfigValidatorTest, ValidateStringWithErrors) {
    ConfigValidator validator;
    auto result = validator.validateString(
        "keymap Global\n"
        "if (SYMBOL)\n"
        "# missing endif\n",
        "test.mayu"
    );
    EXPECT_FALSE(result.isValid());
}

//=============================================================================
// Error formatting tests
//=============================================================================

TEST_F(ConfigValidatorTest, ErrorFormatContainsLineNumber) {
    ValidationError err(42, ValidationSeverity::Error, "E001", "Test error");
    std::string formatted = err.format();
    EXPECT_TRUE(formatted.find("42") != std::string::npos);
    EXPECT_TRUE(formatted.find("error") != std::string::npos);
    EXPECT_TRUE(formatted.find("Test error") != std::string::npos);
}

TEST_F(ConfigValidatorTest, WarningFormatContainsWarning) {
    ValidationError err(10, ValidationSeverity::Warning, "W001", "Test warning");
    std::string formatted = err.format();
    EXPECT_TRUE(formatted.find("warning") != std::string::npos);
}

TEST_F(ConfigValidatorTest, ResultFormatAll) {
    ValidationResult result;
    result.errors.push_back(ValidationError(1, ValidationSeverity::Error, "E001", "Error 1"));
    result.errors.push_back(ValidationError(2, ValidationSeverity::Warning, "W001", "Warning 1"));
    result.hasErrors = true;
    result.hasWarnings = true;

    std::string formatted = result.formatAll();
    EXPECT_TRUE(formatted.find("Error 1") != std::string::npos);
    EXPECT_TRUE(formatted.find("Warning 1") != std::string::npos);
}

//=============================================================================
// Performance tests
//=============================================================================

TEST_F(ConfigValidatorTest, ValidationTimeIsRecorded) {
    std::string path = createConfig("simple.mayu",
        "keymap Global\n"
        "if (A)\n"
        "endif\n"
    );
    ConfigValidator validator;
    auto result = validator.validate(path);
    EXPECT_GT(result.validationTimeMs, 0.0);
}

TEST_F(ConfigValidatorTest, ValidationIsFast) {
    // Generate a moderately sized config
    std::string content;
    for (int i = 0; i < 100; ++i) {
        content += "keymap Keymap" + std::to_string(i) + "\n";
        content += "# Some comment " + std::to_string(i) + "\n";
    }
    std::string path = createConfig("large.mayu", content);

    ConfigValidator validator;
    auto result = validator.validate(path);
    // Should complete in under 100ms for a ~200 line file
    EXPECT_LT(result.validationTimeMs, 100.0);
}

//=============================================================================
// Options tests
//=============================================================================

TEST_F(ConfigValidatorTest, CustomIncludeDepth) {
    // Create a chain of 5 includes
    for (int i = 0; i < 6; ++i) {
        std::string name = "depth" + std::to_string(i) + ".mayu";
        std::string nextName = "depth" + std::to_string(i + 1) + ".mayu";
        if (i < 5) {
            createConfig(name, "include \"" + nextName + "\"\n");
        } else {
            createConfig(name, "keymap Global\n");
        }
    }

    std::string path = (testDir / "depth0.mayu").string();

    // With depth limit of 3, should fail
    ConfigValidator::Options opts;
    opts.maxIncludeDepth = 3;
    ConfigValidator validator(opts);
    auto result = validator.validate(path);
    EXPECT_FALSE(result.isValid());

    // With depth limit of 10, should succeed
    opts.maxIncludeDepth = 10;
    validator.setOptions(opts);
    result = validator.validate(path);
    EXPECT_TRUE(result.isValid());
}

TEST_F(ConfigValidatorTest, AddIncludePath) {
    // Create include directory
    auto includeDir = testDir / "includes";
    fs::create_directories(includeDir);

    // Create included file in include directory
    std::ofstream ofs(includeDir / "lib.mayu");
    ofs << "keymap LibKeymap\n";
    ofs.close();

    // Create main config that includes from the include directory
    std::string path = createConfig("main.mayu",
        "keymap Global\n"
        "include \"lib.mayu\"\n"
    );

    ConfigValidator validator;
    validator.addIncludePath(includeDir.string());
    auto result = validator.validate(path);
    EXPECT_TRUE(result.isValid());
}

//=============================================================================
// UTF-8 BOM handling
//=============================================================================

TEST_F(ConfigValidatorTest, HandleUTF8BOM) {
    // Create file with UTF-8 BOM
    auto path = testDir / "bom.mayu";
    std::ofstream ofs(path, std::ios::binary);
    // UTF-8 BOM: EF BB BF
    ofs << '\xEF' << '\xBB' << '\xBF';
    ofs << "keymap Global\n";
    ofs.close();

    ConfigValidator validator;
    auto result = validator.validate(path.string());
    EXPECT_TRUE(result.isValid());
}
