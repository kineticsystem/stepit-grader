// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "../testutils/Fakes.h"
#include "../testutils/TestImages.h"

#include "cli/Grader.h"
#include "cli/Options.h"
#include "core/StandardPipeline.h"
#include "imageio/TiffReader.h"
#include "imageio/TiffWriter.h"

#include <gtest/gtest.h>

#include <fstream>

namespace fs = std::filesystem;

TEST(OptionsTest, ReadsEveryOption)
{
    const auto options = cli::parseOptions(
        {"-s",
         "look.xmp",
         "--output",
         "out",
         "-c",
         "adobe-rgb",
         "--compress",
         "--overwrite",
         "a.tif",
         "b.CR2"});
    ASSERT_TRUE(options) << options.error();
    EXPECT_EQ(options->settings, fs::path{"look.xmp"});
    EXPECT_EQ(options->output, fs::path{"out"});
    EXPECT_EQ(options->colourSpace, imageio::ColourSpace::AdobeRgb);
    EXPECT_TRUE(options->compress);
    EXPECT_TRUE(options->overwrite);
    EXPECT_EQ(options->inputs, (std::vector<fs::path>{"a.tif", "b.CR2"}));
}

TEST(OptionsTest, DefaultsToSrgbAndEachPicturesOwnSidecar)
{
    const auto options = cli::parseOptions({"-o", "out", "a.tif"});
    ASSERT_TRUE(options);
    EXPECT_FALSE(options->settings);
    EXPECT_EQ(options->colourSpace, imageio::ColourSpace::Srgb);
    EXPECT_FALSE(options->overwrite);
}

TEST(OptionsTest, RejectsWhatIsWrongWithAMessage)
{
    EXPECT_EQ(cli::parseOptions({"-o", "out"}).error(), "no picture given");
    EXPECT_TRUE(cli::parseOptions({"a.tif"}).error().contains("--output"));
    EXPECT_TRUE(cli::parseOptions({"-o", "out", "-c", "cmyk", "a.tif"}).error().contains("cmyk"));
    EXPECT_TRUE(cli::parseOptions({"-o", "out", "--fast", "a.tif"}).error().contains("--fast"));
    EXPECT_TRUE(cli::parseOptions({"a.tif", "-o"}).error().contains("needs a value"));
    // After --, a name that starts with a dash is a picture.
    const auto options = cli::parseOptions({"-o", "out", "--", "-odd.tif"});
    ASSERT_TRUE(options);
    EXPECT_EQ(options->inputs.front(), fs::path{"-odd.tif"});
    EXPECT_TRUE(cli::parseOptions({"--help"})->help);
}

namespace {

struct GraderTest : ::testing::Test
{
    std::shared_ptr<testutils::FakeStore> store = std::make_shared<testutils::FakeStore>();
    std::shared_ptr<const core::Pipeline> pipeline = core::makeStandardPipeline();
    cli::Grader grader{std::make_shared<testutils::FakeReader>(), store, pipeline};
    fs::path folder = testutils::outputFolder();

    cli::Options options(std::vector<fs::path> inputs)
    {
        cli::Options options;
        options.output = folder / "graded";
        options.inputs = std::move(inputs);
        return options;
    }

    float green(const fs::path &tiff) { return imageio::TiffReader{}.read(tiff).pixel(10, 10)[1]; }
};

core::Settings exposure(double stops)
{
    core::Settings settings;
    settings.setValue("Exposure2012", stops);
    return settings;
}

} // namespace

TEST_F(GraderTest, AppliesTheSettingsGivenToEveryPicture)
{
    const auto results = grader.run(options({folder / "a.tif", folder / "b.tif"}), exposure(1.0));
    ASSERT_EQ(results.size(), 2u);
    for (const auto &result : results) {
        EXPECT_TRUE(result.graded) << result.message;
        // The fake picture is middle grey: one stop up is 0.36.
        EXPECT_NEAR(green(result.output), 0.36f, 0.003f);
    }
    EXPECT_EQ(results[1].output, folder / "graded" / "b.tif");
}

TEST_F(GraderTest, WithoutSettingsUsesEachPicturesOwnSidecar)
{
    store->stored = exposure(-1.0);
    const auto own = grader.run(options({folder / "a.tif"}), std::nullopt);
    ASSERT_TRUE(own[0].graded);
    EXPECT_NEAR(green(own[0].output), 0.09f, 0.003f);

    store->stored.reset();
    auto opts = options({folder / "b.tif"});
    const auto none = grader.run(opts, std::nullopt);
    ASSERT_TRUE(none[0].graded);
    EXPECT_TRUE(none[0].message.contains("no sidecar"));
    EXPECT_NEAR(green(none[0].output), 0.18f, 0.003f);
}

TEST_F(GraderTest, NeverReplacesAPictureNorATiffUnlessAsked)
{
    // The output folder is the pictures' own: a.tif would be replaced by its own TIFF.
    auto same = options({folder / "a.tif"});
    same.output = folder;
    imageio::TiffWriter{}.write(testutils::uniform(4, 4, 0.5f, 0.5f, 0.5f), folder / "a.tif");
    const auto results = grader.run(same, exposure(1.0));
    EXPECT_FALSE(results[0].graded);
    EXPECT_TRUE(results[0].message.contains("replace the picture"));

    auto opts = options({folder / "b.tif"});
    EXPECT_TRUE(grader.run(opts, exposure(1.0))[0].graded);
    const auto again = grader.run(opts, exposure(1.0));
    EXPECT_FALSE(again[0].graded);
    EXPECT_TRUE(again[0].message.contains("--overwrite"));
    opts.overwrite = true;
    EXPECT_TRUE(grader.run(opts, exposure(1.0))[0].graded);
}

TEST_F(GraderTest, ReportsAPictureThatFailsAndGradesTheOthers)
{
    std::vector<std::size_t> progress;
    const auto results = grader.run(
        options({folder / "a.jpg", folder / "b.tif", folder / "b.CR2"}),
        exposure(0.5),
        [&](const cli::GradeResult &, std::size_t done, std::size_t total) {
            EXPECT_EQ(total, 3u);
            progress.push_back(done);
        });
    ASSERT_EQ(results.size(), 3u);
    EXPECT_FALSE(results[0].graded);
    EXPECT_TRUE(results[0].message.contains("a.jpg"));
    EXPECT_TRUE(results[1].graded);
    // b.CR2 would replace b.tif's TIFF, written by this same run.
    EXPECT_FALSE(results[2].graded);
    EXPECT_TRUE(results[2].message.contains("same name"));
    EXPECT_EQ(progress, (std::vector<std::size_t>{1, 2, 3}));
}
