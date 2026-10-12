// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#pragma once

#include "core/Image.h"

#include <filesystem>

namespace testutils {

/// Every pixel the same colour.
core::Image uniform(int width, int height, float r, float g, float b);

/// Left half `dark`, right half `bright`, grey.
core::Image step(int width, int height, float dark, float bright);

/// A grey checkerboard of squares `cell` pixels wide, alternating `low` and `high`, around `mean`.
core::Image checker(int width, int height, int cell, float low, float high);

/// The mean of one channel over a rectangle.
double mean(const core::Image &image, int channel, int x0, int y0, int x1, int y1);

/// The standard deviation of the luminance over a rectangle.
double luminanceSpread(const core::Image &image, int x0, int y0, int x1, int y1);

/// The largest difference between two images of the same size.
float maxDifference(const core::Image &a, const core::Image &b);

/// A folder of its own for the test running, emptied first.
std::filesystem::path outputFolder();

} // namespace testutils
