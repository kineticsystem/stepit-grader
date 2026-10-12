# The command line: its options and the grading of many pictures. No Qt.
add_library(cli-lib STATIC
    src/Grader.cpp
    src/Options.cpp)
target_include_directories(cli-lib PUBLIC include)
target_link_libraries(cli-lib PUBLIC core-lib io-lib)
stepit_grader_warnings(cli-lib)
