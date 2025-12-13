# CMakeLists.txt for Yamy Example Plugin
#
# This demonstrates how to build a Yamy plugin as a shared library.
# You can use this as a template for your own plugins.
#
# Build instructions:
#   mkdir build && cd build
#   cmake ..
#   make
#
# Install:
#   cp libexample_plugin.so ~/.local/share/yamy/plugins/

cmake_minimum_required(VERSION 3.16)
project(yamy_example_plugin VERSION 1.0.0 LANGUAGES CXX)

# C++17 is required for Yamy plugins
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find the Yamy source directory (parent of examples)
# In a real installed setup, you'd use find_package() instead
get_filename_component(YAMY_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)

# Include Yamy headers
include_directories(${YAMY_SOURCE_DIR}/src)

# Create the plugin as a shared library
add_library(example_plugin SHARED
    example_plugin.cpp
)

# Plugin-specific compile options
target_compile_options(example_plugin PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

# Remove 'lib' prefix for cleaner plugin names (optional)
# This makes the output "example_plugin.so" instead of "libexample_plugin.so"
set_target_properties(example_plugin PROPERTIES
    PREFIX ""
    # Ensure symbols are exported for dlsym
    POSITION_INDEPENDENT_CODE ON
)

# If building standalone (not as part of main Yamy build),
# we need to link against the notification dispatcher
# In the main build, this is handled automatically
if(NOT TARGET yamy)
    # When building standalone, link against Yamy's stub library
    # or provide the necessary object files
    message(STATUS "Building plugin standalone - ensure Yamy headers are available")
endif()

# Installation target
# Installs to the standard Yamy plugins directory
install(TARGETS example_plugin
    LIBRARY DESTINATION "${CMAKE_INSTALL_PREFIX}/share/yamy/plugins"
)

# Custom target to install to user's plugins directory
add_custom_target(install-user
    COMMAND ${CMAKE_COMMAND} -E make_directory "$ENV{HOME}/.local/share/yamy/plugins"
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:example_plugin> "$ENV{HOME}/.local/share/yamy/plugins/"
    DEPENDS example_plugin
    COMMENT "Installing plugin to ~/.local/share/yamy/plugins/"
)
