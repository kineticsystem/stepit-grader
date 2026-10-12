// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "editor/WhiteBalancePicker.h"

#include "core/Sampling.h"
#include "core/effects/WhiteBalanceEffect.h"

#include <QCoreApplication>

std::expected<WhiteBalancePick, QString> pickWhiteBalance(
    const core::Image &image, const core::Settings &settings, QPointF point, double radius)
{
    const auto tr = [](const char *text) {
        return QCoreApplication::translate("WhiteBalancePicker", text);
    };
    const core::sampling::Sample sample
        = core::sampling::meanColour(image, point.x(), point.y(), radius);
    if (sample.used == 0) {
        return std::unexpected{
            sample.clipped > 0 ? tr("The area is overexposed: pick a white that is not clipped.")
                               : tr("Pick a point on the picture.")};
    }
    const auto neutral = core::WhiteBalanceEffect::neutralFor(sample.rgb);
    if (!neutral) {
        return std::unexpected{tr("The area is too dark to measure its colour.")};
    }
    WhiteBalancePick pick{settings, neutral->clamped, sample.clipped};
    pick.settings.setValue(core::WhiteBalanceEffect::TEMPERATURE.id, neutral->temperature);
    pick.settings.setValue(core::WhiteBalanceEffect::TINT.id, neutral->tint);
    return pick;
}
