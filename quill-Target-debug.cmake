# Avoid multiple calls to find_package to append duplicated properties to the targets
include_guard()########### VARIABLES #######################################################################
#############################################################################################
set(quill_FRAMEWORKS_FOUND_DEBUG "") # Will be filled later
conan_find_apple_frameworks(quill_FRAMEWORKS_FOUND_DEBUG "${quill_FRAMEWORKS_DEBUG}" "${quill_FRAMEWORK_DIRS_DEBUG}")

set(quill_LIBRARIES_TARGETS "") # Will be filled later


######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
if(NOT TARGET quill_DEPS_TARGET)
    add_library(quill_DEPS_TARGET INTERFACE IMPORTED)
endif()

set_property(TARGET quill_DEPS_TARGET
             APPEND PROPERTY INTERFACE_LINK_LIBRARIES
             $<$<CONFIG:Debug>:${quill_FRAMEWORKS_FOUND_DEBUG}>
             $<$<CONFIG:Debug>:${quill_SYSTEM_LIBS_DEBUG}>
             $<$<CONFIG:Debug>:fmt::fmt>)

####### Find the libraries declared in cpp_info.libs, create an IMPORTED target for each one and link the
####### quill_DEPS_TARGET to all of them
conan_package_library_targets("${quill_LIBS_DEBUG}"    # libraries
                              "${quill_LIB_DIRS_DEBUG}" # package_libdir
                              "${quill_BIN_DIRS_DEBUG}" # package_bindir
                              "${quill_LIBRARY_TYPE_DEBUG}"
                              "${quill_IS_HOST_WINDOWS_DEBUG}"
                              quill_DEPS_TARGET
                              quill_LIBRARIES_TARGETS  # out_libraries_targets
                              "_DEBUG"
                              "quill"    # package_name
                              "${quill_NO_SONAME_MODE_DEBUG}")  # soname

# FIXME: What is the result of this for multi-config? All configs adding themselves to path?
set(CMAKE_MODULE_PATH ${quill_BUILD_DIRS_DEBUG} ${CMAKE_MODULE_PATH})

########## GLOBAL TARGET PROPERTIES Debug ########################################
    set_property(TARGET quill::quill
                 APPEND PROPERTY INTERFACE_LINK_LIBRARIES
                 $<$<CONFIG:Debug>:${quill_OBJECTS_DEBUG}>
                 $<$<CONFIG:Debug>:${quill_LIBRARIES_TARGETS}>
                 )

    if("${quill_LIBS_DEBUG}" STREQUAL "")
        # If the package is not declaring any "cpp_info.libs" the package deps, system libs,
        # frameworks etc are not linked to the imported targets and we need to do it to the
        # global target
        set_property(TARGET quill::quill
                     APPEND PROPERTY INTERFACE_LINK_LIBRARIES
                     quill_DEPS_TARGET)
    endif()

    set_property(TARGET quill::quill
                 APPEND PROPERTY INTERFACE_LINK_OPTIONS
                 $<$<CONFIG:Debug>:${quill_LINKER_FLAGS_DEBUG}>)
    set_property(TARGET quill::quill
                 APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                 $<$<CONFIG:Debug>:${quill_INCLUDE_DIRS_DEBUG}>)
    # Necessary to find LINK shared libraries in Linux
    set_property(TARGET quill::quill
                 APPEND PROPERTY INTERFACE_LINK_DIRECTORIES
                 $<$<CONFIG:Debug>:${quill_LIB_DIRS_DEBUG}>)
    set_property(TARGET quill::quill
                 APPEND PROPERTY INTERFACE_COMPILE_DEFINITIONS
                 $<$<CONFIG:Debug>:${quill_COMPILE_DEFINITIONS_DEBUG}>)
    set_property(TARGET quill::quill
                 APPEND PROPERTY INTERFACE_COMPILE_OPTIONS
                 $<$<CONFIG:Debug>:${quill_COMPILE_OPTIONS_DEBUG}>)

########## For the modules (FindXXX)
set(quill_LIBRARIES_DEBUG quill::quill)
