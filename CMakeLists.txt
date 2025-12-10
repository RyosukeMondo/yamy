cmake_minimum_required(VERSION 3.10)

# Project Definition
project(yamy LANGUAGES CXX)
if(WIN32)
    enable_language(RC)
endif()

# Standard Settings
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_UNICODE ON) # For Windows Unicode support if using CMake 3.20+, else we define UNICODE manually

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
    src/core/engine/engine_input.cpp
    src/core/engine/engine_modifier.cpp
    src/core/engine/engine_generator.cpp
    src/core/engine/engine_focus.cpp
    src/core/engine/engine_window.cpp
    src/core/engine/engine_setting.cpp
    src/core/engine/engine_log.cpp
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
    src/core/settings/setting.cpp
    src/core/settings/setting_loader.cpp
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

    add_executable(yamy_engine_new WIN32 ${ENGINE_SOURCES})
    set_target_properties(yamy_engine_new PROPERTIES OUTPUT_NAME "yamy${YAMY_ARCH_SUFFIX}")
    target_link_libraries(yamy_engine_new PRIVATE yamy_hook wtsapi32 advapi32 comctl32 shell32 ole32 shlwapi comdlg32 gdi32)

    target_include_directories(yamy_engine_new PRIVATE
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
        src/core/engine/engine_input.cpp
        src/core/engine/engine_modifier.cpp
        src/core/engine/engine_generator.cpp
        src/core/engine/engine_focus.cpp
        src/core/engine/engine_window.cpp
        src/core/engine/engine_setting.cpp
        src/core/engine/engine_log.cpp
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
        src/core/input/keyboard.cpp
        src/core/input/keymap.cpp
        src/core/input/modifier_state.cpp
        src/core/input/vkeytable.cpp
        src/core/window/layoutmanager.cpp
        src/core/window/target.cpp
        src/core/settings/parser.cpp
        src/core/settings/setting.cpp
        src/core/settings/setting_loader.cpp
        src/core/functions/strexpr.cpp
        src/utils/stringtool.cpp
    )

    # Linux platform backend sources
    set(STUB_SOURCES
        src/app/main_linux.cpp
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
    )

    # Combine Core and platform sources for Linux build
    list(APPEND STUB_SOURCES ${CORE_SOURCES})

    # Qt5 GUI for Linux (optional)
    option(BUILD_QT_GUI "Build Qt5 GUI for Linux" ON)

    if(BUILD_QT_GUI)
        # Find Qt5 packages
        find_package(Qt5 COMPONENTS Core Widgets Gui)

        if(Qt5_FOUND)
            message(STATUS "Qt5 found - building with GUI support")
            add_subdirectory(src/ui/qt)

            # Use Qt main instead of headless main
            list(REMOVE_ITEM STUB_SOURCES src/app/main_linux.cpp)
            list(APPEND STUB_SOURCES src/app/main_qt.cpp)

            add_executable(yamy_stub ${STUB_SOURCES})
            target_include_directories(yamy_stub PRIVATE ${UDEV_INCLUDE_DIRS})
            target_link_libraries(yamy_stub PRIVATE
                pthread ${X11_LIBRARIES} ${XRANDR_LIBRARIES} ${UDEV_LIBRARIES}
                yamy_qt_gui
            )
        else()
            message(WARNING "Qt5 not found - building headless version")
            add_executable(yamy_stub ${STUB_SOURCES})
            target_include_directories(yamy_stub PRIVATE ${UDEV_INCLUDE_DIRS})
            target_link_libraries(yamy_stub PRIVATE pthread ${X11_LIBRARIES} ${XRANDR_LIBRARIES} ${UDEV_LIBRARIES})
        endif()
    else()
        message(STATUS "Building without Qt5 GUI (headless mode)")
        add_executable(yamy_stub ${STUB_SOURCES})
        target_include_directories(yamy_stub PRIVATE ${UDEV_INCLUDE_DIRS})
        target_link_libraries(yamy_stub PRIVATE pthread ${X11_LIBRARIES} ${XRANDR_LIBRARIES} ${UDEV_LIBRARIES})
    endif()

    target_include_directories(yamy_stub PRIVATE
        src
        src/ui
        src/ui/qt
        src/core/platform
        src/core/engine
        src/core/settings
        src/core/input
        src/core/window
        src/core/functions
        src/core/commands
        src/platform/linux
        src/utils
        src/app
    )

    # Note: Full engine build (yamy_engine_new) is disabled on Linux until Core Refactoring
    # (Branch 2 & 10) is complete and merged.
endif()

# -----------------------------------------------------------------------------
# Verification: Check for missing sources
# -----------------------------------------------------------------------------
if(WIN32)
    if(CMAKE_HOST_WIN32)
        add_custom_command(TARGET yamy_engine_new PRE_BUILD
            COMMAND powershell -ExecutionPolicy Bypass -File ${CMAKE_SOURCE_DIR}/scripts/check_missing_sources.ps1
            COMMAND powershell -ExecutionPolicy Bypass -File ${CMAKE_SOURCE_DIR}/scripts/check_header_guards.ps1
            COMMAND powershell -ExecutionPolicy Bypass -File ${CMAKE_SOURCE_DIR}/scripts/check_encoding.ps1
            COMMENT "Running QA Checks: Sources, Header Guards, Encoding..."
            VERBATIM
        )
    elseif(CMAKE_HOST_UNIX)
        add_custom_command(TARGET yamy_engine_new PRE_BUILD
            COMMAND bash ${CMAKE_SOURCE_DIR}/scripts/check_missing_sources.sh || true
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

    if(BUILD_LINUX_TESTING)
        enable_testing()

        set(GTEST_DIR src/tests/googletest)

        # Linux platform test sources
        set(LINUX_TEST_SOURCES
            src/tests/platform/test_main_linux.cpp
            src/tests/platform/window_system_linux_test.cpp
            src/tests/platform/input_injector_linux_test.cpp
            src/tests/platform/input_hook_linux_test.cpp
            src/tests/platform/ipc_linux_test.cpp
            src/platform/linux/window_system_linux_queries.cpp
            src/platform/linux/keycode_mapping.cpp
            src/platform/linux/input_hook_linux.cpp
            src/platform/linux/device_manager_linux.cpp
            src/platform/linux/ipc_linux.cpp
        )

        add_executable(yamy_linux_test
            ${LINUX_TEST_SOURCES}
            src/tests/googletest/src/gtest-all.cc
        )

        target_include_directories(yamy_linux_test PRIVATE
            ${GTEST_DIR}/include
            ${GTEST_DIR}
            src
            src/core
            src/core/platform
            src/core/input
            src/utils
            src/platform/linux
        )

        target_link_libraries(yamy_linux_test PRIVATE
            pthread
            ${X11_LIBRARIES}
            ${UDEV_LIBRARIES}
        )

        # Add test to CTest
        add_test(NAME yamy_linux_test COMMAND yamy_linux_test)

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
            src/platform/linux/input_injector_linux.cpp
            src/platform/linux/keycode_mapping.cpp
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
        )

        add_executable(yamy_keyremap_test
            src/tests/platform/test_main_linux.cpp
            src/tests/platform/key_remap_linux_test.cpp
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
        )

        target_link_libraries(yamy_keyremap_test PRIVATE
            pthread
            ${X11_LIBRARIES}
            ${XRANDR_LIBRARIES}
        )

        add_test(NAME yamy_keyremap_test COMMAND yamy_keyremap_test)
    endif()
endif()
