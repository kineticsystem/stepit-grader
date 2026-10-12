# Where the libraries are found.
#
# - Qt 6, for the editor and the tests only: from Qt6_DIR, e.g.
#   ~/Qt/6.11.0/gcc_64/lib/cmake/Qt6, or CMAKE_PREFIX_PATH.
# - LibRaw and Little CMS: from deps/, made by install-deps.sh, or from the system.
# - libtiff, pugixml and GoogleTest: from Lentigram's lib/, made by lentigram-deps,
#   when LENTIGRAM_LIB points to it, or from the system.

list(APPEND CMAKE_PREFIX_PATH "${PROJECT_SOURCE_DIR}/deps")

# Next to this repository when built on its own; a bigger project's libraries
# otherwise.
if(STEPIT_GRADER_TOP_LEVEL)
    set(LENTIGRAM_DEFAULT "${PROJECT_SOURCE_DIR}/../lentigram/lib")
else()
    set(LENTIGRAM_DEFAULT "")
endif()
set(LENTIGRAM_LIB "${LENTIGRAM_DEFAULT}" CACHE PATH
    "Lentigram's lib/ folder, whose libtiff, pugixml and GoogleTest the editor uses")
set(LENTIGRAM_PACKAGES tiff pugixml gtest)
if(LENTIGRAM_LIB AND EXISTS "${LENTIGRAM_LIB}")
    message(STATUS "Using Lentigram's libraries in ${LENTIGRAM_LIB}")
    foreach(package ${LENTIGRAM_PACKAGES})
        list(APPEND CMAKE_PREFIX_PATH "${LENTIGRAM_LIB}/${package}")
    endforeach()
endif()

if(STEPIT_GRADER_BUILD_EDITOR OR STEPIT_GRADER_BUILD_TESTS)
    find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Concurrent Svg Test)
endif()
find_package(TIFF REQUIRED)
find_package(pugixml REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LCMS2 REQUIRED IMPORTED_TARGET lcms2)
find_package(Threads REQUIRED)

# The libraries outside the build, on the run path of the programs built, so
# they start without LD_LIBRARY_PATH.
set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
list(APPEND CMAKE_BUILD_RPATH "${PROJECT_SOURCE_DIR}/deps/lib")
# Lentigram's libtiff needs Lentigram's libjpeg-turbo, with its 12-bit API, at
# link and at run time.
if(LENTIGRAM_LIB AND EXISTS "${LENTIGRAM_LIB}")
    foreach(package ${LENTIGRAM_PACKAGES} jpeg zstd zlib)
        list(APPEND CMAKE_BUILD_RPATH "${LENTIGRAM_LIB}/${package}/lib")
    endforeach()
endif()

function(stepit_grader_warnings target)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    elseif(MSVC)
        target_compile_options(${target} PRIVATE /W4)
    endif()
endfunction()
