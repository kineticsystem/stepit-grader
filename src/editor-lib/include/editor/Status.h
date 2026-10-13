// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#pragma once

#include <QMetaType>
#include <QObject>
#include <QString>

#include <atomic>

/// How a long task running on another thread is going: what it does, and how far it is.
struct Status
{
    enum class State { Running, Finished, Stopped, Failed };

    QString message;
    /// From 0 to 100.
    int value = 0;
    State state = State::Running;
};

Q_DECLARE_METATYPE(Status)

/**
 * Between a long task and the window that follows it. The task reports its Status, from any
 * thread, and the window receives it on its own thread through status(); the window asks the task
 * to stop with stop(), which the task reads with isStopped() between its steps.
 */
class StatusController : public QObject
{
    Q_OBJECT

public:
    StatusController();

    /// Thread-safe: may be called from the task's thread.
    void setStatus(const Status &status);
    bool isStopped() const { return m_stopped; }

public Q_SLOTS:
    /// Asks the task to stop at its next step.
    void stop();

Q_SIGNALS:
    void status(const Status &status);
    void stopRequested();

private:
    std::atomic<bool> m_stopped{false};
};
