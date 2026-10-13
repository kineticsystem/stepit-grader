// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#pragma once

#include <QIcon>
#include <QString>

/**
 * The editor's own icons, built into the program, each drawn for a light and for a dark theme:
 * which drawing to use, as the theme's colours say.
 */
namespace IconUtils {

/**
 * An icon of the editor, e.g. "zoom-in": its dark drawing on a light
 * theme, its light drawing on a dark one, as the window's colour says.
 */
QIcon themedIcon(const QString &name);

/// Whether the theme is dark: its window's colour is.
bool isDarkTheme();

} // namespace IconUtils
