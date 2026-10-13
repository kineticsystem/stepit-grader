// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#pragma once

#include "editor/Status.h"

#include <QDialog>

#include <memory>

class QLabel;
class QProgressBar;
class QPushButton;

/**
 * A modal window that follows a long task: what it does, a progress bar, and a button that asks it
 * to stop. Closing the window asks the same. The task decides what stopping means, e.g. keeping
 * the best result found so far, and reports it with its last Status.
 */
class WaitingDialog : public QDialog
{
    Q_OBJECT

public:
    WaitingDialog(
        std::shared_ptr<StatusController> controller,
        const QString &stopText,
        QWidget *parent = nullptr);

    QProgressBar *progressBar() const { return m_progress; }
    QPushButton *stopButton() const { return m_stop; }
    QString message() const;

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void stop();

    std::shared_ptr<StatusController> m_controller;
    QLabel *m_message;
    QProgressBar *m_progress;
    QPushButton *m_stop;
};
