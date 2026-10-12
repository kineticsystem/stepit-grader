// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#pragma once

#include "core/Image.h"

namespace core::geometry {

/**
 * The image reduced by the smallest whole factor that makes its longer side at most `maxSide`,
 * each pixel the mean of the block it replaces. The image itself when it is small enough.
 */
Image downscale(const Image &image, int maxSide);

/// The image turned upright, from its TIFF/EXIF orientation, 1 to 8. Other values do nothing.
Image orient(const Image &image, int orientation);

} // namespace core::geometry
