# The editor's program: only main(), which puts the parts together.
qt_add_executable(stepit-grader WIN32 MACOSX_BUNDLE src/main.cpp)
target_compile_definitions(stepit-grader PRIVATE STEPIT_GRADER_VERSION="${PROJECT_VERSION}")
target_link_libraries(stepit-grader PRIVATE editor-lib)
stepit_grader_warnings(stepit-grader)
