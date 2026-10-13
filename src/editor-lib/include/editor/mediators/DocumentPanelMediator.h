// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#pragma once

#include <QObject>

class AdjustmentsPanel;
class CurveView;
class Document;
class PreviewRenderer;

/**
 * Keeps the document, the tools panel and the renderer in step, which know nothing of each other:
 * a slider moved sets the document's settings; the document's settings, from a slider or not, set
 * the sliders, draw the tone curve and are rendered; a picture opened is the renderer's source,
 * and enables the sliders.
 */
class DocumentPanelMediator : public QObject
{
    Q_OBJECT

public:
    DocumentPanelMediator(
        Document *document,
        AdjustmentsPanel *panel,
        CurveView *curve,
        PreviewRenderer *renderer,
        QObject *parent = nullptr);
};
