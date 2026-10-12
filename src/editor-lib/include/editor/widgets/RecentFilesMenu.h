// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#pragma once

#include <QMenu>
#include <QStringList>

/**
 * A menu of the files opened last, the latest first, then Clear. An entry shows the file's name,
 * and its full path as a tooltip. The menu is disabled while empty.
 *
 * It keeps the list, but not where the list is stored: its client gives it the files with
 * setFiles() and stores them on filesChanged(), e.g. in QSettings.
 */
class RecentFilesMenu : public QMenu
{
    Q_OBJECT

public:
    explicit RecentFilesMenu(const QString &title, int maxFiles, QWidget *parent = nullptr);

    QStringList files() const { return m_files; }
    /// Shows the files, the first the latest, without emitting filesChanged().
    void setFiles(const QStringList &files);

    /// Puts the file first, removing it from further down, and the oldest past the maximum.
    void addFile(const QString &path);
    /// Removes the file, e.g. one that no longer exists.
    void removeFile(const QString &path);

public Q_SLOTS:
    void clearFiles();

Q_SIGNALS:
    /// The user chose a file.
    void fileSelected(const QString &path);
    /// The list changed: by addFile(), removeFile() or clearFiles().
    void filesChanged(const QStringList &files);

private:
    void change(QStringList files);
    void updateActions();

    int m_maxFiles;
    QStringList m_files;
    QList<QAction *> m_fileActions;
    QAction *m_clearAction;
};
