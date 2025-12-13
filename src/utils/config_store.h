#pragma once
#ifndef _CONFIG_STORE_H
#define _CONFIG_STORE_H

#include "misc.h"
#include <list>
#include <string>
#include <cstdint>

/// Abstract interface for configuration storage (Registry, Ini, etc.)
class ConfigStore
{
public:
    typedef std::list<std::string> strings;

    virtual ~ConfigStore() {}

    /// remove key or value
    virtual bool remove(const std::string &i_name = "") const = 0;

    /// check existence
    virtual bool doesExist() const = 0;

    /// read integer
    virtual bool read(const std::string &i_name, int *o_value, int i_defaultValue = 0) const = 0;

    /// write integer
    virtual bool write(const std::string &i_name, int i_value) const = 0;

    /// read string
    virtual bool read(const std::string &i_name, std::string *o_value,
                      const std::string &i_defaultValue = "") const = 0;

    /// write string
    virtual bool write(const std::string &i_name, const std::string &i_value) const = 0;

#ifndef USE_INI
    /// read list of strings
    virtual bool read(const std::string &i_name, strings *o_value,
                      const strings &i_defaultValue = strings()) const = 0;

    /// write list of strings
    virtual bool write(const std::string &i_name, const strings &i_value) const = 0;
#endif //!USE_INI

    /// read binary data
    virtual bool read(const std::string &i_name, uint8_t *o_value, uint32_t *i_valueSize,
                      const uint8_t *i_defaultValue = nullptr, uint32_t i_defaultValueSize = 0) const = 0;

    /// write binary data
    virtual bool write(const std::string &i_name, const uint8_t *i_value,
                       uint32_t i_valueSize) const = 0;
};

#endif // _CONFIG_STORE_H
