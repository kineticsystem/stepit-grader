// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#pragma once

#include <QObject>

class HistogramView;
class ImageView;
class PreviewRenderer;

/**
 * Shows each picture the renderer sends, a preview or the full one: in the image view, and its
 * histogram, counted again from it, in the histogram view.
 */
class PreviewMediator : public QObject
{
    Q_OBJECT

public:
    PreviewMediator(
        PreviewRenderer *renderer,
        ImageView *view,
        HistogramView *histogram,
        QObject *parent = nullptr);
};
