// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "editor/Icons.h"

#include <QApplication>
#include <QPainter>
#include <QPalette>
#include <QPixmap>
#include <QSvgRenderer>

namespace {

/// The sizes drawn, so the icon is sharp in a menu, a tool button and on high-density screens.
constexpr int SIZES[] = {16, 22, 24, 32, 48, 64};

/**
 * An SVG of one colour, drawn in `colour` at each size: the drawing's shape, the theme's
 * colour, so one file suits a light and a dark theme.
 */
QIcon tintedIcon(const QString &resource, const QColor &colour)
{
    QSvgRenderer renderer{resource};
    QIcon icon;
    for (int size : SIZES) {
        QPixmap pixmap{size, size};
        pixmap.fill(Qt::transparent);
        QPainter painter{&pixmap};
        painter.setRenderHint(QPainter::Antialiasing);
        renderer.render(&painter);
        painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
        painter.fillRect(pixmap.rect(), colour);
        painter.end();
        icon.addPixmap(pixmap);
    }
    return icon;
}

} // namespace

QIcon eyedropperIcon()
{
    return tintedIcon(":/icons/eyedropper.svg", QApplication::palette().color(QPalette::ButtonText));
}
