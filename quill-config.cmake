########## MACROS ###########################################################################
#############################################################################################

# Requires CMake > 3.15
if(${CMAKE_VERSION} VERSION_LESS "3.15")
    message(FATAL_ERROR "The 'CMakeDeps' generator only works with CMake >= 3.15")
endif()

if(quill_FIND_QUIETLY)
    set(quill_MESSAGE_MODE VERBOSE)
else()
    set(quill_MESSAGE_MODE STATUS)
endif()

include(${CMAKE_CURRENT_LIST_DIR}/cmakedeps_macros.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/quillTargets.cmake)
include(CMakeFindDependencyMacro)

check_build_type_defined()

foreach(_DEPENDENCY ${quill_FIND_DEPENDENCY_NAMES} )
    # Check that we have not already called a find_package with the transitive dependency
    if(NOT ${_DEPENDENCY}_FOUND)
        find_dependency(${_DEPENDENCY} REQUIRED ${${_DEPENDENCY}_FIND_MODE})
    endif()
endforeach()

set(quill_VERSION_STRING "2.9.2")
set(quill_INCLUDE_DIRS ${quill_INCLUDE_DIRS_DEBUG} )
set(quill_INCLUDE_DIR ${quill_INCLUDE_DIRS_DEBUG} )
set(quill_LIBRARIES ${quill_LIBRARIES_DEBUG} )
set(quill_DEFINITIONS ${quill_DEFINITIONS_DEBUG} )


# Definition of extra CMake variables from cmake_extra_variables


# Only the last installed configuration BUILD_MODULES are included to avoid the collision
foreach(_BUILD_MODULE ${quill_BUILD_MODULES_PATHS_DEBUG} )
    message(${quill_MESSAGE_MODE} "Conan: Including build module from '${_BUILD_MODULE}'")
    include(${_BUILD_MODULE})
endforeach()


