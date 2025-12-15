# Avoid multiple calls to find_package to append duplicated properties to the targets
include_guard()########### VARIABLES #######################################################################
#############################################################################################
set(rapidcheck_FRAMEWORKS_FOUND_DEBUG "") # Will be filled later
conan_find_apple_frameworks(rapidcheck_FRAMEWORKS_FOUND_DEBUG "${rapidcheck_FRAMEWORKS_DEBUG}" "${rapidcheck_FRAMEWORK_DIRS_DEBUG}")

set(rapidcheck_LIBRARIES_TARGETS "") # Will be filled later


######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
if(NOT TARGET rapidcheck_DEPS_TARGET)
    add_library(rapidcheck_DEPS_TARGET INTERFACE IMPORTED)
endif()

set_property(TARGET rapidcheck_DEPS_TARGET
             APPEND PROPERTY INTERFACE_LINK_LIBRARIES
             $<$<CONFIG:Debug>:${rapidcheck_FRAMEWORKS_FOUND_DEBUG}>
             $<$<CONFIG:Debug>:${rapidcheck_SYSTEM_LIBS_DEBUG}>
             $<$<CONFIG:Debug>:>)

####### Find the libraries declared in cpp_info.libs, create an IMPORTED target for each one and link the
####### rapidcheck_DEPS_TARGET to all of them
conan_package_library_targets("${rapidcheck_LIBS_DEBUG}"    # libraries
                              "${rapidcheck_LIB_DIRS_DEBUG}" # package_libdir
                              "${rapidcheck_BIN_DIRS_DEBUG}" # package_bindir
                              "${rapidcheck_LIBRARY_TYPE_DEBUG}"
                              "${rapidcheck_IS_HOST_WINDOWS_DEBUG}"
                              rapidcheck_DEPS_TARGET
                              rapidcheck_LIBRARIES_TARGETS  # out_libraries_targets
                              "_DEBUG"
                              "rapidcheck"    # package_name
                              "${rapidcheck_NO_SONAME_MODE_DEBUG}")  # soname

# FIXME: What is the result of this for multi-config? All configs adding themselves to path?
set(CMAKE_MODULE_PATH ${rapidcheck_BUILD_DIRS_DEBUG} ${CMAKE_MODULE_PATH})

########## COMPONENTS TARGET PROPERTIES Debug ########################################

    ########## COMPONENT rapidcheck #############

        set(rapidcheck_rapidcheck_FRAMEWORKS_FOUND_DEBUG "")
        conan_find_apple_frameworks(rapidcheck_rapidcheck_FRAMEWORKS_FOUND_DEBUG "${rapidcheck_rapidcheck_FRAMEWORKS_DEBUG}" "${rapidcheck_rapidcheck_FRAMEWORK_DIRS_DEBUG}")

        set(rapidcheck_rapidcheck_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET rapidcheck_rapidcheck_DEPS_TARGET)
            add_library(rapidcheck_rapidcheck_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET rapidcheck_rapidcheck_DEPS_TARGET
                     APPEND PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Debug>:${rapidcheck_rapidcheck_FRAMEWORKS_FOUND_DEBUG}>
                     $<$<CONFIG:Debug>:${rapidcheck_rapidcheck_SYSTEM_LIBS_DEBUG}>
                     $<$<CONFIG:Debug>:${rapidcheck_rapidcheck_DEPENDENCIES_DEBUG}>
                     )

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'rapidcheck_rapidcheck_DEPS_TARGET' to all of them
        conan_package_library_targets("${rapidcheck_rapidcheck_LIBS_DEBUG}"
                              "${rapidcheck_rapidcheck_LIB_DIRS_DEBUG}"
                              "${rapidcheck_rapidcheck_BIN_DIRS_DEBUG}" # package_bindir
                              "${rapidcheck_rapidcheck_LIBRARY_TYPE_DEBUG}"
                              "${rapidcheck_rapidcheck_IS_HOST_WINDOWS_DEBUG}"
                              rapidcheck_rapidcheck_DEPS_TARGET
                              rapidcheck_rapidcheck_LIBRARIES_TARGETS
                              "_DEBUG"
                              "rapidcheck_rapidcheck"
                              "${rapidcheck_rapidcheck_NO_SONAME_MODE_DEBUG}")


        ########## TARGET PROPERTIES #####################################
        set_property(TARGET rapidcheck
                     APPEND PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Debug>:${rapidcheck_rapidcheck_OBJECTS_DEBUG}>
                     $<$<CONFIG:Debug>:${rapidcheck_rapidcheck_LIBRARIES_TARGETS}>
                     )

        if("${rapidcheck_rapidcheck_LIBS_DEBUG}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET rapidcheck
                         APPEND PROPERTY INTERFACE_LINK_LIBRARIES
                         rapidcheck_rapidcheck_DEPS_TARGET)
        endif()

        set_property(TARGET rapidcheck APPEND PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Debug>:${rapidcheck_rapidcheck_LINKER_FLAGS_DEBUG}>)
        set_property(TARGET rapidcheck APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Debug>:${rapidcheck_rapidcheck_INCLUDE_DIRS_DEBUG}>)
        set_property(TARGET rapidcheck APPEND PROPERTY INTERFACE_LINK_DIRECTORIES
                     $<$<CONFIG:Debug>:${rapidcheck_rapidcheck_LIB_DIRS_DEBUG}>)
        set_property(TARGET rapidcheck APPEND PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Debug>:${rapidcheck_rapidcheck_COMPILE_DEFINITIONS_DEBUG}>)
        set_property(TARGET rapidcheck APPEND PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Debug>:${rapidcheck_rapidcheck_COMPILE_OPTIONS_DEBUG}>)


    ########## AGGREGATED GLOBAL TARGET WITH THE COMPONENTS #####################
    set_property(TARGET rapidcheck::rapidcheck APPEND PROPERTY INTERFACE_LINK_LIBRARIES rapidcheck)

########## For the modules (FindXXX)
set(rapidcheck_LIBRARIES_DEBUG rapidcheck::rapidcheck)
