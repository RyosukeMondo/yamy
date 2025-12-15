########### AGGREGATED COMPONENTS AND DEPENDENCIES FOR THE MULTI CONFIG #####################
#############################################################################################

list(APPEND rapidcheck_COMPONENT_NAMES rapidcheck)
list(REMOVE_DUPLICATES rapidcheck_COMPONENT_NAMES)
if(DEFINED rapidcheck_FIND_DEPENDENCY_NAMES)
  list(APPEND rapidcheck_FIND_DEPENDENCY_NAMES )
  list(REMOVE_DUPLICATES rapidcheck_FIND_DEPENDENCY_NAMES)
else()
  set(rapidcheck_FIND_DEPENDENCY_NAMES )
endif()

########### VARIABLES #######################################################################
#############################################################################################
set(rapidcheck_PACKAGE_FOLDER_DEBUG "/home/rmondo/.conan2/p/b/rapid790066fac2dcf/p")
set(rapidcheck_BUILD_MODULES_PATHS_DEBUG )


set(rapidcheck_INCLUDE_DIRS_DEBUG "${rapidcheck_PACKAGE_FOLDER_DEBUG}/include")
set(rapidcheck_RES_DIRS_DEBUG )
set(rapidcheck_DEFINITIONS_DEBUG )
set(rapidcheck_SHARED_LINK_FLAGS_DEBUG )
set(rapidcheck_EXE_LINK_FLAGS_DEBUG )
set(rapidcheck_OBJECTS_DEBUG )
set(rapidcheck_COMPILE_DEFINITIONS_DEBUG )
set(rapidcheck_COMPILE_OPTIONS_C_DEBUG )
set(rapidcheck_COMPILE_OPTIONS_CXX_DEBUG )
set(rapidcheck_LIB_DIRS_DEBUG "${rapidcheck_PACKAGE_FOLDER_DEBUG}/lib")
set(rapidcheck_BIN_DIRS_DEBUG )
set(rapidcheck_LIBRARY_TYPE_DEBUG STATIC)
set(rapidcheck_IS_HOST_WINDOWS_DEBUG 0)
set(rapidcheck_LIBS_DEBUG rapidcheck)
set(rapidcheck_SYSTEM_LIBS_DEBUG )
set(rapidcheck_FRAMEWORK_DIRS_DEBUG )
set(rapidcheck_FRAMEWORKS_DEBUG )
set(rapidcheck_BUILD_DIRS_DEBUG )
set(rapidcheck_NO_SONAME_MODE_DEBUG FALSE)


# COMPOUND VARIABLES
set(rapidcheck_COMPILE_OPTIONS_DEBUG
    "$<$<COMPILE_LANGUAGE:CXX>:${rapidcheck_COMPILE_OPTIONS_CXX_DEBUG}>"
    "$<$<COMPILE_LANGUAGE:C>:${rapidcheck_COMPILE_OPTIONS_C_DEBUG}>")
set(rapidcheck_LINKER_FLAGS_DEBUG
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${rapidcheck_SHARED_LINK_FLAGS_DEBUG}>"
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${rapidcheck_SHARED_LINK_FLAGS_DEBUG}>"
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${rapidcheck_EXE_LINK_FLAGS_DEBUG}>")


set(rapidcheck_COMPONENTS_DEBUG rapidcheck)
########### COMPONENT rapidcheck VARIABLES ############################################

set(rapidcheck_rapidcheck_INCLUDE_DIRS_DEBUG "${rapidcheck_PACKAGE_FOLDER_DEBUG}/include")
set(rapidcheck_rapidcheck_LIB_DIRS_DEBUG "${rapidcheck_PACKAGE_FOLDER_DEBUG}/lib")
set(rapidcheck_rapidcheck_BIN_DIRS_DEBUG )
set(rapidcheck_rapidcheck_LIBRARY_TYPE_DEBUG STATIC)
set(rapidcheck_rapidcheck_IS_HOST_WINDOWS_DEBUG 0)
set(rapidcheck_rapidcheck_RES_DIRS_DEBUG )
set(rapidcheck_rapidcheck_DEFINITIONS_DEBUG )
set(rapidcheck_rapidcheck_OBJECTS_DEBUG )
set(rapidcheck_rapidcheck_COMPILE_DEFINITIONS_DEBUG )
set(rapidcheck_rapidcheck_COMPILE_OPTIONS_C_DEBUG "")
set(rapidcheck_rapidcheck_COMPILE_OPTIONS_CXX_DEBUG "")
set(rapidcheck_rapidcheck_LIBS_DEBUG rapidcheck)
set(rapidcheck_rapidcheck_SYSTEM_LIBS_DEBUG )
set(rapidcheck_rapidcheck_FRAMEWORK_DIRS_DEBUG )
set(rapidcheck_rapidcheck_FRAMEWORKS_DEBUG )
set(rapidcheck_rapidcheck_DEPENDENCIES_DEBUG )
set(rapidcheck_rapidcheck_SHARED_LINK_FLAGS_DEBUG )
set(rapidcheck_rapidcheck_EXE_LINK_FLAGS_DEBUG )
set(rapidcheck_rapidcheck_NO_SONAME_MODE_DEBUG FALSE)

# COMPOUND VARIABLES
set(rapidcheck_rapidcheck_LINKER_FLAGS_DEBUG
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${rapidcheck_rapidcheck_SHARED_LINK_FLAGS_DEBUG}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${rapidcheck_rapidcheck_SHARED_LINK_FLAGS_DEBUG}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${rapidcheck_rapidcheck_EXE_LINK_FLAGS_DEBUG}>
)
set(rapidcheck_rapidcheck_COMPILE_OPTIONS_DEBUG
    "$<$<COMPILE_LANGUAGE:CXX>:${rapidcheck_rapidcheck_COMPILE_OPTIONS_CXX_DEBUG}>"
    "$<$<COMPILE_LANGUAGE:C>:${rapidcheck_rapidcheck_COMPILE_OPTIONS_C_DEBUG}>")