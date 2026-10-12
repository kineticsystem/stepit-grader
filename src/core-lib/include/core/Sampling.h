// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#pragma once

#include "core/Image.h"

#include <array>

namespace core::sampling {

/// The mean colour of an area, and how many of its pixels it was made of.
struct Sample
{
    std::array<float, 3> rgb{};
    int used = 0;
    /// Pixels left out because a channel was clipped.
    int clipped = 0;
};

/// A channel at or above this is taken for clipped: its true value, and so its colour, is lost.
constexpr float CLIPPED = 0.98f;

/**
 * The mean colour of the pixels within `radius` of (x, y), in pixels of the image, leaving out
 * those with a clipped channel. A radius below 1 still takes the pixel under the point. `used`
 * is 0 when no pixel is usable, e.g. outside the image or all clipped; `clipped` then says why.
 */
Sample meanColour(const Image &image, double x, double y, double radius);

} // namespace core::sampling
