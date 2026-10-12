// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "core/Geometry.h"
#include "core/Parallel.h"

#include <algorithm>

namespace core::geometry {

Image downscale(const Image &image, int maxSide)
{
    const int longer = std::max(image.width, image.height);
    if (maxSide <= 0 || longer <= maxSide) {
        return image;
    }
    const int factor = (longer + maxSide - 1) / maxSide;
    Image out{image.width / factor, image.height / factor};
    const float norm = 1.0f / static_cast<float>(factor * factor);
    parallelFor(0, out.height, [&](int begin, int end) {
        for (int y = begin; y < end; ++y) {
            for (int x = 0; x < out.width; ++x) {
                float sum[3] = {0, 0, 0};
                for (int dy = 0; dy < factor; ++dy) {
                    for (int dx = 0; dx < factor; ++dx) {
                        const float *p = image.pixel(x * factor + dx, y * factor + dy);
                        sum[0] += p[0];
                        sum[1] += p[1];
                        sum[2] += p[2];
                    }
                }
                float *o = out.pixel(x, y);
                o[0] = sum[0] * norm;
                o[1] = sum[1] * norm;
                o[2] = sum[2] * norm;
            }
        }
    });
    return out;
}

Image orient(const Image &image, int orientation)
{
    if (orientation < 2 || orientation > 8) {
        return image;
    }
    // Orientations 5 to 8 swap the sides.
    const bool swap = orientation >= 5;
    Image out = swap ? Image{image.height, image.width} : Image{image.width, image.height};
    const int w = image.width, h = image.height;
    parallelFor(0, h, [&](int begin, int end) {
        for (int y = begin; y < end; ++y) {
            for (int x = 0; x < w; ++x) {
                int ox = x, oy = y;
                switch (orientation) {
                case 2:
                    ox = w - 1 - x;
                    break;
                case 3:
                    ox = w - 1 - x;
                    oy = h - 1 - y;
                    break;
                case 4:
                    oy = h - 1 - y;
                    break;
                case 5:
                    ox = y;
                    oy = x;
                    break;
                case 6:
                    ox = h - 1 - y;
                    oy = x;
                    break;
                case 7:
                    ox = h - 1 - y;
                    oy = w - 1 - x;
                    break;
                case 8:
                    ox = y;
                    oy = w - 1 - x;
                    break;
                }
                const float *p = image.pixel(x, y);
                std::copy(p, p + 3, out.pixel(ox, oy));
            }
        }
    });
    return out;
}

} // namespace core::geometry
