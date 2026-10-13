// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "editor/FileUtils.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QSettings>

namespace FileUtils {

QString lastFolder(const QSettings &settings, const char *key, const QString &fallback)
{
    if (key == nullptr) {
        return fallback;
    }
    return settings.value(key, fallback).toString();
}

void rememberFolder(QSettings &settings, const char *key, const QString &file)
{
    if (key != nullptr && !file.isEmpty()) {
        settings.setValue(key, QFileInfo{file}.absolutePath());
    }
}

QString getOpenFile(
    QWidget *parent,
    const QString &caption,
    const QString &filter,
    const char *key,
    const QString &fallback)
{
    QSettings settings;
    const QString file
        = QFileDialog::getOpenFileName(parent, caption, lastFolder(settings, key, fallback), filter);
    rememberFolder(settings, key, file);
    return file;
}

QStringList getOpenFiles(
    QWidget *parent,
    const QString &caption,
    const QString &filter,
    const char *key,
    const QString &fallback)
{
    QSettings settings;
    const QStringList files
        = QFileDialog::getOpenFileNames(parent, caption, lastFolder(settings, key, fallback), filter);
    if (!files.isEmpty()) {
        rememberFolder(settings, key, files.front());
    }
    return files;
}

} // namespace FileUtils
