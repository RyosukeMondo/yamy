//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// config_watcher.cpp
// Implementation of config file watcher with debouncing

#include "config_watcher.h"
#include <QFileInfo>
#include <QDir>

ConfigWatcher::ConfigWatcher(QObject* parent)
    : QObject(parent)
    , m_watcher(new QFileSystemWatcher(this))
    , m_debounceTimer(new QTimer(this))
    , m_watching(false)
    , m_autoReloadEnabled(true)
    , m_fileExisted(false)
    , m_changeCallback(nullptr)
{
    m_debounceTimer->setSingleShot(true);
    m_debounceTimer->setInterval(DEBOUNCE_DELAY_MS);

    connect(m_watcher, &QFileSystemWatcher::fileChanged,
            this, &ConfigWatcher::onFileChanged);
    connect(m_watcher, &QFileSystemWatcher::directoryChanged,
            this, &ConfigWatcher::onDirectoryChanged);
    connect(m_debounceTimer, &QTimer::timeout,
            this, &ConfigWatcher::onDebounceTimeout);
}

ConfigWatcher::~ConfigWatcher()
{
    stop();
}

void ConfigWatcher::setConfigPath(const std::string& path)
{
    bool wasWatching = m_watching;
    if (wasWatching) {
        stop();
    }

    m_configPath = path;

    if (wasWatching && !path.empty()) {
        start();
    }
}

void ConfigWatcher::start()
{
    if (m_watching || m_configPath.empty()) {
        return;
    }

    QString qpath = QString::fromStdString(m_configPath);
    QFileInfo fileInfo(qpath);

    m_fileExisted = fileInfo.exists();

    if (m_fileExisted) {
        m_watcher->addPath(qpath);
    }

    // Watch parent directory to detect file creation/restoration
    QString parentDir = fileInfo.absolutePath();
    if (!parentDir.isEmpty() && QDir(parentDir).exists()) {
        m_watcher->addPath(parentDir);
    }

    m_watching = true;
}

void ConfigWatcher::stop()
{
    if (!m_watching) {
        return;
    }

    m_debounceTimer->stop();

    // Remove all watched paths
    QStringList files = m_watcher->files();
    if (!files.isEmpty()) {
        m_watcher->removePaths(files);
    }

    QStringList dirs = m_watcher->directories();
    if (!dirs.isEmpty()) {
        m_watcher->removePaths(dirs);
    }

    m_watching = false;
}

void ConfigWatcher::setChangeCallback(ConfigFileChangedCallback callback)
{
    m_changeCallback = callback;
}

void ConfigWatcher::setAutoReloadEnabled(bool enabled)
{
    m_autoReloadEnabled = enabled;
}

void ConfigWatcher::onFileChanged(const QString& path)
{
    if (!m_watching || !m_autoReloadEnabled) {
        return;
    }

    QString expectedPath = QString::fromStdString(m_configPath);
    if (path != expectedPath) {
        return;
    }

    // Check if file was deleted
    if (!fileExists()) {
        m_fileExisted = false;
        emit configFileDeleted(path);
        // Don't trigger reload for deleted files
        // Directory watcher will detect if file is recreated
        return;
    }

    // Some editors (vim, emacs) save by creating temp file and renaming
    // This removes the watch, so we need to re-add it
    readdWatch();

    // Start/restart debounce timer
    m_debounceTimer->start();
}

void ConfigWatcher::onDirectoryChanged(const QString& path)
{
    if (!m_watching) {
        return;
    }

    // Check if our watched file was restored/created
    QString expectedPath = QString::fromStdString(m_configPath);
    QFileInfo fileInfo(expectedPath);

    if (!m_fileExisted && fileInfo.exists()) {
        // File was restored after deletion
        m_fileExisted = true;

        // Re-add file watch
        m_watcher->addPath(expectedPath);

        emit configFileRestored(expectedPath);

        if (m_autoReloadEnabled) {
            // Start debounce timer for the restored file
            m_debounceTimer->start();
        }
    }
}

void ConfigWatcher::onDebounceTimeout()
{
    if (!m_watching || !m_autoReloadEnabled) {
        return;
    }

    if (!fileExists()) {
        // File was deleted during debounce period, skip reload
        return;
    }

    QString qpath = QString::fromStdString(m_configPath);
    emit configFileChanged(qpath);

    if (m_changeCallback) {
        m_changeCallback(m_configPath);
    }
}

void ConfigWatcher::readdWatch()
{
    QString qpath = QString::fromStdString(m_configPath);

    // Check if path is still being watched
    if (!m_watcher->files().contains(qpath)) {
        if (QFileInfo::exists(qpath)) {
            m_watcher->addPath(qpath);
            m_fileExisted = true;
        }
    }
}

bool ConfigWatcher::fileExists() const
{
    return QFileInfo::exists(QString::fromStdString(m_configPath));
}
