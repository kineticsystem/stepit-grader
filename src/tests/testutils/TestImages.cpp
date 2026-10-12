// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "TestImages.h"

#include "core/Color.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>

namespace testutils {

core::Image uniform(int width, int height, float r, float g, float b)
{
    core::Image image{width, height};
    for (std::size_t i = 0; i < image.pixelCount(); ++i) {
        image.data[i * 3] = r;
        image.data[i * 3 + 1] = g;
        image.data[i * 3 + 2] = b;
    }
    return image;
}

core::Image step(int width, int height, float dark, float bright)
{
    core::Image image{width, height};
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const float v = x < width / 2 ? dark : bright;
            std::fill_n(image.pixel(x, y), 3, v);
        }
    }
    return image;
}

core::Image checker(int width, int height, int cell, float low, float high)
{
    core::Image image{width, height};
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const float v = ((x / cell) + (y / cell)) % 2 == 0 ? low : high;
            std::fill_n(image.pixel(x, y), 3, v);
        }
    }
    return image;
}

double mean(const core::Image &image, int channel, int x0, int y0, int x1, int y1)
{
    double sum = 0;
    for (int y = y0; y < y1; ++y) {
        for (int x = x0; x < x1; ++x) {
            sum += image.pixel(x, y)[channel];
        }
    }
    return sum / ((x1 - x0) * (y1 - y0));
}

double luminanceSpread(const core::Image &image, int x0, int y0, int x1, int y1)
{
    double sum = 0, squares = 0;
    const int n = (x1 - x0) * (y1 - y0);
    for (int y = y0; y < y1; ++y) {
        for (int x = x0; x < x1; ++x) {
            const double v = core::color::luminance(image.pixel(x, y));
            sum += v;
            squares += v * v;
        }
    }
    const double m = sum / n;
    return std::sqrt(std::max(0.0, squares / n - m * m));
}

float maxDifference(const core::Image &a, const core::Image &b)
{
    float worst = 0;
    for (std::size_t i = 0; i < a.data.size(); ++i) {
        worst = std::max(worst, std::abs(a.data[i] - b.data[i]));
    }
    return worst;
}

std::filesystem::path outputFolder()
{
    const auto *info = ::testing::UnitTest::GetInstance()->current_test_info();
    const auto folder = std::filesystem::temp_directory_path() / "stepit-grader-tests"
                        / info->test_suite_name() / info->name();
    std::filesystem::remove_all(folder);
    std::filesystem::create_directories(folder);
    return folder;
}

} // namespace testutils
