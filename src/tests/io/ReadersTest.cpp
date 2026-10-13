// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "../testutils/TestImages.h"

#include "imageio/Error.h"
#include "imageio/ImageLoader.h"
#include "imageio/RawReader.h"
#include "imageio/TiffReader.h"

#include <rawconv/tiff_output.hpp>

#include <gtest/gtest.h>
#include <tiffio.h>

#include <cstdint>
#include <fstream>
#include <vector>

namespace fs = std::filesystem;

namespace {

/// Writes an RGB TIFF of `bits` per sample, every pixel `value`, with `icc` when given.
template<typename T>
void writeTiff(
    const fs::path &path,
    int width,
    int height,
    T value,
    int bits,
    const std::vector<std::uint8_t> &icc = {},
    int orientation = 1)
{
    TIFF *tif = TIFFOpen(path.string().c_str(), "w");
    ASSERT_NE(tif, nullptr);
    TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, width);
    TIFFSetField(tif, TIFFTAG_IMAGELENGTH, height);
    TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, 3);
    TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, bits);
    TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
    TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
    TIFFSetField(tif, TIFFTAG_ORIENTATION, orientation);
    if (!icc.empty()) {
        TIFFSetField(tif, TIFFTAG_ICCPROFILE, static_cast<uint32_t>(icc.size()), icc.data());
    }
    std::vector<T> row(static_cast<std::size_t>(width) * 3, value);
    for (int y = 0; y < height; ++y) {
        TIFFWriteScanline(tif, row.data(), y, 0);
    }
    TIFFClose(tif);
}

fs::path samples()
{
    return fs::path{RAW_SAMPLES_DIR};
}

} // namespace

TEST(TiffReaderTest, ReadsA16BitLinearRec2020TiffUnchanged)
{
    const fs::path file = testutils::outputFolder() / "linear.tif";
    writeTiff<std::uint16_t>(
        file, 8, 4, 0.18 * 65535, 16, rawconv::iccProfile(rawconv::ColorSpace::REC2020));
    const core::Image image = imageio::TiffReader{}.read(file);
    ASSERT_EQ(image.width, 8);
    ASSERT_EQ(image.height, 4);
    for (int c = 0; c < 3; ++c) {
        EXPECT_NEAR(image.pixel(3, 2)[c], 0.18f, 0.002f);
    }
}

TEST(TiffReaderTest, TakesAnUntagged8BitTiffForSrgb)
{
    const fs::path file = testutils::outputFolder() / "srgb.tif";
    writeTiff<std::uint8_t>(file, 4, 4, 118, 8);
    const core::Image image = imageio::TiffReader{}.read(file);
    // sRGB 118 is middle grey, linear 0.18.
    EXPECT_NEAR(image.pixel(0, 0)[1], 0.18f, 0.005f);
}

TEST(TiffReaderTest, TurnsTheImageUpright)
{
    const fs::path file = testutils::outputFolder() / "turned.tif";
    writeTiff<std::uint8_t>(file, 6, 2, 100, 8, {}, 6);
    const core::Image image = imageio::TiffReader{}.read(file);
    EXPECT_EQ(image.width, 2);
    EXPECT_EQ(image.height, 6);
}

TEST(TiffReaderTest, ReportsAFileThatIsNotATiff)
{
    const fs::path file = testutils::outputFolder() / "broken.tif";
    std::ofstream{file} << "not a tiff";
    EXPECT_THROW(imageio::TiffReader{}.read(file), imageio::Error);
}

TEST(ImageLoaderTest, ChoosesTheReaderByExtension)
{
    const auto loader = imageio::makeStandardImageReader();
    EXPECT_TRUE(loader->canRead("a.TIF"));
    EXPECT_TRUE(loader->canRead("a.dng"));
    EXPECT_TRUE(loader->canRead("a.CR2"));
    EXPECT_FALSE(loader->canRead("a.jpg"));
    EXPECT_THROW(loader->read("a.jpg"), imageio::Error);
}

TEST(RawReaderTest, ReadsACanonRawFile)
{
    const fs::path file = samples() / "Canon_EOS_5D_Mark_II_1.CR2";
    if (!fs::exists(file)) {
        GTEST_SKIP() << "No " << file << ": run modules/raw-converter/bin/fetch-samples.sh";
    }
    const core::Image image = imageio::RawReader{}.read(file);
    EXPECT_EQ(std::max(image.width, image.height), 5616);
    EXPECT_EQ(std::min(image.width, image.height), 3744);
    const double green = testutils::mean(image, 1, 0, 0, image.width, image.height);
    EXPECT_GT(green, 0.001);
    EXPECT_LT(green, 1.0);
}

TEST(RawReaderTest, ReadsACameraShotWithItsJpeg)
{
    const fs::path file = samples() / "Canon_EOS_5D_Mark_II_1.CR2";
    if (!fs::exists(file)) {
        GTEST_SKIP() << "No " << file << ": run modules/raw-converter/bin/fetch-samples.sh";
    }
    const imageio::CameraShot shot = imageio::readCameraShot(file);
    EXPECT_EQ(shot.image.width, imageio::RawReader{}.read(file).width);
    ASSERT_GT(shot.cameraJpeg.size(), 2u);
    // A JPEG starts with its marker, FF D8.
    EXPECT_EQ(shot.cameraJpeg[0], 0xFF);
    EXPECT_EQ(shot.cameraJpeg[1], 0xD8);
    EXPECT_GE(shot.orientation, 1);
    EXPECT_LE(shot.orientation, 8);
    EXPECT_THROW(imageio::readCameraShot("no-such-file.CR2"), imageio::Error);
}

TEST(RawReaderTest, ReadsADng)
{
    const fs::path file = samples() / "adobe" / "Canon_EOS_5D_Mark_III.dng";
    if (!fs::exists(file)) {
        GTEST_SKIP() << "No " << file << ": run modules/raw-converter/bin/fetch-samples.sh";
    }
    const core::Image image = imageio::makeStandardImageReader()->read(file);
    EXPECT_GT(image.width, 5000);
}
