// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#pragma once

#include "core/Image.h"

namespace core::filters {

/// The mean over a (2 * radius + 1) square around each value; the edges average what is inside.
Plane boxBlur(const Plane &plane, int radius);

/// A Gaussian blur of standard deviation `sigma`, made of three box blurs. Below 0.5, a copy.
Plane gaussianBlur(const Plane &plane, double sigma);

/**
 * The plane smoothed by the guided filter, guided by itself (He et al., 2010): flat where the
 * values vary less than sqrt(epsilon) over `radius`, and kept where they vary more, so strong
 * edges stay sharp and draw no halo.
 */
Plane guidedFilter(const Plane &plane, int radius, float epsilon);

} // namespace core::filters
