# The command line program: only main(), which puts the parts together. A
# console program on every system, so it prints, even on Windows.
add_executable(stepit-grader-cli src/main.cpp)
target_compile_definitions(stepit-grader-cli PRIVATE STEPIT_GRADER_VERSION="${PROJECT_VERSION}")
target_link_libraries(stepit-grader-cli PRIVATE cli-lib)
stepit_grader_warnings(stepit-grader-cli)

if(STEPIT_GRADER_TOP_LEVEL)
    include(GNUInstallDirs)
    install(TARGETS stepit-grader-cli RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
