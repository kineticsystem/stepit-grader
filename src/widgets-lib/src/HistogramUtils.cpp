// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "widgets/HistogramUtils.h"

#include <algorithm>
#include <cmath>

namespace HistogramUtils {

namespace {

/// The luminance weights of Rec. 709, in 1/1024: those of the sRGB screen.
constexpr int LUMA_R = 218;
constexpr int LUMA_G = 732;
constexpr int LUMA_B = 74;

} // namespace

Histogram histogramOf(const QImage &image, int maxPixels)
{
    Histogram histogram;
    if (image.isNull()) {
        return histogram;
    }
    const QImage rgb = image.format() == QImage::Format_RGB888
                           ? image
                           : image.convertToFormat(QImage::Format_RGB888);
    // An even grid of samples: every step-th pixel of every step-th row.
    const double ratio = static_cast<double>(rgb.width()) * rgb.height() / std::max(1, maxPixels);
    const int step = std::max(1, static_cast<int>(std::ceil(std::sqrt(ratio))));
    for (int y = 0; y < rgb.height(); y += step) {
        const uchar *line = rgb.constScanLine(y);
        for (int x = 0; x < rgb.width(); x += step) {
            const uchar r = line[x * 3];
            const uchar g = line[x * 3 + 1];
            const uchar b = line[x * 3 + 2];
            ++histogram.counts[Histogram::RED][r];
            ++histogram.counts[Histogram::GREEN][g];
            ++histogram.counts[Histogram::BLUE][b];
            ++histogram.counts[Histogram::LUMINANCE][(LUMA_R * r + LUMA_G * g + LUMA_B * b) >> 10];
            ++histogram.pixels;
            if (r == 0 || g == 0 || b == 0) {
                ++histogram.clippedBlack;
            }
            if (r == 255 || g == 255 || b == 255) {
                ++histogram.clippedWhite;
            }
        }
    }
    return histogram;
}

} // namespace HistogramUtils
