// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "widgets/RecentFilesMenu.h"

#include <QDir>
#include <QFileInfo>

RecentFilesMenu::RecentFilesMenu(const QString &title, int maxFiles, QWidget *parent)
    : QMenu{title, parent}
    , m_maxFiles{maxFiles}
{
    setToolTipsVisible(true);
    // One action per place, made once, shown or hidden as the list changes.
    for (int i = 0; i < m_maxFiles; ++i) {
        auto *action = addAction(QString{});
        action->setVisible(false);
        connect(action, &QAction::triggered, this, [this, action] {
            Q_EMIT fileSelected(action->data().toString());
        });
        m_fileActions.append(action);
    }
    addSeparator();
    m_clearAction = addAction(tr("Clear"), this, &RecentFilesMenu::clearFiles);
    updateActions();
}

void RecentFilesMenu::setFiles(const QStringList &files)
{
    m_files = files.mid(0, m_maxFiles);
    updateActions();
}

void RecentFilesMenu::addFile(const QString &path)
{
    QStringList files = m_files;
    files.removeAll(path);
    files.prepend(path);
    change(files.mid(0, m_maxFiles));
}

void RecentFilesMenu::removeFile(const QString &path)
{
    if (m_files.contains(path)) {
        QStringList files = m_files;
        files.removeAll(path);
        change(files);
    }
}

void RecentFilesMenu::clearFiles()
{
    if (!m_files.isEmpty()) {
        change({});
    }
}

void RecentFilesMenu::change(QStringList files)
{
    m_files = std::move(files);
    updateActions();
    Q_EMIT filesChanged(m_files);
}

void RecentFilesMenu::updateActions()
{
    for (int i = 0; i < m_maxFiles; ++i) {
        QAction *action = m_fileActions[i];
        if (i < m_files.size()) {
            const QString &path = m_files[i];
            // Numbered 1 to 9 for the keyboard, as most applications do.
            const QString name = QFileInfo{path}.fileName().replace('&', "&&");
            action->setText(i < 9 ? QString{"&%1 %2"}.arg(i + 1).arg(name) : name);
            action->setToolTip(QDir::toNativeSeparators(path));
            action->setData(path);
            action->setVisible(true);
        } else {
            action->setVisible(false);
        }
    }
    setEnabled(!m_files.isEmpty());
}
