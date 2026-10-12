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
#include "widgets/ImageView.h"
#include "widgets/ParameterSlider.h"
#include "widgets/Sections.h"

#include <QApplication>
#include <QDir>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QProxyStyle>
#include <QScrollBar>
#include <QSignalSpy>
#include <QSlider>
#include <QStyle>
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
    ASSERT_EQ(sections.size(), 7u);
    EXPECT_EQ(sections[0].title, "White Balance");
    EXPECT_EQ(sections[1].title, "Noise Reduction");
    EXPECT_EQ(sections[1].parameters.size(), 3u);
    EXPECT_EQ(sections[2].title, "Defringe");
    EXPECT_EQ(sections[3].title, "Tone");
    EXPECT_EQ(sections[3].parameters.front().id, "Exposure2012");
    EXPECT_EQ(sections[5].title, "HDR");
    EXPECT_EQ(sections[6].title, "Sharpening");
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
    EXPECT_EQ(panel.sectionTitles().size(), 7);
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

TEST(AdjustmentsPanelTest, SectionsFoldAndScrollWithTheSlidersDisabled)
{
    AdjustmentsPanel panel{sectionsOf(*core::makeStandardPipeline())};
    panel.resize(300, 200);
    panel.show();
    panel.setControlsEnabled(false);
    EXPECT_FALSE(panel.slider("Exposure2012")->isEnabled());

    // A real click, through the window, on the title: it must still fold the section.
    const auto headers = panel.findChildren<QToolButton *>();
    const auto tone = std::ranges::find_if(headers, [](QToolButton *b) {
        return b->text() == "Tone";
    });
    ASSERT_NE(tone, headers.end());
    EXPECT_TRUE((*tone)->isEnabled());
    QTest::mouseClick(*tone, Qt::LeftButton);
    EXPECT_FALSE(panel.isSectionExpanded("Tone"));

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
