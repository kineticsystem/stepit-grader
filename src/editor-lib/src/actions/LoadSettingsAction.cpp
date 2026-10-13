// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "editor/actions/LoadSettingsAction.h"
#include "editor/Document.h"
#include "editor/FileUtils.h"
#include "editor/SessionProperties.h"

#include <QFileInfo>
#include <QWidget>

LoadSettingsAction::LoadSettingsAction(Document *document, QWidget *parent)
    : QAction{tr("&Load Settings…"), parent}
{
    setShortcut(QKeySequence{tr("Ctrl+L")});
    setToolTip(tr("Apply the settings of a sidecar, e.g. one saved for another picture"));
    connect(this, &QAction::triggered, this, [document, parent] {
        const QString path = FileUtils::getOpenFile(
            parent,
            tr("Load Settings"),
            tr("Settings (%1)").arg(document->settingsNameFilter()),
            SessionProperties::Files::LAST_SETTINGS_FOLDER,
            QFileInfo{document->path()}.absolutePath());
        if (!path.isEmpty()) {
            document->loadSettings(path);
        }
    });
}
