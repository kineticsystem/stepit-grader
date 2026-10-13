// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#pragma once

#include <QAction>

class Document;

/**
 * File > Load Settings…: asks for a file of settings, e.g. a sidecar saved for another picture,
 * starting where settings were last loaded from, or the picture's folder, and has the document
 * apply it. The document reads it, and says how it went.
 */
class LoadSettingsAction : public QAction
{
    Q_OBJECT

public:
    LoadSettingsAction(Document *document, QWidget *parent);
};
