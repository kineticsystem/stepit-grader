// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "../testutils/Fakes.h"
#include "../testutils/TestImages.h"

#include "core/StandardPipeline.h"
#include "editor/Document.h"
#include "editor/PreviewRenderer.h"
#include "editor/mediators/DocumentPanelMediator.h"
#include "editor/mediators/PreviewMediator.h"
#include "editor/mediators/StatusBarMediator.h"
#include "editor/viewmodel/CameraLookViewModel.h"
#include "widgets/AdjustmentsPanel.h"
#include "widgets/CurveView.h"
#include "widgets/HistogramView.h"
#include "widgets/ImageView.h"
#include "widgets/SectionUtils.h"

#include <QLabel>
#include <QSignalSpy>
#include <QStatusBar>
#include <QTest>

#include <gtest/gtest.h>

#include <chrono>

namespace {

constexpr int WAIT_MS = 5000;

struct MediatorsTest : ::testing::Test
{
    std::shared_ptr<const core::Pipeline> pipeline = core::makeStandardPipeline();
    Document document{
        std::make_shared<testutils::FakeReader>(),
        std::make_shared<testutils::FakeStore>(),
        pipeline->parameters()};
    PreviewRenderer renderer{pipeline, 100, std::chrono::milliseconds{10}};

    void open()
    {
        QSignalSpy opened{&document, &Document::opened};
        document.open("/pictures/a.tif");
        ASSERT_TRUE(QTest::qWaitFor([&] { return !opened.isEmpty(); }, WAIT_MS));
    }
};

core::Settings contrast(double value)
{
    core::Settings settings;
    settings.setValue("Contrast2012", value);
    return settings;
}

} // namespace

TEST_F(MediatorsTest, TheDocumentAndThePanelFollowEachOtherAndTheEditIsRendered)
{
    AdjustmentsPanel panel{SectionUtils::sectionsOf(*pipeline)};
    CurveView curve;
    DocumentPanelMediator mediator{&document, &panel, &curve, &renderer};
    EXPECT_FALSE(curve.points().empty());

    QSignalSpy rendered{&renderer, &PreviewRenderer::rendered};
    open();
    // The picture opened is the renderer's source, and its settings are rendered.
    ASSERT_TRUE(QTest::qWaitFor([&] { return !rendered.isEmpty(); }, WAIT_MS));

    document.setSettings(contrast(20));
    EXPECT_DOUBLE_EQ(panel.settings().values().at("Contrast2012"), 20);

    Q_EMIT panel.settingsChanged(contrast(40));
    EXPECT_DOUBLE_EQ(document.settings().values().at("Contrast2012"), 40);
}

TEST_F(MediatorsTest, ThePictureRenderedIsShownWithItsHistogram)
{
    ImageView view;
    HistogramView histogram;
    PreviewMediator mediator{&renderer, &view, &histogram};
    QImage image{40, 20, QImage::Format_RGB888};
    image.fill(Qt::gray);
    Q_EMIT renderer.rendered(image, QSize{400, 200}, false);
    EXPECT_EQ(view.fullSize(), QSize(400, 200));
    EXPECT_EQ(histogram.histogram().pixels, 40 * 20);
}

TEST_F(MediatorsTest, TheStatusBarSaysWhatThePictureIsAndHowItIsShown)
{
    QStatusBar bar;
    StatusBarMediator status{&bar};
    CameraLookViewModel
        cameraLook{&document, pipeline, std::make_shared<testutils::FakeShotReader>()};
    status.followDocument(&document, &cameraLook);
    status.followRenderer(&renderer);
    const auto label = [&](const char *name) { return bar.findChild<QLabel *>(name)->text(); };

    open();
    EXPECT_TRUE(label("file").contains("a.tif"));
    EXPECT_TRUE(label("file").contains("300 × 200"));

    document.setSettings(contrast(20));
    EXPECT_EQ(label("render"), "Rendering…");
    Q_EMIT renderer.rendered(QImage{}, QSize{}, false);
    EXPECT_EQ(label("render"), "Preview");
    Q_EMIT renderer.rendered(QImage{}, QSize{}, true);
    EXPECT_EQ(label("render"), "");

    // What a match found stays, though its grade's save follows at once.
    status.followCameraLook(&cameraLook);
    Q_EMIT cameraLook.finished("Matched the camera's look");
    Q_EMIT document.saved("/pictures/a.xmp");
    EXPECT_EQ(bar.currentMessage(), "Matched the camera's look");

    status.showBefore(true);
    EXPECT_FALSE(bar.findChild<QLabel *>("before")->isHidden());
    status.showBefore(false);
    EXPECT_TRUE(bar.findChild<QLabel *>("before")->isHidden());
}
