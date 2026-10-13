// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#pragma once

#include "core/Image.h"
#include "core/Settings.h"

#include <QPointF>
#include <QString>

#include <expected>

/**
 * Measuring the white balance of an area of the picture: the temperature and the tint that make
 * it neutral. Runs on any thread; `WhiteBalanceViewModel` runs it in the background and merges its
 * result into the picture's settings.
 */
namespace WhiteBalanceUtils {

/// What a pick of the white balance gives.
struct WhiteBalancePick
{
    /// The settings given, with the temperature and the tint that make the area neutral.
    core::Settings settings;
    /// The sliders' range was not enough: the area is only nearer neutral.
    bool clamped = false;
    /// The pixels of the area left out as clipped.
    int clipped = 0;
};

/**
 * The white balance that makes the area around `point`, of `radius`, neutral, in `image`, the
 * picture before any edit, both in pixels of it. The other values of `settings` are kept. Fails,
 * with a message for the user, when the area is clipped, black, or outside the picture.
 */
std::expected<WhiteBalancePick, QString> pickWhiteBalance(
    const core::Image &image, const core::Settings &settings, QPointF point, double radius);

} // namespace WhiteBalanceUtils
