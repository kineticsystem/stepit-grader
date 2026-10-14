cmake_minimum_required(VERSION 3.22)

# The oldest macOS StepIt Grader runs on, before project(), which reads it. The
# only place it is set: install-deps.sh and setup/mac read it from here. A
# bigger project that adds StepIt Grader sets its own.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "26.0" CACHE STRING "The oldest macOS to run on")
endif()

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
# The engine alone, for a bigger project that grades its own pictures:
# stepit-grader::core and, with Qt 6 Widgets, the panel of sliders,
# stepit-grader::widgets. No readers, no programs, nothing of deps/.
option(STEPIT_GRADER_ENGINE_ONLY "Build only the engine and its widgets" OFF)
option(STEPIT_GRADER_BUILD_WIDGETS "Build the widgets, which need Qt 6 Widgets" ${STEPIT_GRADER_TOP_LEVEL})
if(STEPIT_GRADER_ENGINE_ONLY)
    set(STEPIT_GRADER_BUILD_EDITOR OFF)
    set(STEPIT_GRADER_BUILD_TESTS OFF)
elseif(STEPIT_GRADER_BUILD_EDITOR OR STEPIT_GRADER_BUILD_TESTS)
    set(STEPIT_GRADER_BUILD_WIDGETS ON)
endif()

if(STEPIT_GRADER_TOP_LEVEL)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
endif()

include(cmake/Dependencies.cmake)

add_subdirectory(src/core-lib)
if(STEPIT_GRADER_BUILD_WIDGETS)
    add_subdirectory(src/widgets-lib)
endif()
if(STEPIT_GRADER_ENGINE_ONLY)
    return()
endif()

# raw-converter decodes and renders the raw files. Only its library: its
# command line tool and tests are off when it is a subproject. A bigger project
# that builds raw-converter already shares its own.
if(NOT TARGET rawconv)
    add_subdirectory(modules/raw-converter EXCLUDE_FROM_ALL)
    # MinGW declares POSIX's localtime_r(), which raw-converter calls, only when asked to.
    if(MINGW)
        target_compile_definitions(rawconv PRIVATE _POSIX_THREAD_SAFE_FUNCTIONS)
    endif()
endif()

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
