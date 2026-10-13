// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "widgets/CurveView.h"

#include <QPainter>
#include <QPainterPath>

namespace {

/// The height of the drawing against its width: a little less than a square, to save room.
constexpr double ASPECT = 0.8;
/// The width the drawing asks for, in pixels, before the panel gives it its own.
constexpr int PREFERRED_WIDTH = 260;
/// The space left around the plot, in pixels, so the frame's and the curve's lines are whole.
constexpr double MARGIN = 2.0;
/// The room below the drawing, in pixels, so the slider under it does not touch it.
constexpr int BOTTOM_SPACE = 10;
/// The width of the curve, in pixels.
constexpr double CURVE_WIDTH = 2.0;
/// How strong the grid, the diagonal and the borders are against the text colour, 0 to 255.
constexpr int GRID_ALPHA = 40;
constexpr int DIAGONAL_ALPHA = 70;
constexpr int BORDER_ALPHA = 90;

QColor withAlpha(QColor colour, int alpha)
{
    colour.setAlpha(alpha);
    return colour;
}

} // namespace

CurveView::CurveView(QWidget *parent)
    : QWidget{parent}
    , m_points{{0.0, 0.0}, {1.0, 1.0}}
{
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
}

void CurveView::setCurve(std::vector<QPointF> points, std::vector<double> borders)
{
    m_points = std::move(points);
    m_borders = std::move(borders);
    update();
}

int CurveView::heightForWidth(int width) const
{
    return static_cast<int>(width * ASPECT) + BOTTOM_SPACE;
}

QSize CurveView::sizeHint() const
{
    return {PREFERRED_WIDTH, heightForWidth(PREFERRED_WIDTH)};
}

QRectF CurveView::plotArea() const
{
    return QRectF{rect()}.adjusted(MARGIN, MARGIN, -MARGIN, -MARGIN - BOTTOM_SPACE);
}

QPointF CurveView::toWidget(QPointF tone) const
{
    const QRectF area = plotArea();
    return {area.left() + tone.x() * area.width(), area.bottom() - tone.y() * area.height()};
}

void CurveView::paintEvent(QPaintEvent *)
{
    QPainter painter{this};
    painter.setRenderHint(QPainter::Antialiasing);
    const QPalette &colours = palette();
    const QColor text = colours.color(QPalette::WindowText);
    const QRectF area = plotArea();

    painter.fillRect(area, colours.color(QPalette::Base));

    // The quarters of the tones, as a grid.
    painter.setPen(QPen{withAlpha(text, GRID_ALPHA), 1.0});
    for (int i = 1; i < 4; ++i) {
        const double t = i / 4.0;
        painter.drawLine(toWidget({t, 0}), toWidget({t, 1}));
        painter.drawLine(toWidget({0, t}), toWidget({1, t}));
    }

    // The borders between the regions.
    painter.setPen(QPen{withAlpha(text, BORDER_ALPHA), 1.0, Qt::DotLine});
    for (const double border : m_borders) {
        painter.drawLine(toWidget({border, 0}), toWidget({border, 1}));
    }

    // Where nothing changes.
    painter.setPen(QPen{withAlpha(text, DIAGONAL_ALPHA), 1.0, Qt::DashLine});
    painter.drawLine(toWidget({0, 0}), toWidget({1, 1}));

    if (m_points.size() >= 2) {
        QPainterPath path{toWidget(m_points.front())};
        for (std::size_t i = 1; i < m_points.size(); ++i) {
            path.lineTo(toWidget(m_points[i]));
        }
        painter.setPen(QPen{colours.color(QPalette::Highlight), CURVE_WIDTH});
        painter.drawPath(path);
    }

    painter.setPen(QPen{withAlpha(text, BORDER_ALPHA), 1.0});
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(area);
}
