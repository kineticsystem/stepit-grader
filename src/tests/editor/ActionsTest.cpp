// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "../testutils/Fakes.h"
#include "../testutils/TestImages.h"

#include "core/StandardPipeline.h"
#include "editor/Document.h"
#include "editor/IconUtils.h"
#include "editor/PreviewRenderer.h"
#include "editor/ThemeMenu.h"
#include "editor/ThemeUtils.h"
#include "editor/actions/BeforeAfterToggleAction.h"

#include <QSettings>
#include <QSignalSpy>
#include <QTest>

#include <gtest/gtest.h>

#include <chrono>

namespace {

constexpr int WAIT_MS = 5000;

/// The action of `menu` labelled `label`, without its `&`.
QAction *actionNamed(const QMenu &menu, const QString &label)
{
    for (QAction *action : menu.actions()) {
        if (action->text().remove('&') == label) {
            return action;
        }
    }
    return nullptr;
}

} // namespace

TEST(BeforeAfterToggleActionTest, ShowsTheBeforeUntilTheEditChanges)
{
    const auto pipeline = core::makeStandardPipeline();
    Document document{
        std::make_shared<testutils::FakeReader>(),
        std::make_shared<testutils::FakeStore>(),
        pipeline->parameters()};
    PreviewRenderer renderer{pipeline, 100, std::chrono::milliseconds{10}};
    QSignalSpy opened{&document, &Document::opened};
    document.open("/pictures/a.tif");
    ASSERT_TRUE(QTest::qWaitFor([&] { return !opened.isEmpty(); }, WAIT_MS));

    BeforeAfterToggleAction action{&document, &renderer, nullptr};
    QSignalSpy shown{&action, &BeforeAfterToggleAction::beforeShown};
    QSignalSpy toggled{&action, &QAction::toggled};
    action.setChecked(true);
    ASSERT_EQ(shown.count(), 1);
    EXPECT_TRUE(shown.last().at(0).toBool());

    // A change is seen on the edit: the action unchecks itself, without toggled().
    core::Settings changed;
    changed.setValue("Contrast2012", 20);
    document.setSettings(changed);
    EXPECT_FALSE(action.isChecked());
    ASSERT_EQ(shown.count(), 2);
    EXPECT_FALSE(shown.last().at(0).toBool());
    EXPECT_EQ(toggled.count(), 1);
}

TEST(ThemeMenuTest, ChecksTheThemeKeptAndKeepsTheOneChosen)
{
    QSettings settings{
        QString::fromStdU16String((testutils::outputFolder() / "settings.ini").u16string()),
        QSettings::IniFormat};
    settings.setValue("theme", "dark");
    ThemeMenu menu{&settings};
    ASSERT_EQ(menu.actions().size(), 3);
    EXPECT_TRUE(actionNamed(menu, "Dark")->isChecked());
    EXPECT_FALSE(actionNamed(menu, "System")->isChecked());

    QSignalSpy changed{&menu, &ThemeMenu::themeChanged};
    actionNamed(menu, "Light")->trigger();
    EXPECT_EQ(changed.count(), 1);
    EXPECT_EQ(settings.value("theme").toString(), "light");
    EXPECT_FALSE(IconUtils::isDarkTheme());
    EXPECT_TRUE(actionNamed(menu, "Light")->isChecked());
    EXPECT_FALSE(actionNamed(menu, "Dark")->isChecked());

    ThemeUtils::applyTheme(ThemeUtils::Theme::System);
}
