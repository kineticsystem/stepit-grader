// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "../testutils/Fakes.h"
#include "../testutils/TestImages.h"

#include "core/StandardPipeline.h"
#include "editor/Document.h"
#include "editor/PreviewRenderer.h"

#include <QSignalSpy>
#include <QTest>

#include <gtest/gtest.h>

namespace {

using testutils::FakeReader;
using testutils::FakeStore;

struct Fixture : ::testing::Test
{
    std::shared_ptr<FakeStore> store = std::make_shared<FakeStore>();
    std::shared_ptr<const core::Pipeline> pipeline = core::makeStandardPipeline();
    Document document{std::make_shared<FakeReader>(), store, pipeline->parameters()};

    void openAndWait(const QString &path)
    {
        QSignalSpy opened{&document, &Document::opened};
        QSignalSpy failed{&document, &Document::openFailed};
        document.open(path);
        ASSERT_TRUE(QTest::qWaitFor([&] { return opened.count() + failed.count() > 0; }, 5000));
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

TEST_F(DocumentTest, TracksChangesAgainstTheSavedSettings)
{
    openAndWait("/pictures/a.tif");
    QSignalSpy modified{&document, &Document::modifiedChanged};

    core::Settings settings;
    settings.setValue("Contrast2012", 20);
    document.setSettings(settings);
    EXPECT_TRUE(document.isModified());

    // Back to the defaults is back to what was saved, i.e. nothing.
    document.resetSettings();
    EXPECT_FALSE(document.isModified());
    EXPECT_EQ(modified.count(), 2);

    document.setSettings(settings);
    ASSERT_TRUE(document.save());
    EXPECT_FALSE(document.isModified());
    EXPECT_EQ(store->saves, 1);
    EXPECT_DOUBLE_EQ(store->stored->values().at("Contrast2012"), 20);

    document.resetSettings();
    document.revertSettings();
    EXPECT_DOUBLE_EQ(document.settings().values().at("Contrast2012"), 20);
    EXPECT_FALSE(document.isModified());
}

TEST_F(DocumentTest, ReportsSettingsThatCannotBeSaved)
{
    openAndWait("/pictures/a.tif");
    core::Settings settings;
    settings.setValue("Contrast2012", 20);
    document.setSettings(settings);
    store->failSaving = true;
    QSignalSpy failed{&document, &Document::saveFailed};
    EXPECT_FALSE(document.save());
    EXPECT_EQ(failed.count(), 1);
    EXPECT_TRUE(document.isModified());
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
