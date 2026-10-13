// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "editor/ImageExport.h"
#include "../testutils/TestImages.h"
#include "core/StandardPipeline.h"
#include "imageio/TiffReader.h"

#include <QImage>

#include <gtest/gtest.h>

#include <filesystem>

namespace {

core::Settings oneStopBrighter()
{
    core::Settings settings;
    settings.setValue("Exposure2012", 1.0);
    return settings;
}

QString pathOf(const std::filesystem::path &path)
{
    return QString::fromStdU16String(path.u16string());
}

} // namespace

TEST(ImageExportTest, TheFormatsAreTiffsInEachColourSpaceAndAJpeg)
{
    const auto &formats = imageFormats();
    ASSERT_EQ(formats.size(), 4u);
    EXPECT_EQ(formats.front().colourSpace, imageio::ColourSpace::Srgb);
    EXPECT_FALSE(formats.front().jpeg);
    EXPECT_TRUE(formats.back().jpeg);
    EXPECT_EQ(formats.back().suffix, "jpg");
}

TEST(ImageExportTest, SavesTheGradedPictureAsATiff)
{
    const core::Image picture = testutils::uniform(32, 24, 0.1f, 0.1f, 0.1f);
    const auto path = testutils::outputFolder() / "graded.tif";
    std::filesystem::remove(path);
    const auto saved = saveImage(
        picture, oneStopBrighter(), *core::makeStandardPipeline(), pathOf(path), imageFormats()[0]);
    ASSERT_TRUE(saved.has_value()) << saved.error().toStdString();

    // Read back into the working space: the grade is in it, twice the light.
    const core::Image read = imageio::TiffReader{}.read(path);
    EXPECT_EQ(read.width, 32);
    EXPECT_EQ(read.height, 24);
    EXPECT_NEAR(read.pixel(5, 5)[1], 0.2f, 0.005f);
}

TEST(ImageExportTest, SavesTheGradedPictureAsAJpeg)
{
    const core::Image picture = testutils::uniform(32, 24, 0.1f, 0.1f, 0.1f);
    const auto path = testutils::outputFolder() / "graded.jpg";
    std::filesystem::remove(path);
    const auto saved = saveImage(
        picture, oneStopBrighter(), *core::makeStandardPipeline(), pathOf(path), imageFormats()[3]);
    ASSERT_TRUE(saved.has_value()) << saved.error().toStdString();

    const QImage read{pathOf(path)};
    ASSERT_FALSE(read.isNull());
    EXPECT_EQ(read.width(), 32);
    // Linear 0.2 is sRGB 124 of 255.
    EXPECT_NEAR(read.pixelColor(5, 5).green(), 124, 3);
}

TEST(ImageExportTest, ReportsAFileThatCannotBeWritten)
{
    const core::Image picture = testutils::uniform(8, 8, 0.1f, 0.1f, 0.1f);
    for (const ImageFormat &format : {imageFormats()[0], imageFormats()[3]}) {
        const auto saved = saveImage(
            picture,
            core::Settings{},
            *core::makeStandardPipeline(),
            "/no/such/folder/graded." + format.suffix,
            format);
        EXPECT_FALSE(saved.has_value()) << format.suffix.toStdString();
    }
}
