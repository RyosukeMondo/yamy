#pragma once
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// registry.h


#ifndef _REGISTRY_H
#  define _REGISTRY_H

#  include "stringtool.h"
#  include "../utils/config_store.h"
#  include <list>
#  include <string>


/// registry access class
class Registry : public ConfigStore
{
    HKEY m_root;                    /// registry root
    std::string m_path;             /// path from registry root

public:
    typedef ConfigStore::strings strings;

public:
    ///
    Registry() : m_root(nullptr) {
        setRoot(nullptr, "");
    }
    ///
    Registry(HKEY i_root, const std::string &i_path)
            : m_root(i_root), m_path(i_path) {
        setRoot(i_root, i_path);
    }
    // Legacy constructor
    Registry(HKEY i_root, const tstring &i_path)
            : m_root(i_root), m_path(to_string(i_path)) {
        setRoot(i_root, to_string(i_path));
    }

    /// set registry root and path
    void setRoot(HKEY i_root, const std::string &i_path);
    // Legacy setRoot
    void setRoot(HKEY i_root, const tstring &i_path) {
        setRoot(i_root, to_string(i_path));
    }

    /// remvoe
    bool remove(const std::string &i_name = "") const override {
        return remove(m_root, m_path, i_name);
    }
    using ConfigStore::remove;

    /// does exist the key ?
    bool doesExist() const override {
        return doesExist(m_root, m_path);
    }

    /// read DWORD
    bool read(const std::string &i_name, int *o_value, int i_defaultValue = 0)
    const override {
        return read(m_root, m_path, i_name, o_value, i_defaultValue);
    }
    using ConfigStore::read;

    /// write DWORD
    bool write(const std::string &i_name, int i_value) const override {
        return write(m_root, m_path, i_name, i_value);
    }
    using ConfigStore::write;

    /// read string
    bool read(const std::string &i_name, std::string *o_value,
              const std::string &i_defaultValue = "") const override {
        return read(m_root, m_path, i_name, o_value, i_defaultValue);
    }

    /// write string
    bool write(const std::string &i_name, const std::string &i_value) const override {
        return write(m_root, m_path, i_name, i_value);
    }

#ifndef USE_INI
    /// read list of string
    bool read(const std::string &i_name, strings *o_value,
              const strings &i_defaultValue = strings()) const override {
        return read(m_root, m_path, i_name, o_value, i_defaultValue);
    }

    /// write list of string
    bool write(const std::string &i_name, const strings &i_value) const override {
        return write(m_root, m_path, i_name, i_value);
    }
#endif //!USE_INI

    /// read binary data
    bool read(const std::string &i_name, BYTE *o_value, DWORD *i_valueSize,
              const BYTE *i_defaultValue = nullptr, DWORD i_defaultValueSize = 0)
    const override {
        return read(m_root, m_path, i_name, o_value, i_valueSize, i_defaultValue,
                    i_defaultValueSize);
    }
    /// write binary data
    bool write(const std::string &i_name, const BYTE *i_value,
               DWORD i_valueSize) const override {
        return write(m_root, m_path, i_name, i_value, i_valueSize);
    }

public:
    /// remove
    static bool remove(HKEY i_root, const std::string &i_path,
                       const std::string &i_name = "");
    static bool remove(HKEY i_root, const tstring &i_path, const tstring &i_name = _T("")) {
        return remove(i_root, to_string(i_path), to_string(i_name));
    }

    /// does exist the key ?
    static bool doesExist(HKEY i_root, const std::string &i_path);
    static bool doesExist(HKEY i_root, const tstring &i_path) {
        return doesExist(i_root, to_string(i_path));
    }

    /// read DWORD
    static bool read(HKEY i_root, const std::string &i_path, const std::string &i_name,
                     int *o_value, int i_defaultValue = 0);
    static bool read(HKEY i_root, const tstring &i_path, const tstring &i_name,
                     int *o_value, int i_defaultValue = 0) {
        return read(i_root, to_string(i_path), to_string(i_name), o_value, i_defaultValue);
    }
    /// write DWORD
    static bool write(HKEY i_root, const std::string &i_path, const std::string &i_name,
                      int i_value);
    static bool write(HKEY i_root, const tstring &i_path, const tstring &i_name,
                      int i_value) {
        return write(i_root, to_string(i_path), to_string(i_name), i_value);
    }

    /// read string
    static bool read(HKEY i_root, const std::string &i_path, const std::string &i_name,
                     std::string *o_value, const std::string &i_defaultValue = "");
    static bool read(HKEY i_root, const tstring &i_path, const tstring &i_name,
                     tstring *o_value, const tstring &i_defaultValue = _T("")) {
        std::string val;
        if (read(i_root, to_string(i_path), to_string(i_name), &val, to_string(i_defaultValue))) {
            *o_value = to_tstring(val);
            return true;
        }
        return false;
    }
    /// write string
    static bool write(HKEY i_root, const std::string &i_path, const std::string &i_name,
                      const std::string &i_value);
    static bool write(HKEY i_root, const tstring &i_path, const tstring &i_name,
                      const tstring &i_value) {
        return write(i_root, to_string(i_path), to_string(i_name), to_string(i_value));
    }

#ifndef USE_INI
    /// read list of string
    static bool read(HKEY i_root, const std::string &i_path, const std::string &i_name,
                     strings *o_value, const strings &i_defaultValue = strings());
    static bool read(HKEY i_root, const tstring &i_path, const tstring &i_name,
                     tstrings *o_value, const tstrings &i_defaultValue = tstrings()) {
        strings val, def;
        for (const auto& s : i_defaultValue) def.push_back(to_string(s));
        if (read(i_root, to_string(i_path), to_string(i_name), &val, def)) {
            o_value->clear();
            for (const auto& s : val) o_value->push_back(to_tstring(s));
            return true;
        }
        return false;
    }
    /// write list of string
    static bool write(HKEY i_root, const std::string &i_path, const std::string &i_name,
                      const strings &i_value);
    static bool write(HKEY i_root, const tstring &i_path, const tstring &i_name,
                      const tstrings &i_value) {
        strings val;
        for (const auto& s : i_value) val.push_back(to_string(s));
        return write(i_root, to_string(i_path), to_string(i_name), val);
    }
#endif //!USE_INI

    /// read binary data
    static bool read(HKEY i_root, const std::string &i_path, const std::string &i_name,
                     BYTE *o_value, DWORD *i_valueSize,
                     const BYTE *i_defaultValue = nullptr,
                     DWORD i_defaultValueSize = 0);
    static bool read(HKEY i_root, const tstring &i_path, const tstring &i_name,
                     BYTE *o_value, DWORD *i_valueSize,
                     const BYTE *i_defaultValue = nullptr,
                     DWORD i_defaultValueSize = 0) {
        return read(i_root, to_string(i_path), to_string(i_name), o_value, i_valueSize, i_defaultValue, i_defaultValueSize);
    }
    /// write binary data
    static bool write(HKEY i_root, const std::string &i_path, const std::string &i_name,
                      const BYTE *i_value, DWORD i_valueSize);
    static bool write(HKEY i_root, const tstring &i_path, const tstring &i_name,
                      const BYTE *i_value, DWORD i_valueSize) {
        return write(i_root, to_string(i_path), to_string(i_name), i_value, i_valueSize);
    }
    /// read LOGFONT
    static bool read(HKEY i_root, const std::string &i_path, const std::string &i_name,
                     LOGFONT *o_value, const std::string &i_defaultStringValue);
    /// write LOGFONT
    static bool write(HKEY i_root, const std::string &i_path, const std::string &i_name,
                      const LOGFONT &i_value);

    // Legacy static helpers
    static bool read(HKEY i_root, const tstring &i_path, const tstring &i_name,
                     LOGFONT *o_value, const tstring &i_defaultStringValue) {
        return read(i_root, to_string(i_path), to_string(i_name), o_value, to_string(i_defaultStringValue));
    }
    static bool write(HKEY i_root, const tstring &i_path, const tstring &i_name,
                      const LOGFONT &i_value) {
        return write(i_root, to_string(i_path), to_string(i_name), i_value);
    }
};


#endif // !_REGISTRY_H
