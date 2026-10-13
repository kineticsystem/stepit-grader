// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "../testutils/Fakes.h"
#include "../testutils/TestImages.h"

#include "core/StandardPipeline.h"
#include "editor/viewmodel/CameraLookViewModel.h"
#include "editor/Document.h"

#include <QBuffer>
#include <QImage>
#include <QSignalSpy>
#include <QTest>

#include <gtest/gtest.h>

namespace {

using testutils::FakeShotReader;
using testutils::FakeStore;

/// A match takes many trials: a test stops it once it has seen it work.
constexpr int TRIALS_TO_SEE = 12;
constexpr int WAIT_MS = 30000;

/// Opens raw files as well as TIFFs, as the editor's reader does: a grey picture for either.
class RawAndTiffReader : public testutils::FakeReader
{
public:
    std::vector<std::string> extensions() const override { return {"tif", "cr2"}; }
    core::Image read(const std::filesystem::path &) const override
    {
        return testutils::uniform(40, 20, 0.1f, 0.1f, 0.1f);
    }
};

/// A camera JPEG of 40 × 20, a light grey: the camera made the dark raw brighter.
std::vector<std::uint8_t> lightJpeg()
{
    QImage image{40, 20, QImage::Format_RGB888};
    image.fill(QColor{180, 180, 180});
    QByteArray bytes;
    QBuffer buffer{&bytes};
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "JPEG", 95);
    return {bytes.begin(), bytes.end()};
}

struct CameraLookViewModelTest : ::testing::Test
{
    std::shared_ptr<const core::Pipeline> pipeline = core::makeStandardPipeline();
    std::shared_ptr<FakeShotReader> shots = [] {
        auto reader = std::make_shared<FakeShotReader>();
        reader->shot = {testutils::uniform(40, 20, 0.1f, 0.1f, 0.1f), lightJpeg(), 1};
        return reader;
    }();
    Document document{
        std::make_shared<RawAndTiffReader>(),
        std::make_shared<FakeStore>(),
        pipeline->parameters()};
    CameraLookViewModel viewModel{&document, pipeline, shots};

    void open(const QString &path)
    {
        QSignalSpy opened{&document, &Document::opened};
        document.open(path);
        ASSERT_TRUE(QTest::qWaitFor([&] { return !opened.isEmpty(); }, 5000));
    }

    /// Matches `raws`, stopped after a few trials; whether it found a grade.
    bool matchAndWait(const QStringList &raws)
    {
        QSignalSpy finished{&viewModel, &CameraLookViewModel::finished};
        QSignalSpy failed{&viewModel, &CameraLookViewModel::failed};
        const auto status = viewModel.start(raws);
        if (!status) {
            return false;
        }
        QObject::connect(status.get(), &StatusController::status, [status](const Status &s) {
            if (s.message.contains(QString{"trial %1,"}.arg(TRIALS_TO_SEE))) {
                status->stop();
            }
        });
        return QTest::qWaitFor([&] { return finished.count() + failed.count() > 0; }, WAIT_MS)
               && !finished.isEmpty();
    }
};

} // namespace

TEST_F(CameraLookViewModelTest, MatchesTheOpenPictureOnlyWhenItIsARawFile)
{
    EXPECT_FALSE(viewModel.matchesOpenPicture());
    open("/pictures/a.CR2");
    EXPECT_TRUE(viewModel.matchesOpenPicture());
    EXPECT_EQ(viewModel.picture(), "/pictures/a.CR2");
    open("/pictures/a.tif");
    EXPECT_FALSE(viewModel.matchesOpenPicture());
    EXPECT_EQ(viewModel.folder(), "/pictures");
}

TEST_F(CameraLookViewModelTest, OffersTheRawFilesTheReaderReadsInEitherCase)
{
    EXPECT_TRUE(viewModel.rawNameFilter().contains("*.cr2 *.CR2"));
}

TEST_F(CameraLookViewModelTest, FindsAGradeAndAppliesItOverThePicturesOwn)
{
    open("/pictures/a.tif");
    core::Settings own;
    own.setValue("Clarity2012", 30);
    document.setSettings(own);
    EXPECT_TRUE(viewModel.summary().isEmpty());

    ASSERT_TRUE(matchAndWait({"/raws/a.cr2"}));
    EXPECT_FALSE(viewModel.isRunning());
    EXPECT_TRUE(viewModel.summary().contains("stopped"));

    viewModel.apply();
    // The camera's JPEG is lighter: the grade found brightens the picture; the clarity, which the
    // match does not set, stays.
    ASSERT_TRUE(document.settings().has("Exposure2012"));
    EXPECT_GT(document.settings().values().at("Exposure2012"), 0.0);
    EXPECT_DOUBLE_EQ(document.settings().values().at("Clarity2012"), 30);
}

TEST_F(CameraLookViewModelTest, ReportsARawFileWithoutItsCameraJpeg)
{
    open("/pictures/a.tif");
    shots->shot.cameraJpeg.clear();
    QSignalSpy failed{&viewModel, &CameraLookViewModel::failed};
    EXPECT_FALSE(matchAndWait({"/raws/a.cr2"}));
    ASSERT_EQ(failed.count(), 1);
    EXPECT_TRUE(failed.first().at(0).toString().contains("no camera JPEG"));
    EXPECT_FALSE(viewModel.isRunning());

    // Nothing found: nothing to apply.
    viewModel.apply();
    EXPECT_TRUE(document.settings().values().empty());
}

TEST_F(CameraLookViewModelTest, StartsOneMatchAtATimeAndNoneWithoutFiles)
{
    open("/pictures/a.tif");
    EXPECT_EQ(viewModel.start({}), nullptr);
    const auto first = viewModel.start({"/raws/a.cr2"});
    ASSERT_NE(first, nullptr);
    EXPECT_EQ(viewModel.start({"/raws/a.cr2"}), nullptr);
    first->stop();
    ASSERT_TRUE(QTest::qWaitFor([&] { return !viewModel.isRunning(); }, WAIT_MS));
}
