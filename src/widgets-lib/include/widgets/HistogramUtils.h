// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#pragma once

#include "widgets/HistogramView.h"

#include <QImage>

/**
 * Counting the histogram of an image as the screen shows it, for `HistogramView` to draw. Runs
 * on any thread; the editor counts it from every image the renderer sends.
 */
namespace HistogramUtils {

/**
 * The histogram of an image as the screen shows it, 8 bits a channel. An image of more than
 * `maxPixels` is sampled on an even grid, so a full picture costs no more than a preview; the
 * luminance is that of Rec. 709, the screen's.
 */
Histogram histogramOf(const QImage &image, int maxPixels = 500'000);

} // namespace HistogramUtils
