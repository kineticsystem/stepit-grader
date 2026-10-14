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
 * width: the circle's centre reaches both ends. Given its default, it marks on the line how far the
 * slider moved from it.
 *
 * `ParameterSlider` uses it; `apply()` gives the same look to any other slider of a program.
 */
class SliderStyle : public QProxyStyle
{
    Q_OBJECT

public:
    /// One style for every slider, owned by the application.
    static SliderStyle *instance();

    /**
     * Draws the slider with this style, and lights its circle up under the mouse. The wheel does
     * not move it: it scrolls the panel the slider is in, which would otherwise move whichever
     * slider passed under the mouse.
     */
    static void apply(QSlider *slider);

    /**
     * Marks the slider's default, in its own steps: the line is then drawn in the highlight
     * colour from the default to the circle, to show how far the slider moved. Without one, the
     * line is plain.
     */
    static void setDefaultValue(QSlider *slider, int value);

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
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    SliderStyle() = default;
};
