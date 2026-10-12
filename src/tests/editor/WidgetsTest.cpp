// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "../testutils/TestImages.h"

#include "core/StandardPipeline.h"
#include "editor/Sections.h"
#include "editor/WhiteBalancePicker.h"
#include "editor/widgets/AdjustmentsPanel.h"
#include "editor/widgets/CollapsibleSection.h"
#include "editor/widgets/ImageView.h"
#include "editor/widgets/ParameterSlider.h"

#include <QDoubleSpinBox>
#include <QLabel>
#include <QScrollBar>
#include <QSignalSpy>
#include <QSlider>
#include <QTest>
#include <QToolButton>

#include <gtest/gtest.h>

TEST(ParameterSliderTest, EmitsUserChangesOnly)
{
    ParameterSlider slider{"Exposure", -5, 5, 0, 2};
    QSignalSpy spy{&slider, &ParameterSlider::valueChanged};

    slider.setValue(1.5);
    EXPECT_EQ(spy.count(), 0);
    EXPECT_DOUBLE_EQ(slider.value(), 1.5);

    slider.findChild<QSlider *>()->setValue(-125);
    ASSERT_EQ(spy.count(), 1);
    EXPECT_DOUBLE_EQ(spy.takeFirst().at(0).toDouble(), -1.25);
    EXPECT_DOUBLE_EQ(slider.findChild<QDoubleSpinBox *>()->value(), -1.25);

    slider.findChild<QDoubleSpinBox *>()->setValue(2.0);
    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(slider.findChild<QSlider *>()->value(), 200);
}

TEST(ParameterSliderTest, DoubleClickResets)
{
    ParameterSlider slider{"Contrast", -100, 100, 0, 0};
    slider.setValue(40);
    QSignalSpy spy{&slider, &ParameterSlider::valueChanged};
    QTest::mouseDClick(slider.findChild<QSlider *>(), Qt::LeftButton);
    EXPECT_DOUBLE_EQ(slider.value(), 0);
    ASSERT_GE(spy.count(), 1);
    EXPECT_DOUBLE_EQ(spy.last().at(0).toDouble(), 0);
}

TEST(AdjustmentsPanelTest, SectionsFollowThePipeline)
{
    const auto sections = sectionsOf(*core::makeStandardPipeline());
    ASSERT_EQ(sections.size(), 6u);
    EXPECT_EQ(sections[0].title, "White Balance");
    EXPECT_EQ(sections[1].title, "Defringe");
    EXPECT_EQ(sections[2].title, "Tone");
    EXPECT_EQ(sections[2].parameters.front().id, "Exposure2012");
    EXPECT_EQ(sections[4].title, "HDR");
    EXPECT_EQ(sections[5].title, "Sharpening");
}

TEST(AdjustmentsPanelTest, EmitsTheSettingsWhenASliderMoves)
{
    AdjustmentsPanel panel{sectionsOf(*core::makeStandardPipeline())};
    QSignalSpy spy{&panel, &AdjustmentsPanel::settingsChanged};

    core::Settings shown;
    shown.setValue("Shadows2012", 30);
    panel.setSettings(shown);
    EXPECT_EQ(spy.count(), 0);
    EXPECT_DOUBLE_EQ(panel.slider("Shadows2012")->value(), 30);

    panel.slider("Exposure2012")->findChild<QSlider *>()->setValue(50);
    ASSERT_EQ(spy.count(), 1);
    const auto settings = spy.takeFirst().at(0).value<core::Settings>();
    EXPECT_DOUBLE_EQ(settings.values().at("Exposure2012"), 0.5);
    EXPECT_DOUBLE_EQ(settings.values().at("Shadows2012"), 30);
}

TEST(ImageViewTest, FitsThenZoomsRelativeToTheFullSize)
{
    ImageView view;
    view.resize(400, 300);
    view.show();
    QSignalSpy spy{&view, &ImageView::zoomChanged};

    QImage preview{200, 100, QImage::Format_RGB888};
    preview.fill(Qt::gray);
    view.setImage(preview, QSize{2000, 1000});
    EXPECT_TRUE(view.isFitToWindow());
    EXPECT_NEAR(view.zoom(), 0.2, 0.01);

    view.actualSize();
    EXPECT_FALSE(view.isFitToWindow());
    EXPECT_DOUBLE_EQ(view.zoom(), 1.0);

    // The full image replacing the preview keeps the zoom.
    QImage full{2000, 1000, QImage::Format_RGB888};
    full.fill(Qt::gray);
    view.setImage(full);
    EXPECT_DOUBLE_EQ(view.zoom(), 1.0);

    view.fitToWindow();
    EXPECT_TRUE(view.isFitToWindow());
    EXPECT_GE(spy.count(), 3);
}

TEST(CollapsibleSectionTest, ClickingTheTitleFoldsTheContent)
{
    CollapsibleSection section{"Tone"};
    auto *content = new QLabel{"inside"};
    section.addWidget(content);
    section.show();
    QSignalSpy spy{&section, &CollapsibleSection::expandedChanged};
    EXPECT_TRUE(section.isExpanded());
    EXPECT_TRUE(content->isVisible());

    QTest::mouseClick(section.findChild<QToolButton *>(), Qt::LeftButton);
    EXPECT_FALSE(section.isExpanded());
    EXPECT_FALSE(content->isVisible());
    ASSERT_EQ(spy.count(), 1);
    EXPECT_FALSE(spy.takeFirst().at(0).toBool());

    // Set by the client: shown, but not reported back.
    section.setExpanded(true);
    EXPECT_TRUE(content->isVisible());
    EXPECT_EQ(spy.count(), 0);
}

TEST(AdjustmentsPanelTest, SectionsFoldAndReportIt)
{
    AdjustmentsPanel panel{sectionsOf(*core::makeStandardPipeline())};
    panel.show();
    QSignalSpy spy{&panel, &AdjustmentsPanel::sectionExpandedChanged};
    EXPECT_EQ(panel.sectionTitles().size(), 6);
    EXPECT_TRUE(panel.isSectionExpanded("Defringe"));

    panel.setSectionExpanded("Defringe", false);
    EXPECT_FALSE(panel.isSectionExpanded("Defringe"));
    EXPECT_FALSE(panel.slider("DefringePurpleAmount")->isVisible());
    EXPECT_EQ(spy.count(), 0);

    const auto headers = panel.findChildren<QToolButton *>();
    const auto tone = std::ranges::find_if(headers, [](QToolButton *b) {
        return b->text() == "Tone";
    });
    ASSERT_NE(tone, headers.end());
    QTest::mouseClick(*tone, Qt::LeftButton);
    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.first().at(0).toString(), "Tone");
    EXPECT_FALSE(spy.first().at(1).toBool());
}

TEST(ImageViewTest, PickModeReportsTheClickInPixelsOfTheFullImage)
{
    ImageView view;
    view.resize(400, 300);
    view.show();
    QImage preview{200, 100, QImage::Format_RGB888};
    preview.fill(Qt::gray);
    view.setImage(preview, QSize{2000, 1000});
    view.actualSize();
    QSignalSpy picked{&view, &ImageView::pointPicked};
    QSignalSpy cancelled{&view, &ImageView::pickCancelled};

    // Not picking: a click drags, and picks nothing.
    QTest::mouseClick(view.viewport(), Qt::LeftButton, {}, QPoint{200, 150});
    EXPECT_EQ(picked.count(), 0);

    view.setPickMode(true);
    const double x = view.horizontalScrollBar()->value() + 200;
    const double y = view.verticalScrollBar()->value() + 150;
    QTest::mouseClick(view.viewport(), Qt::LeftButton, {}, QPoint{200, 150});
    ASSERT_EQ(picked.count(), 1);
    const QPointF point = picked.first().at(0).toPointF();
    EXPECT_NEAR(point.x(), x, 1.0);
    EXPECT_NEAR(point.y(), y, 1.0);
    // At 100%, the circle's screen pixels are the image's.
    EXPECT_DOUBLE_EQ(picked.first().at(1).toDouble(), ImageView::PICK_RADIUS);

    QTest::keyClick(&view, Qt::Key_Escape);
    EXPECT_FALSE(view.isPickMode());
    EXPECT_EQ(cancelled.count(), 1);
}

TEST(WhiteBalancePickerTest, SetsTheWhiteBalanceAndKeepsTheOtherValues)
{
    core::Image image = testutils::uniform(100, 100, 0.5f, 0.5f, 0.5f);
    // A warm grey card in the middle.
    for (int y = 40; y < 60; ++y) {
        for (int x = 40; x < 60; ++x) {
            float *p = image.pixel(x, y);
            p[0] = 0.6f, p[1] = 0.5f, p[2] = 0.35f;
        }
    }
    core::Settings settings;
    settings.setValue("Exposure2012", 0.4);
    const auto pick = pickWhiteBalance(image, settings, QPointF{50, 50}, 5);
    ASSERT_TRUE(pick);
    EXPECT_DOUBLE_EQ(pick->settings.values().at("Exposure2012"), 0.4);
    // Warm light is corrected by cooling: a temperature below 0.
    EXPECT_LT(pick->settings.values().at("IncrementalTemperature"), 0);
    EXPECT_FALSE(pick->clamped);
}

TEST(WhiteBalancePickerTest, ExplainsAnAreaItCannotUse)
{
    const core::Image white = testutils::uniform(20, 20, 1.0f, 1.0f, 1.0f);
    const auto clipped = pickWhiteBalance(white, {}, QPointF{10, 10}, 3);
    ASSERT_FALSE(clipped);
    EXPECT_TRUE(clipped.error().contains("overexposed"));

    const core::Image black = testutils::uniform(20, 20, 0, 0, 0);
    EXPECT_FALSE(pickWhiteBalance(black, {}, QPointF{10, 10}, 3));
}
