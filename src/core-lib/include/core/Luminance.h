// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#pragma once

#include "core/Image.h"

namespace core::luminance {

/**
 * The luminance of each pixel in stops from middle grey: log2(Y / 0.18). 0 is middle grey, -1 is
 * half as bright. The tone and detail effects work on it, so that a change is the same for a
 * dark and a bright part of the picture, and leaves the colours as they are.
 */
Plane logLuminance(const Image &image);

/**
 * Multiplies each pixel by 2^stops, so its luminance changes by that many stops and its
 * colour stays the same.
 */
void applyStops(Image &image, const Plane &stops);

/**
 * The log luminance of the area around each pixel: smoothed over about a fiftieth of the image's
 * diagonal, without crossing the edges where it changes by more than about half a stop. What the
 * highlights and the shadows call bright and dark, so both agree.
 */
Plane areaLuminance(const Plane &logLuminance);

/// A smooth step from 0 at `edge0` to 1 at `edge1`; `edge0` may be above `edge1`.
float smoothstep(float edge0, float edge1, float x);

} // namespace core::luminance
