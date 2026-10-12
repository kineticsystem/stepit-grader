// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "core/Filters.h"
#include "core/Parallel.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace core::filters {

namespace {

void blurRows(const Plane &in, Plane &out, int radius)
{
    parallelFor(0, in.height, [&](int begin, int end) {
        for (int y = begin; y < end; ++y) {
            const float *src = &in.data[static_cast<std::size_t>(y) * in.width];
            float *dst = &out.data[static_cast<std::size_t>(y) * in.width];
            double sum = 0;
            int count = 0;
            for (int x = 0; x <= std::min(radius, in.width - 1); ++x) {
                sum += src[x];
                ++count;
            }
            for (int x = 0; x < in.width; ++x) {
                dst[x] = static_cast<float>(sum / count);
                const int add = x + radius + 1;
                const int remove = x - radius;
                if (add < in.width) {
                    sum += src[add];
                    ++count;
                }
                if (remove >= 0) {
                    sum -= src[remove];
                    --count;
                }
            }
        }
    });
}

// Each thread blurs a band of columns, moving down the rows, so that it reads whole runs of
// memory rather than one value per row.
void blurColumns(const Plane &in, Plane &out, int radius)
{
    parallelFor(0, in.width, [&](int begin, int end) {
        const int band = end - begin;
        std::vector<double> sum(band, 0.0);
        int count = 0;
        auto row = [&](int y) { return &in.data[static_cast<std::size_t>(y) * in.width + begin]; };
        for (int y = 0; y <= std::min(radius, in.height - 1); ++y) {
            const float *src = row(y);
            for (int i = 0; i < band; ++i) {
                sum[i] += src[i];
            }
            ++count;
        }
        for (int y = 0; y < in.height; ++y) {
            float *dst = &out.data[static_cast<std::size_t>(y) * in.width + begin];
            for (int i = 0; i < band; ++i) {
                dst[i] = static_cast<float>(sum[i] / count);
            }
            const int add = y + radius + 1;
            const int remove = y - radius;
            if (add < in.height) {
                const float *src = row(add);
                for (int i = 0; i < band; ++i) {
                    sum[i] += src[i];
                }
                ++count;
            }
            if (remove >= 0) {
                const float *src = row(remove);
                for (int i = 0; i < band; ++i) {
                    sum[i] -= src[i];
                }
                --count;
            }
        }
    });
}

} // namespace

Plane boxBlur(const Plane &plane, int radius)
{
    if (radius <= 0 || plane.data.empty()) {
        return plane;
    }
    Plane rows{plane.width, plane.height};
    blurRows(plane, rows, radius);
    Plane out{plane.width, plane.height};
    blurColumns(rows, out, radius);
    return out;
}

Plane gaussianBlur(const Plane &plane, double sigma)
{
    if (sigma < 0.5) {
        return plane;
    }
    // Three box blurs of this radius have about the variance of the Gaussian.
    const int radius = std::max(
        1, static_cast<int>(std::lround((std::sqrt(4.0 * sigma * sigma + 1.0) - 1.0) / 2.0)));
    return boxBlur(boxBlur(boxBlur(plane, radius), radius), radius);
}

Plane guidedFilter(const Plane &plane, int radius, float epsilon)
{
    if (radius <= 0 || plane.data.empty()) {
        return plane;
    }
    const std::size_t n = plane.data.size();
    Plane squares{plane.width, plane.height};
    parallelForEach(n, [&](std::size_t i) { squares.data[i] = plane.data[i] * plane.data[i]; });
    const Plane mean = boxBlur(plane, radius);
    const Plane meanOfSquares = boxBlur(squares, radius);

    Plane a{plane.width, plane.height};
    Plane b{plane.width, plane.height};
    parallelForEach(n, [&](std::size_t i) {
        const float variance = std::max(0.0f, meanOfSquares.data[i] - mean.data[i] * mean.data[i]);
        a.data[i] = variance / (variance + epsilon);
        b.data[i] = mean.data[i] * (1.0f - a.data[i]);
    });
    const Plane meanA = boxBlur(a, radius);
    const Plane meanB = boxBlur(b, radius);

    Plane out{plane.width, plane.height};
    parallelForEach(n, [&](std::size_t i) {
        out.data[i] = meanA.data[i] * plane.data[i] + meanB.data[i];
    });
    return out;
}

} // namespace core::filters
