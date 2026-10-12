// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#pragma once

#include "core/Image.h"

#include <cstdint>
#include <vector>

namespace core::display {

/**
 * The image as 8-bit sRGB, 3 bytes per pixel, rows packed: converted from linear Rec. 2020 to
 * sRGB, clipped to [0, 1], and encoded with the sRGB curve. For the screen only: the colours
 * outside sRGB are clipped.
 */
std::vector<std::uint8_t> toSrgb8(const Image &image);

} // namespace core::display
