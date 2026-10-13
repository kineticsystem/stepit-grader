// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#pragma once

#include <QString>
#include <QStringList>

class QSettings;
class QWidget;

/**
 * The editor's open dialogs, which start where the user last chose a file of the same kind: each
 * kind keeps its folder in `QSettings` under a key of `SessionProperties::Files`, e.g. the pictures
 * under `LAST_FOLDER`. A dialog given no key starts in its fallback folder and keeps nothing. The
 * actions and the window ask through these, not through `QFileDialog`, so every dialog remembers
 * the same way.
 */
namespace FileUtils {

/// The folder kept under `key` in `settings`, or `fallback` when none is, or `key` is null.
QString lastFolder(const QSettings &settings, const char *key, const QString &fallback = {});

/// Keeps the folder of `file` under `key` in `settings`, for the next dialog; nothing for a null
/// `key` or an empty `file`.
void rememberFolder(QSettings &settings, const char *key, const QString &file);

/**
 * Asks for a file to open of `filter`, e.g. "Images (*.tif *.dng)", starting in the folder kept
 * under `key`, or in `fallback`, and keeps the folder of the file chosen. Empty when cancelled.
 */
QString getOpenFile(
    QWidget *parent,
    const QString &caption,
    const QString &filter,
    const char *key,
    const QString &fallback = {});

/// As getOpenFile(), for one file or more; the folder kept is that of the first.
QStringList getOpenFiles(
    QWidget *parent,
    const QString &caption,
    const QString &filter,
    const char *key,
    const QString &fallback = {});

} // namespace FileUtils
