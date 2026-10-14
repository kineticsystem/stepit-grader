// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "../testutils/TestImages.h"

#include "core/Color.h"
#include "core/Sampling.h"
#include "core/StandardPipeline.h"
#include "core/effects/BlacksEffect.h"
#include "core/effects/ColorMixerEffect.h"
#include "core/effects/ColorNoiseEffect.h"
#include "core/effects/ContrastEffect.h"
#include "core/effects/DefringeEffect.h"
#include "core/effects/DetailEffect.h"
#include "core/effects/ExposureEffect.h"
#include "core/effects/HighlightsEffect.h"
#include "core/effects/LuminanceNoiseEffect.h"
#include "core/effects/SaturationEffect.h"
#include "core/effects/ShadowsEffect.h"
#include "core/effects/SharpnessEffect.h"
#include "core/effects/StructureEffect.h"
#include "core/effects/ToneCompressionEffect.h"
#include "core/effects/ToneCurveEffect.h"
#include "core/effects/VibranceEffect.h"
#include "core/effects/WhiteBalanceEffect.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <numeric>
#include <random>
#include <set>

using namespace core;
using testutils::mean;

namespace {

Image applied(const Effect &effect, Image image, const std::string &id, double value)
{
    Settings settings;
    settings.setValue(id, value);
    effect.apply(image, settings, RenderContext{});
    return image;
}

Image colourful()
{
    Image image{64, 64};
    for (int y = 0; y < 64; ++y) {
        for (int x = 0; x < 64; ++x) {
            float *p = image.pixel(x, y);
            p[0] = 0.05f + x / 80.0f;
            p[1] = 0.05f + y / 80.0f;
            p[2] = 0.05f + (x + y) / 160.0f;
        }
    }
    return image;
}

} // namespace

// Every effect at its defaults leaves the image as it is: a new sidecar changes nothing.
TEST(EffectsTest, EveryEffectAtItsDefaultsChangesNothing)
{
    const Image original = colourful();
    const auto pipeline = makeStandardPipeline();
    for (const auto &effect : pipeline->effects()) {
        Image image = original;
        effect->apply(image, Settings{}, RenderContext{});
        EXPECT_LT(testutils::maxDifference(image, original), 1e-5f) << effect->section();
        EXPECT_TRUE(effect->isIdentity(Settings{}));
    }
}

// The camera match renders only the pixels it compares, in a row, through the pointwise effects.
TEST(EffectsTest, PointwiseEffectsGiveEachPixelTheSameWhereverItIs)
{
    const Image original = colourful();
    std::vector<std::size_t> order(original.pixelCount());
    std::iota(order.begin(), order.end(), 0);
    std::ranges::shuffle(order, std::mt19937{1});
    Image row{static_cast<int>(order.size()), 1};
    for (std::size_t i = 0; i < order.size(); ++i) {
        std::copy_n(&original.data[order[i] * 3], 3, &row.data[i * 3]);
    }

    int pointwise = 0;
    // Kept alive: GCC 13 does not extend a temporary's life over a range-for loop as C++23 does.
    const auto pipeline = makeStandardPipeline();
    for (const auto &effect : pipeline->effects()) {
        if (!effect->isPointwise()) {
            continue;
        }
        ++pointwise;
        Settings settings;
        for (const Parameter &parameter : effect->parameters()) {
            const double end = parameter.maximum > parameter.defaultValue ? parameter.maximum
                                                                          : parameter.minimum;
            settings.setValue(
                parameter.id, parameter.defaultValue + 0.3 * (end - parameter.defaultValue));
        }
        Image image = original;
        effect->apply(image, settings, RenderContext{});
        Image packed = row;
        effect->apply(packed, settings, RenderContext{});
        float worst = 0;
        for (std::size_t i = 0; i < order.size(); ++i) {
            for (int c = 0; c < 3; ++c) {
                worst = std::max(
                    worst, std::abs(packed.data[i * 3 + c] - image.data[order[i] * 3 + c]));
            }
        }
        EXPECT_LT(worst, 1e-6f) << effect->parameters().front().id;
        EXPECT_GT(testutils::maxDifference(image, original), 1e-3f)
            << effect->parameters().front().id;
    }
    EXPECT_EQ(pointwise, 8);
}

TEST(EffectsTest, ParametersHaveUniqueIdsAndDefaultsInRange)
{
    std::set<std::string> ids;
    for (const Parameter &parameter : makeStandardPipeline()->parameters()) {
        EXPECT_TRUE(ids.insert(parameter.id).second) << parameter.id;
        EXPECT_LE(parameter.minimum, parameter.defaultValue);
        EXPECT_GE(parameter.maximum, parameter.defaultValue);
    }
    EXPECT_EQ(ids.size(), 52u);
}

TEST(EffectsTest, ExposureOfOneStopDoublesTheLight)
{
    const Image image = applied(
        ExposureEffect{},
        testutils::uniform(4, 4, 0.1f, 0.2f, 0.3f),
        ExposureEffect::EXPOSURE.id,
        1.0);
    EXPECT_FLOAT_EQ(image.pixel(0, 0)[0], 0.2f);
    EXPECT_FLOAT_EQ(image.pixel(0, 0)[2], 0.6f);
}

TEST(EffectsTest, PositiveTemperatureWarmsAndPositiveTintAddsMagenta)
{
    const Image grey = testutils::uniform(4, 4, 0.18f, 0.18f, 0.18f);
    const Image warm = applied(WhiteBalanceEffect{}, grey, WhiteBalanceEffect::TEMPERATURE.id, 50);
    EXPECT_GT(warm.pixel(0, 0)[0], warm.pixel(0, 0)[2]);
    const Image cool = applied(WhiteBalanceEffect{}, grey, WhiteBalanceEffect::TEMPERATURE.id, -50);
    EXPECT_LT(cool.pixel(0, 0)[0], cool.pixel(0, 0)[2]);
    const Image magenta = applied(WhiteBalanceEffect{}, grey, WhiteBalanceEffect::TINT.id, 50);
    const float *p = magenta.pixel(0, 0);
    EXPECT_GT((p[0] + p[2]) / 2, p[1]);
}

TEST(EffectsTest, ContrastSpreadsTonesAroundMiddleGrey)
{
    const Image image = testutils::step(32, 8, 0.05f, 0.6f);
    const Image more = applied(ContrastEffect{}, image, ContrastEffect::CONTRAST.id, 100);
    EXPECT_LT(more.pixel(0, 0)[1], 0.05f);
    EXPECT_GT(more.pixel(31, 0)[1], 0.6f);
    const Image grey = applied(
        ContrastEffect{},
        testutils::uniform(4, 4, 0.18f, 0.18f, 0.18f),
        ContrastEffect::CONTRAST.id,
        100);
    EXPECT_NEAR(grey.pixel(0, 0)[1], 0.18f, 1e-4f);
}

TEST(EffectsTest, HighlightsChangeTheBrightAreaOnly)
{
    const Image image = testutils::step(200, 50, 0.02f, 1.2f);
    const Image lower = applied(HighlightsEffect{}, image, HighlightsEffect::HIGHLIGHTS.id, -100);
    EXPECT_LT(mean(lower, 1, 150, 0, 200, 50), 0.8);
    EXPECT_NEAR(mean(lower, 1, 0, 0, 50, 50), 0.02, 1e-3);
}

TEST(EffectsTest, ShadowsChangeTheDarkAreaOnly)
{
    const Image image = testutils::step(200, 50, 0.01f, 0.5f);
    const Image lifted = applied(ShadowsEffect{}, image, ShadowsEffect::SHADOWS.id, 100);
    EXPECT_GT(mean(lifted, 1, 0, 0, 50, 50), 0.025);
    EXPECT_NEAR(mean(lifted, 1, 150, 0, 200, 50), 0.5, 0.02);
}

TEST(EffectsTest, BlacksCrushOrLiftTheDarkestTonesOnly)
{
    const Image image = testutils::step(32, 8, 0.005f, 0.5f);
    const Image crushed = applied(BlacksEffect{}, image, BlacksEffect::BLACKS.id, -100);
    EXPECT_LT(crushed.pixel(0, 0)[1], 0.002f);
    EXPECT_NEAR(crushed.pixel(31, 0)[1], 0.5f, 0.025f);
    const Image lifted
        = applied(BlacksEffect{}, testutils::uniform(4, 4, 0, 0, 0), BlacksEffect::BLACKS.id, 100);
    EXPECT_GT(lifted.pixel(0, 0)[1], 0.01f);
    EXPECT_FLOAT_EQ(lifted.pixel(0, 0)[0], lifted.pixel(0, 0)[2]);
}

TEST(EffectsTest, SaturationOfMinusHundredIsBlackAndWhite)
{
    const Image image = applied(
        SaturationEffect{},
        testutils::uniform(4, 4, 0.4f, 0.1f, 0.05f),
        SaturationEffect::SATURATION.id,
        -100);
    const float *p = image.pixel(0, 0);
    EXPECT_NEAR(p[0], p[1], 1e-4f);
    EXPECT_NEAR(p[1], p[2], 1e-4f);
}

TEST(EffectsTest, VibranceChangesDullColoursMoreThanVividOnes)
{
    auto chroma = [](const Image &image) {
        float lab[3];
        color::rec2020ToOklab(image.pixel(0, 0), lab);
        return std::hypot(lab[1], lab[2]);
    };
    const Image dull = testutils::uniform(4, 4, 0.20f, 0.18f, 0.17f);
    const Image vivid = testutils::uniform(4, 4, 0.6f, 0.02f, 0.02f);
    const double dullGain = chroma(applied(VibranceEffect{}, dull, VibranceEffect::VIBRANCE.id, 100))
                            / chroma(dull);
    const double vividGain = chroma(
                                 applied(VibranceEffect{}, vivid, VibranceEffect::VIBRANCE.id, 100))
                             / chroma(vivid);
    EXPECT_GT(dullGain, 1.5);
    EXPECT_LT(vividGain, 1.1);
}

TEST(EffectsTest, DetailBoostsFineTextureAndLeavesFlatAreas)
{
    const Image texture = testutils::checker(64, 64, 4, 0.15f, 0.21f);
    const Image boosted = applied(DetailEffect{}, texture, DetailEffect::DETAIL.id, 100);
    EXPECT_GT(
        testutils::luminanceSpread(boosted, 8, 8, 56, 56),
        1.2 * testutils::luminanceSpread(texture, 8, 8, 56, 56));
    const Image smoothed = applied(DetailEffect{}, texture, DetailEffect::DETAIL.id, -100);
    EXPECT_LT(
        testutils::luminanceSpread(smoothed, 8, 8, 56, 56),
        testutils::luminanceSpread(texture, 8, 8, 56, 56));
    const Image flat = testutils::uniform(16, 16, 0.18f, 0.18f, 0.18f);
    EXPECT_LT(
        testutils::maxDifference(applied(DetailEffect{}, flat, DetailEffect::DETAIL.id, 100), flat),
        1e-6f);
}

TEST(EffectsTest, StructureBoostsMediumSizedShapes)
{
    const Image shapes = testutils::checker(400, 400, 4, 0.12f, 0.25f);
    const Image boosted = applied(StructureEffect{}, shapes, StructureEffect::STRUCTURE.id, 100);
    EXPECT_GT(
        testutils::luminanceSpread(boosted, 50, 50, 350, 350),
        1.1 * testutils::luminanceSpread(shapes, 50, 50, 350, 350));
}

TEST(EffectsTest, SharpnessSteepensEdges)
{
    const Image edge = testutils::step(32, 8, 0.1f, 0.4f);
    const Image sharp = applied(SharpnessEffect{}, edge, SharpnessEffect::SHARPNESS.id, 100);
    // Overshoot on both sides of the edge, flat far from it.
    EXPECT_LT(sharp.pixel(15, 4)[1], 0.1f);
    EXPECT_GT(sharp.pixel(16, 4)[1], 0.4f);
    EXPECT_NEAR(sharp.pixel(2, 4)[1], 0.1f, 1e-4f);
}

namespace {

float chromaAt(const Image &image, int x, int y)
{
    float lab[3];
    color::rec2020ToOklab(image.pixel(x, y), lab);
    return std::hypot(lab[1], lab[2]);
}

/**
 * A dark bar on a bright background, with a 2-pixel fringe of `fringe` beside its edges, and a
 * flat patch of the same colour far from any edge.
 */
Image fringed(const float *fringe)
{
    Image image = testutils::uniform(120, 60, 0.8f, 0.8f, 0.8f);
    for (int y = 0; y < 60; ++y) {
        for (int x = 0; x < 120; ++x) {
            float *p = image.pixel(x, y);
            if (x >= 20 && x < 30) {
                std::fill_n(p, 3, 0.02f);
            } else if (x == 30 || x == 31) {
                std::copy(fringe, fringe + 3, p);
            }
        }
    }
    // The patch: 30 pixels wide, its inside 10 pixels from its own edges.
    for (int y = 15; y < 45; ++y) {
        for (int x = 70; x < 100; ++x) {
            std::copy(fringe, fringe + 3, image.pixel(x, y));
        }
    }
    return image;
}

constexpr float PURPLE[3] = {0.35f, 0.12f, 0.55f};
constexpr float GREEN[3] = {0.15f, 0.45f, 0.12f};

} // namespace

TEST(EffectsTest, DefringeRemovesPurpleBesideEdgesOnly)
{
    const Image image = fringed(PURPLE);
    const Image fixed = applied(DefringeEffect{}, image, DefringeEffect::AMOUNT.id, 20);
    // The fringe loses its colour, the patch away from edges keeps it.
    EXPECT_LT(chromaAt(fixed, 30, 30), 0.2f * chromaAt(image, 30, 30));
    EXPECT_NEAR(chromaAt(fixed, 85, 30), chromaAt(image, 85, 30), 1e-4f);
}

TEST(EffectsTest, DefringeLeavesHuesOutsideTheRange)
{
    const Image image = fringed(GREEN);
    const Image fixed = applied(DefringeEffect{}, image, DefringeEffect::AMOUNT.id, 20);
    EXPECT_NEAR(chromaAt(fixed, 30, 30), chromaAt(image, 30, 30), 1e-4f);
}

TEST(EffectsTest, DefringeWithoutAnAmountChangesNothingWhateverTheHues)
{
    Settings settings;
    settings.setValue(DefringeEffect::HUE_LOW.id, 0);
    settings.setValue(DefringeEffect::HUE_HIGH.id, 100);
    EXPECT_TRUE(DefringeEffect{}.isIdentity(settings));
    settings.setValue(DefringeEffect::AMOUNT.id, 5);
    EXPECT_FALSE(DefringeEffect{}.isIdentity(settings));
}

TEST(EffectsTest, DefringeFindsTheSameFringeInAPreview)
{
    // A third of the size, as the preview: the fringe is a pixel wide there, beside the edge.
    Image image = fringed(PURPLE);
    Image small{40, 20};
    for (int y = 0; y < 20; ++y) {
        for (int x = 0; x < 40; ++x) {
            std::copy(
                image.pixel(x * 3 + 1, y * 3 + 1),
                image.pixel(x * 3 + 1, y * 3 + 1) + 3,
                small.pixel(x, y));
        }
    }
    Settings settings;
    settings.setValue(DefringeEffect::AMOUNT.id, 20);
    Image fixed = small;
    DefringeEffect{}.apply(fixed, settings, RenderContext{1.0 / 3.0});
    EXPECT_LT(chromaAt(fixed, 10, 10), 0.2f * chromaAt(small, 10, 10));
    EXPECT_NEAR(chromaAt(fixed, 28, 10), chromaAt(small, 28, 10), 1e-4f);
}

TEST(EffectsTest, WhiteBalanceFromAColourMakesItNeutral)
{
    // A grey card under warm, slightly green light; then a cool, magenta one.
    for (const std::array<float, 3> cast :
         {std::array{0.22f, 0.19f, 0.13f}, std::array{0.15f, 0.17f, 0.22f}}) {
        const auto neutral = WhiteBalanceEffect::neutralFor(cast);
        ASSERT_TRUE(neutral);
        EXPECT_FALSE(neutral->clamped);
        Settings settings;
        settings.setValue(WhiteBalanceEffect::TEMPERATURE.id, neutral->temperature);
        settings.setValue(WhiteBalanceEffect::TINT.id, neutral->tint);
        Image image = testutils::uniform(2, 2, cast[0], cast[1], cast[2]);
        WhiteBalanceEffect{}.apply(image, settings, RenderContext{});
        const float *p = image.pixel(0, 0);
        // Neutral within the sliders' whole steps: under 1%.
        EXPECT_NEAR(p[0] / p[1], 1.0f, 0.01f);
        EXPECT_NEAR(p[2] / p[1], 1.0f, 0.01f);
    }
}

TEST(EffectsTest, WhiteBalanceOfAGreyChangesNothingAndBlackHasNone)
{
    const auto grey = WhiteBalanceEffect::neutralFor({0.18f, 0.18f, 0.18f});
    ASSERT_TRUE(grey);
    EXPECT_EQ(grey->temperature, 0);
    EXPECT_EQ(grey->tint, 0);
    EXPECT_FALSE(WhiteBalanceEffect::neutralFor({0, 0, 0}));
}

TEST(EffectsTest, WhiteBalanceBeyondTheSlidersIsClamped)
{
    const auto neutral = WhiteBalanceEffect::neutralFor({0.3f, 0.1f, 0.01f});
    ASSERT_TRUE(neutral);
    EXPECT_TRUE(neutral->clamped);
    EXPECT_GE(neutral->temperature, WhiteBalanceEffect::TEMPERATURE.minimum);
    EXPECT_LE(neutral->temperature, WhiteBalanceEffect::TEMPERATURE.maximum);
}

TEST(SamplingTest, AveragesADiscAndLeavesClippedPixelsOut)
{
    Image image = testutils::uniform(20, 20, 0.2f, 0.4f, 0.6f);
    image.pixel(10, 10)[1] = 1.0f;  // clipped, in the middle
    image.pixel(0, 0)[0] = 0.9f;    // outside the disc
    const sampling::Sample sample = sampling::meanColour(image, 10.5, 10.5, 3.0);
    EXPECT_EQ(sample.clipped, 1);
    EXPECT_EQ(sample.used, 28);
    EXPECT_NEAR(sample.rgb[0], 0.2f, 1e-6f);
    EXPECT_NEAR(sample.rgb[1], 0.4f, 1e-6f);

    const sampling::Sample outside = sampling::meanColour(image, -50, -50, 3.0);
    EXPECT_EQ(outside.used, 0);
}

TEST(EffectsTest, ToneCompressionCurveRollsOffAboveTheKneeOnly)
{
    const float knee = 0.4f, white = 8.0f;
    EXPECT_FLOAT_EQ(ToneCompressionEffect::compress(0.2f, knee, white), 0.2f);
    EXPECT_FLOAT_EQ(ToneCompressionEffect::compress(knee, knee, white), knee);
    // The white given ends at white, smoothly: no step at the knee, and ever brighter.
    EXPECT_NEAR(ToneCompressionEffect::compress(white, knee, white), 1.0f, 1e-4f);
    EXPECT_NEAR(ToneCompressionEffect::compress(knee + 1e-3f, knee, white), knee + 1e-3f, 1e-5f);
    float previous = knee;
    for (float v = 0.5f; v < white; v *= 1.3f) {
        const float out = ToneCompressionEffect::compress(v, knee, white);
        EXPECT_GT(out, previous);
        EXPECT_LT(out, 1.0f);
        previous = out;
    }
    // Without anything above white to fit, it is the identity.
    EXPECT_FLOAT_EQ(ToneCompressionEffect::compress(0.95f, knee, 1.0f), 0.95f);
}

TEST(EffectsTest, ToneCompressionBringsHighlightsBackAndKeepsTheirHue)
{
    // A sky three stops above white, orange-ish, beside a mid-grey ground.
    Image image = testutils::step(200, 50, 0.18f, 1.0f);
    for (int y = 0; y < 50; ++y) {
        for (int x = 100; x < 200; ++x) {
            float *p = image.pixel(x, y);
            p[0] = 8.0f, p[1] = 6.0f, p[2] = 4.0f;
        }
    }
    Settings settings;
    settings.setValue(ToneCompressionEffect::AMOUNT.id, 100);
    settings.setValue(ToneCompressionEffect::LOCAL.id, 0);
    ToneCompressionEffect{}.apply(image, settings, RenderContext{});
    const float *sky = image.pixel(150, 25);
    EXPECT_LE(sky[0], 1.0f);
    EXPECT_GT(sky[0], 0.8f);
    EXPECT_NEAR(sky[1] / sky[0], 0.75f, 1e-4f);
    EXPECT_NEAR(sky[2] / sky[0], 0.5f, 1e-4f);
    // The ground, below the knee, is untouched.
    EXPECT_FLOAT_EQ(image.pixel(20, 25)[1], 0.18f);
}

TEST(EffectsTest, ToneCompressionWithLocalContrastKeepsTheTextureOfBrightAreas)
{
    // Clouds: a bright checker, 2 and 6 times white, on 400 pixels.
    const Image clouds = testutils::checker(400, 400, 6, 2.0f, 6.0f);
    auto spread = [&](double local) {
        Settings settings;
        settings.setValue(ToneCompressionEffect::AMOUNT.id, 100);
        settings.setValue(ToneCompressionEffect::LOCAL.id, local);
        Image image = clouds;
        ToneCompressionEffect{}.apply(image, settings, RenderContext{});
        return testutils::luminanceSpread(image, 50, 50, 350, 350);
    };
    EXPECT_GT(spread(100), 1.5 * spread(0));
}

namespace {

/**
 * Grey, 0.18, with a step to 0.5 at the middle, and grain as a sensor makes it, growing with the
 * square root of the light: the same for the three channels (luminance noise) or each its own
 * (colour noise).
 */
Image grainy(int width, int height, float noise, bool coloured)
{
    std::mt19937 random{42};
    std::normal_distribution<float> gauss{0.0f, noise};
    Image image{width, height};
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const float base = x < width / 2 ? 0.18f : 0.5f;
            const float common = gauss(random);
            float *p = image.pixel(x, y);
            for (int c = 0; c < 3; ++c) {
                p[c] = base + std::sqrt(base) * (coloured ? gauss(random) : common);
            }
        }
    }
    return image;
}

double chromaSpread(const Image &image, int x0, int y0, int x1, int y1)
{
    double sum = 0;
    for (int y = y0; y < y1; ++y) {
        for (int x = x0; x < x1; ++x) {
            float lab[3];
            color::rec2020ToOklab(image.pixel(x, y), lab);
            sum += std::hypot(lab[1], lab[2]);
        }
    }
    return sum / ((x1 - x0) * (y1 - y0));
}

} // namespace

TEST(EffectsTest, LuminanceNoiseSmoothsTheGrainAndKeepsTheEdge)
{
    const Image image = grainy(200, 100, 0.03f, false);
    const Image clean = applied(LuminanceNoiseEffect{}, image, LuminanceNoiseEffect::AMOUNT.id, 100);
    // The grain of each side, a third of what it was or less.
    EXPECT_LT(
        testutils::luminanceSpread(clean, 10, 10, 90, 90),
        0.4 * testutils::luminanceSpread(image, 10, 10, 90, 90));
    EXPECT_LT(
        testutils::luminanceSpread(clean, 110, 10, 190, 90),
        0.4 * testutils::luminanceSpread(image, 110, 10, 190, 90));
    // The edge stays where it was, as steep: the two pixels beside it keep their sides' levels.
    EXPECT_NEAR(testutils::mean(clean, 1, 98, 10, 99, 90), 0.18, 0.02);
    EXPECT_NEAR(testutils::mean(clean, 1, 101, 10, 102, 90), 0.5, 0.04);
}

TEST(EffectsTest, LuminanceNoiseDetailPutsBackPartOfTheGrain)
{
    const Image image = grainy(100, 100, 0.03f, false);
    Settings settings;
    settings.setValue(LuminanceNoiseEffect::AMOUNT.id, 100);
    auto spread = [&](double detail) {
        settings.setValue(LuminanceNoiseEffect::DETAIL.id, detail);
        Image out = image;
        LuminanceNoiseEffect{}.apply(out, settings, RenderContext{});
        return testutils::luminanceSpread(out, 10, 10, 40, 90);
    };
    EXPECT_LT(spread(0), spread(100));
    // The colour of a grey stays grey.
    Image out = image;
    LuminanceNoiseEffect{}.apply(out, settings, RenderContext{});
    EXPECT_NEAR(out.pixel(20, 20)[0], out.pixel(20, 20)[2], 1e-5f);
}

TEST(EffectsTest, ColorNoiseRemovesTheBlotchesAndKeepsTheLuminance)
{
    const Image image = grainy(200, 100, 0.04f, true);
    const Image clean = applied(ColorNoiseEffect{}, image, ColorNoiseEffect::AMOUNT.id, 100);
    EXPECT_LT(chromaSpread(clean, 10, 10, 90, 90), 0.3 * chromaSpread(image, 10, 10, 90, 90));
    // Oklab's lightness, so the luminance and its detail, as they were.
    float before[3], after[3];
    color::rec2020ToOklab(image.pixel(50, 50), before);
    color::rec2020ToOklab(clean.pixel(50, 50), after);
    EXPECT_NEAR(after[0], before[0], 1e-4f);
}

TEST(EffectsTest, ColorNoiseKeepsColoursToTheirSideOfAnEdge)
{
    // Red beside blue, of different lightness, as a red object on a blue sky.
    Image image{100, 40};
    for (int y = 0; y < 40; ++y) {
        for (int x = 0; x < 100; ++x) {
            float *p = image.pixel(x, y);
            p[0] = x < 50 ? 0.4f : 0.02f;
            p[1] = x < 50 ? 0.05f : 0.05f;
            p[2] = x < 50 ? 0.02f : 0.3f;
        }
    }
    const Image clean = applied(ColorNoiseEffect{}, image, ColorNoiseEffect::AMOUNT.id, 100);
    // Next to the edge, each side keeps its colour.
    float red[3], blue[3];
    color::rec2020ToOklab(clean.pixel(48, 20), red);
    color::rec2020ToOklab(clean.pixel(51, 20), blue);
    float redBefore[3], blueBefore[3];
    color::rec2020ToOklab(image.pixel(48, 20), redBefore);
    color::rec2020ToOklab(image.pixel(51, 20), blueBefore);
    EXPECT_NEAR(red[1], redBefore[1], 0.02f);
    EXPECT_NEAR(blue[2], blueBefore[2], 0.02f);
}

namespace {

/// The linear sRGB colour, at `level`, as linear Rec. 2020: e.g. a pure sRGB red.
Image fromSrgb(float r, float g, float b)
{
    const Matrix3 toRec2020 = inverse(color::rec2020ToSrgb());
    Image image{4, 4};
    for (std::size_t i = 0; i < image.pixelCount(); ++i) {
        for (int row = 0; row < 3; ++row) {
            image.data[i * 3 + row] = static_cast<float>(
                toRec2020[row * 3] * r + toRec2020[row * 3 + 1] * g + toRec2020[row * 3 + 2] * b);
        }
    }
    return image;
}

std::array<float, 3> oklchOf(const Image &image)
{
    float lab[3];
    color::rec2020ToOklab(image.pixel(0, 0), lab);
    const float hue = std::atan2(lab[2], lab[1]) * 180 / 3.14159265f;
    return {lab[0], std::hypot(lab[1], lab[2]), hue < 0 ? hue + 360 : hue};
}

} // namespace

TEST(EffectsTest, ToneCurveMovesItsRegionAndLeavesTheOthers)
{
    // A dark grey, about a tenth up the tones, and a light one, about four fifths up.
    const Image dark = testutils::uniform(4, 4, 0.012f, 0.012f, 0.012f);
    const Image light = testutils::uniform(4, 4, 0.6f, 0.6f, 0.6f);
    const auto id = ToneCurveEffect::SHADOWS.id;
    EXPECT_GT(applied(ToneCurveEffect{}, dark, id, 100).pixel(0, 0)[1], 0.02f);
    EXPECT_LT(applied(ToneCurveEffect{}, dark, id, -100).pixel(0, 0)[1], 0.008f);
    EXPECT_NEAR(applied(ToneCurveEffect{}, light, id, 100).pixel(0, 0)[1], 0.6f, 0.01f);

    const auto highlights = ToneCurveEffect::HIGHLIGHTS.id;
    EXPECT_GT(applied(ToneCurveEffect{}, light, highlights, 100).pixel(0, 0)[1], 0.65f);
    EXPECT_NEAR(applied(ToneCurveEffect{}, dark, highlights, 100).pixel(0, 0)[1], 0.012f, 1e-3f);
}

TEST(EffectsTest, ToneCurveKeepsTheColourAndLeavesWhiteAndAbove)
{
    const Image orange = testutils::uniform(4, 4, 0.3f, 0.15f, 0.05f);
    const Image darker = applied(ToneCurveEffect{}, orange, ToneCurveEffect::DARKS.id, -100);
    const float *p = darker.pixel(0, 0);
    EXPECT_LT(p[1], 0.15f);
    EXPECT_NEAR(p[0] / p[1], 2.0f, 1e-4f);
    EXPECT_NEAR(p[2] / p[1], 1.0f / 3, 1e-4f);

    const Image bright = testutils::uniform(4, 4, 2.0f, 1.5f, 1.2f);
    const Image same = applied(ToneCurveEffect{}, bright, ToneCurveEffect::LIGHTS.id, 100);
    EXPECT_EQ(testutils::maxDifference(same, bright), 0.0f);
}

TEST(EffectsTest, ToneCurveNeverGoesDownWhateverItsSliders)
{
    Settings settings;
    settings.setValue(ToneCurveEffect::SHADOWS.id, -100);
    settings.setValue(ToneCurveEffect::DARKS.id, 100);
    settings.setValue(ToneCurveEffect::LIGHTS.id, -100);
    settings.setValue(ToneCurveEffect::HIGHLIGHTS.id, 100);
    // The splits crossed: the midtones split below the shadows split.
    settings.setValue(ToneCurveEffect::SHADOW_SPLIT.id, 45);
    settings.setValue(ToneCurveEffect::MIDTONE_SPLIT.id, 25);
    settings.setValue(ToneCurveEffect::HIGHLIGHT_SPLIT.id, 55);
    const auto knots = ToneCurveEffect::knots(settings);
    EXPECT_DOUBLE_EQ(ToneCurveEffect::curve(knots, 0), 0);
    EXPECT_DOUBLE_EQ(ToneCurveEffect::curve(knots, 1), 1);
    double previous = 0;
    for (int i = 1; i <= 1000; ++i) {
        const double value = ToneCurveEffect::curve(knots, i / 1000.0);
        EXPECT_GE(value, previous) << i;
        previous = value;
    }
}

TEST(EffectsTest, ToneCurveSplitsComeInOrderWithRoomBetween)
{
    EXPECT_EQ(ToneCurveEffect::splits(Settings{}), (std::array<double, 3>{0.25, 0.5, 0.75}));
    Settings crossed;
    crossed.setValue(ToneCurveEffect::SHADOW_SPLIT.id, 45);
    crossed.setValue(ToneCurveEffect::MIDTONE_SPLIT.id, 25);
    crossed.setValue(ToneCurveEffect::HIGHLIGHT_SPLIT.id, 55);
    const auto splits = ToneCurveEffect::splits(crossed);
    EXPECT_GT(splits[1], splits[0]);
    EXPECT_GT(splits[2], splits[1]);
    EXPECT_LT(splits[2], 1.0);
}

TEST(EffectsTest, ToneCurveSplitsAloneChangeNothing)
{
    Settings settings;
    settings.setValue(ToneCurveEffect::SHADOW_SPLIT.id, 10);
    settings.setValue(ToneCurveEffect::HIGHLIGHT_SPLIT.id, 90);
    EXPECT_TRUE(ToneCurveEffect{}.isIdentity(settings));
    Image image = colourful();
    ToneCurveEffect{}.apply(image, settings, RenderContext{});
    EXPECT_LT(testutils::maxDifference(image, colourful()), 1e-5f);
}

TEST(EffectsTest, ColorMixerBandsFollowTheColourWheel)
{
    const auto &middles = ColorMixerEffect::bandHues();
    // From red round to magenta, each band further round the wheel than the one before.
    for (int band = 1; band < ColorMixerEffect::BANDS; ++band) {
        EXPECT_GT(middles[band], middles[band - 1]) << ColorMixerEffect::BAND_NAMES[band];
    }
    EXPECT_EQ(ColorMixerEffect{}.parameters().size(), 3u * ColorMixerEffect::BANDS);
    EXPECT_EQ(ColorMixerEffect::hues()[0].id, "HueAdjustmentRed");
    EXPECT_EQ(ColorMixerEffect::saturations()[5].id, "SaturationAdjustmentBlue");
    EXPECT_EQ(ColorMixerEffect::luminances()[7].id, "LuminanceAdjustmentMagenta");
}

TEST(EffectsTest, ColorMixerChangesItsBandOnly)
{
    const Image red = fromSrgb(0.3f, 0.0f, 0.0f);
    const Image blue = fromSrgb(0.0f, 0.0f, 0.3f);
    const auto redSaturation = ColorMixerEffect::saturations()[0].id;
    EXPECT_LT(oklchOf(applied(ColorMixerEffect{}, red, redSaturation, -100))[1], 1e-3f);
    EXPECT_GT(oklchOf(applied(ColorMixerEffect{}, red, redSaturation, 50))[1], 1.4f * oklchOf(red)[1]);
    EXPECT_LT(
        testutils::maxDifference(applied(ColorMixerEffect{}, blue, redSaturation, -100), blue),
        1e-5f);

    // The red's hue turned toward orange, the blue's luminance down a stop.
    const auto redHue = ColorMixerEffect::hues()[0].id;
    EXPECT_GT(oklchOf(applied(ColorMixerEffect{}, red, redHue, 100))[2], oklchOf(red)[2] + 20);
    const auto blueLuminance = ColorMixerEffect::luminances()[5].id;
    const Image darker = applied(ColorMixerEffect{}, blue, blueLuminance, -100);
    EXPECT_NEAR(color::luminance(darker.pixel(0, 0)), color::luminance(blue.pixel(0, 0)) / 2, 1e-4f);
}

TEST(EffectsTest, ColorMixerLeavesGreysAlone)
{
    const Image grey = testutils::uniform(4, 4, 0.18f, 0.18f, 0.18f);
    Settings settings;
    for (const Parameter &parameter : ColorMixerEffect{}.parameters()) {
        settings.setValue(parameter.id, 100);
    }
    Image image = grey;
    ColorMixerEffect{}.apply(image, settings, RenderContext{});
    EXPECT_LT(testutils::maxDifference(image, grey), 1e-5f);
}
