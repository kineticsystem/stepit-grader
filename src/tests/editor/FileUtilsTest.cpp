// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "../testutils/TestImages.h"

#include "editor/FileUtils.h"
#include "editor/SessionProperties.h"

#include <QFileInfo>
#include <QSettings>

#include <gtest/gtest.h>

namespace {

/// A settings file of the test's own, not the user's.
QString settingsFile()
{
    return QString::fromStdU16String((testutils::outputFolder() / "settings.ini").u16string());
}

constexpr const char *KEY = SessionProperties::Files::LAST_FOLDER;

} // namespace

TEST(FileUtilsTest, StartsInTheFallbackUntilAFolderIsKept)
{
    QSettings settings{settingsFile(), QSettings::IniFormat};
    EXPECT_EQ(FileUtils::lastFolder(settings, KEY, "/pictures"), "/pictures");
    EXPECT_EQ(FileUtils::lastFolder(settings, KEY), "");
}

TEST(FileUtilsTest, KeepsTheFolderOfTheFileChosen)
{
    QSettings settings{settingsFile(), QSettings::IniFormat};
    FileUtils::rememberFolder(settings, KEY, "/pictures/2026/a.tif");
    // As the system writes it: with the drive in front on Windows.
    const QString folder = QFileInfo{"/pictures/2026"}.absoluteFilePath();
    EXPECT_EQ(FileUtils::lastFolder(settings, KEY, "/elsewhere"), folder);

    // A dialog cancelled keeps the folder kept before.
    FileUtils::rememberFolder(settings, KEY, "");
    EXPECT_EQ(FileUtils::lastFolder(settings, KEY), folder);
}

TEST(FileUtilsTest, ADialogWithoutAKeyKeepsNothing)
{
    QSettings settings{settingsFile(), QSettings::IniFormat};
    FileUtils::rememberFolder(settings, nullptr, "/raws/a.cr2");
    EXPECT_TRUE(settings.allKeys().isEmpty());
    EXPECT_EQ(FileUtils::lastFolder(settings, nullptr, "/pictures"), "/pictures");
}
