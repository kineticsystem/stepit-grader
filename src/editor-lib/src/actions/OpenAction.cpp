// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "editor/actions/OpenAction.h"
#include "editor/Document.h"
#include "editor/FileUtils.h"
#include "editor/SessionProperties.h"

#include <QWidget>

OpenAction::OpenAction(const Document *document, QWidget *parent)
    : QAction{tr("&Open…"), parent}
{
    setShortcut(QKeySequence::Open);
    connect(this, &QAction::triggered, this, [this, document, parent] {
        const QString path = FileUtils::getOpenFile(
            parent,
            tr("Open Image"),
            tr("Images (%1)").arg(document->nameFilters().join(' ')),
            SessionProperties::Files::LAST_FOLDER);
        if (!path.isEmpty()) {
            Q_EMIT fileChosen(path);
        }
    });
}
