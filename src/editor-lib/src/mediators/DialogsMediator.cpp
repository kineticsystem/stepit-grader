// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "editor/mediators/DialogsMediator.h"
#include "editor/Document.h"
#include "editor/viewmodel/LeavePictureViewModel.h"

#include <QMessageBox>
#include <QWidget>

DialogsMediator::DialogsMediator(
    QWidget *parent, const Document *document, LeavePictureViewModel *leavePicture)
    : QObject{parent}
{
    connect(document, &Document::openFailed, this, [parent](const QString &message) {
        QMessageBox::warning(parent, tr("Cannot Open"), message);
    });
    connect(document, &Document::sidecarUnreadable, this, [parent](const QString &message) {
        QMessageBox::warning(
            parent,
            tr("Cannot Read the Settings"),
            tr("%1\n\nThe picture opens with no edit. The first change replaces the settings "
               "file, and keeps the damaged one beside it, with .damaged after its name.")
                .arg(message));
    });
    connect(document, &Document::settingsLoadFailed, this, [parent](const QString &message) {
        QMessageBox::warning(parent, tr("Cannot Load Settings"), message);
    });
    // Only when the edit would be lost is the user asked.
    connect(
        leavePicture,
        &LeavePictureViewModel::editWouldBeLost,
        this,
        [parent, leavePicture](const QString &fileName) {
            const auto answer = QMessageBox::warning(
                parent,
                tr("Cannot Save"),
                tr("The settings of %1 cannot be saved. Leave them unsaved?").arg(fileName),
                QMessageBox::Discard | QMessageBox::Cancel,
                QMessageBox::Cancel);
            if (answer == QMessageBox::Discard) {
                leavePicture->discard();
            } else {
                leavePicture->stay();
            }
        });
}
