#pragma once

#ifndef _QSETTINGS_CONFIG_STORE_H
#define _QSETTINGS_CONFIG_STORE_H

#include "config_store.h"
#include <QSettings>
#include <QString>
#include <QStringList>
#include <memory>
#include <algorithm>

/**
 * @brief ConfigStore implementation using QSettings.
 * Useful for cross-platform persistence in Qt-based applications.
 */
class QSettingsConfigStore : public ConfigStore {
public:
    QSettingsConfigStore(const QString& organization, const QString& application)
        : m_settings(new QSettings(organization, application)) {}

    virtual ~QSettingsConfigStore() {}

    bool remove(const std::string &i_name = "") const override {
        if (i_name.empty()) {
            m_settings->clear();
            return true;
        }
        m_settings->remove(QString::fromStdString(i_name));
        return true;
    }

    bool doesExist() const override {
        // QSettings always "exists" in a way, or creates on write.
        return true;
    }

    bool read(const std::string &i_name, int *o_value, int i_defaultValue = 0) const override {
        if (!o_value) return false;
        QVariant val = m_settings->value(QString::fromStdString(i_name), i_defaultValue);
        bool ok = false;
        int result = val.toInt(&ok);
        if (ok) {
            *o_value = result;
            return true;
        }
        *o_value = i_defaultValue;
        return false;
    }

    bool write(const std::string &i_name, int i_value) const override {
        m_settings->setValue(QString::fromStdString(i_name), i_value);
        return true;
    }

    bool read(const std::string &i_name, std::string *o_value,
              const std::string &i_defaultValue = "") const override {
        if (!o_value) return false;
        QString val = m_settings->value(QString::fromStdString(i_name), QString::fromStdString(i_defaultValue)).toString();
        *o_value = val.toStdString();
        return true;
    }

    bool write(const std::string &i_name, const std::string &i_value) const override {
        m_settings->setValue(QString::fromStdString(i_name), QString::fromStdString(i_value));
        return true;
    }

#ifndef USE_INI
    bool read(const std::string &i_name, strings *o_value,
              const strings &i_defaultValue = strings()) const override {
        if (!o_value) return false;
        
        // QSettings stores lists as QVariantList or QStringList
        QVariant val = m_settings->value(QString::fromStdString(i_name));
        if (val.isValid() && val.canConvert<QStringList>()) {
            QStringList list = val.toStringList();
            o_value->clear();
            for (const QString& str : list) {
                o_value->push_back(str.toStdString());
            }
            return true;
        }
        
        *o_value = i_defaultValue;
        return false;
    }

    bool write(const std::string &i_name, const strings &i_value) const override {
        QStringList list;
        for (const std::string& str : i_value) {
            list.append(QString::fromStdString(str));
        }
        m_settings->setValue(QString::fromStdString(i_name), list);
        return true;
    }
#endif //!USE_INI

    bool read(const std::string &i_name, uint8_t *o_value, uint32_t *i_valueSize,
              const uint8_t *i_defaultValue = nullptr, uint32_t i_defaultValueSize = 0) const override {
        // Binary data usually stored as QByteArray
        QVariant val = m_settings->value(QString::fromStdString(i_name));
        QByteArray data;
        
        if (val.isValid() && val.canConvert<QByteArray>()) {
            data = val.toByteArray();
        } else if (i_defaultValue) {
             data = QByteArray(reinterpret_cast<const char*>(i_defaultValue), i_defaultValueSize);
        } else {
            return false;
        }

        if (o_value && i_valueSize) {
            uint32_t copySize = std::min<uint32_t>(*i_valueSize, static_cast<uint32_t>(data.size()));
            std::memcpy(o_value, data.constData(), copySize);
            *i_valueSize = copySize;
            return true;
        }
        return false;
    }

    bool write(const std::string &i_name, const uint8_t *i_value,
               uint32_t i_valueSize) const override {
        QByteArray data(reinterpret_cast<const char*>(i_value), i_valueSize);
        m_settings->setValue(QString::fromStdString(i_name), data);
        return true;
    }

private:
    std::unique_ptr<QSettings> m_settings;
};

#endif // _QSETTINGS_CONFIG_STORE_H
