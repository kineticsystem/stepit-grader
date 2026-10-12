// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "editor/Icons.h"

#include <QApplication>
#include <QPainter>
#include <QPalette>
#include <QPixmap>
#include <QSvgRenderer>

namespace {

/// The sizes drawn, so the icon is sharp in a menu, a toolbar and on high-density screens.
constexpr int SIZES[] = {16, 22, 24, 32, 48, 64};

} // namespace

bool isDarkTheme()
{
    return QApplication::palette().color(QPalette::Window).lightness() < 128;
}

QIcon themedIcon(const QString &name)
{
    // Drawn by Qt's SVG module, rather than an icon plugin that would have to be shipped beside
    // the program.
    QSvgRenderer renderer{QString{":/icons/%1/%2.svg"}.arg(isDarkTheme() ? "dark" : "light", name)};
    QIcon icon;
    if (!renderer.isValid()) {
        return icon;
    }
    for (int size : SIZES) {
        QPixmap pixmap{size, size};
        pixmap.fill(Qt::transparent);
        QPainter painter{&pixmap};
        painter.setRenderHint(QPainter::Antialiasing);
        renderer.render(&painter);
        painter.end();
        icon.addPixmap(pixmap);
    }
    return icon;
}
