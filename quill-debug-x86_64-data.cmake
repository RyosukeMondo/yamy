########### AGGREGATED COMPONENTS AND DEPENDENCIES FOR THE MULTI CONFIG #####################
#############################################################################################

set(quill_COMPONENT_NAMES "")
if(DEFINED quill_FIND_DEPENDENCY_NAMES)
  list(APPEND quill_FIND_DEPENDENCY_NAMES fmt)
  list(REMOVE_DUPLICATES quill_FIND_DEPENDENCY_NAMES)
else()
  set(quill_FIND_DEPENDENCY_NAMES fmt)
endif()
set(fmt_FIND_MODE "NO_MODULE")

########### VARIABLES #######################################################################
#############################################################################################
set(quill_PACKAGE_FOLDER_DEBUG "/home/rmondo/.conan2/p/b/quillb1d610def0a9a/p")
set(quill_BUILD_MODULES_PATHS_DEBUG )


set(quill_INCLUDE_DIRS_DEBUG "${quill_PACKAGE_FOLDER_DEBUG}/include")
set(quill_RES_DIRS_DEBUG )
set(quill_DEFINITIONS_DEBUG "-DQUILL_FMT_EXTERNAL")
set(quill_SHARED_LINK_FLAGS_DEBUG )
set(quill_EXE_LINK_FLAGS_DEBUG )
set(quill_OBJECTS_DEBUG )
set(quill_COMPILE_DEFINITIONS_DEBUG "QUILL_FMT_EXTERNAL")
set(quill_COMPILE_OPTIONS_C_DEBUG )
set(quill_COMPILE_OPTIONS_CXX_DEBUG )
set(quill_LIB_DIRS_DEBUG "${quill_PACKAGE_FOLDER_DEBUG}/lib")
set(quill_BIN_DIRS_DEBUG )
set(quill_LIBRARY_TYPE_DEBUG STATIC)
set(quill_IS_HOST_WINDOWS_DEBUG 0)
set(quill_LIBS_DEBUG quill)
set(quill_SYSTEM_LIBS_DEBUG pthread)
set(quill_FRAMEWORK_DIRS_DEBUG )
set(quill_FRAMEWORKS_DEBUG )
set(quill_BUILD_DIRS_DEBUG )
set(quill_NO_SONAME_MODE_DEBUG FALSE)


# COMPOUND VARIABLES
set(quill_COMPILE_OPTIONS_DEBUG
    "$<$<COMPILE_LANGUAGE:CXX>:${quill_COMPILE_OPTIONS_CXX_DEBUG}>"
    "$<$<COMPILE_LANGUAGE:C>:${quill_COMPILE_OPTIONS_C_DEBUG}>")
set(quill_LINKER_FLAGS_DEBUG
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${quill_SHARED_LINK_FLAGS_DEBUG}>"
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${quill_SHARED_LINK_FLAGS_DEBUG}>"
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${quill_EXE_LINK_FLAGS_DEBUG}>")


set(quill_COMPONENTS_DEBUG )