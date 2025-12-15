#pragma once

#include <unordered_set>
#include <vector>
#include <string>
#include <filesystem>
#include <sstream>
#include "../../utils/errormessage.h"  // For ErrorMessage

namespace yamy {

/// Tracks include file stack to prevent circular dependencies
class IncludeContext {
private:
    std::unordered_set<std::string> m_loadedFiles;  ///< Absolute paths of loaded files
    std::vector<std::string> m_includeStack;        ///< Current include stack
    size_t m_maxDepth;                              ///< Maximum include depth

public:
    /// Constructor
    /// @param maxDepth Maximum allowed include depth (default: 32)
    explicit IncludeContext(size_t maxDepth = 32)
        : m_maxDepth(maxDepth) {}

    /// Check if a file can be included (not circular, not too deep)
    /// @param filePath Path to the file to check
    /// @return true if file can be included, false if already loaded
    /// @throws ErrorMessage if include depth exceeded
    bool canInclude(const std::string& filePath) const {
        if (m_includeStack.size() >= m_maxDepth) {
            throw ErrorMessage() << "Include depth exceeded (max "
                                << m_maxDepth << "). Current stack:\n"
                                << formatIncludeStack("");
        }

        try {
            std::string absPath = std::filesystem::absolute(filePath).string();
            return m_loadedFiles.find(absPath) == m_loadedFiles.end();
        } catch (const std::filesystem::filesystem_error&) {
            // If path resolution fails, assume it's a new file
            return m_loadedFiles.find(filePath) == m_loadedFiles.end();
        }
    }

    /// Push a file onto the include stack
    /// @param filePath Path to the file being included
    /// @throws ErrorMessage if circular include detected
    void pushInclude(const std::string& filePath) {
        std::string absPath;
        try {
            absPath = std::filesystem::absolute(filePath).string();
        } catch (const std::filesystem::filesystem_error&) {
            absPath = filePath;  // Use original path if resolution fails
        }

        if (m_loadedFiles.find(absPath) != m_loadedFiles.end()) {
            throw ErrorMessage() << "Circular include detected:\n"
                                << formatIncludeStack(absPath);
        }

        m_loadedFiles.insert(absPath);
        m_includeStack.push_back(absPath);
    }

    /// Pop the top file from the include stack
    void popInclude() {
        if (!m_includeStack.empty()) {
            // Remove from stack but keep in loaded files set
            // This allows re-including the same file from different paths
            m_includeStack.pop_back();
        }
    }

    /// Get current include depth
    /// @return Number of files currently on the include stack
    size_t getDepth() const {
        return m_includeStack.size();
    }

    /// Format the include stack for error messages
    /// @param newFile Optional file being added (for circular include errors)
    /// @return Formatted string showing the include chain
    std::string formatIncludeStack(const std::string& newFile) const {
        std::ostringstream oss;
        for (size_t i = 0; i < m_includeStack.size(); ++i) {
            oss << "  [" << i << "] " << m_includeStack[i] << "\n";
        }
        if (!newFile.empty()) {
            oss << "  [" << m_includeStack.size() << "] " << newFile << " (CIRCULAR!)";
        }
        return oss.str();
    }

    /// Reset the context (for testing or multiple config loads)
    void reset() {
        m_loadedFiles.clear();
        m_includeStack.clear();
    }
};

/// RAII guard for include stack management
class IncludeGuard {
private:
    IncludeContext& m_context;
    bool m_pushed;

public:
    /// Constructor - pushes file onto include stack
    /// @param context Include context to manage
    /// @param filePath Path to the file being included
    /// @throws ErrorMessage if circular include or depth exceeded
    IncludeGuard(IncludeContext& context, const std::string& filePath)
        : m_context(context), m_pushed(false) {
        if (!m_context.canInclude(filePath)) {
            throw ErrorMessage() << "Cannot include file (circular dependency):\n"
                                << m_context.formatIncludeStack(filePath);
        }
        m_context.pushInclude(filePath);
        m_pushed = true;
    }

    /// Destructor - pops file from include stack
    ~IncludeGuard() {
        if (m_pushed) {
            m_context.popInclude();
        }
    }

    // Non-copyable, non-movable
    IncludeGuard(const IncludeGuard&) = delete;
    IncludeGuard& operator=(const IncludeGuard&) = delete;
    IncludeGuard(IncludeGuard&&) = delete;
    IncludeGuard& operator=(IncludeGuard&&) = delete;
};

} // namespace yamy
