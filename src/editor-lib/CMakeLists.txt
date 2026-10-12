# The editor's user interface, in Qt: the reusable widgets, the document, the
# renderer and the main window.
set(CMAKE_AUTOMOC ON)

add_library(editor-lib STATIC
    include/editor/Document.h
    include/editor/MainWindow.h
    include/editor/PreviewRenderer.h
    include/editor/widgets/AdjustmentsPanel.h
    include/editor/widgets/CollapsibleSection.h
    include/editor/widgets/ImageView.h
    include/editor/widgets/ParameterSlider.h
    include/editor/widgets/RecentFilesMenu.h
    src/Document.cpp
    src/Icons.cpp
    src/ImageConversion.cpp
    src/MainWindow.cpp
    src/PreviewRenderer.cpp
    src/Sections.cpp
    src/widgets/AdjustmentsPanel.cpp
    src/widgets/CollapsibleSection.cpp
    src/widgets/ImageView.cpp
    src/widgets/ParameterSlider.cpp
    src/widgets/RecentFilesMenu.cpp
    src/WhiteBalancePicker.cpp)
target_include_directories(editor-lib PUBLIC include)
# The icons, built into the library: nothing to install beside the program.
qt_add_resources(editor-lib icons
    PREFIX /
    BASE resources
    FILES resources/icons/eyedropper.svg)
target_link_libraries(editor-lib
    PUBLIC core-lib io-lib Qt6::Widgets Qt6::Concurrent Qt6::Svg)
stepit_grader_warnings(editor-lib)
