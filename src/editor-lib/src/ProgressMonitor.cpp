// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "editor/ProgressMonitor.h"

ProgressMonitor::ProgressMonitor()
{
    // A Status crosses threads in a queued signal: Qt must know the type.
    qRegisterMetaType<Status>("Status");
}

void ProgressMonitor::setStatus(const Status &status)
{
    Q_EMIT this->status(status);
}

void ProgressMonitor::stop()
{
    m_stopped = true;
    Q_EMIT stopRequested();
}
