// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "editor/Icons.h"

#include <QApplication>
#include <QPainter>
#include <QPainterPath>
#include <QPalette>
#include <QPixmap>

namespace {

/// Drawn on a 16 x 16 grid, at twice the size for sharp screens.
constexpr int SIZE = 32;

QPixmap drawEyedropper(const QColor &colour)
{
    QPixmap pixmap{SIZE, SIZE};
    pixmap.fill(Qt::transparent);
    QPainter painter{&pixmap};
    painter.setRenderHint(QPainter::Antialiasing);
    painter.scale(SIZE / 16.0, SIZE / 16.0);
    // The bulb, top right, and the glass tube down to the tip, bottom left.
    painter.setPen(Qt::NoPen);
    painter.setBrush(colour);
    QPainterPath bulb;
    bulb.addRoundedRect(QRectF{10.0, 1.0, 5.0, 5.0}, 2.0, 2.0);
    painter.drawPath(bulb);
    painter.setPen(QPen{colour, 1.6, Qt::SolidLine, Qt::RoundCap});
    painter.drawLine(QPointF{8.5, 4.5}, QPointF{11.5, 7.5});
    painter.setPen(QPen{colour, 2.2, Qt::SolidLine, Qt::RoundCap});
    painter.drawLine(QPointF{10.0, 6.0}, QPointF{3.0, 13.0});
    painter.setPen(QPen{colour, 1.2, Qt::SolidLine, Qt::RoundCap});
    painter.drawLine(QPointF{3.0, 13.0}, QPointF{1.5, 14.5});
    return pixmap;
}

} // namespace

QIcon eyedropperIcon()
{
    const QColor colour = QApplication::palette().color(QPalette::ButtonText);
    return QIcon::fromTheme("color-picker", QIcon{drawEyedropper(colour)});
}
