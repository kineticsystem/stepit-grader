# The editor's user interface, in Qt: the document, the renderer, the main
# window and its menu of recent files. The window uses the widgets of
# widgets-lib.
set(CMAKE_AUTOMOC ON)

add_library(editor-lib STATIC
    include/editor/CameraLookUtils.h
    include/editor/ConcurrentUtils.h
    include/editor/Document.h
    include/editor/FileUtils.h
    include/editor/ImageExportUtils.h
    include/editor/MainWindow.h
    include/editor/PreviewRenderer.h
    include/editor/ProgressMonitor.h
    include/editor/RecentFilesMenu.h
    include/editor/SessionProperties.h
    include/editor/ThemeMenu.h
    include/editor/WaitingDialog.h
    include/editor/actions/BeforeAfterToggleAction.h
    include/editor/actions/LoadSettingsAction.h
    include/editor/actions/MatchCameraLookAction.h
    include/editor/actions/OpenAction.h
    include/editor/actions/PickWhiteBalanceAction.h
    include/editor/actions/SaveImageAction.h
    include/editor/mediators/DialogsMediator.h
    include/editor/mediators/DocumentPanelMediator.h
    include/editor/mediators/PreviewMediator.h
    include/editor/mediators/StatusBarMediator.h
    include/editor/viewmodel/CameraLookViewModel.h
    include/editor/viewmodel/LeavePictureViewModel.h
    include/editor/viewmodel/SaveImageViewModel.h
    include/editor/viewmodel/WhiteBalanceViewModel.h
    src/CameraLookUtils.cpp
    src/Document.cpp
    src/FileUtils.cpp
    src/IconUtils.cpp
    src/ImageConversionUtils.cpp
    src/ImageExportUtils.cpp
    src/MainWindow.cpp
    src/PreviewRenderer.cpp
    src/ProgressMonitor.cpp
    src/RecentFilesMenu.cpp
    src/ThemeMenu.cpp
    src/ThemeUtils.cpp
    src/WaitingDialog.cpp
    src/WhiteBalanceUtils.cpp
    src/actions/BeforeAfterToggleAction.cpp
    src/actions/LoadSettingsAction.cpp
    src/actions/MatchCameraLookAction.cpp
    src/actions/OpenAction.cpp
    src/actions/PickWhiteBalanceAction.cpp
    src/actions/SaveImageAction.cpp
    src/mediators/DialogsMediator.cpp
    src/mediators/DocumentPanelMediator.cpp
    src/mediators/PreviewMediator.cpp
    src/mediators/StatusBarMediator.cpp
    src/viewmodel/CameraLookViewModel.cpp
    src/viewmodel/LeavePictureViewModel.cpp
    src/viewmodel/SaveImageViewModel.cpp
    src/viewmodel/WhiteBalanceViewModel.cpp)
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
