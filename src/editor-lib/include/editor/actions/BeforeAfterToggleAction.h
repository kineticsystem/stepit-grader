// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#pragma once

#include <QAction>

class Document;
class PreviewRenderer;

/**
 * View > Before and After, `\`: while checked, the renderer shows the picture before its edit, the
 * picture with no settings; unchecked, with the document's. A change of the settings is seen on
 * the edit, so it unchecks the action, without toggled(). beforeShown() says each change, either
 * way, e.g. for a label.
 */
class BeforeAfterToggleAction : public QAction
{
    Q_OBJECT

public:
    BeforeAfterToggleAction(Document *document, PreviewRenderer *renderer, QObject *parent);

Q_SIGNALS:
    void beforeShown(bool before);
};
