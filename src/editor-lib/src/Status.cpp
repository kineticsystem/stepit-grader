// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "editor/Status.h"

StatusController::StatusController()
{
    // A Status crosses threads in a queued signal: Qt must know the type.
    qRegisterMetaType<Status>("Status");
}

void StatusController::setStatus(const Status &status)
{
    Q_EMIT this->status(status);
}

void StatusController::stop()
{
    m_stopped = true;
    Q_EMIT stopRequested();
}
