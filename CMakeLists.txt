cmake_minimum_required(VERSION 3.10)

# Project Definition
project(yamy LANGUAGES CXX RC)

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
endif()

# Determine Architecture for Output Names
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(YAMY_SUFFIX "64")
else()
    set(YAMY_SUFFIX "32")
endif()

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

# Determine architecture suffix for naming yamy-64-hook / yamy-32-hook
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(YAMY_ARCH_SUFFIX "64")
else()
    set(YAMY_ARCH_SUFFIX "32")
endif()

target_compile_definitions(yamy_hook PRIVATE YAMY32DLL_EXPORTS)
set_target_properties(yamy_hook PROPERTIES OUTPUT_NAME "yamy-${YAMY_ARCH_SUFFIX}-hook")

target_include_directories(yamy_hook PRIVATE
    src/utils
    src/platform/windows
)

target_link_libraries(yamy_hook PRIVATE imm32 user32)

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
endif()

# -----------------------------------------------------------------------------
# Target: yamy_engine (EXE)
# -----------------------------------------------------------------------------
set(ENGINE_SOURCES
    src/utils/compiler_specific_func.cpp
    src/ui/dlgeditsetting.cpp
    src/ui/dlginvestigate.cpp
    src/ui/dlglog.cpp
    src/ui/dlgsetting.cpp
    src/ui/dlgversion.cpp
    src/core/engine/engine.cpp
    src/core/engine/engine_lifecycle.cpp
    src/core/engine/engine_input.cpp
    src/core/engine/engine_modifier.cpp
    src/core/engine/engine_generator.cpp
    src/core/engine/engine_focus.cpp
    src/core/engine/engine_window.cpp
    src/core/engine/engine_setting.cpp
    src/core/engine/engine_log.cpp
    src/platform/windows/fixscancodemap.cpp
    src/core/window/focus.cpp
    src/core/functions/function.cpp
    src/core/functions/function_creator.cpp
    src/core/input/keyboard.cpp
    src/core/input/keymap.cpp
    src/core/window/layoutmanager.cpp
    src/app/mayu.cpp
    src/core/settings/parser.cpp
    src/platform/windows/registry.cpp
    src/platform/windows/window_system_win32.cpp
    src/platform/windows/input_injector_win32.cpp
    src/platform/windows/input_hook_win32.cpp
    src/platform/windows/input_driver_win32.cpp
    src/core/settings/setting.cpp
    src/core/settings/setting_loader.cpp
    src/core/functions/strexpr.cpp
    src/utils/stringtool.cpp
    src/core/window/target.cpp
    src/core/input/vkeytable.cpp
    src/platform/windows/windowstool.cpp
    src/ui/mayu.rc
)

add_executable(yamy_engine WIN32 ${ENGINE_SOURCES})
set_target_properties(yamy_engine PROPERTIES OUTPUT_NAME "yamy-${YAMY_ARCH_SUFFIX}-hook")
target_link_libraries(yamy_engine PRIVATE yamy_hook wtsapi32 advapi32 comctl32 shell32 ole32 shlwapi)

# Include directories
target_include_directories(yamy_engine PRIVATE
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

# -----------------------------------------------------------------------------
# Target: yamy_launcher (EXE)
# -----------------------------------------------------------------------------
add_executable(yamy_launcher WIN32
    src/app/yamy.cpp
    src/utils/stringtool.cpp
    src/ui/mayu.rc
)

set_target_properties(yamy_launcher PROPERTIES OUTPUT_NAME "yamy")

target_include_directories(yamy_launcher PRIVATE
    src/utils
    src/ui
)

# -----------------------------------------------------------------------------
# Target: yamy_test (EXE)
# -----------------------------------------------------------------------------
option(BUILD_TESTING "Build the testing tree." ON)

if(BUILD_TESTING)
    enable_testing()
    
    # Using the existing gtest source in the repo as per vcxproj
    set(GTEST_DIR src/tests/googletest)
    
    # We might need to compile gtest as a library first if not already managed
    # For simplicity, including gtest-all.cc directly as the vcxproj did
    
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
