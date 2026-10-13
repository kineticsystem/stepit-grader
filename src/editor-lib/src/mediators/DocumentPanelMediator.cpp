// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "editor/mediators/DocumentPanelMediator.h"
#include "core/effects/ToneCurveEffect.h"
#include "editor/Document.h"
#include "editor/PreviewRenderer.h"
#include "widgets/AdjustmentsPanel.h"
#include "widgets/CurveView.h"

#include <vector>

namespace {

/// The points the curve of the Tone Curve section is drawn with: enough for it to look smooth.
constexpr int CURVE_POINTS = 129;

/// Draws the tone curve the settings make, with the borders of its regions.
void showCurve(CurveView &view, const core::Settings &settings)
{
    const auto knots = core::ToneCurveEffect::knots(settings);
    std::vector<QPointF> points;
    points.reserve(CURVE_POINTS);
    for (int i = 0; i < CURVE_POINTS; ++i) {
        const double tone = static_cast<double>(i) / (CURVE_POINTS - 1);
        points.emplace_back(tone, core::ToneCurveEffect::curve(knots, tone));
    }
    const auto splits = core::ToneCurveEffect::splits(settings);
    view.setCurve(std::move(points), {splits.begin(), splits.end()});
}

} // namespace

DocumentPanelMediator::DocumentPanelMediator(
    Document *document,
    AdjustmentsPanel *panel,
    CurveView *curve,
    PreviewRenderer *renderer,
    QObject *parent)
    : QObject{parent}
{
    showCurve(*curve, core::Settings{});
    connect(panel, &AdjustmentsPanel::settingsChanged, document, &Document::setSettings);
    connect(
        document,
        &Document::settingsChanged,
        this,
        [panel, curve, renderer](const core::Settings &settings) {
            panel->setSettings(settings);
            showCurve(*curve, settings);
            renderer->render(settings);
        });
    // The settings are rendered once the document announces them, right after.
    connect(document, &Document::opened, this, [document, panel, renderer] {
        renderer->setSource(document->image());
        panel->setControlsEnabled(true);
    });
}
