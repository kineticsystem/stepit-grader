# Finds pugixml: in deps/pugixml, built by install-deps.sh, or in the system.
#
# Result variables:
#   Pugixml_FOUND, PUGIXML_INCLUDE_DIR, PUGIXML_LIBRARY
# Imported target:
#   Pugixml::Pugixml

find_path(PUGIXML_INCLUDE_DIR
    NAMES pugixml.hpp
    HINTS "${STEPIT_GRADER_DEPS_DIR}/pugixml/include")
find_library(PUGIXML_LIBRARY
    NAMES pugixml
    HINTS "${STEPIT_GRADER_DEPS_DIR}/pugixml/lib")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Pugixml
    REQUIRED_VARS PUGIXML_LIBRARY PUGIXML_INCLUDE_DIR)

if(Pugixml_FOUND AND NOT TARGET Pugixml::Pugixml)
    add_library(Pugixml::Pugixml UNKNOWN IMPORTED)
    set_target_properties(Pugixml::Pugixml PROPERTIES
        IMPORTED_LOCATION "${PUGIXML_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${PUGIXML_INCLUDE_DIR}")
endif()

mark_as_advanced(PUGIXML_INCLUDE_DIR PUGIXML_LIBRARY)
