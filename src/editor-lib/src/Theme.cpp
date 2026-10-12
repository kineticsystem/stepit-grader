// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "editor/Theme.h"

#include <QApplication>
#include <QPalette>
#include <QProxyStyle>
#include <QStyle>
#include <QStyleFactory>

namespace {

/**
 * Fusion, light or dark grey. Fusion draws the same on every system, so the
 * dark colours look the same on Linux, Windows and macOS.
 */
class FusionStyle : public QProxyStyle
{
public:
    explicit FusionStyle(bool dark)
        : QProxyStyle{QStyleFactory::create("Fusion")}
        , m_dark{dark}
    {}

    QPalette standardPalette() const override
    {
        if (!m_dark) {
            return QProxyStyle::standardPalette();
        }
        QPalette palette;
        palette.setColor(QPalette::Window, QColor{53, 53, 53});
        palette.setColor(QPalette::WindowText, Qt::white);
        palette.setColor(QPalette::Base, QColor{42, 42, 42});
        palette.setColor(QPalette::AlternateBase, QColor{66, 66, 66});
        palette.setColor(QPalette::ToolTipBase, Qt::white);
        palette.setColor(QPalette::ToolTipText, QColor{53, 53, 53});
        palette.setColor(QPalette::Text, Qt::white);
        palette.setColor(QPalette::Dark, QColor{35, 35, 35});
        palette.setColor(QPalette::Shadow, QColor{20, 20, 20});
        palette.setColor(QPalette::Button, QColor{53, 53, 53});
        palette.setColor(QPalette::ButtonText, Qt::white);
        palette.setColor(QPalette::BrightText, Qt::red);
        palette.setColor(QPalette::Link, QColor{42, 130, 218});
        palette.setColor(QPalette::Highlight, QColor{42, 130, 218});
        palette.setColor(QPalette::HighlightedText, Qt::white);
        palette.setColor(QPalette::Disabled, QPalette::Light, Qt::transparent);
        palette.setColor(QPalette::Disabled, QPalette::WindowText, QColor{127, 127, 127});
        palette.setColor(QPalette::Disabled, QPalette::Text, QColor{127, 127, 127});
        palette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor{127, 127, 127});
        palette.setColor(QPalette::Disabled, QPalette::HighlightedText, QColor{127, 127, 127});
        palette.setColor(QPalette::Disabled, QPalette::Highlight, QColor{80, 80, 80});
        palette.setColor(QPalette::Disabled, QPalette::Base, QColor{66, 66, 66});
        return palette;
    }

    QPixmap generatedIconPixmap(
        QIcon::Mode mode, const QPixmap &pixmap, const QStyleOption *option) const override
    {
        if (!m_dark || mode != QIcon::Disabled) {
            return QProxyStyle::generatedIconPixmap(mode, pixmap, option);
        }
        // Fusion darkens a disabled icon, which on a dark theme hides it: grey instead.
        QImage image = pixmap.toImage().convertToFormat(QImage::Format_ARGB32);
        for (int y = 0; y < image.height(); ++y) {
            auto *line = reinterpret_cast<QRgb *>(image.scanLine(y));
            for (int x = 0; x < image.width(); ++x) {
                line[x] = qRgba(128, 128, 128, qAlpha(line[x]));
            }
        }
        return QPixmap::fromImage(image);
    }

private:
    bool m_dark;
};

/// The desktop's style, as the application started with it.
QString &systemStyle()
{
    static QString name = QApplication::style()->name();
    return name;
}

} // namespace

void applyTheme(Theme theme)
{
    const QString system = systemStyle();
    QStyle *style = nullptr;
    switch (theme) {
    case Theme::System:
        style = QStyleFactory::create(system);
        break;
    case Theme::Light:
        style = new FusionStyle{false};
        break;
    case Theme::Dark:
        style = new FusionStyle{true};
        break;
    }
    if (style == nullptr) {
        style = QStyleFactory::create("Fusion");
    }
    // The application owns the style from here.
    QApplication::setStyle(style);
    QApplication::setPalette(style->standardPalette());
}

QString nameOf(Theme theme)
{
    switch (theme) {
    case Theme::System:
        return "system";
    case Theme::Light:
        return "light";
    case Theme::Dark:
        return "dark";
    }
    return "system";
}

std::optional<Theme> themeNamed(const QString &name)
{
    for (Theme theme : {Theme::System, Theme::Light, Theme::Dark}) {
        if (nameOf(theme) == name) {
            return theme;
        }
    }
    return std::nullopt;
}
