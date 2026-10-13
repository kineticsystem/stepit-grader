// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "editor/mediators/PreviewMediator.h"
#include "editor/PreviewRenderer.h"
#include "widgets/HistogramUtils.h"
#include "widgets/HistogramView.h"
#include "widgets/ImageView.h"

PreviewMediator::PreviewMediator(
    PreviewRenderer *renderer, ImageView *view, HistogramView *histogram, QObject *parent)
    : QObject{parent}
{
    connect(
        renderer,
        &PreviewRenderer::rendered,
        this,
        [view, histogram](const QImage &image, QSize fullSize, bool) {
            view->setImage(image, fullSize);
            histogram->setHistogram(HistogramUtils::histogramOf(image));
        });
}
