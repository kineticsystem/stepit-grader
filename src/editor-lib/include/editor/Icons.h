// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#pragma once

#include <QIcon>
#include <QString>

/**
 * An icon of the editor, from Lentigram's icons, e.g. "zoom-in": its dark drawing on a light
 * theme, its light drawing on a dark one, as the window's colour says.
 */
QIcon themedIcon(const QString &name);

/// Whether the theme is dark: its window's colour is.
bool isDarkTheme();
