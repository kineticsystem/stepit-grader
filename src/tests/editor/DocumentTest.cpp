// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "../testutils/Fakes.h"
#include "../testutils/TestImages.h"

#include "core/StandardPipeline.h"
#include "core/effects/ExposureEffect.h"
#include "editor/Document.h"
#include "editor/PreviewRenderer.h"

#include <QSignalSpy>
#include <QTest>

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>

namespace {

using testutils::FakeReader;
using testutils::FakeStore;

/// The document's wait before it saves, short so the tests run fast, long enough to check before.
constexpr std::chrono::milliseconds SAVE_DELAY{200};
/// Long enough for a save to have happened had one been due.
constexpr int SETTLE_MS = 3 * SAVE_DELAY.count();

core::Settings contrast(double value)
{
    core::Settings settings;
    settings.setValue("Contrast2012", value);
    return settings;
}

struct Fixture : ::testing::Test
{
    std::shared_ptr<FakeStore> store = std::make_shared<FakeStore>();
    std::shared_ptr<const core::Pipeline> pipeline = core::makeStandardPipeline();
    Document document{std::make_shared<FakeReader>(), store, pipeline->parameters(), SAVE_DELAY};

    void openAndWait(const QString &path)
    {
        QSignalSpy opened{&document, &Document::opened};
        QSignalSpy failed{&document, &Document::openFailed};
        document.open(path);
        ASSERT_TRUE(QTest::qWaitFor([&] { return opened.count() + failed.count() > 0; }, 5000));
    }

    double savedContrast(const std::filesystem::path &image) const
    {
        return store->files.at(image).values().at("Contrast2012");
    }
};

using DocumentTest = Fixture;
using PreviewRendererTest = Fixture;

} // namespace

TEST_F(DocumentTest, OpensTheImageWithItsSavedSettings)
{
    core::Settings saved;
    saved.setValue("Exposure2012", 0.7);
    store->stored = saved;
    QSignalSpy changed{&document, &Document::settingsChanged};

    openAndWait("/pictures/a.tif");
    ASSERT_TRUE(document.isOpen());
    EXPECT_EQ(document.image()->width, 300);
    EXPECT_EQ(document.path(), "/pictures/a.tif");
    EXPECT_EQ(document.settingsPath(), "/pictures/a.xmp");
    EXPECT_DOUBLE_EQ(document.settings().values().at("Exposure2012"), 0.7);
    EXPECT_FALSE(document.isModified());
    EXPECT_EQ(changed.count(), 1);
}

TEST_F(DocumentTest, ReportsAnImageThatCannotBeRead)
{
    QSignalSpy failed{&document, &Document::openFailed};
    openAndWait("/pictures/a.jpg");
    EXPECT_FALSE(document.isOpen());
    ASSERT_EQ(failed.count(), 1);
    EXPECT_TRUE(failed.first().at(0).toString().contains("a.jpg"));
}

TEST_F(DocumentTest, TracksChangesNotWrittenYet)
{
    openAndWait("/pictures/a.tif");
    QSignalSpy modified{&document, &Document::modifiedChanged};

    document.setSettings(contrast(20));
    EXPECT_TRUE(document.isModified());

    // Back to the defaults is back to what the picture has without a sidecar.
    document.resetSettings();
    EXPECT_FALSE(document.isModified());
    EXPECT_EQ(modified.count(), 2);

    document.setSettings(contrast(20));
    ASSERT_TRUE(document.save());
    EXPECT_FALSE(document.isModified());
    EXPECT_EQ(store->saves, 1);
    EXPECT_DOUBLE_EQ(savedContrast("/pictures/a.tif"), 20);
}

TEST_F(DocumentTest, SavesAChangeAfterTheDelay)
{
    openAndWait("/pictures/a.tif");
    QSignalSpy saved{&document, &Document::saved};

    document.setSettings(contrast(20));
    QTest::qWait(SAVE_DELAY.count() / 4);
    EXPECT_EQ(store->saves, 0);
    EXPECT_TRUE(document.isModified());

    ASSERT_TRUE(QTest::qWaitFor([&] { return store->saves > 0; }, 5000));
    EXPECT_EQ(store->saves, 1);
    EXPECT_DOUBLE_EQ(savedContrast("/pictures/a.tif"), 20);
    EXPECT_FALSE(document.isModified());
    ASSERT_EQ(saved.count(), 1);
    EXPECT_EQ(saved.first().at(0).toString(), "/pictures/a.xmp");
}

TEST_F(DocumentTest, SavesQuickChangesOnce)
{
    openAndWait("/pictures/a.tif");
    // As a slider dragged: a change every few milliseconds, each well within the delay.
    for (int i = 1; i <= 10; ++i) {
        document.setSettings(contrast(i * 5));
        QTest::qWait(SAVE_DELAY.count() / 10);
    }
    EXPECT_EQ(store->saves, 0);
    QTest::qWait(SETTLE_MS);
    EXPECT_EQ(store->saves, 1);
    EXPECT_DOUBLE_EQ(savedContrast("/pictures/a.tif"), 50);
}

TEST_F(DocumentTest, SavesNothingWithoutAChange)
{
    openAndWait("/pictures/a.tif");
    // A change undone before the delay leaves nothing to write either.
    document.setSettings(contrast(20));
    document.resetSettings();
    QTest::qWait(SETTLE_MS);
    EXPECT_TRUE(document.save());
    EXPECT_EQ(store->saves, 0);
    EXPECT_TRUE(store->files.empty());
}

TEST_F(DocumentTest, SavesAtOnceOnRequest)
{
    openAndWait("/pictures/a.tif");
    document.setSettings(contrast(20));
    ASSERT_TRUE(document.save());
    EXPECT_EQ(store->saves, 1);
    EXPECT_FALSE(document.isModified());
    // The save waiting was that one.
    QTest::qWait(SETTLE_MS);
    EXPECT_EQ(store->saves, 1);
}

TEST_F(DocumentTest, SavesBeforeAnotherPictureOpens)
{
    openAndWait("/pictures/a.tif");
    document.setSettings(contrast(20));
    openAndWait("/pictures/b.tif");

    EXPECT_EQ(document.path(), "/pictures/b.tif");
    EXPECT_EQ(store->saves, 1);
    EXPECT_DOUBLE_EQ(savedContrast("/pictures/a.tif"), 20);
    EXPECT_FALSE(store->files.contains("/pictures/b.tif"));
    EXPECT_FALSE(document.settings().has("Contrast2012"));
    EXPECT_FALSE(document.isModified());
    QTest::qWait(SETTLE_MS);
    EXPECT_EQ(store->saves, 1);
}

TEST_F(DocumentTest, RevertsToTheSettingsItWasOpenedWith)
{
    core::Settings opened;
    opened.setValue("Exposure2012", 0.7);
    store->stored = opened;
    openAndWait("/pictures/a.tif");

    core::Settings changed = opened;
    changed.setValue("Contrast2012", 20);
    document.setSettings(changed);
    ASSERT_TRUE(document.save());

    // Reverting is a change like any other, saved in turn.
    document.revertSettings();
    EXPECT_TRUE(document.settings().sameAs(opened, pipeline->parameters()));
    EXPECT_TRUE(document.isModified());
    ASSERT_TRUE(QTest::qWaitFor([&] { return store->saves == 2; }, 5000));
    EXPECT_TRUE(store->files.at("/pictures/a.tif").sameAs(opened, pipeline->parameters()));
    EXPECT_FALSE(document.isModified());
}

TEST_F(DocumentTest, ReportsSettingsThatCannotBeSaved)
{
    openAndWait("/pictures/a.tif");
    document.setSettings(contrast(20));
    store->failSaving = true;
    QSignalSpy failed{&document, &Document::saveFailed};
    EXPECT_FALSE(document.save());
    EXPECT_EQ(failed.count(), 1);
    EXPECT_TRUE(failed.first().at(0).toString().contains("disk full"));
    EXPECT_TRUE(document.isModified());
}

TEST_F(DocumentTest, RetriesAFailedSaveAtTheNextChange)
{
    openAndWait("/pictures/a.tif");
    store->failSaving = true;
    QSignalSpy failed{&document, &Document::saveFailed};
    document.setSettings(contrast(20));
    ASSERT_TRUE(QTest::qWaitFor([&] { return failed.count() > 0; }, 5000));
    EXPECT_TRUE(document.isModified());

    // No retry in a loop: the next attempt waits for the next change.
    QTest::qWait(SETTLE_MS);
    EXPECT_EQ(failed.count(), 1);

    store->failSaving = false;
    document.setSettings(contrast(30));
    ASSERT_TRUE(QTest::qWaitFor([&] { return store->saves > 0; }, 5000));
    EXPECT_DOUBLE_EQ(savedContrast("/pictures/a.tif"), 30);
    EXPECT_FALSE(document.isModified());
}

TEST_F(DocumentTest, LoadsTheSettingsOfAnotherFileAsAChange)
{
    openAndWait("/pictures/a.tif");
    // A grading saved for another picture: every value of it, the others at their defaults.
    core::Settings other;
    other.setValue("Exposure2012", 0.8);
    store->files["/looks/warm.xmp"] = other;
    document.setSettings(contrast(20));
    QSignalSpy loaded{&document, &Document::settingsLoaded};

    ASSERT_TRUE(document.loadSettings("/looks/warm.xmp"));
    ASSERT_EQ(loaded.count(), 1);
    EXPECT_EQ(loaded.first().at(0).toString(), "/looks/warm.xmp");
    EXPECT_DOUBLE_EQ(document.settings().value(core::ExposureEffect::EXPOSURE), 0.8);
    EXPECT_FALSE(document.settings().has("Contrast2012"));

    // Saved to the picture's own sidecar, as any change.
    ASSERT_TRUE(QTest::qWaitFor([&] { return store->files.contains("/pictures/a.tif"); }, 5000));
    EXPECT_DOUBLE_EQ(store->files["/pictures/a.tif"].values().at("Exposure2012"), 0.8);

    // Undone as any change.
    document.revertSettings();
    EXPECT_FALSE(document.settings().has("Exposure2012"));
    EXPECT_EQ(document.settingsNameFilter(), "*.xmp");
}

TEST_F(DocumentTest, ReportsSettingsThatCannotBeLoaded)
{
    openAndWait("/pictures/a.tif");
    document.setSettings(contrast(20));
    QSignalSpy failed{&document, &Document::settingsLoadFailed};
    EXPECT_FALSE(document.loadSettings("/looks/missing.xmp"));
    ASSERT_EQ(failed.count(), 1);
    EXPECT_TRUE(failed.first().at(0).toString().contains("missing.xmp"));
    // The settings are as they were.
    EXPECT_DOUBLE_EQ(document.settings().values().at("Contrast2012"), 20);
}

TEST(DocumentWithoutPictureTest, LoadsNoSettings)
{
    Document document{
        std::make_shared<testutils::FakeReader>(),
        std::make_shared<testutils::FakeStore>(),
        core::makeStandardPipeline()->parameters()};
    EXPECT_FALSE(document.loadSettings("/looks/warm.xmp"));
}

TEST_F(PreviewRendererTest, RendersAPreviewThenTheFullImage)
{
    openAndWait("/pictures/a.tif");
    PreviewRenderer renderer{pipeline, 100, std::chrono::milliseconds{10}};
    QSignalSpy rendered{&renderer, &PreviewRenderer::rendered};
    renderer.setSource(document.image());

    core::Settings settings;
    settings.setValue("Exposure2012", 1.0);
    renderer.render(settings);

    ASSERT_TRUE(QTest::qWaitFor([&] { return rendered.count() >= 2; }, 5000));
    const QImage preview = rendered.at(0).at(0).value<QImage>();
    EXPECT_EQ(preview.width(), 100);
    EXPECT_EQ(rendered.at(0).at(1).toSize(), QSize(300, 200));
    EXPECT_FALSE(rendered.at(0).at(2).toBool());

    const QImage full = rendered.at(1).at(0).value<QImage>();
    EXPECT_EQ(full.size(), QSize(300, 200));
    EXPECT_TRUE(rendered.at(1).at(2).toBool());
    // Middle grey one stop brighter: linear 0.36, sRGB about 161.
    EXPECT_NEAR(qGreen(full.pixel(10, 10)), 161, 2);
}

TEST_F(PreviewRendererTest, RendersOnlyTheLatestSettingsInFull)
{
    openAndWait("/pictures/a.tif");
    PreviewRenderer renderer{pipeline, 100, std::chrono::milliseconds{50}};
    QSignalSpy rendered{&renderer, &PreviewRenderer::rendered};
    renderer.setSource(document.image());

    for (int i = 1; i <= 5; ++i) {
        core::Settings settings;
        settings.setValue("Exposure2012", i * 0.1);
        renderer.render(settings);
    }
    ASSERT_TRUE(
        QTest::qWaitFor([&] { return !rendered.isEmpty() && rendered.last().at(2).toBool(); }, 5000));
    QTest::qWait(200);
    int fullPasses = 0;
    for (const auto &arguments : rendered) {
        fullPasses += arguments.at(2).toBool() ? 1 : 0;
    }
    EXPECT_EQ(fullPasses, 1);
    EXPECT_LE(rendered.count(), 6);
}

TEST_F(PreviewRendererTest, ShowsSettingsRenderedAlreadyAtOnce)
{
    openAndWait("/pictures/a.tif");
    PreviewRenderer renderer{pipeline, 100, std::chrono::milliseconds{10}};
    QSignalSpy rendered{&renderer, &PreviewRenderer::rendered};
    renderer.setSource(document.image());
    const auto waitForFull = [&] {
        return QTest::qWaitFor(
            [&] { return !rendered.isEmpty() && rendered.last().at(2).toBool(); }, 5000);
    };

    // After, then before: both rendered in full.
    core::Settings after;
    after.setValue("Exposure2012", 1.0);
    renderer.render(after);
    ASSERT_TRUE(waitForFull());
    const QImage afterImage = rendered.last().at(0).value<QImage>();
    rendered.clear();
    renderer.render(core::Settings{});
    ASSERT_TRUE(waitForFull());
    rendered.clear();

    // Back to after: shown in full at once, from the cache, with no pass.
    renderer.render(after);
    ASSERT_EQ(rendered.count(), 1);
    EXPECT_TRUE(rendered.first().at(2).toBool());
    EXPECT_EQ(rendered.first().at(0).value<QImage>(), afterImage);
    QTest::qWait(100);
    EXPECT_EQ(rendered.count(), 1);

    // A new picture forgets the pictures kept.
    renderer.setSource(document.image());
    rendered.clear();
    renderer.render(after);
    EXPECT_EQ(rendered.count(), 0);
    ASSERT_TRUE(waitForFull());
}
