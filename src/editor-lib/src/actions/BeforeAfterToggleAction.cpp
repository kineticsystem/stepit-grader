// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "editor/actions/BeforeAfterToggleAction.h"
#include "editor/Document.h"
#include "editor/PreviewRenderer.h"

#include <QSignalBlocker>

BeforeAfterToggleAction::BeforeAfterToggleAction(
    Document *document, PreviewRenderer *renderer, QObject *parent)
    : QAction{tr("&Before and After"), parent}
{
    setShortcut(QKeySequence{Qt::Key_Backslash});
    setCheckable(true);
    setToolTip(tr("Show the picture before the edit, or after (\\)"));
    connect(this, &QAction::toggled, this, [this, document, renderer](bool before) {
        // The picture before the edit is the picture with no settings: the renderer shows it from
        // its cache once rendered, so switching back and forth is at once.
        renderer->render(before ? core::Settings{} : document->settings());
        Q_EMIT beforeShown(before);
    });
    // A change is seen on the edited picture: the picture before the edit gives way to it.
    connect(document, &Document::settingsChanged, this, [this] {
        if (isChecked()) {
            {
                const QSignalBlocker blocker{this};
                setChecked(false);
            }
            Q_EMIT beforeShown(false);
        }
    });
}
