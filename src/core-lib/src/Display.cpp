// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "core/Display.h"
#include "core/Color.h"
#include "core/Parallel.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace core::display {

namespace {

constexpr int LUT_SIZE = 4096;

// The sRGB curve, sampled: linear [0, 1] to 0..255.
const std::array<std::uint8_t, LUT_SIZE + 1> &encodingTable()
{
    static const auto table = [] {
        std::array<std::uint8_t, LUT_SIZE + 1> t{};
        for (int i = 0; i <= LUT_SIZE; ++i) {
            const double v = static_cast<double>(i) / LUT_SIZE;
            const double e = v <= 0.0031308 ? 12.92 * v : 1.055 * std::pow(v, 1.0 / 2.4) - 0.055;
            t[i] = static_cast<std::uint8_t>(std::lround(std::clamp(e, 0.0, 1.0) * 255.0));
        }
        return t;
    }();
    return table;
}

} // namespace

std::vector<std::uint8_t> toSrgb8(const Image &image)
{
    std::vector<std::uint8_t> out(image.pixelCount() * 3);
    const auto &table = encodingTable();
    std::array<float, 9> m{};
    for (int i = 0; i < 9; ++i) {
        m[i] = static_cast<float>(color::rec2020ToSrgb()[i]);
    }
    parallelFor(0, image.height, [&](int begin, int end) {
        for (int y = begin; y < end; ++y) {
            for (int x = 0; x < image.width; ++x) {
                const float *p = image.pixel(x, y);
                std::uint8_t *o = &out[(static_cast<std::size_t>(y) * image.width + x) * 3];
                for (int c = 0; c < 3; ++c) {
                    const float v = m[c * 3] * p[0] + m[c * 3 + 1] * p[1] + m[c * 3 + 2] * p[2];
                    o[c] = table[static_cast<int>(std::clamp(v, 0.0f, 1.0f) * LUT_SIZE + 0.5f)];
                }
            }
        }
    });
    return out;
}

} // namespace core::display
