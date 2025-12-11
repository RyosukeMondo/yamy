$prompts = @{}

# --- Track 2: Configuration ---

$prompts["prompt_wave2_2a.txt"] = @'
Role: Senior C++ Developer
Task: Implement ConfigManager Logic
Files: src/core/settings/config_manager.h, src/core/settings/config_manager.cpp
Context:
We need a singleton class `ConfigManager` to manage the list of available `.mayu` configuration files in `~/.yamy/`.
The class should NOT depend on GUI classes.

Requirements:
1.  **Singleton Pattern**: Implement `ConfigManager& instance()`.
2.  **Storage**: Use `std::vector<std::string>` to store paths to config files.
3.  **Methods**:
    *   `void refreshList()`: Scan `~/.yamy/` for `.mayu` files.
    *   `std::vector<std::string> listConfigs()`: Return the list.
    *   `std::string getActiveConfig()`: Return path of currently active config.
    *   `void setActiveConfig(const std::string& path)`: Update active config state.
    *   `bool addConfig(const std::string& path)`: Copy a file into the config dir.
    *   `bool removeConfig(const std::string& path)`: Delete a config file.
4.  **Persistence**: Use `ConfigStore` (existing) to save the index of the active config.

Constraint:
*   Create NEW FILES only.
*   Do NOT edit `engine.cpp` yet.
*   Use `std::filesystem` for file operations.
'@

$prompts["prompt_wave2_2b.txt"] = @'
Role: Senior C++ Developer
Task: Implement ConfigValidator
Files: src/core/parser/config_validator.h, src/core/parser/config_validator.cpp
Context:
We need a standalone class to validate the syntax of `.mayu` files without loading them into the main engine.

Requirements:
1.  **Class**: `ConfigValidator`.
2.  **Struct**: `ValidationError` { int line; string message; Level level; }.
3.  **Method**: `std::vector<ValidationError> validate(const std::string& path)`.
    *   Read the file line by line.
    *   Check for basic syntax errors (e.g., matching parentheses, valid keywords like `include`, `keymap`, `window`).
    *   This does NOT need to be a full parser, just a linter.
    *   Check for circular `include` references.
4.  **Performance**: Must be fast (<100ms).

Constraint:
*   Create NEW FILES only.
*   Do NOT depend on `Engine`.
'@

$prompts["prompt_wave2_2c.txt"] = @'
Role: Senior C++ Developer
Task: Implement ConfigWatcher
Files: src/core/settings/config_watcher.h, src/core/settings/config_watcher.cpp
Context:
We need a class to watch the active configuration file for changes and trigger a reload.

Requirements:
1.  **Class**: `ConfigWatcher` (inherits from `QObject` if using Qt, or use standard filesystem watcher). Let's use `QFileSystemWatcher` since we are linking Qt.
2.  **Signals**: `void fileChanged(const QString& path)`.
3.  **Methods**:
    *   `void setWatchPath(const QString& path)`.
    *   `void start()`, `void stop()`.
4.  **Debouncing**: Implement a 300ms debounce timer to avoid triggering multiple times for one save.

Constraint:
*   Create NEW FILES only.
*   This class handles the *watching*, not the *reloading*.
'@

$prompts["prompt_wave2_2d.txt"] = @'
Role: Senior C++ Developer
Task: Implement ConfigBackup Logic
Files: src/core/settings/config_backup.h, src/core/settings/config_backup.cpp
Context:
We need a helper class to manage backing up configuration files.

Requirements:
1.  **Class**: `ConfigBackup`.
2.  **Methods**:
    *   `bool createBackup(const std::string& configPath)`: Copy file to `.bak.<timestamp>`.
    *   `std::vector<std::string> listBackups(const std::string& configPath)`.
    *   `bool restoreBackup(const std::string& backupPath)`.
    *   `void pruneBackups(const std::string& configPath, int maxCount)`: Keep only last N backups.
3.  **Pathing**: Store backups in a hidden `.backups` subdirectory.

Constraint:
*   Create NEW FILES only.
*   Use `std::filesystem`.
'@

$prompts["prompt_wave2_2e.txt"] = @'
Role: Senior C++ Developer
Task: Implement ConfigMetadata Storage
Files: src/core/settings/config_metadata.h, src/core/settings/config_metadata.cpp
Context:
We need to store metadata (Author, Description, Tags) for each configuration file, separate from the content.

Requirements:
1.  **Class**: `ConfigMetadata`.
2.  **Fields**: Name, Description, Author, CreatedDate, ModifiedDate, Tags (vector<string>).
3.  **Storage**: JSON format in a `.meta` file (e.g., `default.mayu.meta`).
4.  **Methods**:
    *   `bool load(const std::string& configPath)`.
    *   `bool save(const std::string& configPath)`.
    *   `void updateModifiedDate()`.

Constraint:
*   Create NEW FILES only.
*   Use a simple JSON parser or manual JSON formatting (avoid heavy deps if possible, or use QJsonDocument).
'@

$prompts["prompt_wave2_2f.txt"] = @'
Role: Senior C++ Developer
Task: Implement ConfigTemplate Manager
Files: src/resources/templates/template_manager.h, src/resources/templates/template_manager.cpp
Context:
We need to generate new config files from templates.

Requirements:
1.  **Class**: `TemplateManager`.
2.  **Templates**: Embed 3 internal string templates:
    *   `Default`: Basic remapping.
    *   `Emacs`: Emacs-like bindings.
    *   `Vim`: Vim-like bindings.
3.  **Methods**:
    *   `std::vector<std::string> listTemplates()`.
    *   `bool createFromTemplate(const std::string& templateName, const std::string& targetPath)`.

Constraint:
*   Create NEW FILES only.
*   Hardcode the template strings in the .cpp file for now.
'@

$prompts["prompt_wave2_2g.txt"] = @'
Role: Qt GUI Developer
Task: Implement ConfigManagerDialog
Files: src/ui/qt/config_manager_dialog.h, src/ui/qt/config_manager_dialog.cpp
Context:
The main GUI for managing configurations.

Requirements:
1.  **Class**: `ConfigManagerDialog` (inherits `QDialog`).
2.  **UI Elements**:
    *   `QListWidget` for the list of configs.
    *   Buttons: New, Delete, Rename, Set Active, Edit.
3.  **Logic**:
    *   On construction, populate list (mock this or assume `ConfigManager` exists via header, but prioritize UI layout).
    *   Selection handling: Enable buttons only when item selected.
    *   Double-click: Open editor.
4.  **Layout**: Use `QVBoxLayout` and `QHBoxLayout`.

Constraint:
*   Create NEW FILES only.
*   You can include `config_manager.h` (assume it exists).
'@

$prompts["prompt_wave2_2h.txt"] = @'
Role: Qt GUI Developer
Task: Implement ConfigMetadataDialog
Files: src/ui/qt/config_metadata_dialog.h, src/ui/qt/config_metadata_dialog.cpp
Context:
A dialog to edit the metadata (Name, Author, etc.) of a config.

Requirements:
1.  **Class**: `ConfigMetadataDialog` (inherits `QDialog`).
2.  **Fields**:
    *   Name (`QLineEdit`).
    *   Description (`QTextEdit`).
    *   Author (`QLineEdit`).
    *   Tags (`QLineEdit` - comma separated).
3.  **Buttons**: Save, Cancel.
4.  **Methods**:
    *   `setMetadata(...)`: Populate fields.
    *   `getMetadata()`: Return updated values.

Constraint:
*   Create NEW FILES only.
'@

$prompts["prompt_wave2_2i.txt"] = @'
Role: Qt GUI Developer
Task: Implement ConfigSearchPanel
Files: src/ui/qt/config_search_panel.h, src/ui/qt/config_search_panel.cpp
Context:
A widget to search/filter the configuration list.

Requirements:
1.  **Class**: `ConfigSearchPanel` (inherits `QWidget`).
2.  **UI Elements**:
    *   `QLineEdit` for search text.
    *   `QComboBox` for filter type (Name, Tag, Status).
3.  **Signals**:
    *   `filterChanged(QString text, FilterType type)`.
4.  **Layout**: Horizontal layout.

Constraint:
*   Create NEW FILES only.
'@

# --- Track 4: Log UI ---

$prompts["prompt_wave2_4a.txt"] = @'
Role: Qt GUI Developer
Task: Implement DialogLogQt Skeleton
Files: src/ui/qt/dialog_log_qt.h, src/ui/qt/dialog_log_qt.cpp
Context:
The main Log Viewer window.

Requirements:
1.  **Class**: `DialogLogQt` (inherits `QDialog`).
2.  **UI Elements**:
    *   Top Bar: Placeholder for Controls.
    *   Center: `QTextEdit` (Read-only) for logs.
    *   Bottom: Placeholder for Stats.
3.  **Methods**:
    *   `appendLog(const QString& msg)`: Thread-safe slot.
    *   `clearLog()`.

Constraint:
*   Create NEW FILES only.
*   Focus on the container structure.
'@

$prompts["prompt_wave2_4b.txt"] = @'
Role: Qt GUI Developer
Task: Implement LogSyntaxHighlighter
Files: src/ui/qt/log_syntax_highlighter.h, src/ui/qt/log_syntax_highlighter.cpp
Context:
Syntax highlighter for the log viewer.

Requirements:
1.  **Class**: `LogSyntaxHighlighter` (inherits `QSyntaxHighlighter`).
2.  **Rules**:
    *   "ERROR" / "Error" -> Red.
    *   "WARNING" / "Warning" -> Orange/Yellow.
    *   "INFO" -> Blue.
    *   Keywords "DOWN", "UP" -> Bold.
3.  **Input**: Works on a `QTextDocument`.

Constraint:
*   Create NEW FILES only.
'@

$prompts["prompt_wave2_4c.txt"] = @'
Role: Qt GUI Developer
Task: Implement LogFontControl
Files: src/ui/qt/log_font_control.h, src/ui/qt/log_font_control.cpp
Context:
A widget to control the font size/family of the log viewer.

Requirements:
1.  **Class**: `LogFontControl` (inherits `QWidget`).
2.  **UI Elements**:
    *   `QFontComboBox`.
    *   `QSpinBox` (Size 6-24).
3.  **Signals**:
    *   `fontChanged(const QFont& font)`.

Constraint:
*   Create NEW FILES only.
'@

$prompts["prompt_wave2_4d.txt"] = @'
Role: Qt GUI Developer
Task: Implement LogSearchWidget
Files: src/ui/qt/log_search_widget.h, src/ui/qt/log_search_widget.cpp
Context:
A widget to search text within the log viewer.

Requirements:
1.  **Class**: `LogSearchWidget` (inherits `QWidget`).
2.  **UI Elements**:
    *   `QLineEdit` (Search text).
    *   Buttons: "Next", "Previous".
    *   Start simple.
3.  **Signals**:
    *   `searchNext(QString text)`.
    *   `searchPrev(QString text)`.

Constraint:
*   Create NEW FILES only.
'@

$prompts["prompt_wave2_4e.txt"] = @'
Role: Qt GUI Developer
Task: Implement LogStatsPanel
Files: src/ui/qt/log_stats_panel.h, src/ui/qt/log_stats_panel.cpp
Context:
A panel showing log statistics.

Requirements:
1.  **Class**: `LogStatsPanel` (inherits `QWidget`).
2.  **Fields**:
    *   Labels for: Error Count, Warning Count, Total Lines.
3.  **Methods**:
    *   `incrementError()`.
    *   `incrementWarning()`.
    *   `setTotalLines(int count)`.
    *   `reset()`.

Constraint:
*   Create NEW FILES only.
'@

# --- Track 5: Notifications ---

$prompts["prompt_wave2_5a.txt"] = @'
Role: Qt GUI Developer
Task: Implement NotificationHistory Dialog
Files: src/ui/qt/notification_history.h, src/ui/qt/notification_history.cpp
Context:
A dialog showing a history of recent notifications.

Requirements:
1.  **Class**: `NotificationHistory` (inherits `QDialog`).
2.  **UI**: `QListWidget` or `QTableWidget` list of events.
3.  **Data**:
    *   Timestamp, Title, Message.
4.  **Methods**:
    *   `addEntry(const QString& title, const QString& message)`.
    *   `clear()`.

Constraint:
*   Create NEW FILES only.
'@

$prompts["prompt_wave2_5b.txt"] = @'
Role: Qt GUI Developer
Task: Implement StatusWidgetQt
Files: src/ui/qt/status_widget_qt.h, src/ui/qt/status_widget_qt.cpp
Context:
A dashboard widget to be embedded in the main window or settings dialog.

Requirements:
1.  **Class**: `StatusWidgetQt` (inherits `QWidget`).
2.  **UI Elements**: Labels for:
    *   Engine Status (Stopped/Running).
    *   Current Config Name.
    *   Uptime.
    *   Keys Processed.
3.  **Methods**:
    *   `setStatus(EngineState state)`.
    *   `setConfigName(QString name)`.
    *   `updateUptime(int seconds)`.
    *   `setKeyCount(int count)`.

Constraint:
*   Create NEW FILES only.
'@

$prompts["prompt_wave2_5c.txt"] = @'
Role: Senior C++ Developer
Task: Implement SoundManager
Files: src/core/audio/sound_manager.h, src/core/audio/sound_manager.cpp
Context:
Manage and play notification sounds.

Requirements:
1.  **Class**: `SoundManager`.
2.  **Dependencies**: Use `QSoundEffect` or `QMediaPlayer` (add module `multimedia` to Qt requirements later if needed, assume available for now).
3.  **Methods**:
    *   `playSound(NotificationType type)`.
    *   `setVolume(int percent)`.
    *   `setEnabled(bool enabled)`.

Constraint:
*   Create NEW FILES only.
'@

# --- Write All Files ---
foreach ($key in $prompts.Keys) {
    Set-Content -Path $key -Value $prompts[$key] -Encoding UTF8
    Write-Host "Created $key"
}
