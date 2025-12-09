#pragma once
#ifndef _CONFIG_STORE_H
#define _CONFIG_STORE_H

#include "stringtool.h"
#include "misc.h"
#include <list>
#include <string>

/// Abstract interface for configuration storage (Registry, Ini, etc.)
class ConfigStore
{
public:
    typedef std::list<tstring> tstrings; // Legacy
    typedef std::list<std::string> strings;

    virtual ~ConfigStore() {}

    /// remove key or value
    virtual bool remove(const std::string &i_name = "") const = 0;
    // Legacy support
    virtual bool remove(const tstring &i_name) const {
        return remove(to_string(i_name));
    }

    /// check existence
    virtual bool doesExist() const = 0;

    /// read integer
    virtual bool read(const std::string &i_name, int *o_value, int i_defaultValue = 0) const = 0;
    // Legacy support
    virtual bool read(const tstring &i_name, int *o_value, int i_defaultValue = 0) const {
        return read(to_string(i_name), o_value, i_defaultValue);
    }
    
    /// write integer
    virtual bool write(const std::string &i_name, int i_value) const = 0;
    // Legacy support
    virtual bool write(const tstring &i_name, int i_value) const {
        return write(to_string(i_name), i_value);
    }

    /// read string
    virtual bool read(const std::string &i_name, std::string *o_value,
                      const std::string &i_defaultValue = "") const = 0;
    // Legacy support
    virtual bool read(const tstring &i_name, tstring *o_value,
                      const tstring &i_defaultValue = _T("")) const {
        std::string val, def = to_string(i_defaultValue);
        if (read(to_string(i_name), &val, def)) {
            *o_value = to_tstring(val);
            return true;
        }
        *o_value = i_defaultValue;
        return false;
    }
    
    /// write string
    virtual bool write(const std::string &i_name, const std::string &i_value) const = 0;
    // Legacy support
    virtual bool write(const tstring &i_name, const tstring &i_value) const {
        return write(to_string(i_name), to_string(i_value));
    }

#ifndef USE_INI
    /// read list of strings
    virtual bool read(const std::string &i_name, strings *o_value,
                      const strings &i_defaultValue = strings()) const = 0;
    // Legacy support
    virtual bool read(const tstring &i_name, tstrings *o_value,
                      const tstrings &i_defaultValue = tstrings()) const {
        strings val, def;
        for (const auto& s : i_defaultValue) def.push_back(to_string(s));

        if (read(to_string(i_name), &val, def)) {
            o_value->clear();
            for (const auto& s : val) o_value->push_back(to_tstring(s));
            return true;
        }
        *o_value = i_defaultValue;
        return false;
    }
    
    /// write list of strings
    virtual bool write(const std::string &i_name, const strings &i_value) const = 0;
    // Legacy support
    virtual bool write(const tstring &i_name, const tstrings &i_value) const {
        strings val;
        for (const auto& s : i_value) val.push_back(to_string(s));
        return write(to_string(i_name), val);
    }
#endif //!USE_INI

    /// read binary data
    virtual bool read(const std::string &i_name, BYTE *o_value, DWORD *i_valueSize,
                      const BYTE *i_defaultValue = nullptr, DWORD i_defaultValueSize = 0) const = 0;
    // Legacy support
    virtual bool read(const tstring &i_name, BYTE *o_value, DWORD *i_valueSize,
                      const BYTE *i_defaultValue = nullptr, DWORD i_defaultValueSize = 0) const {
        return read(to_string(i_name), o_value, i_valueSize, i_defaultValue, i_defaultValueSize);
    }
    
    /// write binary data
    virtual bool write(const std::string &i_name, const BYTE *i_value,
                       DWORD i_valueSize) const = 0;
    // Legacy support
    virtual bool write(const tstring &i_name, const BYTE *i_value,
                       DWORD i_valueSize) const {
        return write(to_string(i_name), i_value, i_valueSize);
    }
};

#endif // _CONFIG_STORE_H
