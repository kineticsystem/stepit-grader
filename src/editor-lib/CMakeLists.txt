# The editor's user interface, in Qt: the document, the renderer, the main
# window and its menu of recent files. The window uses the widgets of
# widgets-lib.
set(CMAKE_AUTOMOC ON)

add_library(editor-lib STATIC
    include/editor/Document.h
    include/editor/MainWindow.h
    include/editor/PreviewRenderer.h
    include/editor/RecentFilesMenu.h
    src/Document.cpp
    src/Icons.cpp
    src/ImageConversion.cpp
    src/MainWindow.cpp
    src/PreviewRenderer.cpp
    src/RecentFilesMenu.cpp
    src/Theme.cpp
    src/WhiteBalancePicker.cpp)
target_include_directories(editor-lib PUBLIC include)
# The icons, a light and a dark drawing of each, built into the library:
# nothing to install beside the program.
qt_add_resources(editor-lib icons
    PREFIX /
    BASE resources
    FILES
    resources/icons/light/color-picker.svg
    resources/icons/light/document-open.svg
    resources/icons/light/document-save.svg
    resources/icons/light/image-v-split.svg
    resources/icons/light/zoom-fit-best.svg
    resources/icons/light/zoom-in.svg
    resources/icons/light/zoom-original.svg
    resources/icons/light/zoom-out.svg
    resources/icons/dark/color-picker.svg
    resources/icons/dark/document-open.svg
    resources/icons/dark/document-save.svg
    resources/icons/dark/image-v-split.svg
    resources/icons/dark/zoom-fit-best.svg
    resources/icons/dark/zoom-in.svg
    resources/icons/dark/zoom-original.svg
    resources/icons/dark/zoom-out.svg)
target_link_libraries(editor-lib
    PUBLIC core-lib widgets-lib io-lib Qt6::Widgets Qt6::Concurrent Qt6::Svg)
stepit_grader_warnings(editor-lib)
