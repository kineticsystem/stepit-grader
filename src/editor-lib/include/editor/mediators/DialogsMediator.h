// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#pragma once

#include <QObject>

class Document;
class LeavePictureViewModel;
class QWidget;

/**
 * The windows the user must answer or close, over `parent`: a picture that cannot be opened,
 * settings that cannot be read or loaded, and, when leaving a picture would lose its edit, whether
 * to leave it, which it answers to the view model.
 */
class DialogsMediator : public QObject
{
    Q_OBJECT

public:
    DialogsMediator(QWidget *parent, const Document *document, LeavePictureViewModel *leavePicture);
};
