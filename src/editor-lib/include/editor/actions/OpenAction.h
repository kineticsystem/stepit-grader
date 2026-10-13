// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#pragma once

#include <QAction>

class Document;

/**
 * File > Open…: asks for a picture of a kind the document reads, starting in the folder one was
 * last opened from, and says which with fileChosen(). Opening it, and refusing it, is the
 * window's, as for a file dropped or a recent one.
 */
class OpenAction : public QAction
{
    Q_OBJECT

public:
    OpenAction(const Document *document, QWidget *parent);

Q_SIGNALS:
    void fileChosen(const QString &path);
};
