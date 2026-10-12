cmake_minimum_required(VERSION 3.22)

project(stepit-grader
    VERSION 0.1.0
    DESCRIPTION "A photo editor for the tones and colours of a picture, with Lightroom-compatible sidecars"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include(cmake/Dependencies.cmake)

# raw-converter decodes and renders the raw files. Only its library: its
# command line tool and tests are off when it is a subproject.
add_subdirectory(modules/raw-converter EXCLUDE_FROM_ALL)

add_subdirectory(src/core-lib)
add_subdirectory(src/io-lib)
add_subdirectory(src/editor-lib)
add_subdirectory(src/editor-app)

option(STEPIT_GRADER_BUILD_TESTS "Build the tests" ON)
if(STEPIT_GRADER_BUILD_TESTS)
    enable_testing()
    add_subdirectory(src/tests)
endif()
