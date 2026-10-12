# Finds Little CMS: in deps/lcms2, built by install-deps.sh, or in the system.
#
# Result variables:
#   LCMS2_FOUND, LCMS2_INCLUDE_DIR, LCMS2_LIBRARY
# Imported target:
#   LCMS2::LCMS2

find_path(LCMS2_INCLUDE_DIR
    NAMES lcms2.h
    HINTS "${STEPIT_GRADER_DEPS_DIR}/lcms2/include")
find_library(LCMS2_LIBRARY
    NAMES lcms2 liblcms2
    HINTS "${STEPIT_GRADER_DEPS_DIR}/lcms2/lib")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LCMS2
    REQUIRED_VARS LCMS2_LIBRARY LCMS2_INCLUDE_DIR)

if(LCMS2_FOUND AND NOT TARGET LCMS2::LCMS2)
    add_library(LCMS2::LCMS2 UNKNOWN IMPORTED)
    set_target_properties(LCMS2::LCMS2 PROPERTIES
        IMPORTED_LOCATION "${LCMS2_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${LCMS2_INCLUDE_DIR}")
endif()

mark_as_advanced(LCMS2_INCLUDE_DIR LCMS2_LIBRARY)
