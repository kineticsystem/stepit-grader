// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#pragma once

#include <QString>

#include <optional>

/// The look of the application.
enum class Theme {
    /// The desktop's own style and colours.
    System,
    /// Qt's Fusion style, light.
    Light,
    /// Qt's Fusion style, dark grey.
    Dark,
};

/**
 * Gives the application the theme's style and colours. The first call remembers the desktop's
 * style, which System gives back. Widgets made afterwards take the new colours at once; icons made
 * before keep the drawing of the theme they were made in, see themedIcon().
 */
void applyTheme(Theme theme);

/// The key the theme's name is kept under in QSettings.
inline constexpr const char *THEME_SETTING = "theme";

/// The name the theme is saved under, e.g. "dark".
QString nameOf(Theme theme);
/// The theme of a saved name, or nothing.
std::optional<Theme> themeNamed(const QString &name);
