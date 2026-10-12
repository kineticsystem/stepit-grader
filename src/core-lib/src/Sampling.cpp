// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "core/Sampling.h"

#include <algorithm>
#include <cmath>

namespace core::sampling {

Sample meanColour(const Image &image, double x, double y, double radius)
{
    Sample sample;
    const double r = std::max(radius, 0.5);
    const int x0 = std::max(0, static_cast<int>(std::floor(x - r)));
    const int x1 = std::min(image.width - 1, static_cast<int>(std::ceil(x + r)));
    const int y0 = std::max(0, static_cast<int>(std::floor(y - r)));
    const int y1 = std::min(image.height - 1, static_cast<int>(std::ceil(y + r)));
    double sum[3] = {0, 0, 0};
    for (int py = y0; py <= y1; ++py) {
        for (int px = x0; px <= x1; ++px) {
            // A pixel is in the disc when its centre is.
            const double dx = px + 0.5 - x, dy = py + 0.5 - y;
            if (dx * dx + dy * dy > r * r) {
                continue;
            }
            const float *p = image.pixel(px, py);
            if (std::max({p[0], p[1], p[2]}) >= CLIPPED) {
                ++sample.clipped;
                continue;
            }
            for (int c = 0; c < 3; ++c) {
                sum[c] += p[c];
            }
            ++sample.used;
        }
    }
    if (sample.used > 0) {
        for (int c = 0; c < 3; ++c) {
            sample.rgb[c] = static_cast<float>(sum[c] / sample.used);
        }
    }
    return sample;
}

} // namespace core::sampling
