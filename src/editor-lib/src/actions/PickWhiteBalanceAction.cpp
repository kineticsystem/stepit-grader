// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "editor/actions/PickWhiteBalanceAction.h"
#include "editor/viewmodel/WhiteBalanceViewModel.h"
#include "widgets/ImageView.h"

PickWhiteBalanceAction::PickWhiteBalanceAction(
    WhiteBalanceViewModel *viewModel, ImageView *view, QObject *parent)
    : QAction{tr("Pick &White Balance"), parent}
{
    setShortcut(QKeySequence{tr("W")});
    setCheckable(true);
    setToolTip(tr("Click an area that should be white or grey (W)"));
    connect(this, &QAction::toggled, view, &ImageView::setPickMode);
    connect(view, &ImageView::pointPicked, this, [this, viewModel](QPointF point, double radius) {
        setChecked(false);
        viewModel->pick(point, radius);
    });
    connect(view, &ImageView::pickCancelled, this, [this] { setChecked(false); });
}
