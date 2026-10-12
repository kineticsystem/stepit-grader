# Finds GoogleTest: in deps/gtest, built by install-deps.sh, or in the system.
#
# Result variables:
#   GTest_FOUND, GTEST_INCLUDE_DIRS, GTEST_LIBRARIES
# Imported target:
#   GTest::gtest, with the threads it needs.

find_path(GTEST_INCLUDE_DIR
    NAMES gtest/gtest.h
    HINTS "${STEPIT_GRADER_DEPS_DIR}/gtest/include")
find_library(GTEST_LIBRARY
    NAMES gtest
    HINTS "${STEPIT_GRADER_DEPS_DIR}/gtest/lib")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GTest
    REQUIRED_VARS GTEST_LIBRARY GTEST_INCLUDE_DIR)

if(GTest_FOUND)
    set(GTEST_INCLUDE_DIRS "${GTEST_INCLUDE_DIR}")
    set(GTEST_LIBRARIES "${GTEST_LIBRARY}")
    if(NOT TARGET GTest::gtest)
        find_package(Threads REQUIRED)
        add_library(GTest::gtest UNKNOWN IMPORTED)
        set_target_properties(GTest::gtest PROPERTIES
            IMPORTED_LOCATION "${GTEST_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${GTEST_INCLUDE_DIR}"
            INTERFACE_LINK_LIBRARIES Threads::Threads)
    endif()
endif()

mark_as_advanced(GTEST_INCLUDE_DIR GTEST_LIBRARY)
