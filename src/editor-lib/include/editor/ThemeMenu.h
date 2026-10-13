// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#pragma once

#include <QMenu>

class QSettings;

/**
 * Settings > Theme: System, Light or Dark, the one kept in `settings` checked. Choosing one applies
 * it to the application, keeps it, and says so with themeChanged(), e.g. for the window to draw
 * its icons again.
 */
class ThemeMenu : public QMenu
{
    Q_OBJECT

public:
    ThemeMenu(QSettings *settings, QWidget *parent = nullptr);

Q_SIGNALS:
    void themeChanged();
};
