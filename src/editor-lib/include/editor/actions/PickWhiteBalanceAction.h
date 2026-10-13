// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#pragma once

#include <QAction>

class ImageView;
class WhiteBalanceViewModel;

/**
 * Pick White Balance, `W`: puts the image view in pick mode while checked, and hands the point
 * picked to the view model; one pick, then back to dragging, as most editors do. What the pick
 * sets is the view model's.
 */
class PickWhiteBalanceAction : public QAction
{
    Q_OBJECT

public:
    PickWhiteBalanceAction(WhiteBalanceViewModel *viewModel, ImageView *view, QObject *parent);
};
