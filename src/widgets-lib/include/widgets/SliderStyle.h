// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#pragma once

#include <QProxyStyle>

class QSlider;

/**
 * Draws a horizontal slider as a thin line with a large empty circle to drag, as raw editors do,
 * in the colours of the slider's palette, so it follows the light and the dark themes. Whatever
 * the application's style, which draws everything else.
 *
 * It also says where the circle is, so Qt grabs it anywhere inside, and the line runs the whole
 * width: the circle's centre reaches both ends.
 *
 * `ParameterSlider` uses it; `apply()` gives the same look to any other slider of a program.
 */
class SliderStyle : public QProxyStyle
{
    Q_OBJECT

public:
    /// One style for every slider, owned by the application.
    static SliderStyle *instance();

    /// Draws the slider with this style, and lights its circle up under the mouse.
    static void apply(QSlider *slider);

    int pixelMetric(
        PixelMetric metric, const QStyleOption *option, const QWidget *widget) const override;
    QRect subControlRect(
        ComplexControl control,
        const QStyleOptionComplex *option,
        SubControl subControl,
        const QWidget *widget) const override;
    void drawComplexControl(
        ComplexControl control,
        const QStyleOptionComplex *option,
        QPainter *painter,
        const QWidget *widget) const override;

private:
    SliderStyle() = default;
};
