// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "editor/ThemeMenu.h"
#include "editor/SessionProperties.h"
#include "editor/ThemeUtils.h"

#include <QActionGroup>
#include <QSettings>

#include <utility>

ThemeMenu::ThemeMenu(QSettings *settings, QWidget *parent)
    : QMenu{tr("&Theme"), parent}
{
    auto *group = new QActionGroup{this};
    const ThemeUtils::Theme current
        = ThemeUtils::themeNamed(settings->value(SessionProperties::Options::THEME).toString())
              .value_or(ThemeUtils::Theme::System);
    const std::pair<ThemeUtils::Theme, QString> choices[]
        = {{ThemeUtils::Theme::System, tr("&System")},
           {ThemeUtils::Theme::Light, tr("&Light")},
           {ThemeUtils::Theme::Dark, tr("&Dark")}};
    for (const auto &[theme, label] : choices) {
        QAction *action = addAction(label);
        action->setCheckable(true);
        action->setChecked(theme == current);
        group->addAction(action);
        connect(action, &QAction::triggered, this, [this, settings, theme] {
            ThemeUtils::applyTheme(theme);
            settings->setValue(SessionProperties::Options::THEME, ThemeUtils::nameOf(theme));
            Q_EMIT themeChanged();
        });
    }
}
