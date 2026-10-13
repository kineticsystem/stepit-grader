# One test program for every library: it links them all,
# so it also catches what only shows once they are together.
find_package(GTest REQUIRED)
set(CMAKE_AUTOMOC ON)

add_executable(stepit-grader-tests
    Main.cpp
    testutils/TestImages.cpp
    core/CameraMatchTest.cpp
    core/EffectsTest.cpp
    core/EngineTest.cpp
    io/ReadersTest.cpp
    io/TiffWriterTest.cpp
    io/XmpSidecarTest.cpp
    editor/ActionsTest.cpp
    editor/CameraLookTest.cpp
    editor/CameraLookViewModelTest.cpp
    editor/ConcurrentUtilsTest.cpp
    editor/DocumentTest.cpp
    editor/FileUtilsTest.cpp
    editor/ImageExportTest.cpp
    editor/LeavePictureViewModelTest.cpp
    editor/MediatorsTest.cpp
    editor/SaveImageViewModelTest.cpp
    editor/WhiteBalanceViewModelTest.cpp
    editor/WidgetsTest.cpp
    cli/CliTest.cpp)
target_link_libraries(stepit-grader-tests PRIVATE
    editor-lib widgets-lib cli-lib rawconv::rawconv TIFF::TIFF GTest::gtest Qt6::Test)
# The camera raw samples of raw-converter, downloaded by its bin/fetch-samples.sh.
target_compile_definitions(stepit-grader-tests PRIVATE
    RAW_SAMPLES_DIR="${PROJECT_SOURCE_DIR}/modules/raw-converter/samples")
stepit_grader_warnings(stepit-grader-tests)

include(GoogleTest)
gtest_discover_tests(stepit-grader-tests
    PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    DISCOVERY_TIMEOUT 30)
