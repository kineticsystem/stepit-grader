// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "../testutils/TestImages.h"

#include "imageio/ColourSpace.h"
#include "imageio/Error.h"
#include "imageio/TiffReader.h"
#include "imageio/TiffWriter.h"

#include <gtest/gtest.h>
#include <tiffio.h>

namespace fs = std::filesystem;
using imageio::ColourSpace;

namespace {

/// A picture of a few colours, inside every colour space written, sRGB included.
core::Image colours()
{
    core::Image image = testutils::uniform(16, 8, 0.18f, 0.18f, 0.18f);
    for (int x = 0; x < 16; ++x) {
        float *p = image.pixel(x, 7);
        // Muted colours: Rec. 2020 is wider than sRGB, and a saturated one would be clipped.
        p[0] = 0.15f + x * 0.01f;
        p[1] = 0.2f;
        p[2] = 0.3f - x * 0.01f;
    }
    return image;
}

} // namespace

class TiffWriterRoundTrip : public ::testing::TestWithParam<ColourSpace>
{};

// Written in a colour space, read back through its embedded profile: the same picture.
TEST_P(TiffWriterRoundTrip, ReadsBackTheSameColours)
{
    const fs::path file = testutils::outputFolder() / "graded.tif";
    const core::Image image = colours();
    imageio::TiffWriter{{GetParam(), true}}.write(image, file);

    TIFF *tif = TIFFOpen(file.string().c_str(), "r");
    ASSERT_NE(tif, nullptr);
    uint16_t bits = 0;
    uint32_t iccSize = 0;
    void *icc = nullptr;
    TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &bits);
    EXPECT_EQ(bits, 16);
    EXPECT_TRUE(TIFFGetField(tif, TIFFTAG_ICCPROFILE, &iccSize, &icc));
    EXPECT_GT(iccSize, 100u);
    TIFFClose(tif);

    const core::Image back = imageio::TiffReader{}.read(file);
    ASSERT_EQ(back.width, image.width);
    ASSERT_EQ(back.height, image.height);
    // 16 bits, through two profiles: within half a percent.
    EXPECT_LT(testutils::maxDifference(back, image), 0.005f);
}

INSTANTIATE_TEST_SUITE_P(
    EveryColourSpace,
    TiffWriterRoundTrip,
    ::testing::Values(
        ColourSpace::Srgb, ColourSpace::AdobeRgb, ColourSpace::ProPhoto, ColourSpace::LinearRec2020),
    [](const auto &info) {
        std::string name = imageio::nameOf(info.param);
        std::erase(name, '-');
        return name;
    });

TEST(TiffWriterTest, ClipsWhatIsAboveWhite)
{
    const fs::path file = testutils::outputFolder() / "bright.tif";
    imageio::TiffWriter{}.write(testutils::uniform(4, 4, 3.0f, 3.0f, 3.0f), file);
    EXPECT_NEAR(imageio::TiffReader{}.read(file).pixel(0, 0)[1], 1.0f, 0.002f);
}

TEST(TiffWriterTest, ReplacesAFileWholeAndLeavesNoTemporaryFile)
{
    const fs::path folder = testutils::outputFolder();
    const fs::path file = folder / "graded.tif";
    imageio::TiffWriter{}.write(testutils::uniform(4, 4, 0.1f, 0.1f, 0.1f), file);
    imageio::TiffWriter{}.write(testutils::uniform(8, 2, 0.5f, 0.5f, 0.5f), file);
    EXPECT_EQ(imageio::TiffReader{}.read(file).width, 8);
    EXPECT_EQ(std::distance(fs::directory_iterator{folder}, fs::directory_iterator{}), 1);
}

TEST(TiffWriterTest, ReportsAFolderThatDoesNotExist)
{
    EXPECT_THROW(
        imageio::TiffWriter{}
            .write(testutils::uniform(4, 4, 0.1f, 0.1f, 0.1f), "/no/such/folder/a.tif"),
        imageio::Error);
}

TEST(ColourSpaceTest, NamesRoundTrip)
{
    for (const std::string &name : imageio::colourSpaceNames()) {
        const auto space = imageio::colourSpaceNamed(name);
        ASSERT_TRUE(space) << name;
        EXPECT_EQ(imageio::nameOf(*space), name);
    }
    EXPECT_FALSE(imageio::colourSpaceNamed("cmyk"));
}
