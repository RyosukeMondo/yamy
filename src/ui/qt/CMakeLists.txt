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
    # tray_menu_qt.cpp           # Phase 4
    # dialog_settings_qt.cpp     # Phase 5
    # dialog_log_qt.cpp          # Phase 5
    # dialog_about_qt.cpp        # Phase 5
)

# Qt header files
set(QT_GUI_HEADERS
    tray_icon_qt.h
    # tray_menu_qt.h             # Phase 4
    # dialog_settings_qt.h       # Phase 5
    # dialog_log_qt.h            # Phase 5
    # dialog_about_qt.h          # Phase 5
)

# Qt UI files (Designer)
set(QT_GUI_UI_FILES
    # dialog_settings_qt.ui      # Phase 5
    # dialog_log_qt.ui           # Phase 5
    # dialog_about_qt.ui         # Phase 5
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
)

# Export for parent CMakeLists
set(YAMY_QT_GUI_LIBRARY yamy_qt_gui PARENT_SCOPE)

message(STATUS "Qt5 GUI configured for Linux")
message(STATUS "  Qt5Widgets: ${Qt5Widgets_VERSION}")
