#pragma once
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// config_watcher.h
// Watches configuration files for changes and triggers auto-reload

#ifndef _CONFIG_WATCHER_H
#define _CONFIG_WATCHER_H

#include <QObject>
#include <QFileSystemWatcher>
#include <QTimer>
#include <string>
#include <functional>

/// Callback type for config file changes
using ConfigFileChangedCallback = std::function<void(const std::string& configPath)>;

/// Watches active config file for changes and triggers auto-reload
/// Uses QFileSystemWatcher internally with debouncing to avoid excessive reloads
class ConfigWatcher : public QObject
{
    Q_OBJECT

public:
    /// Debounce delay in milliseconds (avoid multiple reloads on rapid saves)
    static constexpr int DEBOUNCE_DELAY_MS = 300;

    explicit ConfigWatcher(QObject* parent = nullptr);
    ~ConfigWatcher() override;

    /// Set the config file path to watch
    /// @param path Path to .mayu configuration file
    void setConfigPath(const std::string& path);

    /// Get currently watched config path
    std::string configPath() const { return m_configPath; }

    /// Start watching the config file
    void start();

    /// Stop watching the config file
    void stop();

    /// Check if watcher is active
    bool isWatching() const { return m_watching; }

    /// Set callback for config file changes
    void setChangeCallback(ConfigFileChangedCallback callback);

    /// Enable/disable auto-reload on file changes
    void setAutoReloadEnabled(bool enabled);
    bool isAutoReloadEnabled() const { return m_autoReloadEnabled; }

signals:
    /// Emitted when config file has changed (after debounce)
    void configFileChanged(const QString& path);

    /// Emitted when config file was deleted
    void configFileDeleted(const QString& path);

    /// Emitted when watched file is restored after deletion
    void configFileRestored(const QString& path);

private slots:
    /// Handle file changed signal from QFileSystemWatcher
    void onFileChanged(const QString& path);

    /// Handle directory changed signal (for detecting file restoration)
    void onDirectoryChanged(const QString& path);

    /// Handle debounce timer timeout
    void onDebounceTimeout();

private:
    /// Re-add watch on file (needed after some editors save by delete+create)
    void readdWatch();

    /// Check if the watched file exists
    bool fileExists() const;

    QFileSystemWatcher* m_watcher;
    QTimer* m_debounceTimer;
    std::string m_configPath;
    bool m_watching;
    bool m_autoReloadEnabled;
    bool m_fileExisted;  /// Track file existence for deletion detection
    ConfigFileChangedCallback m_changeCallback;
};

#endif // !_CONFIG_WATCHER_H
