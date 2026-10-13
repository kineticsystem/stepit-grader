// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "../testutils/TestImages.h"

#include "core/StandardPipeline.h"
#include "editor/Icons.h"
#include "editor/RecentFilesMenu.h"
#include "editor/Theme.h"
#include "editor/WhiteBalancePicker.h"
#include "widgets/AdjustmentsPanel.h"
#include "widgets/CollapsibleSection.h"
#include "widgets/CurveView.h"
#include "widgets/HistogramView.h"
#include "widgets/ImageView.h"
#include "widgets/ParameterSlider.h"
#include "widgets/Sections.h"
#include "widgets/SliderStyle.h"

#include <QAbstractSpinBox>
#include <QApplication>
#include <QDir>
#include <QLabel>
#include <QLocale>
#include <QProxyStyle>
#include <QScrollBar>
#include <QSignalSpy>
#include <QSlider>
#include <QStyle>
#include <QStyleOptionSlider>
#include <QTest>
#include <QToolButton>

#include <gtest/gtest.h>

#include <cmath>

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
}

/// The text the slider shows for its value: the label that is not its name.
QString shownValue(const ParameterSlider &slider, const QString &name)
{
    for (const QLabel *label : slider.findChildren<QLabel *>()) {
        if (label->text() != name) {
            return label->text();
        }
    }
    return {};
}

TEST(ParameterSliderTest, ShowsTheValueAsTextNotToEdit)
{
    ParameterSlider slider{"Exposure", -5, 5, 0, 2};
    // No box to type in: the slider is the one way to change the value.
    EXPECT_EQ(slider.findChild<QAbstractSpinBox *>(), nullptr);
    EXPECT_EQ(shownValue(slider, "Exposure"), QLocale{}.toString(0.0, 'f', 2));

    slider.setValue(1.2);
    EXPECT_EQ(shownValue(slider, "Exposure"), QLocale{}.toString(1.2, 'f', 2));
    slider.findChild<QSlider *>()->setValue(-125);
    EXPECT_EQ(shownValue(slider, "Exposure"), QLocale{}.toString(-1.25, 'f', 2));
    // A value a hair below 0 shows as 0, as the slider's step, not as -0.00.
    slider.setValue(-0.001);
    EXPECT_EQ(shownValue(slider, "Exposure"), QLocale{}.toString(0.0, 'f', 2));

    ParameterSlider whole{"Contrast", -100, 100, 0, 0};
    whole.setValue(-35);
    EXPECT_EQ(shownValue(whole, "Contrast"), "-35");
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

TEST(ParameterSliderTest, GrabsTheCircleAnywhereAndMovesOnTheLine)
{
    ParameterSlider slider{"Contrast", -100, 100, 0, 0};
    slider.resize(300, slider.sizeHint().height());
    static_cast<QWidget &>(slider).show(); // ParameterSlider::show() is its own, private
    auto *line = slider.findChild<QSlider *>();
    QStyleOptionSlider option;
    option.initFrom(line);
    option.orientation = Qt::Horizontal;
    option.minimum = line->minimum();
    option.maximum = line->maximum();
    option.sliderPosition = line->sliderPosition();
    const QRect circle
        = line->style()->subControlRect(QStyle::CC_Slider, &option, QStyle::SC_SliderHandle, line);
    // A circle large enough to grab at once, in the middle for the default of 0.
    EXPECT_GE(circle.width(), 12);
    EXPECT_EQ(circle.width(), circle.height());
    EXPECT_NEAR(circle.center().x(), line->width() / 2, 2);
    // A plain line: the same colour on both sides of the circle, not filled up to it as most
    // styles fill a slider's groove; and the circle empty, in the window's colour.
    const QImage drawn = line->grab().toImage();
    const int y = circle.center().y();
    EXPECT_EQ(drawn.pixelColor(line->width() / 4, y), drawn.pixelColor(line->width() * 3 / 4, y));
    EXPECT_EQ(drawn.pixelColor(circle.center()), line->palette().color(QPalette::Window));

    // Pressed near its edge, the circle is grabbed: the value stays until it is dragged.
    QTest::mousePress(line, Qt::LeftButton, {}, circle.center() + QPoint{circle.width() / 2 - 2, 0});
    EXPECT_TRUE(line->isSliderDown());
    EXPECT_DOUBLE_EQ(slider.value(), 0);
    QTest::mouseRelease(line, Qt::LeftButton, {}, circle.center());

    // Pressed on the line, away from it, the value moves towards the press.
    QTest::mouseClick(line, Qt::LeftButton, {}, QPoint{line->width() - 4, circle.center().y()});
    EXPECT_GT(slider.value(), 0);
}

TEST(SliderStyleTest, GivesAnySliderTheLook)
{
    // A slider of a program's own, not a ParameterSlider's: the same line and circle.
    QSlider slider{Qt::Horizontal};
    SliderStyle::apply(&slider);
    EXPECT_EQ(slider.style(), SliderStyle::instance());
    EXPECT_TRUE(slider.testAttribute(Qt::WA_Hover));
}

TEST(AdjustmentsPanelTest, SectionsFollowThePipeline)
{
    // Without an order, the sections come as the effects run, each with its parameters in order.
    const auto sections = sectionsOf(*core::makeStandardPipeline());
    ASSERT_EQ(sections.size(), 8u);
    EXPECT_EQ(sections[0].title, "Color");
    EXPECT_EQ(sections[1].title, "Noise Reduction");
    EXPECT_EQ(sections[1].parameters.size(), 3u);
    EXPECT_EQ(sections[2].title, "Chromatic Aberration");
    EXPECT_EQ(sections[3].title, "Light");
    EXPECT_EQ(sections[3].parameters.front().id, "Exposure2012");
    EXPECT_EQ(sections[4].title, "Tone Curve");
    EXPECT_EQ(sections[4].parameters.size(), 7u);
    EXPECT_EQ(sections[5].title, "Effects");
    EXPECT_EQ(sections[6].title, "Color Mixer");
    EXPECT_EQ(sections[6].parameters.size(), 24u);
    EXPECT_EQ(sections[7].title, "HDR");
}

TEST(AdjustmentsPanelTest, SectionsFollowTheOrderGiven)
{
    const auto sections = sectionsOf(*core::makeStandardPipeline(), core::standardSectionOrder());
    QStringList titles;
    for (const auto &section : sections) {
        titles << section.title;
    }
    EXPECT_EQ(
        titles,
        (QStringList{
            "Color",
            "Light",
            "Tone Curve",
            "Color Mixer",
            "Effects",
            "Noise Reduction",
            "Chromatic Aberration",
            "HDR"}));
    // The colour, first, holds the white balance, which runs first too.
    QStringList color;
    for (const auto &parameter : sections[0].parameters) {
        color << QString::fromStdString(parameter.id);
    }
    EXPECT_EQ(color, (QStringList{"IncrementalTemperature", "IncrementalTint"}));
    // The light gathers the tones, then the vibrance and the saturation, which run after them.
    QStringList light;
    for (const auto &parameter : sections[1].parameters) {
        light << QString::fromStdString(parameter.id);
    }
    EXPECT_EQ(
        light,
        (QStringList{
            "Exposure2012",
            "Contrast2012",
            "Highlights2012",
            "Shadows2012",
            "Blacks2012",
            "Vibrance",
            "Saturation"}));
    // The effects gather the texture, the clarity and the sharpening, which runs last.
    QStringList effects;
    for (const auto &parameter : sections[4].parameters) {
        effects << QString::fromStdString(parameter.id);
    }
    EXPECT_EQ(effects, (QStringList{"Texture", "Clarity2012", "Sharpness"}));

    // A section the order does not name follows those it names, in the order of the pipeline.
    const auto partial = sectionsOf(*core::makeStandardPipeline(), {"HDR", "Light"});
    EXPECT_EQ(partial[0].title, "HDR");
    EXPECT_EQ(partial[1].title, "Light");
    EXPECT_EQ(partial[2].title, "Color");
    EXPECT_EQ(partial[3].title, "Noise Reduction");
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
    CollapsibleSection section{"Light"};
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
    EXPECT_EQ(panel.sectionTitles().size(), 8);
    EXPECT_TRUE(panel.isSectionExpanded("Chromatic Aberration"));

    panel.setSectionExpanded("Chromatic Aberration", false);
    EXPECT_FALSE(panel.isSectionExpanded("Chromatic Aberration"));
    EXPECT_FALSE(panel.slider("DefringePurpleAmount")->isVisible());
    EXPECT_EQ(spy.count(), 0);

    const auto headers = panel.findChildren<QToolButton *>();
    const auto light = std::ranges::find_if(headers, [](QToolButton *b) {
        return b->text() == "Light";
    });
    ASSERT_NE(light, headers.end());
    QTest::mouseClick(*light, Qt::LeftButton);
    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.first().at(0).toString(), "Light");
    EXPECT_FALSE(spy.first().at(1).toBool());
}

TEST(ParameterSliderTest, MarksHowFarItMovedFromItsDefault)
{
    ParameterSlider slider{"Contrast", -100, 100, 0, 0};
    slider.resize(300, 50);
    slider.QWidget::show();
    ASSERT_TRUE(QTest::qWaitForWindowExposed(&slider));
    QSlider *line = slider.findChild<QSlider *>();
    const QColor highlight = slider.palette().color(QPalette::Highlight);
    // Highlighted pixels along the line, between two x of the line.
    const auto highlighted = [&](int from, int to) {
        const QImage drawn = line->grab().toImage();
        int count = 0;
        for (int x = from; x < to; ++x) {
            // The rows around the line's middle.
            for (int y = line->height() / 2 - 3; y <= line->height() / 2 + 3; ++y) {
                const QColor c = drawn.pixelColor(x, y);
                if (std::abs(c.red() - highlight.red()) + std::abs(c.green() - highlight.green())
                        + std::abs(c.blue() - highlight.blue())
                    < 40) {
                    ++count;
                    break;
                }
            }
        }
        return count;
    };
    const int middle = line->width() / 2;
    EXPECT_EQ(highlighted(0, line->width()), 0); // At its default: a plain line.

    slider.setValue(-60);
    line->grab().save("/tmp/claude-1000/-home-robot-repo-lentigram/"
                      "b6a69ca5-33e2-4955-bcbd-2fb3203c0f07/scratchpad/moved.png");
    // From the circle, a fifth of the way from the left, to the middle, where the default is.
    EXPECT_GT(highlighted(line->width() / 4, middle - 4), line->width() / 5);
    EXPECT_EQ(highlighted(middle + 4, line->width()), 0);

    slider.setValue(0);
    EXPECT_EQ(highlighted(0, line->width()), 0);
}

TEST(AdjustmentsPanelTest, ASectionWidgetGoesAboveTheSlidersAndFoldsWithThem)
{
    AdjustmentsPanel panel{sectionsOf(*core::makeStandardPipeline())};
    auto *drawing = new QLabel{"curve"};
    panel.addSectionWidget("Tone Curve", drawing);
    // A section the panel does not have takes nothing, and keeps nothing.
    auto *stray = new QLabel{"stray"};
    panel.addSectionWidget("No Such Section", stray);
    delete stray;
    panel.resize(300, 2000);
    panel.show();
    ASSERT_TRUE(QTest::qWaitForWindowExposed(&panel));

    const auto *first = panel.slider("ParametricShadows");
    EXPECT_LT(drawing->mapTo(&panel, QPoint{0, 0}).y(), first->mapTo(&panel, QPoint{0, 0}).y());
    panel.setSectionExpanded("Tone Curve", false);
    EXPECT_FALSE(drawing->isVisible());
}

TEST(AdjustmentsPanelTest, TheTitleIsTheSameBlockOpenOrClosed)
{
    CollapsibleSection section{"Light"};
    section.addWidget(new QLabel{"slider"});
    section.resize(300, 120);
    section.show();
    ASSERT_TRUE(QTest::qWaitForWindowExposed(&section));
    auto *title = section.findChild<QToolButton *>();
    // Right of the text, where only the block is drawn.
    const QPoint inside{title->width() - 20, title->height() / 2};
    const QColor open = title->grab().toImage().pixelColor(inside);

    QTest::mouseClick(title, Qt::LeftButton);
    EXPECT_FALSE(section.isExpanded());
    EXPECT_TRUE(title->isChecked());
    EXPECT_EQ(title->grab().toImage().pixelColor(inside), open);
    QTest::mouseClick(title, Qt::LeftButton);
    EXPECT_TRUE(section.isExpanded());
    section.setExpanded(false);
    EXPECT_EQ(title->grab().toImage().pixelColor(inside), open);
}

TEST(AdjustmentsPanelTest, AHeadingNamesTheGroupBelowItAndStandsApart)
{
    AdjustmentsPanel panel{sectionsOf(*core::makeStandardPipeline())};
    panel.addHeading("HueAdjustmentRed", "Hue");
    panel.addHeading("SaturationAdjustmentRed", "Saturation");
    panel.addHeading("NoSuchSlider", "Nothing");
    panel.resize(300, 3000);
    panel.show();
    ASSERT_TRUE(QTest::qWaitForWindowExposed(&panel));

    const auto labels = panel.findChildren<QLabel *>();
    const auto named = [&labels](const QString &text) {
        // A heading, not the name of a slider, e.g. the Saturation of the Light section.
        const auto it = std::ranges::find_if(labels, [&](QLabel *l) {
            return l->text() == text && !qobject_cast<ParameterSlider *>(l->parentWidget());
        });
        return it == labels.end() ? nullptr : *it;
    };
    ASSERT_NE(named("Hue"), nullptr);
    ASSERT_NE(named("Saturation"), nullptr);
    EXPECT_EQ(named("Nothing"), nullptr);
    const auto top = [&panel](const QWidget *w) { return w->mapTo(&panel, QPoint{0, 0}).y(); };
    const auto bottom = [&](const QWidget *w) { return top(w) + w->height(); };
    // Each heading is just above the first slider of its group.
    EXPECT_LT(top(named("Saturation")), top(panel.slider("SaturationAdjustmentRed")));
    EXPECT_GT(top(named("Saturation")), bottom(panel.slider("HueAdjustmentMagenta")));
    // And stands apart from the group before it.
    EXPECT_GE(top(named("Saturation")) - bottom(panel.slider("HueAdjustmentMagenta")), 12);
}

TEST(AdjustmentsPanelTest, TheSlidersStandApartFromTheirTitle)
{
    CollapsibleSection section{"Light"};
    auto *content = new QLabel{"slider"};
    section.addWidget(content);
    section.show();
    ASSERT_TRUE(QTest::qWaitForWindowExposed(&section));
    const auto *title = section.findChild<QToolButton *>();
    const int gap = content->mapTo(&section, QPoint{0, 0}).y()
                    - (title->mapTo(&section, QPoint{0, 0}).y() + title->height());
    EXPECT_GE(gap, 8);
}

TEST(CurveViewTest, DrawsTheCurveItIsGiven)
{
    CurveView view;
    EXPECT_EQ(view.points().size(), 2u); // The diagonal, before any curve.
    // A curve that lifts the dark tones: at a quarter of the way along, it is halfway up.
    view.setCurve({{0, 0}, {0.25, 0.5}, {1, 1}}, {0.25, 0.5, 0.75});
    EXPECT_EQ(view.borders().size(), 3u);
    view.resize(200, view.heightForWidth(200));
    EXPECT_GE(view.height(), 160);
    const QImage image = view.grab().toImage();

    const QColor curve = view.palette().color(QPalette::Highlight);
    const auto near = [&](QPointF tone) {
        const QPoint at = view.toWidget(tone).toPoint();
        for (int dy = -2; dy <= 2; ++dy) {
            for (int dx = -2; dx <= 2; ++dx) {
                const QColor c = image.pixelColor(at + QPoint{dx, dy});
                if (std::abs(c.red() - curve.red()) + std::abs(c.green() - curve.green())
                        + std::abs(c.blue() - curve.blue())
                    < 60) {
                    return true;
                }
            }
        }
        return false;
    };
    EXPECT_TRUE(near({0.25, 0.5}));
    // Along the straight part after it, from (0.25, 0.5) to white.
    EXPECT_TRUE(near({0.6, 0.5 + (0.6 - 0.25) / 0.75 * 0.5}));
    // Not on the diagonal where the curve has left it.
    EXPECT_FALSE(near({0.25, 0.25}));
}

TEST(HistogramTest, CountsEachLevelAndTheClippedPixels)
{
    QImage image{100, 100, QImage::Format_RGB888};
    image.fill(QColor{128, 128, 128});
    Histogram histogram = histogramOf(image);
    EXPECT_EQ(histogram.pixels, 10000);
    EXPECT_EQ(histogram.counts[Histogram::RED][128], 10000);
    EXPECT_EQ(histogram.counts[Histogram::LUMINANCE][128], 10000);
    EXPECT_EQ(histogram.clippedBlack, 0);
    EXPECT_EQ(histogram.clippedWhite, 0);

    // The top half white in red only, the bottom row black in blue only: each clips one end.
    for (int y = 0; y < 50; ++y) {
        for (int x = 0; x < 100; ++x) {
            image.setPixelColor(x, y, QColor{255, 128, 128});
        }
    }
    for (int x = 0; x < 100; ++x) {
        image.setPixelColor(x, 99, QColor{128, 128, 0});
    }
    histogram = histogramOf(image);
    EXPECT_EQ(histogram.clippedWhite, 5000);
    EXPECT_EQ(histogram.clippedBlack, 100);
    EXPECT_EQ(histogramOf(QImage{}).pixels, 0);
}

TEST(HistogramTest, SamplesALargeImageOnAGrid)
{
    QImage image{4000, 3000, QImage::Format_RGB888};
    image.fill(QColor{10, 200, 90});
    const Histogram histogram = histogramOf(image, 500'000);
    EXPECT_LE(histogram.pixels, 500'000);
    EXPECT_GT(histogram.pixels, 250'000);
    EXPECT_EQ(histogram.counts[Histogram::GREEN][200], histogram.pixels);
    // Another format is read as the screen shows it.
    QImage argb = image.convertToFormat(QImage::Format_ARGB32);
    EXPECT_EQ(histogramOf(argb, 500'000).counts[Histogram::BLUE][90], histogram.pixels);
}

TEST(HistogramViewTest, LightsTheTriangleOfAClippedEndOnly)
{
    HistogramView view;
    view.resize(260, 110);
    QImage image{100, 100, QImage::Format_RGB888};
    image.fill(QColor{255, 255, 255});
    view.setHistogram(histogramOf(image));
    const QImage drawn = view.grab().toImage();
    const QColor lit = view.palette().color(QPalette::Highlight);
    const auto isLit = [&](QPoint at) {
        const QColor c = drawn.pixelColor(at);
        return std::abs(c.red() - lit.red()) + std::abs(c.green() - lit.green())
                   + std::abs(c.blue() - lit.blue())
               < 30;
    };
    // Inside each triangle, near its corner at the top.
    EXPECT_TRUE(isLit({260 - 2 - 4 - 2, 2 + 4 + 2}));
    EXPECT_FALSE(isLit({2 + 4 + 2, 2 + 4 + 2}));
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

TEST(AdjustmentsPanelTest, SectionsFoldAndScrollWithTheSlidersDisabled)
{
    AdjustmentsPanel panel{sectionsOf(*core::makeStandardPipeline())};
    panel.resize(300, 200);
    panel.show();
    panel.setControlsEnabled(false);
    EXPECT_FALSE(panel.slider("Exposure2012")->isEnabled());

    // A real click, through the window, on the title: it must still fold the section.
    const auto headers = panel.findChildren<QToolButton *>();
    const auto light = std::ranges::find_if(headers, [](QToolButton *b) {
        return b->text() == "Light";
    });
    ASSERT_NE(light, headers.end());
    EXPECT_TRUE((*light)->isEnabled());
    QTest::mouseClick(*light, Qt::LeftButton);
    EXPECT_FALSE(panel.isSectionExpanded("Light"));

    // The panel, too small for its sections, scrolls.
    const auto bars = panel.findChildren<QScrollBar *>();
    const auto vertical = std::ranges::find_if(bars, [](QScrollBar *b) {
        return b->orientation() == Qt::Vertical;
    });
    ASSERT_NE(vertical, bars.end());
    EXPECT_TRUE((*vertical)->isEnabled());
    EXPECT_GT((*vertical)->maximum(), 0);
}

namespace {

/// The file entries shown, as their tooltips, the full paths.
QStringList shown(const RecentFilesMenu &menu)
{
    QStringList paths;
    for (QAction *action : menu.actions()) {
        if (action->isVisible() && !action->data().toString().isEmpty()) {
            paths << action->toolTip();
        }
    }
    return paths;
}

} // namespace

TEST(RecentFilesMenuTest, KeepsTheLatestFirstWithoutDuplicates)
{
    RecentFilesMenu menu{"Open Recent", 3};
    QSignalSpy changed{&menu, &RecentFilesMenu::filesChanged};
    EXPECT_FALSE(menu.isEnabled());

    menu.addFile("/p/a.tif");
    menu.addFile("/p/b.CR2");
    menu.addFile("/p/a.tif");
    EXPECT_EQ(menu.files(), (QStringList{"/p/a.tif", "/p/b.CR2"}));
    EXPECT_TRUE(menu.isEnabled());

    // Past the maximum, the oldest goes.
    menu.addFile("/p/c.dng");
    menu.addFile("/p/d.dng");
    EXPECT_EQ(menu.files(), (QStringList{"/p/d.dng", "/p/c.dng", "/p/a.tif"}));
    EXPECT_EQ(shown(menu).size(), 3);
    EXPECT_EQ(changed.count(), 5);
    EXPECT_EQ(changed.last().at(0).toStringList(), menu.files());
}

TEST(RecentFilesMenuTest, ShowsNamesNumberedAndPathsAsTooltips)
{
    RecentFilesMenu menu{"Open Recent", 3};
    menu.setFiles({"/p/one/IMG_1.CR2", "/p/two/IMG_1.CR2"});
    const auto actions = menu.actions();
    EXPECT_EQ(actions[0]->text(), "&1 IMG_1.CR2");
    EXPECT_EQ(actions[1]->text(), "&2 IMG_1.CR2");
    EXPECT_EQ(actions[1]->toolTip(), QDir::toNativeSeparators("/p/two/IMG_1.CR2"));
    EXPECT_FALSE(actions[2]->isVisible());
}

TEST(RecentFilesMenuTest, ReportsTheFileChosenWithItsFullPath)
{
    RecentFilesMenu menu{"Open Recent", 3};
    menu.setFiles({"/p/a.tif", "/p/b.CR2"});
    QSignalSpy selected{&menu, &RecentFilesMenu::fileSelected};
    menu.actions()[1]->trigger();
    ASSERT_EQ(selected.count(), 1);
    EXPECT_EQ(selected.first().at(0).toString(), "/p/b.CR2");
}

TEST(RecentFilesMenuTest, ClearsAndRemoves)
{
    RecentFilesMenu menu{"Open Recent", 3};
    // Given by the client: shown, not reported back.
    QSignalSpy changed{&menu, &RecentFilesMenu::filesChanged};
    menu.setFiles({"/p/a.tif", "/p/b.CR2", "/p/c.dng"});
    EXPECT_EQ(changed.count(), 0);

    menu.removeFile("/p/b.CR2");
    EXPECT_EQ(menu.files(), (QStringList{"/p/a.tif", "/p/c.dng"}));
    menu.removeFile("/p/unknown.tif");
    EXPECT_EQ(changed.count(), 1);

    // The last action is Clear.
    menu.actions().last()->trigger();
    EXPECT_TRUE(menu.files().isEmpty());
    EXPECT_FALSE(menu.isEnabled());
    EXPECT_EQ(shown(menu).size(), 0);
    EXPECT_EQ(changed.count(), 2);
}

TEST(IconsTest, EveryIconIsBuiltInWithADrawingForEachTheme)
{
    const QPalette original = QApplication::palette();
    const QStringList names{
        "color-picker",
        "image-v-split",
        "document-open",
        "document-save",
        "zoom-in",
        "zoom-out",
        "zoom-fit-best",
        "zoom-original"};
    // The mean lightness of an icon's drawing, in a light or a dark theme.
    const auto lightness = [&](const QString &name, bool dark) {
        QPalette palette = original;
        palette.setColor(QPalette::Window, dark ? QColor{40, 40, 40} : QColor{240, 240, 240});
        QApplication::setPalette(palette);
        EXPECT_EQ(isDarkTheme(), dark);
        const QImage image = themedIcon(name).pixmap(48).toImage();
        int opaque = 0;
        double sum = 0;
        for (int y = 0; y < image.height(); ++y) {
            for (int x = 0; x < image.width(); ++x) {
                const QColor pixel = image.pixelColor(x, y);
                if (pixel.alpha() > 200) {
                    ++opaque;
                    sum += pixel.lightness();
                }
            }
        }
        // A drawing, not nothing.
        EXPECT_GT(opaque, 48 * 48 / 20) << name.toStdString();
        return opaque > 0 ? sum / opaque : 0.0;
    };
    for (const QString &name : names) {
        // Each theme has its own drawing: lighter for the dark theme, or, for an icon of two
        // tones whose tones swap, e.g. the before and after, just another one.
        const double light = lightness(name, false);
        const double dark = lightness(name, true);
        if (name == "image-v-split") {
            EXPECT_GT(std::abs(dark - light), 10) << name.toStdString();
        } else {
            EXPECT_GT(dark, light + 50) << name.toStdString();
        }
    }
    EXPECT_TRUE(themedIcon("no-such-icon").isNull());
    QApplication::setPalette(original);
}

TEST(ThemeTest, DarkAndLightAndBackToTheSystemsStyle)
{
    const QString system = QApplication::style()->name();
    const QPalette original = QApplication::palette();

    // Fusion, inside the proxy that gives it the theme's colours.
    const auto fusion = [] {
        const auto *proxy = qobject_cast<QProxyStyle *>(QApplication::style());
        return proxy != nullptr && proxy->baseStyle()->name() == "fusion";
    };
    applyTheme(Theme::Dark);
    EXPECT_TRUE(fusion());
    // The dark theme's grey.
    EXPECT_EQ(QApplication::palette().color(QPalette::Window), QColor(53, 53, 53));
    EXPECT_TRUE(isDarkTheme());
    // A disabled icon is grey, not darkened out of sight.
    const QPixmap disabled = themedIcon("zoom-in").pixmap(24, QIcon::Disabled);
    const QImage image = disabled.toImage();
    bool grey = false;
    for (int y = 0; y < image.height() && !grey; ++y) {
        for (int x = 0; x < image.width() && !grey; ++x) {
            grey = image.pixelColor(x, y).alpha() > 200
                   && image.pixelColor(x, y) == QColor(128, 128, 128);
        }
    }
    EXPECT_TRUE(grey);

    applyTheme(Theme::Light);
    EXPECT_TRUE(fusion());
    EXPECT_FALSE(isDarkTheme());

    applyTheme(Theme::System);
    EXPECT_EQ(QApplication::style()->name(), system);
    QApplication::setPalette(original);
}

TEST(ThemeTest, NamesRoundTrip)
{
    for (Theme theme : {Theme::System, Theme::Light, Theme::Dark}) {
        EXPECT_EQ(themeNamed(nameOf(theme)), theme);
    }
    EXPECT_FALSE(themeNamed("purple"));
}
