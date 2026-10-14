// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "core/CameraMatch.h"
#include "core/Color.h"
#include "core/StandardPipeline.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <memory>
#include <stdexcept>

using namespace core;

namespace {

/// A ramp of colours and tones, in the working space.
Image colourful(int size = 48)
{
    Image image{size, size};
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            float *p = image.pixel(x, y);
            p[0] = 0.02f + 0.5f * x / size;
            p[1] = 0.02f + 0.5f * y / size;
            p[2] = 0.02f + 0.25f * (x + y) / size;
        }
    }
    return image;
}

/// What a camera would make of `source` with `settings`: graded, in linear sRGB, clipped.
Image cameraJpegOf(const Image &source, const Settings &settings)
{
    const auto graded = makeStandardPipeline()->render(source, settings);
    const Matrix3 &toSrgb = color::rec2020ToSrgb();
    Image jpeg{source.width, source.height};
    for (std::size_t i = 0; i < source.pixelCount(); ++i) {
        const float *p = &graded->data[i * 3];
        for (int row = 0; row < 3; ++row) {
            jpeg.data[i * 3 + row] = std::clamp(
                static_cast<float>(
                    toSrgb[row * 3] * p[0] + toSrgb[row * 3 + 1] * p[1]
                    + toSrgb[row * 3 + 2] * p[2]),
                0.0f,
                1.0f);
        }
    }
    return jpeg;
}

Settings known()
{
    Settings settings;
    settings.setValue("Exposure2012", 0.7);
    settings.setValue("Contrast2012", 25);
    settings.setValue("Saturation", 20);
    return settings;
}

match::Options threeSliders()
{
    match::Options options;
    options.stages = {{"Test", {"Exposure2012", "Contrast2012", "Saturation"}}};
    options.maxTrials = 400;
    return options;
}

/// An effect as another does it, but taken to look at a pixel's neighbours.
class NotPointwise : public Effect
{
public:
    explicit NotPointwise(std::shared_ptr<const Effect> effect)
        : m_effect{std::move(effect)}
    {}

    std::string section() const override { return m_effect->section(); }
    std::vector<Parameter> parameters() const override { return m_effect->parameters(); }
    bool isIdentity(const Settings &settings) const override
    {
        return m_effect->isIdentity(settings);
    }
    void apply(Image &image, const Settings &settings, const RenderContext &context) const override
    {
        m_effect->apply(image, settings, context);
    }

private:
    std::shared_ptr<const Effect> m_effect;
};

/// The standard pipeline, made to render every pixel.
Pipeline renderingEveryPixel()
{
    std::vector<std::shared_ptr<const Effect>> effects;
    // Kept alive: GCC 13 does not extend a temporary's life over a range-for loop as C++23 does.
    const auto pipeline = makeStandardPipeline();
    for (const auto &effect : pipeline->effects()) {
        effects.push_back(std::make_shared<NotPointwise>(effect));
    }
    return Pipeline{effects};
}

} // namespace

// The pairs of Sharma, Wu and Dalal (2005), the reference of CIEDE2000.
TEST(CameraMatchTest, DeltaE2000MatchesItsReference)
{
    EXPECT_NEAR(match::deltaE2000({50, 2.6772f, -79.7751f}, {50, 0, -82.7485f}), 2.0425, 1e-3);
    EXPECT_NEAR(match::deltaE2000({50, 3.1571f, -77.2803f}, {50, 0, -82.7485f}), 2.8615, 1e-3);
    EXPECT_NEAR(match::deltaE2000({50, 2.8361f, -74.0200f}, {50, 0, -82.7485f}), 3.4412, 1e-3);
    EXPECT_DOUBLE_EQ(match::deltaE2000({60, 10, -20}, {60, 10, -20}), 0);
}

TEST(CameraMatchTest, LabOfWhiteAndBlack)
{
    const float white[3]{1, 1, 1};
    const float black[3]{0, 0, 0};
    EXPECT_NEAR(match::labOfLinearSrgb(white)[0], 100, 0.01);
    EXPECT_NEAR(match::labOfLinearSrgb(white)[1], 0, 0.01);
    EXPECT_NEAR(match::labOfLinearSrgb(black)[0], 0, 0.01);
    // Brighter than white is white, as on the screen.
    const float bright[3]{2, 2, 2};
    EXPECT_NEAR(match::labOfLinearSrgb(bright)[0], 100, 0.01);
}

TEST(CameraMatchTest, ASampleLeavesOutTheClippedPixelsAndKeepsAtMostItsCount)
{
    const Image source = colourful();
    Image jpeg = cameraJpegOf(source, Settings{});
    for (int x = 0; x < source.width; ++x) {
        jpeg.pixel(x, 0)[1] = 1.0f; // A clipped first row.
    }
    const auto all = match::makeSample(source, jpeg, 1.0);
    EXPECT_EQ(all.pixels.size(), static_cast<std::size_t>(source.width * (source.height - 1)));
    EXPECT_TRUE(std::ranges::none_of(all.pixels, [&](int i) { return i < source.width; }));
    EXPECT_EQ(all.target.size(), all.pixels.size());

    const auto some = match::makeSample(source, jpeg, 1.0, 100);
    EXPECT_EQ(some.pixels.size(), 100u);
    EXPECT_EQ(match::makeSample(source, jpeg, 1.0, 100).pixels, some.pixels);

    EXPECT_THROW(match::makeSample(source, Image{4, 4}, 1.0), std::invalid_argument);
}

TEST(CameraMatchTest, FindsTheGradeThatMadeTheJpeg)
{
    const Image source = colourful();
    const std::vector samples{match::makeSample(source, cameraJpegOf(source, known()), 1.0)};
    const auto result = match::fit(samples, *makeStandardPipeline(), threeSliders());

    EXPECT_GT(result.errorBefore, 5.0);
    EXPECT_LT(result.errorAfter, 0.5);
    EXPECT_FALSE(result.stopped);
    EXPECT_NEAR(result.settings.value(Parameter{"Exposure2012"}), 0.7, 0.15);
    EXPECT_NEAR(result.settings.value(Parameter{"Contrast2012"}), 25, 10);
    EXPECT_NEAR(result.settings.value(Parameter{"Saturation"}), 20, 10);
}

TEST(CameraMatchTest, FitsOverTheSettingsItIsGivenAndReturnsOnlyTheSlidersSearched)
{
    // The camera's look, and a slider the search does not move, already set on the picture.
    Settings picture;
    picture.setValue("Blacks2012", -40);
    Settings camera = known();
    camera.setValue("Blacks2012", -40);
    const Image source = colourful();
    const std::vector samples{match::makeSample(source, cameraJpegOf(source, camera), 1.0)};

    auto options = threeSliders();
    const auto alone = match::fit(samples, *makeStandardPipeline(), options);
    options.base = picture;
    const auto over = match::fit(samples, *makeStandardPipeline(), options);

    EXPECT_LT(over.errorAfter, 0.5);
    EXPECT_LT(over.errorAfter, alone.errorAfter);
    EXPECT_NEAR(over.settings.values().at("Exposure2012"), 0.7, 0.15);
    EXPECT_FALSE(over.settings.has("Blacks2012"));
}

// The search renders only the pixels it compares, and the effects before a stage's once: the
// differences it finds are those of the whole pictures rendered through every effect.
TEST(CameraMatchTest, FindsTheSameWhateverItRenders)
{
    const Image source = colourful();
    const std::vector samples{match::makeSample(source, cameraJpegOf(source, known()), 1.0)};
    auto options = threeSliders();
    // The saturation comes after the exposure and the contrast: its stage starts down the pipeline.
    options.stages = {{"Light", {"Exposure2012", "Contrast2012"}}, {"Colour", {"Saturation"}}};
    const auto fast = match::fit(samples, *makeStandardPipeline(), options);
    const auto full = match::fit(samples, renderingEveryPixel(), options);

    EXPECT_EQ(fast.settings, full.settings);
    EXPECT_DOUBLE_EQ(fast.errorBefore, full.errorBefore);
    EXPECT_DOUBLE_EQ(fast.errorAfter, full.errorAfter);
    EXPECT_EQ(fast.trials, full.trials);
    // The second stage found its slider over the first stage's grade.
    EXPECT_NEAR(fast.settings.values().at("Saturation"), 20, 10);

    // The difference of the grade found, rendered whole through every effect.
    match::Options measure;
    measure.stages = {};
    measure.base = fast.settings;
    EXPECT_DOUBLE_EQ(match::fit(samples, renderingEveryPixel(), measure).errorBefore, fast.errorAfter);
}

TEST(CameraMatchTest, StopsWhenAskedWithTheBestSoFar)
{
    const Image source = colourful();
    const std::vector samples{match::makeSample(source, cameraJpegOf(source, known()), 1.0)};
    auto options = threeSliders();
    int trials = 0;
    std::vector<double> fractions;
    options.progress = [&](const match::Progress &progress) {
        ++trials;
        fractions.push_back(progress.fraction);
    };
    options.stopped = [&trials] { return trials >= 5; };
    const auto result = match::fit(samples, *makeStandardPipeline(), options);

    EXPECT_TRUE(result.stopped);
    EXPECT_EQ(result.trials, 5);
    EXPECT_LT(result.errorAfter, result.errorBefore);
    EXPECT_TRUE(std::ranges::is_sorted(fractions));
}

TEST(CameraMatchTest, ReportsItsProgressToTheEnd)
{
    const Image source = colourful(24);
    const std::vector samples{match::makeSample(source, cameraJpegOf(source, known()), 1.0)};
    auto options = threeSliders();
    std::vector<double> fractions;
    options.progress = [&](const match::Progress &progress) {
        fractions.push_back(progress.fraction);
    };
    match::fit(samples, *makeStandardPipeline(), options);
    ASSERT_FALSE(fractions.empty());
    EXPECT_TRUE(std::ranges::is_sorted(fractions));
    EXPECT_DOUBLE_EQ(fractions.back(), 1.0);
}

TEST(CameraMatchTest, TheStandardStagesNameSlidersOfThePipeline)
{
    const auto parameters = makeStandardPipeline()->parameters();
    for (const auto &stage : match::standardStages()) {
        for (const auto &id : stage.ids) {
            EXPECT_TRUE(std::ranges::any_of(parameters, [&](const Parameter &p) {
                return p.id == id;
            })) << id;
        }
    }
    match::Options options;
    options.stages = {{"Wrong", {"NoSuchSlider"}}};
    EXPECT_THROW(match::fit({}, *makeStandardPipeline(), options), std::invalid_argument);
}
