// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "widgets/SliderStyle.h"

#include <QApplication>
#include <QPainter>
#include <QPointer>
#include <QSlider>
#include <QStyleOptionSlider>

namespace {

/// The circle to drag, in pixels: large enough to grab at once.
constexpr int HANDLE_DIAMETER = 18;
/// The circle's ring and the line, in pixels.
constexpr qreal RING_WIDTH = 1.5;
constexpr qreal LINE_WIDTH = 2.0;
/// How much of the text's colour the line and the ring take, out of 255: the line recedes, the
/// ring stands out, in a light theme as in a dark one.
constexpr int LINE_ALPHA = 90;
constexpr int RING_ALPHA = 210;
/// A disabled slider fades to this share of its colours.
constexpr qreal DISABLED_OPACITY = 0.4;

const QStyleOptionSlider *horizontalSlider(const QStyleOptionComplex *option)
{
    const auto *slider = qstyleoption_cast<const QStyleOptionSlider *>(option);
    return slider && slider->orientation == Qt::Horizontal ? slider : nullptr;
}

QColor withAlpha(QColor colour, int alpha)
{
    colour.setAlpha(alpha);
    return colour;
}

} // namespace

SliderStyle *SliderStyle::instance()
{
    // The application deletes it with its other children, before Qt itself goes.
    static QPointer<SliderStyle> style;
    if (!style) {
        style = new SliderStyle;
        style->setParent(qApp);
    }
    return style;
}

void SliderStyle::apply(QSlider *slider)
{
    slider->setStyle(instance());
    // The ring takes the highlight under the mouse, which needs hover events.
    slider->setAttribute(Qt::WA_Hover);
}

int SliderStyle::pixelMetric(
    PixelMetric metric, const QStyleOption *option, const QWidget *widget) const
{
    switch (metric) {
    case PM_SliderThickness:
        return HANDLE_DIAMETER + 2;
    case PM_SliderLength:
    case PM_SliderControlThickness:
        return HANDLE_DIAMETER;
    default:
        return QProxyStyle::pixelMetric(metric, option, widget);
    }
}

QRect SliderStyle::subControlRect(
    ComplexControl control,
    const QStyleOptionComplex *option,
    SubControl subControl,
    const QWidget *widget) const
{
    const QStyleOptionSlider *slider = control == CC_Slider ? horizontalSlider(option) : nullptr;
    if (!slider) {
        return QProxyStyle::subControlRect(control, option, subControl, widget);
    }
    // The groove is the whole width: Qt moves the handle's left edge from its start to its end
    // minus the handle, so the circle's centre spans the line.
    const QRect &area = slider->rect;
    const QRect
        groove{area.x(), area.center().y() - HANDLE_DIAMETER / 2, area.width(), HANDLE_DIAMETER};
    switch (subControl) {
    case SC_SliderGroove:
        return groove;
    case SC_SliderHandle: {
        const int x = QStyle::sliderPositionFromValue(
            slider->minimum,
            slider->maximum,
            slider->sliderPosition,
            groove.width() - HANDLE_DIAMETER,
            slider->upsideDown);
        return {groove.x() + x, groove.y(), HANDLE_DIAMETER, HANDLE_DIAMETER};
    }
    default:
        return QProxyStyle::subControlRect(control, option, subControl, widget);
    }
}

void SliderStyle::drawComplexControl(
    ComplexControl control,
    const QStyleOptionComplex *option,
    QPainter *painter,
    const QWidget *widget) const
{
    const QStyleOptionSlider *slider = control == CC_Slider ? horizontalSlider(option) : nullptr;
    if (!slider) {
        QProxyStyle::drawComplexControl(control, option, painter, widget);
        return;
    }
    const QPalette &palette = slider->palette;
    const bool enabled = slider->state & State_Enabled;
    const bool active = (slider->state & State_Sunken)
                        || ((slider->state & State_MouseOver)
                            && (slider->activeSubControls & SC_SliderHandle));
    const QColor text = palette.color(QPalette::WindowText);

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    if (!enabled) {
        painter->setOpacity(DISABLED_OPACITY);
    }

    // The line, from the circle's centre at one end to its centre at the other.
    const QRectF groove = subControlRect(CC_Slider, slider, SC_SliderGroove, widget);
    const qreal y = groove.center().y();
    const qreal inset = HANDLE_DIAMETER / 2.0;
    painter->setPen(QPen{withAlpha(text, LINE_ALPHA), LINE_WIDTH, Qt::SolidLine, Qt::RoundCap});
    painter->drawLine(QPointF{groove.left() + inset, y}, QPointF{groove.right() - inset, y});

    // The circle: empty, in the window's colour, so the line does not run through it; its ring
    // takes the highlight while it is held or under the mouse.
    const QRectF handle = QRectF{subControlRect(CC_Slider, slider, SC_SliderHandle, widget)}
                              .adjusted(RING_WIDTH, RING_WIDTH, -RING_WIDTH, -RING_WIDTH);
    const QColor ring = active && enabled ? palette.color(QPalette::Highlight)
                                          : withAlpha(text, RING_ALPHA);
    painter->setPen(QPen{ring, RING_WIDTH});
    painter->setBrush(palette.color(QPalette::Window));
    painter->drawEllipse(handle);

    painter->restore();
}
