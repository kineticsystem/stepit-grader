# Finds libtiff: in deps/tiff, built by install-deps.sh, or in the system.
#
# Result variables:
#   TIFF_FOUND, TIFF_VERSION, TIFF_INCLUDE_DIRS, TIFF_LIBRARIES
# Imported target:
#   TIFF::TIFF, with zlib, which the static libtiff of deps/ does not bring.

find_path(TIFF_INCLUDE_DIR
    NAMES tiffio.h
    HINTS "${STEPIT_GRADER_DEPS_DIR}/tiff/include")
find_library(TIFF_LIBRARY
    NAMES tiff libtiff
    HINTS "${STEPIT_GRADER_DEPS_DIR}/tiff/lib")

if(TIFF_INCLUDE_DIR AND EXISTS "${TIFF_INCLUDE_DIR}/tiffvers.h")
    file(STRINGS "${TIFF_INCLUDE_DIR}/tiffvers.h" _tiff_version REGEX "^#define TIFFLIB_VERSION_STR ")
    string(REGEX REPLACE "^.*Version ([0-9.]+).*$" "\\1" TIFF_VERSION "${_tiff_version}")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(TIFF
    REQUIRED_VARS TIFF_LIBRARY TIFF_INCLUDE_DIR
    VERSION_VAR TIFF_VERSION)

if(TIFF_FOUND)
    set(TIFF_INCLUDE_DIRS "${TIFF_INCLUDE_DIR}")
    set(TIFF_LIBRARIES "${TIFF_LIBRARY}")
    if(NOT TARGET TIFF::TIFF)
        add_library(TIFF::TIFF UNKNOWN IMPORTED)
        set_target_properties(TIFF::TIFF PROPERTIES
            IMPORTED_LOCATION "${TIFF_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${TIFF_INCLUDE_DIR}")
        # Deflate: a shared libtiff links zlib itself, a static one needs it here.
        find_package(ZLIB REQUIRED)
        set_property(TARGET TIFF::TIFF APPEND PROPERTY INTERFACE_LINK_LIBRARIES ZLIB::ZLIB)
    endif()
endif()

mark_as_advanced(TIFF_INCLUDE_DIR TIFF_LIBRARY)
