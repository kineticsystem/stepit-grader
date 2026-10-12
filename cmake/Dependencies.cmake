# Where the libraries are found.
#
# - LibRaw, Little CMS, libtiff, pugixml and GoogleTest: in deps/, made by
#   install-deps.sh from their own source releases; or, without deps/, from
#   the system.
# - Qt 6, for the editor and the tests only: from Qt6_DIR, e.g.
#   ~/Qt/6.11.0/gcc_64/lib/cmake/Qt6, or CMAKE_PREFIX_PATH.

list(APPEND CMAKE_PREFIX_PATH "${PROJECT_SOURCE_DIR}/deps")

if(STEPIT_GRADER_BUILD_EDITOR OR STEPIT_GRADER_BUILD_TESTS)
    find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Concurrent Svg Test)
endif()
find_package(TIFF REQUIRED)
# libtiff's Deflate: named for the static libtiff of deps/, which does not bring
# its own.
find_package(ZLIB REQUIRED)
find_package(pugixml REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LCMS2 REQUIRED IMPORTED_TARGET lcms2)
find_package(Threads REQUIRED)

# The shared libraries of deps/, LibRaw and Little CMS, on the run path of the
# programs built, so they start without LD_LIBRARY_PATH.
set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
list(APPEND CMAKE_BUILD_RPATH "${PROJECT_SOURCE_DIR}/deps/lib")

function(stepit_grader_warnings target)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    elseif(MSVC)
        target_compile_options(${target} PRIVATE /W4)
    endif()
endfunction()
