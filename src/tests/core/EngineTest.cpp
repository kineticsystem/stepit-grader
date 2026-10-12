// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "../testutils/TestImages.h"

#include "core/Color.h"
#include "core/Display.h"
#include "core/Filters.h"
#include "core/Geometry.h"
#include "core/Pipeline.h"
#include "core/effects/ExposureEffect.h"

#include <gtest/gtest.h>

#include <cmath>

using namespace core;

TEST(FiltersTest, BoxBlurKeepsAConstantPlane)
{
    const Plane plane{37, 23, 0.5f};
    const Plane blurred = filters::boxBlur(plane, 5);
    for (float v : blurred.data) {
        EXPECT_NEAR(v, 0.5f, 1e-6f);
    }
}

TEST(FiltersTest, GuidedFilterKeepsAStrongEdgeAndFlattensWeakNoise)
{
    Plane plane{100, 10};
    for (int y = 0; y < 10; ++y) {
        for (int x = 0; x < 100; ++x) {
            plane.at(x, y) = (x < 50 ? -2.0f : 2.0f) + ((x + y) % 2 ? 0.01f : -0.01f);
        }
    }
    const Plane smooth = filters::guidedFilter(plane, 8, 0.25f);
    // Softened a little within the radius of the edge, but never beyond its two sides: no halo.
    EXPECT_NEAR(smooth.at(48, 5), -2.0f, 0.2f);
    EXPECT_NEAR(smooth.at(51, 5), 2.0f, 0.2f);
    for (int x = 0; x < 100; ++x) {
        EXPECT_LE(std::abs(smooth.at(x, 5)), 2.02f) << x;
    }
    EXPECT_NEAR(smooth.at(20, 5), -2.0f, 0.005f);
}

TEST(ColorTest, OklabRoundTrips)
{
    const float rgb[3] = {0.3f, 0.12f, 0.6f};
    float lab[3], back[3];
    color::rec2020ToOklab(rgb, lab);
    color::oklabToRec2020(lab, back);
    for (int c = 0; c < 3; ++c) {
        EXPECT_NEAR(back[c], rgb[c], 1e-5f);
    }
}

TEST(ColorTest, AdaptingD65ToD65ChangesNothing)
{
    const Matrix3 m = color::adaptToD65(color::D65);
    for (int i = 0; i < 9; ++i) {
        EXPECT_NEAR(m[i], i % 4 == 0 ? 1.0 : 0.0, 1e-9);
    }
}

TEST(DisplayTest, MiddleGreyIsSrgb118AndWhiteIs255)
{
    core::Image image = testutils::uniform(2, 1, 0.18f, 0.18f, 0.18f);
    std::fill_n(image.pixel(1, 0), 3, 1.0f);
    const auto bytes = display::toSrgb8(image);
    EXPECT_NEAR(bytes[0], 118, 1);
    EXPECT_EQ(bytes[3], 255);
}

TEST(GeometryTest, DownscaleAveragesBlocksAndKeepsSmallImages)
{
    const Image image = testutils::checker(100, 60, 1, 0.0f, 1.0f);
    const Image small = geometry::downscale(image, 50);
    EXPECT_EQ(small.width, 50);
    EXPECT_EQ(small.height, 30);
    EXPECT_NEAR(small.pixel(10, 10)[0], 0.5f, 1e-6f);
    EXPECT_EQ(geometry::downscale(image, 100).width, 100);
}

TEST(GeometryTest, OrientationSixTurnsClockwise)
{
    Image image{3, 2};
    image.pixel(0, 0)[0] = 1.0f;
    const Image turned = geometry::orient(image, 6);
    EXPECT_EQ(turned.width, 2);
    EXPECT_EQ(turned.height, 3);
    EXPECT_EQ(turned.pixel(1, 0)[0], 1.0f);
}

TEST(PipelineTest, CancelledRenderReturnsNothing)
{
    const Pipeline pipeline{{std::make_shared<ExposureEffect>()}};
    Settings settings;
    settings.setValue(ExposureEffect::EXPOSURE.id, 1.0);
    EXPECT_FALSE(pipeline.render(testutils::uniform(2, 2, 0.1f, 0.1f, 0.1f), settings, {}, [] {
        return true;
    }));
    const auto image = pipeline.render(testutils::uniform(2, 2, 0.1f, 0.1f, 0.1f), settings);
    ASSERT_TRUE(image);
    EXPECT_FLOAT_EQ(image->pixel(0, 0)[0], 0.2f);
}

TEST(FiltersTest, GuidedFilterByAnotherPlaneFollowsItsEdges)
{
    // The input is noise; the guide a step: the result is smooth on each side of the guide's step,
    // and steps where it does.
    Plane guide{100, 10};
    Plane input{100, 10};
    for (int y = 0; y < 10; ++y) {
        for (int x = 0; x < 100; ++x) {
            guide.at(x, y) = x < 50 ? 0.2f : 0.8f;
            input.at(x, y) = (x < 50 ? -1.0f : 1.0f) + ((x * 7 + y * 3) % 5 - 2) * 0.1f;
        }
    }
    const Plane out = filters::guidedFilter(guide, input, 8, 1e-4f);
    EXPECT_NEAR(out.at(20, 5), -1.0f, 0.05f);
    EXPECT_NEAR(out.at(80, 5), 1.0f, 0.05f);
    EXPECT_LT(out.at(48, 5), -0.8f);
    EXPECT_GT(out.at(51, 5), 0.8f);
}
