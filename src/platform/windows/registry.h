#pragma once
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// registry.h


#ifndef _REGISTRY_H
#  define _REGISTRY_H

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

    /// set registry root and path
    void setRoot(HKEY i_root, const std::string &i_path);

    /// remove
    bool remove(const std::string &i_name = "") const override {
        return remove(m_root, m_path, i_name);
    }

    /// does exist the key ?
    bool doesExist() const override {
        return doesExist(m_root, m_path);
    }

    /// read DWORD
    bool read(const std::string &i_name, int *o_value, int i_defaultValue = 0)
    const override {
        return read(m_root, m_path, i_name, o_value, i_defaultValue);
    }

    /// write DWORD
    bool write(const std::string &i_name, int i_value) const override {
        return write(m_root, m_path, i_name, i_value);
    }

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
    bool read(const std::string &i_name, uint8_t *o_value, uint32_t *i_valueSize,
              const uint8_t *i_defaultValue = nullptr, uint32_t i_defaultValueSize = 0)
    const override {
        return read(m_root, m_path, i_name, reinterpret_cast<BYTE*>(o_value),
                    reinterpret_cast<DWORD*>(i_valueSize),
                    reinterpret_cast<const BYTE*>(i_defaultValue),
                    static_cast<DWORD>(i_defaultValueSize));
    }
    /// write binary data
    bool write(const std::string &i_name, const uint8_t *i_value,
               uint32_t i_valueSize) const override {
        return write(m_root, m_path, i_name, reinterpret_cast<const BYTE*>(i_value),
                     static_cast<DWORD>(i_valueSize));
    }

public:
    /// remove
    static bool remove(HKEY i_root, const std::string &i_path,
                       const std::string &i_name = "");

    /// does exist the key ?
    static bool doesExist(HKEY i_root, const std::string &i_path);

    /// read DWORD
    static bool read(HKEY i_root, const std::string &i_path, const std::string &i_name,
                     int *o_value, int i_defaultValue = 0);
    /// write DWORD
    static bool write(HKEY i_root, const std::string &i_path, const std::string &i_name,
                      int i_value);

    /// read string
    static bool read(HKEY i_root, const std::string &i_path, const std::string &i_name,
                     std::string *o_value, const std::string &i_defaultValue = "");
    /// write string
    static bool write(HKEY i_root, const std::string &i_path, const std::string &i_name,
                      const std::string &i_value);

#ifndef USE_INI
    /// read list of string
    static bool read(HKEY i_root, const std::string &i_path, const std::string &i_name,
                     strings *o_value, const strings &i_defaultValue = strings());
    /// write list of string
    static bool write(HKEY i_root, const std::string &i_path, const std::string &i_name,
                      const strings &i_value);
#endif //!USE_INI

    /// read binary data
    static bool read(HKEY i_root, const std::string &i_path, const std::string &i_name,
                     BYTE *o_value, DWORD *i_valueSize,
                     const BYTE *i_defaultValue = nullptr,
                     DWORD i_defaultValueSize = 0);
    /// write binary data
    static bool write(HKEY i_root, const std::string &i_path, const std::string &i_name,
                      const BYTE *i_value, DWORD i_valueSize);
    /// read LOGFONT
    static bool read(HKEY i_root, const std::string &i_path, const std::string &i_name,
                     LOGFONT *o_value, const std::string &i_defaultStringValue);
    /// write LOGFONT
    static bool write(HKEY i_root, const std::string &i_path, const std::string &i_name,
                      const LOGFONT &i_value);
};


#endif // !_REGISTRY_H
