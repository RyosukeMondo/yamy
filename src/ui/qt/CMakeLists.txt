# Qt5 GUI for Linux
cmake_minimum_required(VERSION 3.15)

# Find Qt5 packages
find_package(Qt5 REQUIRED COMPONENTS
    Core
    Widgets
    Gui
)

# Enable Qt MOC (Meta-Object Compiler)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Qt source files
set(QT_GUI_SOURCES
    tray_icon_qt.cpp
    dialog_settings_qt.cpp
    dialog_log_qt.cpp
    dialog_about_qt.cpp
    ${CMAKE_SOURCE_DIR}/src/core/settings/config_watcher.cpp
)

# Qt header files
set(QT_GUI_HEADERS
    tray_icon_qt.h
    dialog_settings_qt.h
    dialog_log_qt.h
    dialog_about_qt.h
    ${CMAKE_SOURCE_DIR}/src/core/settings/config_watcher.h
)

# Qt UI files (Designer) - Not used, using code-based layouts
set(QT_GUI_UI_FILES
    # UI files not used - dialogs created programmatically
)

# Qt resources
set(QT_GUI_RESOURCES
    resources.qrc
)

# Create library for Qt GUI
add_library(yamy_qt_gui STATIC
    ${QT_GUI_SOURCES}
    ${QT_GUI_HEADERS}
    ${QT_GUI_UI_FILES}
    ${QT_GUI_RESOURCES}
)

# Link Qt libraries
target_link_libraries(yamy_qt_gui PUBLIC
    Qt5::Core
    Qt5::Widgets
    Qt5::Gui
)

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
