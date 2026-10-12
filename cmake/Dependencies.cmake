# Where the libraries are found.
#
# - LibRaw, Little CMS, libtiff, pugixml and GoogleTest: each in its own folder
#   of deps/, built by install-deps.sh from its own source release; or, without
#   deps/, in the system. The find modules of cmake/ look in deps/<library>
#   first; raw-converter finds LibRaw and Little CMS itself, with pkg-config.
# - Qt 6, for the editor and the tests only: from Qt6_DIR, e.g.
#   ~/Qt/6.11.0/gcc_64/lib/cmake/Qt6, or CMAKE_PREFIX_PATH.

set(STEPIT_GRADER_DEPS_DIR "${PROJECT_SOURCE_DIR}/deps")
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

# The shared libraries of deps/: their lib/ on the programs' run path, so they
# start without LD_LIBRARY_PATH, and their lib/pkgconfig, through
# CMAKE_PREFIX_PATH, for pkg-config.
set(STEPIT_GRADER_SHARED_DEPS libraw lcms2 tiff)
foreach(library ${STEPIT_GRADER_SHARED_DEPS})
    list(APPEND CMAKE_PREFIX_PATH "${STEPIT_GRADER_DEPS_DIR}/${library}")
    list(APPEND CMAKE_BUILD_RPATH "${STEPIT_GRADER_DEPS_DIR}/${library}/lib")
endforeach()
set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

if(STEPIT_GRADER_BUILD_EDITOR OR STEPIT_GRADER_BUILD_TESTS)
    find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Concurrent Svg Test)
elseif(STEPIT_GRADER_BUILD_WIDGETS)
    find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)
endif()
find_package(Threads REQUIRED)
# The engine alone needs nothing else.
if(NOT STEPIT_GRADER_ENGINE_ONLY)
    find_package(TIFF REQUIRED)
    find_package(Pugixml REQUIRED)
    find_package(LCMS2 REQUIRED)
endif()

function(stepit_grader_warnings target)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    elseif(MSVC)
        target_compile_options(${target} PRIVATE /W4)
    endif()
endfunction()
