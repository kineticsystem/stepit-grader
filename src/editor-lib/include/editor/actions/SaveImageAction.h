// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#pragma once

#include <QAction>

class SaveImageViewModel;

/**
 * File > Save Image…: asks for a file and its format, and has the view model save the picture there;
 * tells the user when the view model refuses the file or the save fails. The dialogs only: what is
 * saved, and where it may be, is the view model's.
 */
class SaveImageAction : public QAction
{
    Q_OBJECT

public:
    SaveImageAction(SaveImageViewModel *viewModel, QWidget *parent);

Q_SIGNALS:
    /// The view model took the file, and saves the picture to it in the background.
    void started(const QString &path);

private:
    void execute();

    SaveImageViewModel *m_viewModel;
    QWidget *m_parent;
};
