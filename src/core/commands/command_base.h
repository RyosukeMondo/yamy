#pragma once
#ifndef _COMMAND_BASE_H
#define _COMMAND_BASE_H

#include "../functions/function_data.h"
#include <tuple>
#include <utility>
#include "utils/stringtool.h"

// Helper to check if a type is distinct from another (for SFINAE if needed, though not heavily used here yet)
// We rely on standard C++17 features.

// Generic Command Template
// Derived: The specific command class (CRTP)
// Args: The types of arguments this command accepts
template <typename Derived, typename... Args>
class Command : public FunctionData
{
public:
    using TupleType = std::tuple<Args...>;
    TupleType m_args;

    // Helper to get argument by index
    template <size_t I>
    const auto& getArg() const {
        return std::get<I>(m_args);
    }

    template <size_t I>
    auto& getArg() {
        return std::get<I>(m_args);
    }

    // --- Boilerplate Implementation ---

    // 1. Create
    static FunctionData *create()
    {
        return new Derived();
    }

    // 2. Clone
    virtual FunctionData *clone() const override
    {
        return new Derived(static_cast<const Derived&>(*this));
    }

    // 3. GetName is delegated to Derived::Name
    inline virtual std::string getName() const override
    {
        if constexpr (std::is_same_v<typename std::decay<decltype(*Derived::Name)>::type, char>) {
            return Derived::Name;
        } else {
            return to_UTF_8(Derived::Name);
        }
    }

    // 4. Load
    // Logic: OpenParen -> Load Arg1 -> Comma -> Load Arg2 ... -> CloseParen
    virtual void load(SettingLoader *i_sl) override
    {
        // First, check/consume OpenParen.
        // If Args is empty, we might not expect params, but the original code often does:
        // if (!i_sl->getOpenParen(false, getName())) return; 
        // OR 
        // i_sl->getOpenParen(true, getName());
        
        // This behavior varies slightly in legacy code. 
        // "Default" uses getOpenParen(false), returns if missing.
        // "KeymapPrevPrefix" uses getOpenParen(true) (throws if missing?).
        // Providing a customization point or assuming a standard behavior.
        // Most commands with args imply required parens.
        // Commands without args usually check optional parens.
        
        std::string name = getName();
        const char* cName = name.c_str();

        if constexpr (sizeof...(Args) == 0) {
            // No args: parens are optional but if present must be empty
            if (!i_sl->getOpenParen(false, cName))
                return;
            i_sl->getCloseParen(true, cName);
        } else {
            // Args present: parens required (heuristic based on majority of commands)
            // If strict adherence to legacy "optional paren" logic is needed for commands with args, we might need a flag.
            // For now, assume significant commands require parens.
            
            // Correction: Some legacy code might allow omitted parens even with defaults? 
            // But usually arguments imply parens.
            
            // Let's support the generic flow:
            // Check OpenParen (Strict = true because if we have args, we likely need to parse them)
            i_sl->getOpenParen(true, cName);
            
            loadArgs(i_sl, std::make_index_sequence<sizeof...(Args)>{});
            
            i_sl->getCloseParen(true, cName);
        }
    }

    // 5. Output
    virtual std::ostream &output(std::ostream &i_ost) const override
    {
        // getName() returns std::string, output directly
        i_ost << "&" << getName();
        // For commands with manual load/output (no template args),
        // they may override outputArgs() to provide custom output
        // Check if derived class has overridden outputArgs by calling it
        // For template-arg commands, outputArgs calls outputArgsInternal
        if constexpr (sizeof...(Args) > 0) {
            i_ost << "(";
            outputArgsInternal(i_ost, std::make_index_sequence<sizeof...(Args)>{});
            i_ost << ") ";
        } else {
            // For zero-arg commands that might have manual members,
            // check if they need to output args
            // This is a bit tricky - we'll just call outputArgs which may be overridden
            // but only if there's something to output (derived class responsibility)
        }
        return i_ost;
    }

protected:
    // Virtual method for custom output args - derived classes can override
    // For commands that manually load their members (not using template Args)
    virtual std::ostream &outputArgs(std::ostream &i_ost) const
    {
        // Default: delegate to template-based output if Args exist
        if constexpr (sizeof...(Args) > 0) {
            outputArgsInternal(i_ost, std::make_index_sequence<sizeof...(Args)>{});
        }
        return i_ost;
    }

private:
    // --- Load Helpers ---
    template <size_t I>
    void loadArgsRecursive(SettingLoader *i_sl)
    {
        if constexpr (I < sizeof...(Args)) {
            // For the first argument (I == 0), we just load it.
            // For subsequent arguments (I > 0), we must check for a comma first.
            bool canProceed = true;
            if constexpr (I > 0) {
                 if (!i_sl->getComma(false, getName().c_str())) {
                     canProceed = false;
                 }
            }
            
            if (canProceed) {
                i_sl->load_ARGUMENT(&std::get<I>(m_args));
                // Recurse for the next argument
                loadArgsRecursive<I + 1>(i_sl);
            }
        }
    }

    template <size_t... Is>
    void loadArgs(SettingLoader *i_sl, std::index_sequence<Is...>)
    {
        // Start the recursive loader
        loadArgsRecursive<0>(i_sl);
    }

    // --- Output Helpers ---
    template <size_t... Is>
    void outputArgsInternal(std::ostream &i_ost, std::index_sequence<Is...>) const
    {
        int dummy[] = { 0, (outputOneArg<Is>(i_ost, Is < sizeof...(Args) - 1), 0)... };
        (void)dummy;
    }

    template <size_t I>
    void outputOneArg(std::ostream &i_ost, bool hasMore) const
    {
        i_ost << std::get<I>(m_args);
        if (hasMore) {
            i_ost << ", ";
        }
    }
};

#endif // _COMMAND_BASE_H
