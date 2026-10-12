# Reading and writing images and keeping their settings: TIFF, camera raw and
# DNG files, and XMP sidecars. No Qt.
add_library(io-lib STATIC
    src/ColourProfiles.cpp
    src/ImageLoader.cpp
    src/ImageReader.cpp
    src/RawReader.cpp
    src/TiffReader.cpp
    src/TiffWriter.cpp
    src/XmpSidecar.cpp)
target_include_directories(io-lib PUBLIC include)
target_link_libraries(io-lib
    PUBLIC core-lib
    PRIVATE rawconv::rawconv TIFF::TIFF Pugixml::Pugixml LCMS2::LCMS2)
stepit_grader_warnings(io-lib)
