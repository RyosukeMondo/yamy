cmake_minimum_required(VERSION 3.10)

# Project Definition
project(yamy LANGUAGES CXX)
if(WIN32)
    enable_language(RC)
endif()

if(CMAKE_TOOLCHAIN_FILE AND CMAKE_TOOLCHAIN_FILE MATCHES "conan_toolchain\\.cmake$")
    if(NOT EXISTS "${CMAKE_TOOLCHAIN_FILE}")
        message(FATAL_ERROR "Conan toolchain file missing at '${CMAKE_TOOLCHAIN_FILE}'. Run `conan install . --output-folder=build/<preset>` before configuring.")
    endif()
endif()

# Standard Settings
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_UNICODE ON) # For Windows Unicode support if using CMake 3.20+, else we define UNICODE manually

# AddressSanitizer option (Linux/macOS only)
option(ENABLE_ASAN "Enable AddressSanitizer for memory error detection" OFF)

# Code Coverage option (Linux/macOS only, requires lcov)
option(ENABLE_COVERAGE "Enable code coverage measurement with gcov/lcov" OFF)

if(ENABLE_COVERAGE)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        message(STATUS "Code coverage enabled")
        set(COVERAGE_FLAGS "--coverage -O0 -g -fprofile-arcs -ftest-coverage")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COVERAGE_FLAGS}")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COVERAGE_FLAGS}")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} --coverage")
        add_compile_definitions(COVERAGE_ENABLED)
    else()
        message(WARNING "ENABLE_COVERAGE is only supported with GCC or Clang")
    endif()
endif()

if(ENABLE_ASAN)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        message(STATUS "AddressSanitizer enabled")
        set(ASAN_FLAGS "-fsanitize=address -fno-omit-frame-pointer -g")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${ASAN_FLAGS}")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${ASAN_FLAGS}")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${ASAN_FLAGS}")
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${ASAN_FLAGS}")
        # Ensure symbols are available for ASAN stack traces
        add_compile_definitions(ASAN_ENABLED)
    else()
        message(WARNING "ENABLE_ASAN is only supported with GCC or Clang")
    endif()
endif()

# Linker Configuration (Linux): prefer mold, fallback to LLD
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    find_program(MOLD_LINKER NAMES mold ld.mold)
    if(MOLD_LINKER)
        message(STATUS "Using mold linker: ${MOLD_LINKER}")
        add_link_options("-fuse-ld=mold")
    else()
        find_program(LLD_LINKER NAMES ld.lld lld)
        if(LLD_LINKER)
            message(WARNING "Mold linker not found; falling back to LLD: ${LLD_LINKER}")
            add_link_options("-fuse-ld=lld")
        else()
            message(WARNING "Neither mold nor LLD found. Using default system linker; install mold for fastest builds.")
        endif()
    endif()
endif()

# Linker Configuration (Windows): prefer lld-link when using clang-cl
if(WIN32 AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    find_program(CLANG_LLD_LINKER
        NAMES lld-link lld-link.exe
        HINTS
            "$ENV{ProgramFiles}/LLVM/bin"
            "$ENV{ProgramFiles(x86)}/LLVM/bin"
    )
    if(CLANG_LLD_LINKER)
        message(STATUS "Using lld-link for clang-cl: ${CLANG_LLD_LINKER}")
        set(CMAKE_LINKER "${CLANG_LLD_LINKER}")
        add_link_options("/clang:-fuse-ld=lld")
    else()
        message(WARNING "clang-cl detected but lld-link not found. Install LLVM to enable fast linking; falling back to MSVC link.exe.")
    endif()
endif()

# Compilation cache (ccache): optional, works when available on Linux/Windows
find_program(CCACHE_PROGRAM NAMES ccache)
if(CCACHE_PROGRAM)
    message(STATUS "Enabling ccache: ${CCACHE_PROGRAM}")
    # Configure recommended cache size (5GB) at configure time
    execute_process(
        COMMAND ${CCACHE_PROGRAM} -M 5G
        RESULT_VARIABLE CCACHE_RESULT
        OUTPUT_QUIET ERROR_QUIET
    )
    if(CCACHE_RESULT EQUAL 0)
        message(STATUS "ccache configured with 5G max size")
    else()
        message(WARNING "Failed to configure ccache max size; continuing with defaults")
    endif()

    set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    add_custom_target(ccache-stats
        COMMAND ${CCACHE_PROGRAM} -s
        COMMENT "Display ccache statistics"
        VERBATIM
    )
else()
    message(STATUS "ccache not found; builds will proceed without compilation caching")
endif()

# Third-party dependencies provided by Conan (fmt, Microsoft.GSL, quill)
set(CONAN_INSTALL_HINT "Run `conan install . --output-folder=build/<preset>` for your active preset to generate Conan packages.")

find_package(fmt CONFIG QUIET)
if(NOT fmt_FOUND)
    message(FATAL_ERROR "fmt not found. ${CONAN_INSTALL_HINT}")
endif()

find_package(Microsoft.GSL CONFIG QUIET)
if(NOT Microsoft.GSL_FOUND)
    message(FATAL_ERROR "Microsoft.GSL not found. ${CONAN_INSTALL_HINT}")
endif()

find_package(quill CONFIG QUIET)
if(NOT quill_FOUND)
    message(FATAL_ERROR "quill not found. ${CONAN_INSTALL_HINT}")
endif()

find_package(Catch2 CONFIG QUIET)
if(NOT Catch2_FOUND)
    message(FATAL_ERROR "Catch2 not found. ${CONAN_INSTALL_HINT}")
endif()

find_package(rapidcheck CONFIG QUIET)
if(NOT rapidcheck_FOUND)
    message(FATAL_ERROR "RapidCheck not found. ${CONAN_INSTALL_HINT}")
endif()

set(CATCH2_IMPORTED_TARGET "")
if(TARGET Catch2::Catch2)
    set(CATCH2_IMPORTED_TARGET Catch2::Catch2)
elseif(TARGET Catch2::Catch2WithMain)
    set(CATCH2_IMPORTED_TARGET Catch2::Catch2WithMain)
endif()
if(CATCH2_IMPORTED_TARGET STREQUAL "")
    message(FATAL_ERROR "Catch2 package located but expected targets were not found.")
endif()

set(RAPIDCHECK_IMPORTED_TARGET "")
if(TARGET rapidcheck)
    set(RAPIDCHECK_IMPORTED_TARGET rapidcheck)
elseif(TARGET RapidCheck::rapidcheck)
    set(RAPIDCHECK_IMPORTED_TARGET RapidCheck::rapidcheck)
elseif(TARGET RapidCheck::RapidCheck)
    set(RAPIDCHECK_IMPORTED_TARGET RapidCheck::RapidCheck)
endif()
if(RAPIDCHECK_IMPORTED_TARGET STREQUAL "")
    message(FATAL_ERROR "RapidCheck package located but expected targets were not found.")
endif()

add_library(yamy_dependencies INTERFACE)
target_link_libraries(yamy_dependencies INTERFACE
    fmt::fmt
    Microsoft.GSL::GSL
    quill::quill
)

add_library(yamy_test_dependencies INTERFACE)
target_link_libraries(yamy_test_dependencies INTERFACE
    ${CATCH2_IMPORTED_TARGET}
    ${RAPIDCHECK_IMPORTED_TARGET}
)

# Output Directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Compiler Flags
if(MSVC)
    add_compile_definitions(
        WIN32 _WINDOWS UNICODE _UNICODE
        "VERSION=\"0.04\""
        "LOGNAME=\"unknown\""
        "COMPUTERNAME=\"unknown\""
        _CRT_SECURE_NO_WARNINGS
        MAYU64
        USE_INI
        NOMINMAX
    )
    add_compile_options(/W3 /MP) # Warning level 3, Multi-processor compilation
elseif(MINGW)
    add_compile_definitions(
        WIN32 _WINDOWS UNICODE _UNICODE
        "VERSION=\"0.04\""
        "LOGNAME=\"unknown\""
        "COMPUTERNAME=\"unknown\""
        MAYU64
        USE_INI
    )
    add_compile_options(-Wall)
    add_link_options(-municode)
    # Global static removed to allow dynamic linking for main engine (64-bit primarily)
    # However, for 32-bit, we must link statically to avoid DLL filename conflicts (libstdc++-6.dll)
    if(CMAKE_SIZEOF_VOID_P EQUAL 4)
        add_link_options(-static)
    endif()
    set(CMAKE_SHARED_LIBRARY_PREFIX "")
endif()

# Determine Architecture for Output Names
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(YAMY_SUFFIX "64")
else()
    set(YAMY_SUFFIX "32")
endif()

# Determine architecture suffix for naming yamy64 / yamy32
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(YAMY_ARCH_SUFFIX "64")
else()
    set(YAMY_ARCH_SUFFIX "32")
endif()

if(WIN32)
    # -----------------------------------------------------------------------------
    # Target: yamy_hook (DLL)
    # -----------------------------------------------------------------------------
    add_library(yamy_hook SHARED
        src/platform/windows/hook.cpp
        src/utils/stringtool.cpp
        src/platform/windows/hook.h
        src/utils/misc.h
        src/utils/stringtool.h
    )

    target_compile_definitions(yamy_hook PRIVATE YAMY32DLL_EXPORTS)
    set_target_properties(yamy_hook PROPERTIES OUTPUT_NAME "yamy${YAMY_ARCH_SUFFIX}")

    target_include_directories(yamy_hook PRIVATE
        src/utils
        src/platform/windows
    )

    target_link_libraries(yamy_hook PRIVATE imm32 user32)
    # target_link_options(yamy_hook PRIVATE -static) - Removed to avoid CRT conflict

    # -----------------------------------------------------------------------------
    # Target: yamyd (Helper EXE)
    # -----------------------------------------------------------------------------
    # yamyd32 is specifically for hooking 32-bit apps from 64-bit env.
    # We build it as 'yamyd32' on 32-bit builds.
    if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
        add_executable(yamyd WIN32 src/app/yamyd.cpp)
        set_target_properties(yamyd PROPERTIES OUTPUT_NAME "yamyd32")
        target_link_libraries(yamyd PRIVATE yamy_hook)
        target_include_directories(yamyd PRIVATE src/platform/windows src/app src/utils)
        if(MINGW)
            target_link_options(yamyd PRIVATE -static)
        endif()
    endif()
endif()

# -----------------------------------------------------------------------------
# Target: yamy_engine (EXE)
# -----------------------------------------------------------------------------
set(ENGINE_SOURCES
    src/utils/compiler_specific_func.cpp
    src/core/engine/engine.cpp
    src/core/engine/engine_lifecycle.cpp
    src/core/notification_dispatcher.cpp
    src/core/engine/engine_input.cpp
    src/core/engine/engine_modifier.cpp
    src/core/engine/engine_generator.cpp
    src/core/engine/engine_focus.cpp
    src/core/engine/engine_window.cpp
    src/core/engine/engine_setting.cpp
    src/core/engine/engine_log.cpp
    src/core/engine/engine_event_processor.cpp
    src/core/engine/modifier_key_handler.cpp
    src/core/logging/logger.cpp
    src/core/logger/journey_logger.cpp
    src/core/window/focus.cpp
    src/core/functions/function.cpp
    src/core/functions/function_creator.cpp
    src/core/commands/cmd_keymap_parent.cpp
    src/core/commands/cmd_keymap_window.cpp
    src/core/commands/cmd_other_window_class.cpp
    src/core/commands/cmd_prefix.cpp
    src/core/commands/cmd_keymap.cpp
    src/core/commands/cmd_sync.cpp
    src/core/commands/cmd_toggle.cpp
    src/core/commands/cmd_edit_next_modifier.cpp
    src/core/commands/cmd_variable.cpp
    src/core/commands/cmd_repeat.cpp
    src/core/commands/cmd_undefined.cpp
    src/core/commands/cmd_ignore.cpp
    src/core/commands/cmd_post_message.cpp
    src/core/commands/cmd_wait.cpp
    src/core/commands/cmd_vk.cpp
    src/core/commands/cmd_default.cpp
    src/core/commands/cmd_keymap_prev_prefix.cpp
    src/core/commands/cmd_load_setting.cpp
    src/core/commands/cmd_shell_execute.cpp
    src/core/commands/cmd_set_foreground_window.cpp
    src/core/commands/cmd_investigate_command.cpp
    src/core/commands/cmd_mayu_dialog.cpp
    src/core/commands/cmd_describe_bindings.cpp
    src/core/commands/cmd_help_message.cpp
    src/core/commands/cmd_help_variable.cpp
    src/core/commands/cmd_window_raise.cpp
    src/core/commands/cmd_window_lower.cpp
    src/core/commands/cmd_window_minimize.cpp
    src/core/commands/cmd_window_maximize.cpp
    src/core/commands/cmd_window_h_maximize.cpp
    src/core/commands/cmd_window_v_maximize.cpp
    src/core/commands/cmd_window_hv_maximize.cpp
    src/core/commands/cmd_window_move.cpp
    src/core/commands/cmd_window_move_to.cpp
    src/core/commands/cmd_window_move_visibly.cpp
    src/core/commands/cmd_window_monitor_to.cpp
    src/core/commands/cmd_window_monitor.cpp
    src/core/commands/cmd_window_cling_to_left.cpp
    src/core/commands/cmd_window_cling_to_right.cpp
    src/core/commands/cmd_window_cling_to_top.cpp
    src/core/commands/cmd_window_cling_to_bottom.cpp
    src/core/commands/cmd_window_close.cpp
    src/core/commands/cmd_window_toggle_top_most.cpp
    src/core/commands/cmd_window_identify.cpp
    src/core/commands/cmd_window_set_alpha.cpp
    src/core/commands/cmd_window_redraw.cpp
    src/core/commands/cmd_window_resize_to.cpp
    src/core/commands/cmd_mouse_move.cpp
    src/core/commands/cmd_mouse_wheel.cpp
    src/core/commands/cmd_clipboard_change_case.cpp
    src/core/commands/cmd_clipboard_upcase_word.cpp
    src/core/commands/cmd_clipboard_downcase_word.cpp
    src/core/commands/cmd_clipboard_copy.cpp
    src/core/commands/cmd_emacs_edit_kill_line_pred.cpp
    src/core/commands/cmd_emacs_edit_kill_line_func.cpp
    src/core/commands/cmd_log_clear.cpp
    src/core/commands/cmd_recenter.cpp
    src/core/commands/cmd_direct_sstp.cpp
    src/core/commands/cmd_plugin.cpp
    src/core/commands/cmd_set_ime_status.cpp
    src/core/commands/cmd_set_ime_string.cpp
    src/core/commands/cmd_mouse_hook.cpp
    src/core/commands/cmd_cancel_prefix.cpp
    src/core/commands/cmd_metrics.cpp
    src/utils/metrics.cpp
    src/core/input/keyboard.cpp
    src/core/input/keymap.cpp
    src/core/input/modifier_state.cpp
    src/core/input/vkeytable.cpp
    src/core/window/layoutmanager.cpp
    src/core/window/target.cpp
    src/core/settings/parser.cpp
    src/platform/windows/input_driver_win32.cpp
    src/platform/windows/utf_conversion.cpp
    src/platform/windows/registry.cpp
    src/platform/windows/utf_conversion.cpp
    src/platform/windows/window_system_win32.cpp
    src/platform/windows/windowstool.cpp
    src/platform/windows/platform_paths_win32.cpp
    src/core/settings/setting.cpp
    src/core/settings/setting_loader.cpp
    src/core/settings/config_backup.cpp
    src/core/audio/sound_manager.cpp
    src/resources/templates/template_manager.cpp
    src/core/functions/strexpr.cpp
    src/utils/stringtool.cpp
    src/app/mayu.cpp
)

if(WIN32)
    list(APPEND ENGINE_SOURCES
        src/ui/dlgeditsetting.cpp
        src/ui/dlginvestigate.cpp
        src/ui/dlglog.cpp
        src/ui/dlgsetting.cpp
        src/ui/dlgversion.cpp
        src/ui/mayu.rc
        src/platform/windows/fixscancodemap.cpp
        src/platform/windows/input_injector_win32.cpp
        src/platform/windows/input_hook_win32.cpp
        src/platform/windows/thread_win32.cpp
        src/platform/windows/sync.cpp
        src/platform/windows/hook_data.cpp
    )


    # -----------------------------------------------------------------------------
    # Option: Use Modern Qt GUI on Windows
    # -----------------------------------------------------------------------------
    option(BUILD_QT_GUI "Build with Modern Qt GUI" ON)

    if(BUILD_QT_GUI)
        find_package(Qt6 COMPONENTS Widgets Gui Core REQUIRED)
        
        # Add Qt sources
        add_subdirectory(src/ui/qt)
        
        # Remove Win32 UI sources
        list(REMOVE_ITEM ENGINE_SOURCES
            src/ui/dlgeditsetting.cpp
            src/ui/dlginvestigate.cpp
            src/ui/dlglog.cpp
            src/ui/dlgsetting.cpp
            src/ui/dlgversion.cpp
            src/ui/mayu.rc
            src/app/mayu.cpp
        )
        
        # Add Qt Entry Point and Adapter, and missing Core components
        list(APPEND ENGINE_SOURCES 
            src/app/main_qt.cpp 
            src/app/engine_adapter.cpp
            src/core/settings/config_manager.cpp
            src/core/settings/config_validator.cpp
            src/core/settings/config_metadata.cpp
            src/core/settings/session_manager.cpp
            src/core/plugin_manager.cpp
            src/utils/crash_handler.cpp
            src/platform/windows/ipc_control_server.cpp
        )
        
        # Enable MOC (Disabled for main target to avoid duplicates with yamy_qt_gui)
        # set(CMAKE_AUTOMOC ON)
    endif()

    add_executable(yamy_engine_new WIN32 ${ENGINE_SOURCES})
    set_target_properties(yamy_engine_new PROPERTIES OUTPUT_NAME "yamy${YAMY_ARCH_SUFFIX}")
    
    if(BUILD_QT_GUI)
        target_link_libraries(yamy_engine_new PRIVATE 
            yamy_hook wtsapi32 advapi32 comctl32 shell32 ole32 shlwapi comdlg32 gdi32
            Qt6::Widgets Qt6::Gui Qt6::Core yamy_qt_gui
        )
        target_compile_definitions(yamy_engine_new PRIVATE BUILD_QT_GUI)
    else()
        target_link_libraries(yamy_engine_new PRIVATE yamy_hook wtsapi32 advapi32 comctl32 shell32 ole32 shlwapi comdlg32 gdi32)
    endif()

    target_link_libraries(yamy_engine_new PRIVATE yamy_dependencies)

    target_include_directories(yamy_engine_new PRIVATE
        src
        src/ui
        src/core/engine
        src/core/logging
        src/core/functions
        src/core/input
        src/core/window
        src/core/settings
        src/platform/windows
        src/utils
        src/app
    )
endif()

if(BUILD_LINUX_STUB OR (UNIX AND NOT APPLE AND NOT WIN32))
    # Find required Linux dependencies
    find_package(PkgConfig REQUIRED)

    # Find X11 libraries
    find_package(X11 REQUIRED)
    pkg_check_modules(XRANDR REQUIRED xrandr)

    # Find libudev (optional - for device enumeration)
    pkg_check_modules(UDEV libudev)
    if(UDEV_FOUND)
        message(STATUS "libudev found - device enumeration enabled")
        add_compile_definitions(HAVE_LIBUDEV)
    else()
        message(WARNING "libudev-dev not found - device enumeration disabled. Install with: sudo apt install libudev-dev")
    endif()

    # Core module sources (platform-independent)
    set(CORE_SOURCES
        src/utils/compiler_specific_func.cpp
        src/core/engine/engine.cpp
        src/core/engine/engine_lifecycle.cpp
        src/core/notification_dispatcher.cpp
        src/core/engine/engine_input.cpp
        src/core/engine/engine_modifier.cpp
        src/core/engine/engine_generator.cpp
        src/core/engine/engine_focus.cpp
        src/core/engine/engine_window.cpp
        src/core/engine/engine_setting.cpp
        src/core/engine/engine_log.cpp
        src/core/engine/engine_event_processor.cpp
        src/core/engine/modifier_key_handler.cpp
        src/core/logging/logger.cpp
        src/core/logger/journey_logger.cpp
        src/core/window/focus.cpp
        src/core/functions/function.cpp
        src/core/functions/function_creator.cpp
        src/core/commands/cmd_keymap_parent.cpp
        src/core/commands/cmd_keymap_window.cpp
        src/core/commands/cmd_other_window_class.cpp
        src/core/commands/cmd_prefix.cpp
        src/core/commands/cmd_keymap.cpp
        src/core/commands/cmd_sync.cpp
        src/core/commands/cmd_toggle.cpp
        src/core/commands/cmd_edit_next_modifier.cpp
        src/core/commands/cmd_variable.cpp
        src/core/commands/cmd_repeat.cpp
        src/core/commands/cmd_undefined.cpp
        src/core/commands/cmd_ignore.cpp
        src/core/commands/cmd_post_message.cpp
        src/core/commands/cmd_wait.cpp
        src/core/commands/cmd_vk.cpp
        src/core/commands/cmd_default.cpp
        src/core/commands/cmd_keymap_prev_prefix.cpp
        src/core/commands/cmd_load_setting.cpp
        src/core/commands/cmd_shell_execute.cpp
        src/core/commands/cmd_set_foreground_window.cpp
        src/core/commands/cmd_investigate_command.cpp
        src/core/commands/cmd_mayu_dialog.cpp
        src/core/commands/cmd_describe_bindings.cpp
        src/core/commands/cmd_help_message.cpp
        src/core/commands/cmd_help_variable.cpp
        src/core/commands/cmd_window_raise.cpp
        src/core/commands/cmd_window_lower.cpp
        src/core/commands/cmd_window_minimize.cpp
        src/core/commands/cmd_window_maximize.cpp
        src/core/commands/cmd_window_h_maximize.cpp
        src/core/commands/cmd_window_v_maximize.cpp
        src/core/commands/cmd_window_hv_maximize.cpp
        src/core/commands/cmd_window_move.cpp
        src/core/commands/cmd_window_move_to.cpp
        src/core/commands/cmd_window_move_visibly.cpp
        src/core/commands/cmd_window_monitor_to.cpp
        src/core/commands/cmd_window_monitor.cpp
        src/core/commands/cmd_window_cling_to_left.cpp
        src/core/commands/cmd_window_cling_to_right.cpp
        src/core/commands/cmd_window_cling_to_top.cpp
        src/core/commands/cmd_window_cling_to_bottom.cpp
        src/core/commands/cmd_window_close.cpp
        src/core/commands/cmd_window_toggle_top_most.cpp
        src/core/commands/cmd_window_identify.cpp
        src/core/commands/cmd_window_set_alpha.cpp
        src/core/commands/cmd_window_redraw.cpp
        src/core/commands/cmd_window_resize_to.cpp
        src/core/commands/cmd_mouse_move.cpp
        src/core/commands/cmd_mouse_wheel.cpp
        src/core/commands/cmd_clipboard_change_case.cpp
        src/core/commands/cmd_clipboard_upcase_word.cpp
        src/core/commands/cmd_clipboard_downcase_word.cpp
        src/core/commands/cmd_clipboard_copy.cpp
        src/core/commands/cmd_emacs_edit_kill_line_pred.cpp
        src/core/commands/cmd_emacs_edit_kill_line_func.cpp
        src/core/commands/cmd_log_clear.cpp
        src/core/commands/cmd_recenter.cpp
        src/core/commands/cmd_direct_sstp.cpp
        src/core/commands/cmd_plugin.cpp
        src/core/commands/cmd_set_ime_status.cpp
        src/core/commands/cmd_set_ime_string.cpp
        src/core/commands/cmd_mouse_hook.cpp
        src/core/commands/cmd_cancel_prefix.cpp
        src/core/commands/cmd_metrics.cpp
        src/utils/metrics.cpp
        src/core/input/keyboard.cpp
        src/core/input/keymap.cpp
        src/core/input/modifier_state.cpp
        src/core/input/vkeytable.cpp
        src/core/window/layoutmanager.cpp
        src/core/window/target.cpp
        src/core/settings/parser.cpp
        src/core/settings/setting.cpp
        src/core/settings/setting_loader.cpp
        src/core/settings/config_backup.cpp
        src/core/settings/config_manager.cpp
        src/core/settings/config_validator.cpp
        src/core/settings/config_metadata.cpp
        src/core/settings/config_watcher.cpp
        src/core/settings/session_manager.cpp
        src/core/platform/ipc_channel_interface.cpp
        src/resources/templates/template_manager.cpp
        src/core/functions/strexpr.cpp
        src/utils/stringtool.cpp
        src/utils/crash_handler.cpp
        src/core/plugin_manager.cpp
    )

    # Linux platform backend sources (shared between daemon and GUI)
    set(PLATFORM_LINUX_SOURCES
        src/app/engine_adapter.cpp
        src/platform/linux/x11_connection.cpp
        src/platform/linux/window_system_linux.cpp
        src/platform/linux/window_system_linux_queries.cpp
        src/platform/linux/window_system_linux_manipulation.cpp
        src/platform/linux/window_system_linux_hierarchy.cpp
        src/platform/linux/window_system_linux_mouse.cpp
        src/platform/linux/window_system_linux_monitor.cpp
        src/platform/linux/input_injector_linux.cpp
        src/platform/linux/input_hook_linux.cpp
        src/platform/linux/input_driver_linux.cpp
        src/platform/linux/device_manager_linux.cpp
        src/platform/linux/keycode_mapping.cpp
        src/platform/linux/sync_linux.cpp
        src/platform/linux/thread_linux.cpp
        src/platform/linux/hook_data_linux.cpp
        src/platform/linux/ipc_linux.cpp
        src/core/platform/linux/ipc_channel_qt.cpp
        src/platform/linux/ipc_control_server.cpp
        src/platform/linux/platform_paths_linux.cpp
    )

    # Qt5 Core is required for all builds (config_watcher uses QObject)
    find_package(Qt6 REQUIRED COMPONENTS Core)
    message(STATUS "Qt5 Core found - version ${Qt6Core_VERSION}")

    # Qt5 Network is required for IPC (QLocalSocket/QLocalServer)
    find_package(Qt6 REQUIRED COMPONENTS Network)
    message(STATUS "Qt5 Network found - version ${Qt6Network_VERSION}")

    # Qt5 Test is required for test builds
    find_package(Qt6 REQUIRED COMPONENTS Test)
    message(STATUS "Qt5 Test found - version ${Qt6Test_VERSION}")

    # Qt5 GUI for Linux (optional)
    option(BUILD_QT_GUI "Build Qt5 GUI for Linux" ON)

    add_library(yamy_core STATIC
        ${CORE_SOURCES}
        ${PLATFORM_LINUX_SOURCES}
    )
    set_target_properties(yamy_core PROPERTIES AUTOMOC ON)
    target_include_directories(yamy_core PUBLIC
        ${UDEV_INCLUDE_DIRS}
        src
        src/ui
        src/ui/qt
        src/core/platform
        src/core/engine
        src/core/logging
        src/core/settings
        src/core/input
        src/core/window
        src/core/functions
        src/core/commands
        src/platform/linux
        src/utils
        src/app
    )
    target_link_libraries(yamy_core PUBLIC
        pthread dl ${X11_LIBRARIES} ${XRANDR_LIBRARIES} ${UDEV_LIBRARIES}
        Qt6::Core
        Qt6::Network
        yamy_dependencies
    )

    add_executable(yamy src/app/main.cpp)
    target_link_libraries(yamy PRIVATE yamy_core)

    if(BUILD_QT_GUI)
        # Find optional Qt5 GUI packages
        find_package(Qt6 COMPONENTS Widgets Gui Test)

        if(Qt6Widgets_FOUND AND Qt6Gui_FOUND AND Qt6Gui_FOUND)
            message(STATUS "Qt5 GUI components found - building GUI executables")

            add_subdirectory(src/ui/qt)

            add_executable(yamy-gui
                src/app/main_gui.cpp
            )
            set_target_properties(yamy-gui PROPERTIES AUTOMOC ON)
            target_link_libraries(yamy-gui PRIVATE
                Qt6::Widgets
                Qt6::Gui
                Qt6::Core
                Qt6::Network
                yamy_core
                yamy_qt_gui
            )

            add_executable(yamy_gui_probe
                tools/main_window_gui_probe.cpp
                src/ui/qt/main_window_gui.cpp
                src/ui/qt/ipc_client_gui.cpp
                src/core/platform/linux/ipc_channel_qt.cpp
                src/core/platform/ipc_channel_interface.cpp
            )
            set_target_properties(yamy_gui_probe PROPERTIES AUTOMOC ON)
            target_include_directories(yamy_gui_probe PRIVATE
                src
                src/ui/qt
                src/core/platform
                src/core/platform/linux
            )
            target_link_libraries(yamy_gui_probe PRIVATE
                Qt6::Widgets
                Qt6::Gui
                Qt6::Core
                Qt6::Network
            )
        else()
            message(WARNING "Qt5 GUI components not found - building headless version only")
        endif()
    else()
        message(STATUS "Building without Qt5 GUI (headless mode)")
    endif()

    # Enable Qt MOC for Qt-based sources (ConfigWatcher, IIPCChannel)
    set_target_properties(yamy PROPERTIES AUTOMOC ON)

    target_include_directories(yamy PRIVATE
        src
        src/ui
        src/ui/qt
        src/core/platform
        src/core/engine
        src/core/logging
        src/core/settings
        src/core/input
        src/core/window
        src/core/functions
        src/core/commands
        src/platform/linux
        src/utils
        src/app
    )

    # -----------------------------------------------------------------------------
    # Target: yamy-ctl (Command-line control tool)
    # -----------------------------------------------------------------------------
    add_executable(yamy-ctl
        src/app/yamy_ctl.cpp
    )

    target_include_directories(yamy-ctl PRIVATE
        src
        src/core
        src/core/platform
    )

    # yamy-ctl only needs pthread for potential future extensions
    target_link_libraries(yamy-ctl PRIVATE pthread)

    # Note: Full engine build (yamy_engine_new) is disabled on Linux until Core Refactoring
    # (Branch 2 & 10) is complete and merged.
endif()

# -----------------------------------------------------------------------------
# Verification: Check for missing sources
# -----------------------------------------------------------------------------
if(WIN32)
    if(CMAKE_HOST_WIN32)
        add_custom_command(TARGET yamy_engine_new PRE_BUILD
            COMMAND powershell -ExecutionPolicy Bypass -File ${CMAKE_SOURCE_DIR}/scripts/windows/qa/check_missing_sources.ps1
            COMMAND powershell -ExecutionPolicy Bypass -File ${CMAKE_SOURCE_DIR}/scripts/windows/qa/check_header_guards.ps1
            COMMAND powershell -ExecutionPolicy Bypass -File ${CMAKE_SOURCE_DIR}/scripts/windows/qa/check_antipatterns.ps1
            COMMENT "Running QA Checks: Sources, Header Guards, Encoding..."
            VERBATIM
        )
    elseif(CMAKE_HOST_UNIX)
        add_custom_command(TARGET yamy_engine_new PRE_BUILD
            COMMAND bash ${CMAKE_SOURCE_DIR}/scripts/linux/check_missing_sources.sh || true
            COMMENT "Running QA Checks: Sources..."
            VERBATIM
        )
    endif()
endif()

if(WIN32)
    # -----------------------------------------------------------------------------
    # Target: yamy_launcher (EXE)
    # -----------------------------------------------------------------------------
    add_executable(yamy_launcher WIN32
        src/app/yamy.cpp
        src/utils/stringtool.cpp
        src/platform/windows/utf_conversion.cpp
        src/ui/mayu.rc
    )

    set_target_properties(yamy_launcher PROPERTIES OUTPUT_NAME "yamy")

    target_include_directories(yamy_launcher PRIVATE
        src/utils
        src/ui
    )
endif()

# -----------------------------------------------------------------------------
# Target: yamy_test (EXE)
# -----------------------------------------------------------------------------
option(BUILD_LINUX_STUB "Build Linux stub backend" OFF)
option(BUILD_TESTING "Build the testing tree." OFF)

if(BUILD_TESTING)
    enable_testing()

    # Using the existing gtest source in the repo as per vcxproj
    set(GTEST_DIR src/tests/googletest)
    set(GMOCK_DIR src/tests/googlemock)

    # We might need to compile gtest as a library first if not already managed
    # For simplicity, including gtest-all.cc directly as the vcxproj did

    # Add platform specific sources
    if(WIN32)
        list(APPEND ENGINE_SOURCES src/platform/windows/utf_conversion.cpp)
    endif()

    set(TEST_SOURCES
        ${ENGINE_SOURCES} # Tests often need the whole engine code mostly.
                          # Note: Linking engine object or sources directly.
                          # Excluding main app entry point mayu.cpp to avoid double main
    )
    list(REMOVE_ITEM TEST_SOURCES src/app/mayu.cpp)

    add_executable(yamy_test
        ${TEST_SOURCES}
        src/tests/googletest/src/gtest-all.cc
        src/tests/test_main.cpp
    )

    if(WIN32)
        target_sources(yamy_test PRIVATE src/tests/test_utf_conversion.cpp)
    endif()

    target_include_directories(yamy_test PRIVATE
        ${GTEST_DIR}/include
        ${GTEST_DIR}
        src
        src/ui
        src/core/engine
        src/core/functions
        src/core/input
        src/core/window
        src/core/settings
        src/platform/windows
        src/utils
        src/app
    )

    target_link_libraries(yamy_test PRIVATE yamy_hook)  # Engine depends on hook

    add_test(NAME yamy_test COMMAND yamy_test)
endif()

# -----------------------------------------------------------------------------
# Target: yamy_linux_test (Linux Platform Unit Tests)
# -----------------------------------------------------------------------------
if(UNIX AND NOT APPLE AND NOT WIN32)
    option(BUILD_LINUX_TESTING "Build Linux platform tests" ON)
    option(BUILD_REGRESSION_TESTS "Build comprehensive regression test suite" ON)

    # Enable CTest if any testing is enabled
    if(BUILD_LINUX_TESTING OR BUILD_REGRESSION_TESTS)
        enable_testing()
        message(STATUS "CTest enabled for testing")
    endif()

    # GoogleTest directories (shared by all test targets)
    set(GTEST_DIR src/tests/googletest)
    set(GMOCK_DIR src/tests/googlemock)

    if(BUILD_LINUX_TESTING)
        message(STATUS "Building Linux platform tests")

        # Linux platform test sources
        set(LINUX_TEST_SOURCES
            src/tests/platform/test_main_linux.cpp
            src/tests/platform/window_system_linux_test.cpp
            src/tests/platform/input_injector_linux_test.cpp
            src/tests/platform/input_hook_linux_test.cpp
            src/tests/platform/ipc_linux_test.cpp
            src/tests/platform/ipc_multi_instance_test.cpp
            src/tests/platform/config_manager_test.cpp
            src/tests/platform/config_validator_test.cpp
            src/tests/platform/config_metadata_test.cpp
            src/tests/platform/session_manager_test.cpp
            src/platform/linux/window_system_linux_queries.cpp
            src/platform/linux/x11_connection.cpp
            src/platform/linux/keycode_mapping.cpp
            src/platform/linux/input_hook_linux.cpp
            src/platform/linux/device_manager_linux.cpp
            src/platform/linux/ipc_linux.cpp
            src/core/settings/config_manager.cpp
            src/core/settings/config_validator.cpp
            src/core/settings/config_metadata.cpp
            src/core/settings/config_watcher.cpp
            src/core/settings/session_manager.cpp
            src/core/settings/parser.cpp
            src/utils/stringtool.cpp
            src/utils/metrics.cpp
            src/core/logging/logger.cpp
            src/core/logger/journey_logger.cpp
        )

        add_executable(yamy_linux_test
            ${LINUX_TEST_SOURCES}
            src/tests/googletest/src/gtest-all.cc
        )

        # Enable AUTOMOC for Qt-based sources (ConfigWatcher uses QObject)
        set_target_properties(yamy_linux_test PROPERTIES AUTOMOC ON)

        target_include_directories(yamy_linux_test PRIVATE
            ${GTEST_DIR}/include
            ${GTEST_DIR}
            src
            src/core
            src/core/platform
            src/core/input
            src/core/settings
            src/utils
            src/platform/linux
        )

        target_link_libraries(yamy_linux_test PRIVATE
            pthread
            ${X11_LIBRARIES}
            ${UDEV_LIBRARIES}
            Qt6::Core
        )

        # Add test to CTest
        add_test(NAME yamy_linux_test COMMAND yamy_linux_test)

        # Qt5 GUI-dependent tests (only build if Qt5 GUI components are available)
        if(Qt6Widgets_FOUND AND Qt6Gui_FOUND AND Qt6Gui_FOUND)
            message(STATUS "Building Qt5 GUI-dependent tests")

        # -----------------------------------------------------------------------------
        # Target: yamy_ui_test (UI Unit Tests)
        # -----------------------------------------------------------------------------
        set(UI_TEST_SOURCES
            tests/ui/dialog_investigate_test.cpp
            src/ui/qt/dialog_investigate_qt.cpp
            src/ui/qt/dialog_condition_generator_qt.cpp
            src/ui/qt/crosshair_widget_qt.cpp
        )

        add_executable(yamy_ui_test
            tests/ui/dialog_investigate_test.cpp
            src/tests/googletest/src/gtest-all.cc
            src/tests/googlemock/src/gmock-all.cc
        )

        # Enable AUTOMOC for Qt-based tests (MockIPCChannel uses Q_OBJECT)
        set_target_properties(yamy_ui_test PROPERTIES AUTOMOC ON)

        target_include_directories(yamy_ui_test PRIVATE
            ${GTEST_DIR}/include
            ${GTEST_DIR}
            ${GMOCK_DIR}/include
            ${GMOCK_DIR}
            src
            src/core
            src/core/platform
            src/core/input
            src/core/settings
            src/core/functions
            src/core/engine
            src/core/window
            src/utils
            src/platform/linux
            src/ui/qt
        )

        target_link_libraries(yamy_ui_test PRIVATE
            pthread
            ${X11_LIBRARIES}
            yamy_qt_gui
            Qt6::Core
            Qt6::Widgets
            Qt6::Gui
            Qt6::Gui
        )

        add_test(NAME yamy_ui_test COMMAND yamy_ui_test)

        # -----------------------------------------------------------------------------
        # Target: yamy_investigate_workflow_test (Investigate Workflow Integration Tests)
        # End-to-end tests for investigate window feature
        # -----------------------------------------------------------------------------
        add_executable(yamy_investigate_workflow_test
            tests/integration/investigate_workflow_test.cpp
            src/ui/qt/dialog_investigate_qt.cpp
            src/ui/qt/dialog_condition_generator_qt.cpp
            src/ui/qt/crosshair_widget_qt.cpp
            src/core/platform/ipc_channel_interface.cpp
            src/core/platform/linux/ipc_channel_qt.cpp
            src/platform/linux/window_system_linux.cpp
            src/platform/linux/window_system_linux_queries.cpp
            src/platform/linux/window_system_linux_hierarchy.cpp
            src/platform/linux/x11_connection.cpp
            src/tests/googletest/src/gtest-all.cc
            src/tests/googlemock/src/gmock-all.cc
        )

        # Enable AUTOMOC for Qt-based tests (MockIPCChannel uses Q_OBJECT)
        set_target_properties(yamy_investigate_workflow_test PROPERTIES AUTOMOC ON)

        target_include_directories(yamy_investigate_workflow_test PRIVATE
            ${GTEST_DIR}/include
            ${GTEST_DIR}
            ${GMOCK_DIR}/include
            ${GMOCK_DIR}
            src
            src/core
            src/core/platform
            src/core/input
            src/core/settings
            src/core/functions
            src/core/engine
            src/core/window
            src/utils
            src/platform/linux
            src/ui/qt
        )

        target_link_libraries(yamy_investigate_workflow_test PRIVATE
            pthread
            ${X11_LIBRARIES}
            Qt6::Core
            Qt6::Widgets
            Qt6::Gui
            Qt6::Network
            Qt6::Gui
            Qt6::Test
        )

        add_test(NAME yamy_investigate_workflow_test COMMAND yamy_investigate_workflow_test)

        # -----------------------------------------------------------------------------
        # Target: yamy_log_test (Log Dialog Unit Tests)
        # Comprehensive tests for Logger and DialogLogQt
        # -----------------------------------------------------------------------------
        set(LOG_TEST_SOURCES
            tests/ui/dialog_log_test.cpp
            src/ui/qt/dialog_log_qt.cpp
            src/ui/qt/log_stats_panel.cpp
            src/core/logging/logger.cpp
        )

        add_executable(yamy_log_test
            ${LOG_TEST_SOURCES}
            src/tests/googletest/src/gtest-all.cc
        )

        set_target_properties(yamy_log_test PROPERTIES
            AUTOMOC ON
        )

        target_include_directories(yamy_log_test PRIVATE
            ${GTEST_DIR}/include
            ${GTEST_DIR}
            src
            src/core
            src/core/logging
            src/core/platform
            src/utils
            src/ui/qt
        )

        target_link_libraries(yamy_log_test PRIVATE
            pthread
            Qt6::Core
            Qt6::Widgets
            Qt6::Gui
        )

        add_test(NAME yamy_log_test COMMAND yamy_log_test)

        # -----------------------------------------------------------------------------
        # Target: yamy_notification_history_test (Notification History Tests)
        # Tests for NotificationHistory and NotificationHistoryDialog
        # -----------------------------------------------------------------------------
        set(NOTIFICATION_HISTORY_TEST_SOURCES
            tests/ui/notification_history_test.cpp
            src/ui/qt/notification_history.cpp
        )

        add_executable(yamy_notification_history_test
            ${NOTIFICATION_HISTORY_TEST_SOURCES}
            src/tests/googletest/src/gtest-all.cc
        )

        set_target_properties(yamy_notification_history_test PROPERTIES
            AUTOMOC ON
        )

        target_include_directories(yamy_notification_history_test PRIVATE
            ${GTEST_DIR}/include
            ${GTEST_DIR}
            src
            src/core
            src/core/platform
            src/utils
            src/ui/qt
        )

        target_link_libraries(yamy_notification_history_test PRIVATE
            pthread
            Qt6::Core
            Qt6::Widgets
            Qt6::Gui
        )

        add_test(NAME yamy_notification_history_test COMMAND yamy_notification_history_test)

        # -----------------------------------------------------------------------------
        # Target: yamy_tray_test (Tray Icon Notification Handling Tests)
        # Tests for TrayIconQt notification handling
        # -----------------------------------------------------------------------------
        set(TRAY_TEST_SOURCES
            tests/ui/tray_icon_test.cpp
            src/ui/qt/tray_icon_qt.cpp
            src/ui/qt/global_hotkey.cpp
            src/ui/qt/dialog_settings_qt.cpp
            src/ui/qt/dialog_log_qt.cpp
            src/ui/qt/log_stats_panel.cpp
            src/ui/qt/dialog_about_qt.cpp
            src/ui/qt/dialog_investigate_qt.cpp
            src/ui/qt/dialog_shortcuts_qt.cpp
            src/ui/qt/dialog_examples_qt.cpp
            src/ui/qt/config_manager_dialog.cpp
            src/ui/qt/crosshair_widget_qt.cpp
            src/ui/qt/notification_history.cpp
            src/core/settings/config_manager.cpp
            src/core/settings/config_backup.cpp
            src/core/settings/config_metadata.cpp
            src/core/logging/logger.cpp
        )

        add_executable(yamy_tray_test
            ${TRAY_TEST_SOURCES}
            src/tests/googletest/src/gtest-all.cc
        )

        set_target_properties(yamy_tray_test PROPERTIES
            AUTOMOC ON
        )

        target_include_directories(yamy_tray_test PRIVATE
            ${GTEST_DIR}/include
            ${GTEST_DIR}
            src
            src/core
            src/core/platform
            src/core/settings
            src/utils
            src/ui/qt
        )

        target_link_libraries(yamy_tray_test PRIVATE
            pthread
            Qt6::Core
            Qt6::Widgets
            Qt6::Gui
            Qt6::Test
        )

        add_test(NAME yamy_tray_test COMMAND yamy_tray_test)

        # -----------------------------------------------------------------------------
        # Target: yamy_ipc_channel_qt_test (IPCChannelQt Message Serialization Tests)
        # Tests for Qt-based IPC channel message send/receive, framing, and connection handling
        # -----------------------------------------------------------------------------
        set(IPC_CHANNEL_QT_TEST_SOURCES
            tests/platform/ipc_channel_qt_test.cpp
            src/core/platform/linux/ipc_channel_qt.cpp
            src/core/platform/ipc_channel_interface.cpp
        )

        add_executable(yamy_ipc_channel_qt_test
            ${IPC_CHANNEL_QT_TEST_SOURCES}
            src/tests/googletest/src/gtest-all.cc
        )

        set_target_properties(yamy_ipc_channel_qt_test PROPERTIES
            AUTOMOC ON
        )

        target_include_directories(yamy_ipc_channel_qt_test PRIVATE
            ${GTEST_DIR}/include
            ${GTEST_DIR}
            src
            src/core
            src/core/platform
            src/utils
        )

        target_link_libraries(yamy_ipc_channel_qt_test PRIVATE
            pthread
            Qt6::Core
            Qt6::Network
            Qt6::Test
        )

        add_test(NAME yamy_ipc_channel_qt_test COMMAND yamy_ipc_channel_qt_test)

        # -----------------------------------------------------------------------------
        # Target: yamy_mock_ipc_server (mock daemon for GUI IPC testing)
        # Provides deterministic responses for GUI client integration tests
        # -----------------------------------------------------------------------------
        set(MOCK_IPC_SERVER_SOURCES
            tests/mock_ipc_server.cpp
            src/core/platform/linux/ipc_channel_qt.cpp
            src/core/platform/ipc_channel_interface.cpp
        )

        add_executable(yamy_mock_ipc_server
            ${MOCK_IPC_SERVER_SOURCES}
        )

        set_target_properties(yamy_mock_ipc_server PROPERTIES
            AUTOMOC ON
        )

        target_include_directories(yamy_mock_ipc_server PRIVATE
            src
            src/core
            src/core/platform
            src/utils
            tests
        )

        target_link_libraries(yamy_mock_ipc_server PRIVATE
            pthread
            Qt6::Core
            Qt6::Network
        )

        # -----------------------------------------------------------------------------
        # Target: yamy_ipc_protocol_test (IPC message contract tests)
        # Validates serialization/deserialization for all IPC command/response types
        # -----------------------------------------------------------------------------
        set(IPC_PROTOCOL_TEST_SOURCES
            tests/test_ipc_protocol.cpp
            src/core/platform/linux/ipc_channel_qt.cpp
            src/core/platform/ipc_channel_interface.cpp
        )

        add_executable(yamy_ipc_protocol_test
            ${IPC_PROTOCOL_TEST_SOURCES}
            src/tests/googletest/src/gtest-all.cc
        )

        set_target_properties(yamy_ipc_protocol_test PROPERTIES
            AUTOMOC ON
        )

        target_include_directories(yamy_ipc_protocol_test PRIVATE
            ${GTEST_DIR}/include
            ${GTEST_DIR}
            src
            src/core
            src/core/platform
            src/utils
        )

        target_link_libraries(yamy_ipc_protocol_test PRIVATE
            pthread
            Qt6::Core
            Qt6::Network
            Qt6::Test
        )

        add_test(NAME yamy_ipc_protocol_test COMMAND yamy_ipc_protocol_test)

        # -----------------------------------------------------------------------------
        # Target: yamy_ipc_client_gui_test (IPCClientGUI integration with mock server)
        # Exercises Qt signals/slots against deterministic mock server responses
        # -----------------------------------------------------------------------------
        set(IPC_CLIENT_GUI_TEST_SOURCES
            tests/test_ipc_client_gui.cpp
            src/ui/qt/ipc_client_gui.cpp
            src/core/platform/linux/ipc_channel_qt.cpp
            src/core/platform/ipc_channel_interface.cpp
        )

        add_executable(yamy_ipc_client_gui_test
            ${IPC_CLIENT_GUI_TEST_SOURCES}
            src/tests/googletest/src/gtest-all.cc
        )

        set_target_properties(yamy_ipc_client_gui_test PROPERTIES
            AUTOMOC ON
        )

        target_include_directories(yamy_ipc_client_gui_test PRIVATE
            ${GTEST_DIR}/include
            ${GTEST_DIR}
            src
            src/core
            src/core/platform
            src/ui
            src/ui/qt
            src/utils
        )

        target_link_libraries(yamy_ipc_client_gui_test PRIVATE
            pthread
            Qt6::Core
            Qt6::Network
            Qt6::Test
        )

        add_dependencies(yamy_ipc_client_gui_test yamy_mock_ipc_server)
        add_test(NAME yamy_ipc_client_gui_test COMMAND yamy_ipc_client_gui_test)

        # -----------------------------------------------------------------------------
        # Target: yamy_notification_dispatcher_test (NotificationDispatcher Tests)
        # Tests for core NotificationDispatcher callback system
        # -----------------------------------------------------------------------------
        set(NOTIFICATION_DISPATCHER_TEST_SOURCES
            tests/core/notification_test.cpp
            src/core/notification_dispatcher.cpp
        )

        add_executable(yamy_notification_dispatcher_test
            ${NOTIFICATION_DISPATCHER_TEST_SOURCES}
            src/tests/googletest/src/gtest-all.cc
        )

        target_include_directories(yamy_notification_dispatcher_test PRIVATE
            ${GTEST_DIR}/include
            ${GTEST_DIR}
            src
            src/core
            src/core/platform
            src/utils
        )

        target_link_libraries(yamy_notification_dispatcher_test PRIVATE
            pthread
        )

        add_test(NAME yamy_notification_dispatcher_test COMMAND yamy_notification_dispatcher_test)

        # -----------------------------------------------------------------------------
        # Target: yamy_parser_utf8_test (UTF-8 Parser Tests)
        # Tests for UTF-8 character length detection and validation
        # -----------------------------------------------------------------------------
        set(PARSER_UTF8_TEST_SOURCES
            tests/core/settings/parser_utf8_test.cpp
            src/core/settings/parser.cpp
            src/core/logging/logger.cpp
            src/utils/stringtool.cpp
        )

        add_executable(yamy_parser_utf8_test
            ${PARSER_UTF8_TEST_SOURCES}
            src/tests/googletest/src/gtest-all.cc
        )

        target_include_directories(yamy_parser_utf8_test PRIVATE
            ${GTEST_DIR}/include
            ${GTEST_DIR}
            src
            src/core
            src/core/settings
            src/utils
        )

        target_link_libraries(yamy_parser_utf8_test PRIVATE
            pthread
        )

        add_test(NAME yamy_parser_utf8_test COMMAND yamy_parser_utf8_test)

        # -----------------------------------------------------------------------------
        # Target: yamy_notification_prefs_test (NotificationPrefs Tests)
        # Tests for NotificationPrefs filtering system
        # -----------------------------------------------------------------------------
        set(NOTIFICATION_PREFS_TEST_SOURCES
            tests/ui/notification_ui_test.cpp
            src/ui/qt/notification_prefs.cpp
        )

        add_executable(yamy_notification_prefs_test
            ${NOTIFICATION_PREFS_TEST_SOURCES}
            src/tests/googletest/src/gtest-all.cc
        )

        set_target_properties(yamy_notification_prefs_test PROPERTIES
            AUTOMOC ON
        )

        target_include_directories(yamy_notification_prefs_test PRIVATE
            ${GTEST_DIR}/include
            ${GTEST_DIR}
            src
            src/core
            src/core/platform
            src/utils
            src/ui/qt
        )

        target_link_libraries(yamy_notification_prefs_test PRIVATE
            pthread
            Qt6::Core
            Qt6::Widgets
            Qt6::Gui
            Qt6::Test
        )

        add_test(NAME yamy_notification_prefs_test COMMAND yamy_notification_prefs_test)

        # -----------------------------------------------------------------------------
        # Target: yamy_investigate_performance_test (Investigate Window Performance Benchmarks)
        # Benchmarks for window property queries, IPC latency, and end-to-end workflow
        # -----------------------------------------------------------------------------
        set(INVESTIGATE_PERFORMANCE_TEST_SOURCES
            tests/benchmarks/investigate_performance_test.cpp
            src/core/platform/linux/ipc_channel_qt.cpp
            src/core/platform/ipc_channel_interface.cpp
            src/platform/linux/window_system_linux.cpp
            src/platform/linux/window_system_linux_queries.cpp
            src/platform/linux/window_system_linux_hierarchy.cpp
            src/platform/linux/x11_connection.cpp
        )

        add_executable(yamy_investigate_performance_test
            ${INVESTIGATE_PERFORMANCE_TEST_SOURCES}
            src/tests/googletest/src/gtest-all.cc
        )

        set_target_properties(yamy_investigate_performance_test PROPERTIES
            AUTOMOC ON
        )

        target_include_directories(yamy_investigate_performance_test PRIVATE
            ${GTEST_DIR}/include
            ${GTEST_DIR}
            src
            src/core
            src/core/platform
            src/platform/linux
            src/utils
        )

        target_link_libraries(yamy_investigate_performance_test PRIVATE
            pthread
            ${X11_LIBRARIES}
            Qt6::Core
            Qt6::Network
            Qt6::Test
        )

        add_test(NAME yamy_investigate_performance_test COMMAND yamy_investigate_performance_test)

        # -----------------------------------------------------------------------------
        # Target: yamy_plugin_test (Plugin System Tests)
        # Tests plugin loading, initialization, and callback registration
        # -----------------------------------------------------------------------------
        if(BUILD_EXAMPLES)
            set(PLUGIN_TEST_SOURCES
                tests/plugin/plugin_test.cpp
                src/core/notification_dispatcher.cpp
            )

            add_executable(yamy_plugin_test
                ${PLUGIN_TEST_SOURCES}
            )

            set_target_properties(yamy_plugin_test PROPERTIES AUTOMOC ON)

            target_include_directories(yamy_plugin_test PRIVATE
                src
                src/core
                src/core/platform
            )

            # Export symbols so plugin can use NotificationDispatcher from test binary
            target_link_options(yamy_plugin_test PRIVATE -rdynamic)

            target_link_libraries(yamy_plugin_test PRIVATE
                pthread
                dl
                Qt6::Core
                Qt6::Test
            )

            add_test(NAME yamy_plugin_test COMMAND yamy_plugin_test)
        endif()

        # -----------------------------------------------------------------------------
        # Target: yamy_keyremap_test (Key Remapping Integration Tests)
        # Tests the full key remapping flow including config loading and keymap resolution
        # This is a full integration test using the complete core module
        # -----------------------------------------------------------------------------
        set(KEYREMAP_TEST_CORE_SOURCES
            src/core/input/keyboard.cpp
            src/core/input/keymap.cpp
            src/core/input/modifier_state.cpp
            src/core/input/vkeytable.cpp
            src/core/settings/parser.cpp
            src/core/settings/setting.cpp
            src/core/settings/setting_loader.cpp
            src/core/settings/config_backup.cpp
            src/core/settings/config_manager.cpp
            src/core/settings/config_watcher.cpp
            src/core/settings/config_validator.cpp
            src/core/settings/config_metadata.cpp
            src/core/settings/session_manager.cpp
            src/resources/templates/template_manager.cpp
            src/core/functions/strexpr.cpp
            src/core/functions/function.cpp
            src/core/functions/function_creator.cpp
            src/core/window/focus.cpp
            src/core/window/layoutmanager.cpp
            src/core/window/target.cpp
            src/core/engine/engine.cpp
            src/core/engine/engine_lifecycle.cpp
            src/core/engine/engine_input.cpp
            src/core/engine/engine_modifier.cpp
            src/core/engine/engine_generator.cpp
            src/core/engine/engine_focus.cpp
            src/core/engine/engine_window.cpp
            src/core/engine/engine_setting.cpp
            src/core/engine/engine_log.cpp
            src/core/engine/engine_event_processor.cpp
            src/core/engine/modifier_key_handler.cpp
            src/core/logging/logger.cpp
            src/utils/stringtool.cpp
            src/utils/compiler_specific_func.cpp
            src/platform/linux/sync_linux.cpp
            src/platform/linux/thread_linux.cpp
            src/platform/linux/hook_data_linux.cpp
            src/platform/linux/window_system_linux.cpp
            src/platform/linux/window_system_linux_queries.cpp
            src/platform/linux/window_system_linux_manipulation.cpp
            src/platform/linux/window_system_linux_hierarchy.cpp
            src/platform/linux/window_system_linux_mouse.cpp
            src/platform/linux/window_system_linux_monitor.cpp
            src/platform/linux/x11_connection.cpp
            src/platform/linux/input_injector_linux.cpp
            src/platform/linux/keycode_mapping.cpp
            src/platform/linux/platform_paths_linux.cpp
        )

        # Command sources needed for function_creator linkage
        set(KEYREMAP_TEST_CMD_SOURCES
            src/core/commands/cmd_keymap_parent.cpp
            src/core/commands/cmd_keymap_window.cpp
            src/core/commands/cmd_other_window_class.cpp
            src/core/commands/cmd_prefix.cpp
            src/core/commands/cmd_keymap.cpp
            src/core/commands/cmd_sync.cpp
            src/core/commands/cmd_toggle.cpp
            src/core/commands/cmd_edit_next_modifier.cpp
            src/core/commands/cmd_variable.cpp
            src/core/commands/cmd_repeat.cpp
            src/core/commands/cmd_undefined.cpp
            src/core/commands/cmd_ignore.cpp
            src/core/commands/cmd_post_message.cpp
            src/core/commands/cmd_wait.cpp
            src/core/commands/cmd_vk.cpp
            src/core/commands/cmd_default.cpp
            src/core/commands/cmd_keymap_prev_prefix.cpp
            src/core/commands/cmd_load_setting.cpp
            src/core/commands/cmd_shell_execute.cpp
            src/core/commands/cmd_set_foreground_window.cpp
            src/core/commands/cmd_investigate_command.cpp
            src/core/commands/cmd_mayu_dialog.cpp
            src/core/commands/cmd_describe_bindings.cpp
            src/core/commands/cmd_help_message.cpp
            src/core/commands/cmd_help_variable.cpp
            src/core/commands/cmd_window_raise.cpp
            src/core/commands/cmd_window_lower.cpp
            src/core/commands/cmd_window_minimize.cpp
            src/core/commands/cmd_window_maximize.cpp
            src/core/commands/cmd_window_h_maximize.cpp
            src/core/commands/cmd_window_v_maximize.cpp
            src/core/commands/cmd_window_hv_maximize.cpp
            src/core/commands/cmd_window_move.cpp
            src/core/commands/cmd_window_move_to.cpp
            src/core/commands/cmd_window_move_visibly.cpp
            src/core/commands/cmd_window_monitor_to.cpp
            src/core/commands/cmd_window_monitor.cpp
            src/core/commands/cmd_window_cling_to_left.cpp
            src/core/commands/cmd_window_cling_to_right.cpp
            src/core/commands/cmd_window_cling_to_top.cpp
            src/core/commands/cmd_window_cling_to_bottom.cpp
            src/core/commands/cmd_window_close.cpp
            src/core/commands/cmd_window_toggle_top_most.cpp
            src/core/commands/cmd_window_identify.cpp
            src/core/commands/cmd_window_set_alpha.cpp
            src/core/commands/cmd_window_redraw.cpp
            src/core/commands/cmd_window_resize_to.cpp
            src/core/commands/cmd_mouse_move.cpp
            src/core/commands/cmd_mouse_wheel.cpp
            src/core/commands/cmd_clipboard_change_case.cpp
            src/core/commands/cmd_clipboard_upcase_word.cpp
            src/core/commands/cmd_clipboard_downcase_word.cpp
            src/core/commands/cmd_clipboard_copy.cpp
            src/core/commands/cmd_emacs_edit_kill_line_pred.cpp
            src/core/commands/cmd_emacs_edit_kill_line_func.cpp
            src/core/commands/cmd_log_clear.cpp
            src/core/commands/cmd_recenter.cpp
            src/core/commands/cmd_direct_sstp.cpp
            src/core/commands/cmd_plugin.cpp
            src/core/commands/cmd_set_ime_status.cpp
            src/core/commands/cmd_set_ime_string.cpp
            src/core/commands/cmd_mouse_hook.cpp
            src/core/commands/cmd_cancel_prefix.cpp
            src/core/commands/cmd_metrics.cpp
            src/utils/metrics.cpp
        )

        add_executable(yamy_keyremap_test
            src/tests/platform/test_main_linux.cpp
            src/tests/platform/key_remap_linux_test.cpp
            src/tests/platform/window_context_keymap_test.cpp
            tests/integration/key_remapping_integration_test.cpp
            ${KEYREMAP_TEST_CORE_SOURCES}
            ${KEYREMAP_TEST_CMD_SOURCES}
            src/tests/googletest/src/gtest-all.cc
        )

        target_include_directories(yamy_keyremap_test PRIVATE
            ${GTEST_DIR}/include
            ${GTEST_DIR}
            src
            src/core
            src/core/platform
            src/core/engine
            src/core/settings
            src/core/input
            src/core/window
            src/core/functions
            src/core/commands
            src/sync
            src/misc
            src/utils
            src/app
            src/ui
            src/platform/linux
            tests/integration
        )

        # Enable AUTOMOC for Qt-based sources (ConfigWatcher uses QObject)
        set_target_properties(yamy_keyremap_test PROPERTIES AUTOMOC ON)

        target_link_libraries(yamy_keyremap_test PRIVATE
            pthread
            ${X11_LIBRARIES}
            ${XRANDR_LIBRARIES}
            Qt6::Core
        )

        add_test(NAME yamy_keyremap_test COMMAND yamy_keyremap_test)

        # -----------------------------------------------------------------------------
        # Target: yamy_leak_test (Memory Leak Detection Tests)
        # Tests for memory leaks in platform components using AddressSanitizer
        # Run with: cmake -B build -DENABLE_ASAN=ON && cmake --build build
        # -----------------------------------------------------------------------------
        set(LEAK_TEST_SOURCES
            tests/leak_test.cpp
            src/platform/linux/window_system_linux.cpp
            src/platform/linux/window_system_linux_queries.cpp
            src/platform/linux/window_system_linux_manipulation.cpp
            src/platform/linux/window_system_linux_hierarchy.cpp
            src/platform/linux/window_system_linux_mouse.cpp
            src/platform/linux/window_system_linux_monitor.cpp
            src/platform/linux/x11_connection.cpp
            src/platform/linux/input_injector_linux.cpp
            src/platform/linux/input_hook_linux.cpp
            src/platform/linux/input_driver_linux.cpp
            src/platform/linux/device_manager_linux.cpp
            src/platform/linux/keycode_mapping.cpp
            src/utils/metrics.cpp
            src/core/logger/journey_logger.cpp
        )

        add_executable(yamy_leak_test
            ${LEAK_TEST_SOURCES}
            src/tests/googletest/src/gtest-all.cc
        )

        target_include_directories(yamy_leak_test PRIVATE
            ${GTEST_DIR}/include
            ${GTEST_DIR}
            src
            src/core
            src/core/platform
            src/core/input
            src/utils
            src/platform/linux
        )

        target_link_libraries(yamy_leak_test PRIVATE
            pthread
            ${X11_LIBRARIES}
            ${XRANDR_LIBRARIES}
            ${UDEV_LIBRARIES}
        )

        add_test(NAME yamy_leak_test COMMAND yamy_leak_test)

        # Add a custom target for running leak tests with ASAN
        if(ENABLE_ASAN)
            add_custom_target(run_leak_test
                COMMAND ${CMAKE_CTEST_COMMAND} -R yamy_leak_test --output-on-failure
                DEPENDS yamy_leak_test
                COMMENT "Running memory leak tests with AddressSanitizer"
            )
        endif()

        # -----------------------------------------------------------------------------
        # Target: yamy_event_processor_ut (EventProcessor Unit Tests)
        # Unit tests for Layer 1 (evdevToYamyKeyCode) and Layer 2 (applySubstitution)
        # Part of tasks 3.1 and 3.2 in key-remapping-consistency spec
        # -----------------------------------------------------------------------------
        set(EVENT_PROCESSOR_UT_SOURCES
            tests/test_event_processor_ut.cpp
            src/platform/linux/keycode_mapping.cpp
            src/core/engine/engine_event_processor.cpp
            src/core/engine/modifier_key_handler.cpp
            src/core/logger/journey_logger.cpp
        )

        add_executable(yamy_event_processor_ut
            ${EVENT_PROCESSOR_UT_SOURCES}
            src/tests/googletest/src/gtest-all.cc
        )

        target_include_directories(yamy_event_processor_ut PRIVATE
            ${GTEST_DIR}/include
            ${GTEST_DIR}
            src
            src/core
            src/core/engine
            src/platform/linux
            src/utils
        )

        target_compile_definitions(yamy_event_processor_ut PRIVATE
            YAMY_UNIT_TEST
        )

        target_link_libraries(yamy_event_processor_ut PRIVATE
            pthread
        )

        add_test(NAME yamy_event_processor_ut COMMAND yamy_event_processor_ut)

        # -----------------------------------------------------------------------------
        # Target: yamy_event_processor_it (EventProcessor Integration Tests)
        # Integration tests for complete Layer 1→2→3 composition
        # Part of task 3.4 in key-remapping-consistency spec
        # -----------------------------------------------------------------------------
        set(EVENT_PROCESSOR_IT_SOURCES
            tests/test_event_processor_it.cpp
            src/platform/linux/keycode_mapping.cpp
            src/core/engine/engine_event_processor.cpp
            src/core/engine/modifier_key_handler.cpp
            src/core/logger/journey_logger.cpp
        )

        add_executable(yamy_event_processor_it
            ${EVENT_PROCESSOR_IT_SOURCES}
            src/tests/googletest/src/gtest-all.cc
        )

        target_include_directories(yamy_event_processor_it PRIVATE
            ${GTEST_DIR}/include
            ${GTEST_DIR}
            src
            src/core
            src/core/engine
            src/platform/linux
            src/utils
        )

        target_compile_definitions(yamy_event_processor_it PRIVATE
            YAMY_UNIT_TEST
        )

        target_link_libraries(yamy_event_processor_it PRIVATE
            pthread
        )

        add_test(NAME yamy_event_processor_it COMMAND yamy_event_processor_it)

        # -----------------------------------------------------------------------------
        # Target: yamy_number_modifiers_test (Number Modifiers Unit Tests)
        # Unit tests for ModifierKeyHandler hold/tap detection and state machine
        # Part of task 4.6 in key-remapping-consistency spec
        # -----------------------------------------------------------------------------
        set(NUMBER_MODIFIERS_TEST_SOURCES
            tests/test_number_modifiers.cpp
            src/core/engine/modifier_key_handler.cpp
        )

        add_executable(yamy_number_modifiers_test
            ${NUMBER_MODIFIERS_TEST_SOURCES}
            src/tests/googletest/src/gtest-all.cc
        )

        target_include_directories(yamy_number_modifiers_test PRIVATE
            ${GTEST_DIR}/include
            ${GTEST_DIR}
            src
            src/core
            src/core/engine
            src/utils
        )

        target_compile_definitions(yamy_number_modifiers_test PRIVATE
            YAMY_UNIT_TEST
        )

        target_link_libraries(yamy_number_modifiers_test PRIVATE
            pthread
        )

        add_test(NAME yamy_number_modifiers_test COMMAND yamy_number_modifiers_test)

        # -----------------------------------------------------------------------------
        # Target: yamy_modifier_state_test (ModifierState Modal Modifier Tests)
        # Unit tests for ModifierState modal modifier functionality (mod0-mod19)
        # Part of task 1 in modal-modifier-remapping spec
        # -----------------------------------------------------------------------------
        set(MODIFIER_STATE_TEST_SOURCES
            tests/test_modifier_state.cpp
            src/core/input/modifier_state.cpp
            src/core/input/keyboard.cpp
            src/utils/stringtool.cpp
            src/tests/platform/test_main_linux.cpp
        )

        add_executable(yamy_modifier_state_test
            ${MODIFIER_STATE_TEST_SOURCES}
            src/tests/googletest/src/gtest-all.cc
        )

        target_include_directories(yamy_modifier_state_test PRIVATE
            ${GTEST_DIR}/include
            ${GTEST_DIR}
            src
            src/core
            src/core/input
            src/utils
        )

        target_compile_definitions(yamy_modifier_state_test PRIVATE
            YAMY_UNIT_TEST
        )

        target_link_libraries(yamy_modifier_state_test PRIVATE
            pthread
        )

        add_test(NAME yamy_modifier_state_test COMMAND yamy_modifier_state_test)

        # -----------------------------------------------------------------------------
        # Target: yamy_event_processor_modal_test (EventProcessor Modal Integration Tests)
        # Integration tests for EventProcessor with modal modifier detection
        # Part of task 7 in modal-modifier-remapping spec
        # -----------------------------------------------------------------------------
        set(EVENT_PROCESSOR_MODAL_TEST_SOURCES
            tests/test_event_processor_modal.cpp
            src/platform/linux/keycode_mapping.cpp
            src/core/engine/engine_event_processor.cpp
            src/core/engine/modifier_key_handler.cpp
            src/core/input/modifier_state.cpp
            src/core/input/keyboard.cpp
            src/core/logger/journey_logger.cpp
            src/utils/stringtool.cpp
        )

        add_executable(yamy_event_processor_modal_test
            ${EVENT_PROCESSOR_MODAL_TEST_SOURCES}
            src/tests/googletest/src/gtest-all.cc
        )

        target_include_directories(yamy_event_processor_modal_test PRIVATE
            ${GTEST_DIR}/include
            ${GTEST_DIR}
            src
            src/core
            src/core/engine
            src/core/input
            src/platform/linux
            src/utils
        )

        target_compile_definitions(yamy_event_processor_modal_test PRIVATE
            YAMY_UNIT_TEST
        )

        target_link_libraries(yamy_event_processor_modal_test PRIVATE
            pthread
        )

        add_test(NAME yamy_event_processor_modal_test COMMAND yamy_event_processor_modal_test)

        # -----------------------------------------------------------------------------
        # Target: yamy_modal_e2e_test (Modal Modifier End-to-End Tests)
        # End-to-end integration tests with mock evdev device
        # Part of task 11 in modal-modifier-remapping spec
        # -----------------------------------------------------------------------------
        set(MODAL_E2E_TEST_SOURCES
            tests/test_modal_e2e.cpp
            src/platform/linux/keycode_mapping.cpp
            src/core/engine/engine_event_processor.cpp
            src/core/engine/modifier_key_handler.cpp
            src/core/input/modifier_state.cpp
            src/core/input/keyboard.cpp
            src/core/logger/journey_logger.cpp
            src/utils/stringtool.cpp
        )

        add_executable(yamy_modal_e2e_test
            ${MODAL_E2E_TEST_SOURCES}
            src/tests/googletest/src/gtest-all.cc
        )

        target_include_directories(yamy_modal_e2e_test PRIVATE
            ${GTEST_DIR}/include
            ${GTEST_DIR}
            src
            src/core
            src/core/engine
            src/core/input
            src/platform/linux
            src/utils
        )

        target_compile_definitions(yamy_modal_e2e_test PRIVATE
            YAMY_UNIT_TEST
        )

        target_link_libraries(yamy_modal_e2e_test PRIVATE
            pthread
        )

        add_test(NAME yamy_modal_e2e_test COMMAND yamy_modal_e2e_test)

        # -----------------------------------------------------------------------------
        # Target: benchmark_event_processor (EventProcessor Performance Benchmark)
        # Standalone performance benchmark to measure event processing latency
        # Part of task 5.5 in key-remapping-consistency spec
        # -----------------------------------------------------------------------------
        set(BENCHMARK_EVENT_PROCESSOR_SOURCES
            tests/benchmark_event_processor.cpp
            src/platform/linux/keycode_mapping.cpp
            src/core/engine/engine_event_processor.cpp
            src/core/engine/modifier_key_handler.cpp
            src/core/logger/journey_logger.cpp
        )

        add_executable(benchmark_event_processor
            ${BENCHMARK_EVENT_PROCESSOR_SOURCES}
        )

        target_include_directories(benchmark_event_processor PRIVATE
            src
            src/core
            src/core/engine
            src/platform/linux
            src/utils
        )

        target_compile_definitions(benchmark_event_processor PRIVATE
            YAMY_UNIT_TEST
        )

        target_link_libraries(benchmark_event_processor PRIVATE
            pthread
        )

        # -----------------------------------------------------------------------------
        # Target: benchmark_modal_modifier (Modal Modifier Performance Benchmark)
        # Standalone performance benchmark to measure modal modifier pipeline latency
        # Part of task 12 in modal-modifier-remapping spec
        # -----------------------------------------------------------------------------
        set(BENCHMARK_MODAL_MODIFIER_SOURCES
            tests/benchmark_modal_modifier.cpp
            src/core/engine/modifier_key_handler.cpp
            src/core/input/modifier_state.cpp
            src/core/input/keyboard.cpp
            src/core/logger/journey_logger.cpp
            src/utils/stringtool.cpp
        )

        add_executable(benchmark_modal_modifier
            ${BENCHMARK_MODAL_MODIFIER_SOURCES}
        )

        target_include_directories(benchmark_modal_modifier PRIVATE
            src
            src/core
            src/core/engine
            src/core/input
            src/platform
            src/utils
        )

        target_compile_definitions(benchmark_modal_modifier PRIVATE
            YAMY_UNIT_TEST
        )

        target_link_libraries(benchmark_modal_modifier PRIVATE
            pthread
        )

        else()
            message(STATUS "Skipping Qt5 GUI-dependent tests (Qt5 Widgets/Gui/not found)")
        endif() # Qt6Widgets_FOUND AND Qt6Gui_FOUND AND Qt6Gui_FOUND
    endif() # BUILD_LINUX_TESTING

    # -----------------------------------------------------------------------------
    # Target: yamy_regression_test (Comprehensive Regression Test Suite)
    # Aggregates all unit tests, integration tests, and platform tests
    # Run in CI with: xvfb-run -a ./build/bin/yamy_regression_test
    # Independent of BUILD_LINUX_TESTING to allow coverage builds without individual tests
    # -----------------------------------------------------------------------------
    if(BUILD_REGRESSION_TESTS)
        message(STATUS "Building comprehensive regression test suite")
        # Regression test includes all platform test sources
        set(REGRESSION_TEST_SOURCES
                # Regression suite main entry point
                tests/regression/regression_suite.cpp
                # Platform unit tests
                src/tests/platform/window_system_linux_test.cpp
                src/tests/platform/input_injector_linux_test.cpp
                src/tests/platform/input_hook_linux_test.cpp
                src/tests/platform/ipc_linux_test.cpp
                src/tests/platform/ipc_multi_instance_test.cpp
                src/tests/platform/config_manager_test.cpp
                src/tests/platform/config_validator_test.cpp
                src/tests/platform/config_metadata_test.cpp
                # UTF-8 integration tests
                tests/core/settings/setting_loader_utf8_test.cpp
                # Key remapping tests
                src/tests/platform/key_remap_linux_test.cpp
                src/tests/platform/window_context_keymap_test.cpp
                # Integration tests
                tests/integration/key_remapping_integration_test.cpp
                tests/integration/integration_suite.cpp
                # E2E tests
                tests/e2e/japanese_keyboard_layout_test.cpp
            # Leak tests (without their own main)
            # Note: leak_test.cpp has its own main, so we include individual test cases separately
        )

        # Platform sources needed for tests
        set(REGRESSION_PLATFORM_SOURCES
            src/platform/linux/window_system_linux.cpp
            src/platform/linux/window_system_linux_queries.cpp
            src/platform/linux/window_system_linux_manipulation.cpp
            src/platform/linux/window_system_linux_hierarchy.cpp
            src/platform/linux/window_system_linux_mouse.cpp
            src/platform/linux/window_system_linux_monitor.cpp
            src/platform/linux/x11_connection.cpp
            src/platform/linux/input_injector_linux.cpp
            src/platform/linux/input_hook_linux.cpp
            src/platform/linux/input_driver_linux.cpp
            src/platform/linux/device_manager_linux.cpp
            src/platform/linux/keycode_mapping.cpp
            src/platform/linux/sync_linux.cpp
            src/platform/linux/thread_linux.cpp
            src/platform/linux/hook_data_linux.cpp
            src/platform/linux/ipc_linux.cpp
            src/platform/linux/platform_paths_linux.cpp
            src/utils/metrics.cpp
        )

        # Core sources for integration tests
        set(REGRESSION_CORE_SOURCES
            src/core/input/keyboard.cpp
            src/core/input/keymap.cpp
            src/core/input/modifier_state.cpp
            src/core/input/vkeytable.cpp
            src/core/settings/parser.cpp
            src/core/settings/setting.cpp
            src/core/settings/setting_loader.cpp
            src/core/settings/config_backup.cpp
            src/core/settings/config_manager.cpp
            src/core/settings/config_validator.cpp
            src/core/settings/config_metadata.cpp
            src/core/settings/session_manager.cpp
            src/core/settings/config_watcher.cpp
            src/core/functions/strexpr.cpp
            src/core/functions/function.cpp
            src/core/functions/function_creator.cpp
            src/core/window/focus.cpp
            src/core/window/layoutmanager.cpp
            src/core/window/target.cpp
            src/core/engine/engine.cpp
            src/core/engine/engine_lifecycle.cpp
            src/core/engine/engine_input.cpp
            src/core/engine/engine_modifier.cpp
            src/core/engine/engine_generator.cpp
            src/core/engine/engine_focus.cpp
            src/core/engine/engine_window.cpp
            src/core/engine/engine_setting.cpp
            src/core/engine/engine_log.cpp
            src/core/engine/engine_event_processor.cpp
            src/core/engine/modifier_key_handler.cpp
            src/core/notification_dispatcher.cpp
            src/core/logging/logger.cpp
            src/core/platform/ipc_channel_interface.cpp
            src/utils/stringtool.cpp
            src/utils/compiler_specific_func.cpp
        )

        # Command sources needed for function_creator linkage
        set(REGRESSION_CMD_SOURCES
            src/core/commands/cmd_keymap_parent.cpp
            src/core/commands/cmd_keymap_window.cpp
            src/core/commands/cmd_other_window_class.cpp
            src/core/commands/cmd_prefix.cpp
            src/core/commands/cmd_keymap.cpp
            src/core/commands/cmd_sync.cpp
            src/core/commands/cmd_toggle.cpp
            src/core/commands/cmd_edit_next_modifier.cpp
            src/core/commands/cmd_variable.cpp
            src/core/commands/cmd_repeat.cpp
            src/core/commands/cmd_undefined.cpp
            src/core/commands/cmd_ignore.cpp
            src/core/commands/cmd_post_message.cpp
            src/core/commands/cmd_wait.cpp
            src/core/commands/cmd_vk.cpp
            src/core/commands/cmd_default.cpp
            src/core/commands/cmd_keymap_prev_prefix.cpp
            src/core/commands/cmd_load_setting.cpp
            src/core/commands/cmd_shell_execute.cpp
            src/core/commands/cmd_set_foreground_window.cpp
            src/core/commands/cmd_investigate_command.cpp
            src/core/commands/cmd_mayu_dialog.cpp
            src/core/commands/cmd_describe_bindings.cpp
            src/core/commands/cmd_help_message.cpp
            src/core/commands/cmd_help_variable.cpp
            src/core/commands/cmd_window_raise.cpp
            src/core/commands/cmd_window_lower.cpp
            src/core/commands/cmd_window_minimize.cpp
            src/core/commands/cmd_window_maximize.cpp
            src/core/commands/cmd_window_h_maximize.cpp
            src/core/commands/cmd_window_v_maximize.cpp
            src/core/commands/cmd_window_hv_maximize.cpp
            src/core/commands/cmd_window_move.cpp
            src/core/commands/cmd_window_move_to.cpp
            src/core/commands/cmd_window_move_visibly.cpp
            src/core/commands/cmd_window_monitor_to.cpp
            src/core/commands/cmd_window_monitor.cpp
            src/core/commands/cmd_window_cling_to_left.cpp
            src/core/commands/cmd_window_cling_to_right.cpp
            src/core/commands/cmd_window_cling_to_top.cpp
            src/core/commands/cmd_window_cling_to_bottom.cpp
            src/core/commands/cmd_window_close.cpp
            src/core/commands/cmd_window_toggle_top_most.cpp
            src/core/commands/cmd_window_identify.cpp
            src/core/commands/cmd_window_set_alpha.cpp
            src/core/commands/cmd_window_redraw.cpp
            src/core/commands/cmd_window_resize_to.cpp
            src/core/commands/cmd_mouse_move.cpp
            src/core/commands/cmd_mouse_wheel.cpp
            src/core/commands/cmd_clipboard_change_case.cpp
            src/core/commands/cmd_clipboard_upcase_word.cpp
            src/core/commands/cmd_clipboard_downcase_word.cpp
            src/core/commands/cmd_clipboard_copy.cpp
            src/core/commands/cmd_emacs_edit_kill_line_pred.cpp
            src/core/commands/cmd_emacs_edit_kill_line_func.cpp
            src/core/commands/cmd_log_clear.cpp
            src/core/commands/cmd_recenter.cpp
            src/core/commands/cmd_direct_sstp.cpp
            src/core/commands/cmd_plugin.cpp
            src/core/commands/cmd_set_ime_status.cpp
            src/core/commands/cmd_set_ime_string.cpp
            src/core/commands/cmd_mouse_hook.cpp
            src/core/commands/cmd_cancel_prefix.cpp
            src/core/commands/cmd_metrics.cpp
        )

        add_executable(yamy_regression_test
            ${REGRESSION_TEST_SOURCES}
            ${REGRESSION_PLATFORM_SOURCES}
            ${REGRESSION_CORE_SOURCES}
            ${REGRESSION_CMD_SOURCES}
            src/tests/googletest/src/gtest-all.cc
        )

        # Enable Qt MOC for regression tests
        set_target_properties(yamy_regression_test PROPERTIES AUTOMOC ON)

        target_include_directories(yamy_regression_test PRIVATE
            ${GTEST_DIR}/include
            ${GTEST_DIR}
            src
            src/core
            src/core/platform
            src/core/engine
            src/core/settings
            src/core/input
            src/core/window
            src/core/functions
            src/core/commands
            src/sync
            src/misc
            src/utils
            src/app
            src/ui
            src/platform/linux
            tests/integration
            tests/regression
        )

        target_link_libraries(yamy_regression_test PRIVATE
            pthread
            ${X11_LIBRARIES}
            ${XRANDR_LIBRARIES}
            ${UDEV_LIBRARIES}
            Qt6::Core
            Qt6::Test
        )

        # Register with CTest
        add_test(NAME yamy_regression_test COMMAND yamy_regression_test)

        # Custom target for running regression tests
        add_custom_target(run_regression_tests
            COMMAND ${CMAKE_CTEST_COMMAND} -R yamy_regression_test --output-on-failure -V
            DEPENDS yamy_regression_test
            COMMENT "Running comprehensive regression test suite"
        )

        # Custom target for running regression tests with Xvfb (for CI)
        add_custom_target(run_regression_tests_xvfb
            COMMAND xvfb-run -a ${CMAKE_CTEST_COMMAND} -R yamy_regression_test --output-on-failure -V
            DEPENDS yamy_regression_test
            COMMENT "Running regression tests with Xvfb virtual display"
        )
    endif() # BUILD_REGRESSION_TESTS
endif() # UNIX AND NOT APPLE AND NOT WIN32

# Link Conan-provided test dependencies (Catch2, RapidCheck) to all test targets when present
set(YAMY_TEST_TARGETS
    yamy_test
    yamy_linux_test
    yamy_ui_test
    yamy_investigate_workflow_test
    yamy_log_test
    yamy_notification_history_test
    yamy_tray_test
    yamy_ipc_channel_qt_test
    yamy_ipc_protocol_test
    yamy_ipc_client_gui_test
    yamy_notification_dispatcher_test
    yamy_parser_utf8_test
    yamy_notification_prefs_test
    yamy_investigate_performance_test
    yamy_plugin_test
    yamy_keyremap_test
    yamy_leak_test
    yamy_number_modifiers_test
    yamy_modifier_state_test
    yamy_event_processor_modal_test
    yamy_modal_e2e_test
    yamy_regression_test
    yamy-test
)

foreach(test_target IN LISTS YAMY_TEST_TARGETS)
    if(TARGET ${test_target})
        target_link_libraries(${test_target} PRIVATE yamy_test_dependencies)
    endif()
endforeach()

# -----------------------------------------------------------------------------
# Example Plugins (optional, build with -DBUILD_EXAMPLES=ON)
# -----------------------------------------------------------------------------
option(BUILD_EXAMPLES "Build example plugins" OFF)
if(BUILD_EXAMPLES AND NOT WIN32)
    add_subdirectory(examples/plugins)
endif()

# -----------------------------------------------------------------------------
# Code Coverage Target (requires lcov and genhtml)
# Usage: cmake -B build -DENABLE_COVERAGE=ON && cmake --build build --target coverage
# -----------------------------------------------------------------------------
if(ENABLE_COVERAGE)
    # Find required tools
    find_program(LCOV_PATH lcov)
    find_program(GENHTML_PATH genhtml)

    if(NOT LCOV_PATH)
        message(WARNING "lcov not found - coverage target will not be available. Install with: sudo apt install lcov")
    elseif(NOT GENHTML_PATH)
        message(WARNING "genhtml not found - coverage target will not be available. Install with: sudo apt install lcov")
    else()
        message(STATUS "lcov found: ${LCOV_PATH}")
        message(STATUS "genhtml found: ${GENHTML_PATH}")

        # Coverage output directory
        set(COVERAGE_OUTPUT_DIR ${CMAKE_BINARY_DIR}/coverage)
        set(COVERAGE_INFO_FILE ${COVERAGE_OUTPUT_DIR}/coverage.info)
        set(COVERAGE_HTML_DIR ${COVERAGE_OUTPUT_DIR}/html)

        # Create coverage target
        add_custom_target(coverage
            # Step 1: Create output directory
            COMMAND ${CMAKE_COMMAND} -E make_directory ${COVERAGE_OUTPUT_DIR}

            # Step 2: Zero coverage counters
            COMMAND ${LCOV_PATH} --zerocounters --directory ${CMAKE_BINARY_DIR}

            # Step 3: Run initial capture (baseline)
            COMMAND ${LCOV_PATH} --capture --initial --directory ${CMAKE_BINARY_DIR}
                --output-file ${COVERAGE_OUTPUT_DIR}/baseline.info
                --ignore-errors mismatch,inconsistent

            # Step 4: Run tests via CTest
            COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure || true

            # Step 5: Capture coverage data after tests
            COMMAND ${LCOV_PATH} --capture --directory ${CMAKE_BINARY_DIR}
                --output-file ${COVERAGE_OUTPUT_DIR}/test.info
                --ignore-errors mismatch,inconsistent

            # Step 6: Combine baseline and test coverage
            COMMAND ${LCOV_PATH} --add-tracefile ${COVERAGE_OUTPUT_DIR}/baseline.info
                --add-tracefile ${COVERAGE_OUTPUT_DIR}/test.info
                --output-file ${COVERAGE_INFO_FILE}
                --ignore-errors mismatch,inconsistent

            # Step 7: Remove external code from coverage (system headers, tests, googletest)
            COMMAND ${LCOV_PATH} --remove ${COVERAGE_INFO_FILE}
                '/usr/*'
                '*/tests/*'
                '*/googletest/*'
                '*/gtest/*'
                '*/Qt5/*'
                --output-file ${COVERAGE_INFO_FILE}
                --ignore-errors mismatch,inconsistent,unused

            # Step 8: Generate HTML report
            COMMAND ${GENHTML_PATH} ${COVERAGE_INFO_FILE}
                --output-directory ${COVERAGE_HTML_DIR}
                --title "YAMY Code Coverage"
                --legend --show-details
                --ignore-errors inconsistent

            # Step 9: Print summary
            COMMAND ${LCOV_PATH} --summary ${COVERAGE_INFO_FILE}
                --ignore-errors mismatch,inconsistent

            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Generating code coverage report in ${COVERAGE_HTML_DIR}/index.html"
            VERBATIM
        )

        # Add coverage_check target that fails if coverage is below threshold
        set(COVERAGE_THRESHOLD 80)
        add_custom_target(coverage_check
            DEPENDS coverage
            COMMAND ${CMAKE_COMMAND} -E echo "Checking coverage threshold (${COVERAGE_THRESHOLD}%)..."
            COMMAND sh -c "COVERAGE=$$(${LCOV_PATH} --summary ${COVERAGE_INFO_FILE} 2>&1 | grep 'lines' | grep -oP '[0-9.]+(?=%)' | head -1) && echo \"Line coverage: $${COVERAGE}%\" && if [ $$(echo \"$${COVERAGE} < ${COVERAGE_THRESHOLD}\" | bc -l) -eq 1 ]; then echo \"FAILED: Coverage $${COVERAGE}% is below threshold ${COVERAGE_THRESHOLD}%\" && exit 1; else echo \"PASSED: Coverage meets threshold\"; fi"
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Verifying coverage meets ${COVERAGE_THRESHOLD}% threshold"
            VERBATIM
        )

        # Add target for cleaning coverage data
        add_custom_target(coverage_clean
            COMMAND ${CMAKE_COMMAND} -E remove_directory ${COVERAGE_OUTPUT_DIR}
            COMMAND find ${CMAKE_BINARY_DIR} -name "*.gcda" -delete || true
            COMMAND find ${CMAKE_BINARY_DIR} -name "*.gcno" -delete || true
            COMMENT "Cleaning coverage data"
            VERBATIM
        )
    endif()
endif()

# -----------------------------------------------------------------------------
# Installation Rules
# -----------------------------------------------------------------------------
if(UNIX AND NOT APPLE AND NOT WIN32)
    # Install main executable
    install(TARGETS yamy
        RUNTIME DESTINATION bin
        COMPONENT runtime
    )

    if(TARGET yamy-gui)
        install(TARGETS yamy-gui
            RUNTIME DESTINATION bin
            COMPONENT runtime
        )
    endif()

    # Install control tool
    install(TARGETS yamy-ctl
        RUNTIME DESTINATION bin
        COMPONENT runtime
    )

    # Install documentation
    install(FILES README.md
        DESTINATION share/doc/yamy
        COMPONENT documentation
    )

    # Install example keymaps
    install(DIRECTORY keymaps/
        DESTINATION share/yamy/keymaps
        COMPONENT configuration
        FILES_MATCHING PATTERN "*.mayu"
    )

    # Install template configs
    install(DIRECTORY src/resources/templates/
        DESTINATION share/yamy/templates
        COMPONENT configuration
        FILES_MATCHING PATTERN "*.mayu"
    )

    # Install icons for desktop integration
    install(FILES src/resources/icons/yamy_enabled.png
        DESTINATION share/icons/hicolor/48x48/apps
        RENAME yamy.png
        COMPONENT desktop
    )

    # Install GUI desktop icon (reuses enabled icon asset)
    install(FILES src/resources/icons/yamy_enabled.png
        DESTINATION share/icons/hicolor/48x48/apps
        RENAME yamy-gui.png
        COMPONENT desktop
    )

    # Install desktop entry
    install(FILES packaging/yamy.desktop
        DESTINATION share/applications
        COMPONENT desktop
    )

    # Install GUI desktop entry
    install(FILES resources/yamy-gui.desktop
        DESTINATION share/applications
        COMPONENT desktop
    )
endif()

if(WIN32)
    # Install main executable (Launcher)
    install(TARGETS yamy_launcher
        RUNTIME DESTINATION .
        COMPONENT runtime
    )

    # Install Engine (if built separately, though currently linked or same as launcher depending on config)
    # yamy_engine_new is the main logic
    install(TARGETS yamy_engine_new
        RUNTIME DESTINATION .
        COMPONENT runtime
    )

    # Install Hook DLL
    install(TARGETS yamy_hook
        RUNTIME DESTINATION .
        COMPONENT runtime
    )

    # Install Helper (32-bit on 64-bit OS)
    if(TARGET yamyd)
        install(TARGETS yamyd
            RUNTIME DESTINATION .
            COMPONENT runtime
        )
    else()
        # If we are in 64-bit build, try to find pre-built 32-bit helper and dlls
        if(CMAKE_SIZEOF_VOID_P EQUAL 8)
            set(YAMY_X86_BUILD_DIR "${CMAKE_SOURCE_DIR}/build/windows-msvc-release-x86/bin/Release")
            
            # Helper executable
            if(EXISTS "${YAMY_X86_BUILD_DIR}/yamyd32.exe")
                message(STATUS "Found 32-bit helper: ${YAMY_X86_BUILD_DIR}/yamyd32.exe")
                install(FILES "${YAMY_X86_BUILD_DIR}/yamyd32.exe" DESTINATION . COMPONENT runtime)
            else()
                message(WARNING "32-bit helper (yamyd32.exe) not found at ${YAMY_X86_BUILD_DIR}. Please build windows-msvc-release-x86 preset first.")
            endif()

            # 32-bit Hook DLL
            if(EXISTS "${YAMY_X86_BUILD_DIR}/yamy32.dll")
                message(STATUS "Found 32-bit hook DLL: ${YAMY_X86_BUILD_DIR}/yamy32.dll")
                install(FILES "${YAMY_X86_BUILD_DIR}/yamy32.dll" DESTINATION . COMPONENT runtime)
            else()
                 message(WARNING "32-bit hook DLL (yamy32.dll) not found at ${YAMY_X86_BUILD_DIR}.")
            endif()

            # 32-bit Main Executable (optional but requested)
            if(EXISTS "${YAMY_X86_BUILD_DIR}/yamy32.exe")
                message(STATUS "Found 32-bit engine: ${YAMY_X86_BUILD_DIR}/yamy32.exe")
                install(FILES "${YAMY_X86_BUILD_DIR}/yamy32.exe" DESTINATION . COMPONENT runtime)
            endif()
        endif()
    endif()

    # Install keymaps
    install(DIRECTORY keymaps/
        DESTINATION keymaps
        COMPONENT configuration
        FILES_MATCHING PATTERN "*.mayu"
    )

    # Install docs
    install(DIRECTORY docs/
        DESTINATION docs
        COMPONENT documentation
    )
    install(FILES docs/readme.txt DESTINATION .)

    # Install scripts (launchers)
    install(FILES 
        scripts/windows/launch.ps1
        DESTINATION .
        COMPONENT runtime
    )
endif()

# -----------------------------------------------------------------------------
# CPack Configuration for Package Generation
# -----------------------------------------------------------------------------
set(CPACK_PACKAGE_NAME "yamy")
set(CPACK_PACKAGE_VENDOR "YAMY Project")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Cross-platform keyboard remapper")
set(CPACK_PACKAGE_DESCRIPTION "YAMY is a powerful keyboard remapping tool that allows you to customize keyboard behavior, create macros, and remap keys across different applications. Originally a Windows tool (Yet Another Mado-tsukai no Yuutsu), now ported to Linux with full support for X11/XWayland.")
set(CPACK_PACKAGE_VERSION "1.0.0")
set(CPACK_PACKAGE_VERSION_MAJOR "1")
set(CPACK_PACKAGE_VERSION_MINOR "0")
set(CPACK_PACKAGE_VERSION_PATCH "0")
set(CPACK_PACKAGE_CONTACT "YAMY Project <yamy@example.com>")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/ryosukemondo/yamy")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/README.md")

# Set package file name
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")

# Component grouping
set(CPACK_COMPONENTS_ALL runtime documentation configuration desktop)
set(CPACK_COMPONENT_RUNTIME_DISPLAY_NAME "YAMY Application")
set(CPACK_COMPONENT_RUNTIME_DESCRIPTION "Main YAMY executable and control tool")
set(CPACK_COMPONENT_RUNTIME_REQUIRED ON)
set(CPACK_COMPONENT_DOCUMENTATION_DISPLAY_NAME "Documentation")
set(CPACK_COMPONENT_DOCUMENTATION_DESCRIPTION "User documentation and README")
set(CPACK_COMPONENT_CONFIGURATION_DISPLAY_NAME "Example Configurations")
set(CPACK_COMPONENT_CONFIGURATION_DESCRIPTION "Example keymap configurations")
set(CPACK_COMPONENT_DESKTOP_DISPLAY_NAME "Desktop Integration")
set(CPACK_COMPONENT_DESKTOP_DESCRIPTION "Desktop icons and application menu entry")

# -----------------------------------------------------------------------------
# DEB Package Configuration (Debian/Ubuntu)
# -----------------------------------------------------------------------------
set(CPACK_DEBIAN_PACKAGE_NAME "yamy")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "YAMY Project <yamy@example.com>")
set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")

# Dependencies (Qt5, X11, libudev)
set(CPACK_DEBIAN_PACKAGE_DEPENDS
    "libqt5core5t64 | libqt5core5a, libqt5widgets5t64 | libqt5widgets5, libqt5gui5t64 | libqt5gui5, libqt5x11extras5, libx11-6, libxrandr2, libudev1"
)

# Recommended packages
set(CPACK_DEBIAN_PACKAGE_RECOMMENDS "")

# Suggests
set(CPACK_DEBIAN_PACKAGE_SUGGESTS "")

# Control scripts
set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA
    "${CMAKE_SOURCE_DIR}/packaging/debian/postinst;${CMAKE_SOURCE_DIR}/packaging/debian/postrm"
)

# Use dpkg-shlibdeps to auto-detect dependencies (optional, slower but more accurate)
# set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)

# -----------------------------------------------------------------------------
# RPM Package Configuration (Fedora/SUSE)
# -----------------------------------------------------------------------------
set(CPACK_RPM_PACKAGE_NAME "yamy")
set(CPACK_RPM_PACKAGE_LICENSE "BSD-3-Clause")
set(CPACK_RPM_PACKAGE_GROUP "Applications/System")
set(CPACK_RPM_PACKAGE_SUMMARY "${CPACK_PACKAGE_DESCRIPTION_SUMMARY}")
set(CPACK_RPM_PACKAGE_DESCRIPTION "${CPACK_PACKAGE_DESCRIPTION}")
set(CPACK_RPM_PACKAGE_URL "${CPACK_PACKAGE_HOMEPAGE_URL}")
set(CPACK_RPM_PACKAGE_VENDOR "${CPACK_PACKAGE_VENDOR}")

# RPM release number with optional dist tag
set(CPACK_RPM_PACKAGE_RELEASE "1%{?dist}")

# Dependencies - using package names that work on both Fedora and openSUSE
# Fedora uses: qt5-qtbase, qt5-qtx11extras
# openSUSE uses: libqt5-qtbase, libqt5-qtx11extras
# We use the Fedora names with alternatives for broader compatibility
set(CPACK_RPM_PACKAGE_REQUIRES
    "qt5-qtbase >= 5.12, qt5-qtx11extras >= 5.12, libX11, libXrandr, systemd-udev"
)

# Build dependencies (for SRPM)
set(CPACK_RPM_PACKAGE_BUILDREQUIRES
    "cmake >= 3.10, gcc-c++, qt5-qtbase-devel, qt5-qtx11extras-devel, libX11-devel, libXrandr-devel, systemd-devel"
)

# Architecture
set(CPACK_RPM_PACKAGE_ARCHITECTURE "x86_64")

# Do not auto-detect dependencies (we specify them explicitly)
set(CPACK_RPM_PACKAGE_AUTOREQ OFF)
set(CPACK_RPM_PACKAGE_AUTOPROV OFF)

# RPM scriptlets (post-install and post-uninstall)
set(CPACK_RPM_POST_INSTALL_SCRIPT_FILE "${CMAKE_SOURCE_DIR}/packaging/rpm/post_install.sh")
set(CPACK_RPM_POST_UNINSTALL_SCRIPT_FILE "${CMAKE_SOURCE_DIR}/packaging/rpm/post_uninstall.sh")

# Compression type (xz is modern and well-supported)
set(CPACK_RPM_COMPRESSION_TYPE "xz")

# Package documentation
set(CPACK_RPM_PACKAGE_DOCUMENTATION_DIRECTORY "/usr/share/doc/yamy")

# Exclude debug info (debug packages would be separate)
set(CPACK_RPM_PACKAGE_DEBUG OFF)

# File permissions handling
set(CPACK_RPM_DEFAULT_USER "root")
set(CPACK_RPM_DEFAULT_GROUP "root")

# Enable the generators
if(UNIX AND NOT APPLE)
    set(CPACK_GENERATOR "DEB;RPM;TGZ")
elseif(WIN32)
    set(CPACK_GENERATOR "ZIP")
endif()

# Include CPack module
include(CPack)

# Test tool for automated testing
add_executable(yamy-test
    src/test/yamy_test_main.cpp
)
target_link_libraries(yamy-test PRIVATE pthread)
install(TARGETS yamy-test DESTINATION bin)
