cmake_minimum_required(VERSION 3.22)

project(stepit-grader
    VERSION 0.1.0
    DESCRIPTION "A photo editor for the tones and colours of a picture, with XMP sidecars"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Built on its own, or as a part of a bigger project, with add_subdirectory():
# then only the command line by default, which needs no Qt, and its programs go
# where the bigger project puts its own.
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    set(STEPIT_GRADER_TOP_LEVEL ON)
else()
    set(STEPIT_GRADER_TOP_LEVEL OFF)
endif()
option(STEPIT_GRADER_BUILD_EDITOR "Build the editor, which needs Qt 6" ${STEPIT_GRADER_TOP_LEVEL})
option(STEPIT_GRADER_BUILD_TESTS "Build the tests, which need Qt 6 and GoogleTest" ${STEPIT_GRADER_TOP_LEVEL})

if(STEPIT_GRADER_TOP_LEVEL)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
endif()

include(cmake/Dependencies.cmake)

# raw-converter decodes and renders the raw files. Only its library: its
# command line tool and tests are off when it is a subproject. A bigger project
# that builds raw-converter already shares its own.
if(NOT TARGET rawconv)
    add_subdirectory(modules/raw-converter EXCLUDE_FROM_ALL)
endif()

add_subdirectory(src/core-lib)
add_subdirectory(src/io-lib)
add_subdirectory(src/cli-lib)
add_subdirectory(src/cli-app)
if(STEPIT_GRADER_BUILD_EDITOR OR STEPIT_GRADER_BUILD_TESTS)
    add_subdirectory(src/editor-lib)
endif()
if(STEPIT_GRADER_BUILD_EDITOR)
    add_subdirectory(src/editor-app)
endif()

if(STEPIT_GRADER_BUILD_TESTS)
    enable_testing()
    add_subdirectory(src/tests)
endif()
