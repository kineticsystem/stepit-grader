// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "widgets/HistogramView.h"

#include <QPainter>
#include <QPainterPath>

#include <algorithm>
#include <cmath>

namespace {

/// The size the histogram asks for, in pixels: as wide as the panel, and low.
constexpr int PREFERRED_WIDTH = 260;
constexpr int PREFERRED_HEIGHT = 110;
constexpr int MINIMUM_HEIGHT = 70;
/// The space left around the plot, in pixels, so the frame's lines are whole.
constexpr double MARGIN = 2.0;
/// The size of a clipping triangle, in pixels.
constexpr double TRIANGLE = 9.0;
/// How opaque the three channels are, 0 to 255, so where they overlap still shows.
constexpr int CHANNEL_ALPHA = 110;
/// How strong the luminance line and the frame are against the text colour, 0 to 255.
constexpr int LUMINANCE_ALPHA = 200;
constexpr int FRAME_ALPHA = 90;

QColor withAlpha(QColor colour, int alpha)
{
    colour.setAlpha(alpha);
    return colour;
}

} // namespace

HistogramView::HistogramView(QWidget *parent)
    : QWidget{parent}
{
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
}

void HistogramView::setHistogram(const Histogram &histogram)
{
    m_histogram = histogram;
    update();
}

QSize HistogramView::sizeHint() const
{
    return {PREFERRED_WIDTH, PREFERRED_HEIGHT};
}

QSize HistogramView::minimumSizeHint() const
{
    return {0, MINIMUM_HEIGHT};
}

void HistogramView::paintEvent(QPaintEvent *)
{
    QPainter painter{this};
    painter.setRenderHint(QPainter::Antialiasing);
    const QPalette &colours = palette();
    const QColor text = colours.color(QPalette::WindowText);
    const QRectF area = QRectF{rect()}.adjusted(MARGIN, MARGIN, -MARGIN, -MARGIN);
    painter.fillRect(area, colours.color(QPalette::Base));

    if (m_histogram.pixels > 0) {
        // The tallest level sets the height, leaving out the two ends: a clipped picture piles
        // its pixels there, and would flatten everything else.
        int tallest = 1;
        for (const auto &channel : m_histogram.counts) {
            tallest = std::max(tallest, *std::max_element(channel.begin() + 1, channel.end() - 1));
        }
        const auto pointOf = [&](int level, int count) {
            const double x = area.left() + (level + 0.5) / Histogram::LEVELS * area.width();
            const double height = std::min(1.0, static_cast<double>(count) / tallest);
            return QPointF{x, area.bottom() - height * area.height()};
        };
        const auto shapeOf = [&](const std::array<int, Histogram::LEVELS> &counts) {
            QPainterPath path{QPointF{area.left(), area.bottom()}};
            for (int level = 0; level < Histogram::LEVELS; ++level) {
                path.lineTo(pointOf(level, counts[level]));
            }
            path.lineTo(area.right(), area.bottom());
            path.closeSubpath();
            return path;
        };

        painter.setPen(Qt::NoPen);
        const std::array<QColor, 3> hues{
            QColor{230, 60, 60, CHANNEL_ALPHA},
            QColor{60, 190, 80, CHANNEL_ALPHA},
            QColor{70, 120, 240, CHANNEL_ALPHA}};
        for (int channel = Histogram::RED; channel <= Histogram::BLUE; ++channel) {
            painter.setBrush(hues[channel]);
            painter.drawPath(shapeOf(m_histogram.counts[channel]));
        }

        QPainterPath luminance;
        for (int level = 0; level < Histogram::LEVELS; ++level) {
            const QPointF point = pointOf(level, m_histogram.counts[Histogram::LUMINANCE][level]);
            if (level == 0) {
                luminance.moveTo(point);
            } else {
                luminance.lineTo(point);
            }
        }
        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen{withAlpha(text, LUMINANCE_ALPHA), 1.0});
        painter.drawPath(luminance);
    }

    // The clipping triangles, lit with the share of the picture clipped below them.
    const auto drawClipping = [&](int clipped, bool right) {
        const double share = m_histogram.pixels > 0
                                 ? static_cast<double>(clipped) / m_histogram.pixels
                                 : 0.0;
        const bool lit = share >= CLIPPING_SHOWN;
        const double x = right ? area.right() - 4 : area.left() + 4;
        const double inward = right ? -TRIANGLE : TRIANGLE;
        QPainterPath triangle{QPointF{x, area.top() + 4}};
        triangle.lineTo(x + inward, area.top() + 4);
        triangle.lineTo(x, area.top() + 4 + TRIANGLE);
        triangle.closeSubpath();
        painter.setPen(QPen{withAlpha(text, FRAME_ALPHA), 1.0});
        painter.setBrush(lit ? colours.color(QPalette::Highlight) : Qt::transparent);
        painter.drawPath(triangle);
        if (lit) {
            const QString label = QString::number(share * 100, 'f', share < 0.01 ? 2 : 1) + "%";
            painter.setPen(text);
            const QRectF
                box{right ? area.right() - 64 : area.left() + 4, area.top() + 6 + TRIANGLE, 60, 16};
            painter.drawText(box, (right ? Qt::AlignRight : Qt::AlignLeft) | Qt::AlignTop, label);
        }
    };
    drawClipping(m_histogram.clippedBlack, false);
    drawClipping(m_histogram.clippedWhite, true);

    painter.setPen(QPen{withAlpha(text, FRAME_ALPHA), 1.0});
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(area);
}
