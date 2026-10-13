// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#pragma once

#include <QAction>
#include <QPointer>

class CameraLookViewModel;
class WaitingDialog;

/**
 * Edit > Match Camera Look…: asks for the raw files when the picture open is not one, follows the
 * match in a WaitingDialog, and says why a match failed. The dialogs only: what is matched, and
 * how the grade found is applied over the picture's, is the view model's.
 */
class MatchCameraLookAction : public QAction
{
    Q_OBJECT

public:
    MatchCameraLookAction(CameraLookViewModel *viewModel, QWidget *parent);

private:
    void execute();
    void closeWaiting();
    void showFailed(const QString &message);

    CameraLookViewModel *m_viewModel;
    QWidget *m_parent;
    /// The window following the match running, gone once it ends.
    QPointer<WaitingDialog> m_waiting;
};
