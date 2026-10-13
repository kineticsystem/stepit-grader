// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "../testutils/Fakes.h"
#include "../testutils/TestImages.h"

#include "core/StandardPipeline.h"
#include "editor/Document.h"
#include "editor/ImageExportUtils.h"
#include "editor/viewmodel/SaveImageViewModel.h"

#include <QSettings>
#include <QSignalSpy>
#include <QTest>

#include <gtest/gtest.h>

#include <filesystem>

namespace {

using testutils::FakeReader;
using testutils::FakeStore;

/// The picture the fake reader opens: no file, so only its name matters.
const QString PICTURE = "/pictures/a.tif";

QString pathOf(const std::filesystem::path &path)
{
    return QString::fromStdU16String(path.u16string());
}

struct SaveImageViewModelTest : ::testing::Test
{
    std::filesystem::path folder = testutils::outputFolder();
    /// A file of its own, not the user's settings.
    QSettings settings{pathOf(folder / "settings.ini"), QSettings::IniFormat};
    std::shared_ptr<const core::Pipeline> pipeline = core::makeStandardPipeline();
    Document document{
        std::make_shared<FakeReader>(), std::make_shared<FakeStore>(), pipeline->parameters()};
    SaveImageViewModel viewModel{&document, pipeline, &settings};

    void open()
    {
        QSignalSpy opened{&document, &Document::opened};
        document.open(PICTURE);
        ASSERT_TRUE(QTest::qWaitFor([&] { return !opened.isEmpty(); }, 5000));
    }

    const QString &tiff() const { return ImageExportUtils::imageFormats().front().filter; }
    const QString &jpeg() const { return ImageExportUtils::imageFormats().back().filter; }
};

} // namespace

TEST_F(SaveImageViewModelTest, SuggestsAFileBesideThePictureWithTheFormatsSuffix)
{
    open();
    EXPECT_EQ(viewModel.suggestedPath(tiff()), "/pictures/a-graded.tif");
    EXPECT_EQ(viewModel.suggestedPath(jpeg()), "/pictures/a-graded.jpg");
}

TEST_F(SaveImageViewModelTest, OffersTheFirstFormatUntilOneIsUsed)
{
    EXPECT_EQ(viewModel.preferredFilter(), tiff());
    settings.setValue("imageFormat", "a format since removed");
    EXPECT_EQ(viewModel.preferredFilter(), tiff());
}

TEST_F(SaveImageViewModelTest, OffersTheFormatUsedLast)
{
    open();
    QSignalSpy saved{&viewModel, &SaveImageViewModel::saved};
    ASSERT_TRUE(viewModel.save(pathOf(folder / "graded.jpg"), jpeg()).has_value());
    ASSERT_TRUE(QTest::qWaitFor([&] { return !saved.isEmpty(); }, 5000));

    const SaveImageViewModel next{&document, pipeline, &settings};
    EXPECT_EQ(next.preferredFilter(), jpeg());
}

TEST_F(SaveImageViewModelTest, SavesTheGradedPictureGivingANameWithoutSuffixTheFormats)
{
    open();
    QSignalSpy saved{&viewModel, &SaveImageViewModel::saved};
    QSignalSpy failed{&viewModel, &SaveImageViewModel::saveFailed};
    const auto saving = viewModel.save(pathOf(folder / "graded"), jpeg());
    ASSERT_TRUE(saving.has_value()) << saving.error().toStdString();
    EXPECT_EQ(*saving, pathOf(folder / "graded.jpg"));

    ASSERT_TRUE(QTest::qWaitFor([&] { return saved.count() + failed.count() > 0; }, 5000));
    ASSERT_EQ(saved.count(), 1);
    EXPECT_EQ(saved.first().at(0).toString(), *saving);
    EXPECT_TRUE(std::filesystem::exists(folder / "graded.jpg"));
}

TEST_F(SaveImageViewModelTest, RefusesToReplaceThePicture)
{
    open();
    QSignalSpy saved{&viewModel, &SaveImageViewModel::saved};
    QSignalSpy failed{&viewModel, &SaveImageViewModel::saveFailed};
    // Its own name, and its name without the suffix the format would add.
    EXPECT_FALSE(viewModel.save(PICTURE, tiff()).has_value());
    EXPECT_FALSE(viewModel.save("/pictures/a", tiff()).has_value());

    QTest::qWait(100);
    EXPECT_TRUE(saved.isEmpty());
    EXPECT_TRUE(failed.isEmpty());
}

TEST_F(SaveImageViewModelTest, RefusesWithNoPictureOpen)
{
    EXPECT_FALSE(viewModel.save(pathOf(folder / "graded.tif"), tiff()).has_value());
}

TEST_F(SaveImageViewModelTest, ReportsAFileThatCannotBeWritten)
{
    open();
    QSignalSpy saved{&viewModel, &SaveImageViewModel::saved};
    QSignalSpy failed{&viewModel, &SaveImageViewModel::saveFailed};
    ASSERT_TRUE(viewModel.save("/no/such/folder/graded.tif", tiff()).has_value());

    ASSERT_TRUE(QTest::qWaitFor([&] { return saved.count() + failed.count() > 0; }, 5000));
    EXPECT_EQ(failed.count(), 1);
    EXPECT_TRUE(saved.isEmpty());
}
