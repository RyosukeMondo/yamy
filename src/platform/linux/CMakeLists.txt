# Linux platform implementation for YAMY

cmake_minimum_required(VERSION 3.15)

# Find required libraries
find_package(X11 REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(XRANDR REQUIRED xrandr)
pkg_check_modules(UDEV REQUIRED libudev)

# Source files for Linux platform
# Each track will add its implementation file
set(LINUX_PLATFORM_SOURCES
    # Main delegator (to be created)
    # window_system_linux_main.cpp

    # Track 1: Window Queries
    # window_system_linux_queries.cpp

    # Track 2: Window Manipulation
    # window_system_linux_manipulation.cpp

    # Track 3: Window Hierarchy
    # window_system_linux_hierarchy.cpp

    # Track 4: Mouse & Cursor
    # window_system_linux_mouse.cpp

    # Track 5: Monitor Support
    # window_system_linux_monitor.cpp

    # Track 6: POSIX Synchronization
    sync_linux.cpp

    # Track 7: Unix IPC
    # ipc_linux.cpp

    # Track 8: Hook Data
    # hook_data_linux.cpp

    # Track 9: evdev Input Capture
    # input_hook_linux.cpp
    # device_manager_linux.cpp

    # Track 10: uinput Injection
    # input_injector_linux.cpp

    # Track 11: Key Code Mapping
    # keycode_mapping.cpp

    # Track 12: Thread Management
    # thread_linux.cpp

    # Existing files (keep as-is for now)
    window_system_linux.cpp
)

# Create Linux platform library
add_library(yamy_platform_linux STATIC ${LINUX_PLATFORM_SOURCES})

# Include directories
target_include_directories(yamy_platform_linux PUBLIC
    ${CMAKE_SOURCE_DIR}/src
    ${X11_INCLUDE_DIR}
    ${XRANDR_INCLUDE_DIRS}
    ${UDEV_INCLUDE_DIRS}
)

# Link libraries
target_link_libraries(yamy_platform_linux PUBLIC
    ${X11_LIBRARIES}
    ${XRANDR_LIBRARIES}
    ${UDEV_LIBRARIES}
    pthread
)

# Compiler settings
target_compile_options(yamy_platform_linux PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    $<$<CONFIG:Debug>:-g -O0>
    $<$<CONFIG:Release>:-O3>
)

# Export for parent scope
set(YAMY_PLATFORM_LIBRARY yamy_platform_linux PARENT_SCOPE)

message(STATUS "Linux platform library configured")
message(STATUS "  X11: ${X11_LIBRARIES}")
message(STATUS "  XRandR: ${XRANDR_LIBRARIES}")
message(STATUS "  udev: ${UDEV_LIBRARIES}")
