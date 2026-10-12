# The editor's program: only main(), which puts the parts together.
qt_add_executable(stepit-grader WIN32 MACOSX_BUNDLE src/main.cpp)
target_compile_definitions(stepit-grader PRIVATE STEPIT_GRADER_VERSION="${PROJECT_VERSION}")
target_link_libraries(stepit-grader PRIVATE editor-lib)
stepit_grader_warnings(stepit-grader)

# The application's icon, for the window and the desktop.
qt_add_resources(stepit-grader app-icon
    PREFIX /
    BASE resources
    FILES resources/stepit-grader.png)

# Installed with its desktop file and icon on Linux, e.g. into an AppImage.
if(STEPIT_GRADER_TOP_LEVEL)
    include(GNUInstallDirs)
    install(TARGETS stepit-grader
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        BUNDLE DESTINATION .)
    if(UNIX AND NOT APPLE)
        install(FILES resources/stepit-grader.desktop
            DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
        install(FILES resources/stepit-grader.png
            DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/256x256/apps)
    endif()
endif()
