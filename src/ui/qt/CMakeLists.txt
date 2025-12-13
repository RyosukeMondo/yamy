# Qt5 GUI for Linux
cmake_minimum_required(VERSION 3.15)

# Find Qt5 packages
find_package(Qt5 REQUIRED COMPONENTS
    Core
    Widgets
    Gui
)

if(UNIX AND NOT WIN32)
    find_package(Qt5 REQUIRED COMPONENTS X11Extras)
    # Find X11 for global hotkeys
    find_package(X11 REQUIRED)
endif()

# Enable Qt MOC (Meta-Object Compiler)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Qt source files
set(QT_GUI_SOURCES
    tray_icon_qt.cpp
    dialog_settings_qt.cpp
    dialog_log_qt.cpp
    log_stats_panel.cpp
    dialog_about_qt.cpp
    dialog_investigate_qt.cpp
    dialog_condition_generator_qt.cpp
    dialog_shortcuts_qt.cpp
    dialog_examples_qt.cpp
    config_search_panel.cpp
    status_widget_qt.cpp
    config_manager_dialog.cpp
    config_metadata_dialog.cpp
    crosshair_widget_qt.cpp
    global_hotkey.cpp
    notification_history.cpp
    notification_sound.cpp
    notification_prefs.cpp
    preferences_dialog.cpp
    crash_report_dialog.cpp
    ${CMAKE_SOURCE_DIR}/src/core/settings/config_watcher.cpp
    ${CMAKE_SOURCE_DIR}/src/core/platform/ipc_channel_interface.cpp
)

# Qt header files
set(QT_GUI_HEADERS
    tray_icon_qt.h
    dialog_settings_qt.h
    dialog_log_qt.h
    log_stats_panel.h
    dialog_about_qt.h
    dialog_investigate_qt.h
    dialog_condition_generator_qt.h
    dialog_shortcuts_qt.h
    dialog_examples_qt.h
    config_search_panel.h
    status_widget_qt.h
    config_manager_dialog.h
    config_metadata_dialog.h
    crosshair_widget_qt.h
    global_hotkey.h
    notification_history.h
    notification_sound.h
    notification_prefs.h
    preferences_dialog.h
    crash_report_dialog.h
    ${CMAKE_SOURCE_DIR}/src/core/settings/config_watcher.h
    ${CMAKE_SOURCE_DIR}/src/core/platform/ipc_channel_interface.h
)

# Qt UI files (Designer) - Not used, using code-based layouts
set(QT_GUI_UI_FILES
    # UI files not used - dialogs created programmatically
)

# Qt resources
set(QT_GUI_RESOURCES
    resources.qrc
    ${CMAKE_SOURCE_DIR}/src/core/audio/resources.qrc
)

# Create library for Qt GUI
add_library(yamy_qt_gui STATIC
    ${QT_GUI_SOURCES}
    ${QT_GUI_HEADERS}
    ${QT_GUI_UI_FILES}
    ${QT_GUI_RESOURCES}
)

# Link Qt libraries and X11
target_link_libraries(yamy_qt_gui PUBLIC
    Qt5::Core
    Qt5::Widgets
    Qt5::Gui
)

if(UNIX AND NOT WIN32)
    target_link_libraries(yamy_qt_gui PUBLIC
        Qt5::X11Extras
        ${X11_LIBRARIES}
        xcb
    )
endif()

# Optionally link Qt Multimedia if available
if(Qt5Multimedia_FOUND)
    target_link_libraries(yamy_qt_gui PUBLIC Qt5::Multimedia)
    target_compile_definitions(yamy_qt_gui PRIVATE QT_MULTIMEDIA_AVAILABLE)
endif()

# Include directories
target_include_directories(yamy_qt_gui PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/core/engine
    ${CMAKE_SOURCE_DIR}/src/core/settings
    ${CMAKE_SOURCE_DIR}/src/core/input
    ${CMAKE_SOURCE_DIR}/src/core/window
    ${CMAKE_SOURCE_DIR}/src/core/functions
    ${CMAKE_SOURCE_DIR}/src/core/commands
    ${CMAKE_SOURCE_DIR}/src/core/platform
    ${CMAKE_SOURCE_DIR}/src/platform/linux
    ${CMAKE_SOURCE_DIR}/src/utils
    ${CMAKE_SOURCE_DIR}/src/app
)

# Export for parent CMakeLists
set(YAMY_QT_GUI_LIBRARY yamy_qt_gui PARENT_SCOPE)

message(STATUS "Qt5 GUI configured for Linux")
message(STATUS "  Qt5Widgets: ${Qt5Widgets_VERSION}")
