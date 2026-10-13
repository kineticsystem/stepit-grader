// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "../testutils/Fakes.h"
#include "../testutils/TestImages.h"

#include "core/StandardPipeline.h"
#include "editor/Document.h"
#include "editor/viewmodel/LeavePictureViewModel.h"

#include <QSignalSpy>
#include <QTest>

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>

namespace {

using testutils::FakeReader;
using testutils::FakeStore;
using Refusal = LeavePictureViewModel::Refusal;

/// The document's wait before it saves, short so the tests run fast.
constexpr std::chrono::milliseconds SAVE_DELAY{200};
/// Long enough for a picture to have opened, had it been asked to.
constexpr int SETTLE_MS = 3 * SAVE_DELAY.count();
constexpr int WAIT_MS = 5000;

QString pathOf(const std::filesystem::path &path)
{
    return QString::fromStdU16String(path.u16string());
}

core::Settings contrast(double value)
{
    core::Settings settings;
    settings.setValue("Contrast2012", value);
    return settings;
}

struct LeavePictureViewModelTest : ::testing::Test
{
    std::filesystem::path folder = testutils::outputFolder();
    std::shared_ptr<FakeStore> store = std::make_shared<FakeStore>();
    Document document{
        std::make_shared<FakeReader>(),
        store,
        core::makeStandardPipeline()->parameters(),
        SAVE_DELAY};
    LeavePictureViewModel viewModel{&document};

    /// An empty file in the test's folder: it exists, and the fake reader reads a .tif.
    QString file(const std::string &name) const
    {
        std::ofstream{folder / name};
        return pathOf(folder / name);
    }

    bool waitForOpen(const QString &path)
    {
        const auto opened = [&] { return document.isOpen() && document.path() == path; };
        return QTest::qWaitFor(opened, WAIT_MS);
    }

    void openAndWait(const QString &path)
    {
        ASSERT_TRUE(viewModel.open(path).has_value());
        ASSERT_TRUE(waitForOpen(path));
    }

    /// The settings saved for `path`, read under the store's lock: the file thread writes them.
    std::optional<double> savedContrast(const QString &path) const
    {
        const std::lock_guard lock{store->mutex};
        const auto found = store->files.find(path.toStdU16String());
        if (found == store->files.end() || !found->second.has("Contrast2012")) {
            return std::nullopt;
        }
        return found->second.values().at("Contrast2012");
    }
};

} // namespace

TEST_F(LeavePictureViewModelTest, OpensAtOnceWithNothingToWrite)
{
    QSignalSpy opening{&viewModel, &LeavePictureViewModel::opening};
    const QString a = file("a.tif");
    ASSERT_TRUE(viewModel.open(a).has_value());
    ASSERT_EQ(opening.count(), 1);
    EXPECT_EQ(opening.first().at(0).toString(), a);
    EXPECT_TRUE(waitForOpen(a));
}

TEST_F(LeavePictureViewModelTest, WritesTheEditBeforeOpeningAnother)
{
    const QString a = file("a.tif");
    const QString b = file("b.tif");
    openAndWait(a);
    document.setSettings(contrast(20));

    QSignalSpy opening{&viewModel, &LeavePictureViewModel::opening};
    ASSERT_TRUE(viewModel.open(b).has_value());
    // The edit is written first, in the background: the other picture waits for it.
    EXPECT_TRUE(opening.isEmpty());
    ASSERT_TRUE(waitForOpen(b));
    EXPECT_EQ(savedContrast(a), 20);
}

TEST_F(LeavePictureViewModelTest, RefusesAMissingFileAFileThatIsNotAPictureAndOneAtATime)
{
    EXPECT_EQ(viewModel.open(pathOf(folder / "gone.tif")).error(), Refusal::Missing);
    EXPECT_EQ(viewModel.open(file("c.jpg")).error(), Refusal::NotAPicture);

    ASSERT_TRUE(viewModel.open(file("a.tif")).has_value());
    EXPECT_EQ(viewModel.open(file("b.tif")).error(), Refusal::Busy);
}

TEST_F(LeavePictureViewModelTest, OpensTheLastPictureAskedWhileTheEditIsWritten)
{
    const QString a = file("a.tif");
    const QString c = file("c.tif");
    openAndWait(a);
    document.setSettings(contrast(20));
    ASSERT_TRUE(viewModel.open(file("b.tif")).has_value());
    ASSERT_TRUE(viewModel.open(c).has_value());
    EXPECT_TRUE(waitForOpen(c));
}

TEST_F(LeavePictureViewModelTest, AsksWhenTheEditWouldBeLostAndStays)
{
    const QString a = file("a.tif");
    openAndWait(a);
    store->failSaving = true;
    document.setSettings(contrast(20));

    QSignalSpy lost{&viewModel, &LeavePictureViewModel::editWouldBeLost};
    ASSERT_TRUE(viewModel.open(file("b.tif")).has_value());
    ASSERT_TRUE(QTest::qWaitFor([&] { return !lost.isEmpty(); }, WAIT_MS));
    EXPECT_EQ(lost.first().at(0).toString(), "a.tif");

    viewModel.stay();
    QTest::qWait(SETTLE_MS);
    EXPECT_EQ(document.path(), a);
    EXPECT_TRUE(document.isModified());
}

TEST_F(LeavePictureViewModelTest, AsksWhenTheEditWouldBeLostAndLeavesItOnDiscard)
{
    openAndWait(file("a.tif"));
    store->failSaving = true;
    document.setSettings(contrast(20));

    QSignalSpy lost{&viewModel, &LeavePictureViewModel::editWouldBeLost};
    const QString b = file("b.tif");
    ASSERT_TRUE(viewModel.open(b).has_value());
    ASSERT_TRUE(QTest::qWaitFor([&] { return !lost.isEmpty(); }, WAIT_MS));
    viewModel.discard();
    EXPECT_TRUE(waitForOpen(b));
}

TEST_F(LeavePictureViewModelTest, ClosesAtOnceWithNothingToWrite)
{
    EXPECT_TRUE(viewModel.close());
    openAndWait(file("a.tif"));
    EXPECT_TRUE(viewModel.close());
}

TEST_F(LeavePictureViewModelTest, ClosesOnceTheEditIsWritten)
{
    const QString a = file("a.tif");
    openAndWait(a);
    document.setSettings(contrast(20));

    QSignalSpy ready{&viewModel, &LeavePictureViewModel::readyToClose};
    EXPECT_FALSE(viewModel.close());
    ASSERT_TRUE(QTest::qWaitFor([&] { return !ready.isEmpty(); }, WAIT_MS));
    EXPECT_EQ(savedContrast(a), 20);
    EXPECT_TRUE(viewModel.close());
}

TEST_F(LeavePictureViewModelTest, WritesAChangeMadeWhileTheEditIsWrittenBeforeLeaving)
{
    const QString a = file("a.tif");
    openAndWait(a);
    document.setSettings(contrast(20));

    QSignalSpy ready{&viewModel, &LeavePictureViewModel::readyToClose};
    EXPECT_FALSE(viewModel.close());
    // Changed again while the first save runs: written in turn before the window closes.
    document.setSettings(contrast(40));
    ASSERT_TRUE(QTest::qWaitFor([&] { return !ready.isEmpty(); }, WAIT_MS));
    EXPECT_EQ(savedContrast(a), 40);
    EXPECT_FALSE(document.isModified());
}
