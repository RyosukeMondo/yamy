# Load the debug and release variables
file(GLOB DATA_FILES "${CMAKE_CURRENT_LIST_DIR}/rapidcheck-*-data.cmake")

foreach(f ${DATA_FILES})
    include(${f})
endforeach()

# Create the targets for all the components
foreach(_COMPONENT ${rapidcheck_COMPONENT_NAMES} )
    if(NOT TARGET ${_COMPONENT})
        add_library(${_COMPONENT} INTERFACE IMPORTED)
        message(${rapidcheck_MESSAGE_MODE} "Conan: Component target declared '${_COMPONENT}'")
    endif()
endforeach()

if(NOT TARGET rapidcheck::rapidcheck)
    add_library(rapidcheck::rapidcheck INTERFACE IMPORTED)
    message(${rapidcheck_MESSAGE_MODE} "Conan: Target declared 'rapidcheck::rapidcheck'")
endif()
# Load the debug and release library finders
file(GLOB CONFIG_FILES "${CMAKE_CURRENT_LIST_DIR}/rapidcheck-Target-*.cmake")

foreach(f ${CONFIG_FILES})
    include(${f})
endforeach()