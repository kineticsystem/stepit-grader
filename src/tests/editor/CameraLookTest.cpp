// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "../testutils/Fakes.h"
#include "core/StandardPipeline.h"
#include "editor/CameraLookUtils.h"
#include "editor/ProgressMonitor.h"
#include "editor/WaitingDialog.h"
#include "imageio/RawReader.h"

#include <QBuffer>
#include <QCloseEvent>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QSignalSpy>
#include <QTest>

#include <gtest/gtest.h>

#include <filesystem>

namespace {

std::vector<std::uint8_t> jpegOf(const QImage &image)
{
    QByteArray bytes;
    QBuffer buffer{&bytes};
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "JPEG", 95);
    return {bytes.begin(), bytes.end()};
}

/// A JPEG of 40 × 20: red on its left half, blue on its right.
std::vector<std::uint8_t> halves()
{
    QImage image{40, 20, QImage::Format_RGB888};
    image.fill(Qt::blue);
    for (int y = 0; y < 20; ++y) {
        for (int x = 0; x < 20; ++x) {
            image.setPixelColor(x, y, Qt::red);
        }
    }
    return jpegOf(image);
}

} // namespace

TEST(CameraLookTest, TheJpegComesUprightInLinearSrgb)
{
    const auto upright = CameraLookUtils::cameraJpegImage(halves(), 1, 40, 20);
    ASSERT_TRUE(upright.has_value());
    EXPECT_EQ(upright->width, 40);
    EXPECT_NEAR(upright->pixel(5, 10)[0], 1.0f, 0.03f);
    EXPECT_NEAR(upright->pixel(35, 10)[2], 1.0f, 0.03f);

    // Turned a quarter clockwise, as a raw shot in portrait: 20 wide, red at the top.
    const auto turned = CameraLookUtils::cameraJpegImage(halves(), 6, 20, 40);
    ASSERT_TRUE(turned.has_value());
    EXPECT_EQ(turned->width, 20);
    EXPECT_EQ(turned->height, 40);
    EXPECT_NEAR(turned->pixel(10, 5)[0], 1.0f, 0.03f);
    EXPECT_NEAR(turned->pixel(10, 35)[2], 1.0f, 0.03f);
}

TEST(CameraLookTest, AJpegOfAnotherShapeIsRefusedAndOfAnotherSizeScaled)
{
    EXPECT_FALSE(CameraLookUtils::cameraJpegImage(halves(), 1, 40, 40).has_value());
    EXPECT_FALSE(CameraLookUtils::cameraJpegImage({1, 2, 3}, 1, 40, 20).has_value());
    const auto larger = CameraLookUtils::cameraJpegImage(halves(), 1, 80, 40);
    ASSERT_TRUE(larger.has_value());
    EXPECT_EQ(larger->width, 80);
}

TEST(WaitingDialogTest, FollowsTheStatusAndAsksToStop)
{
    auto monitor = std::make_shared<ProgressMonitor>();
    WaitingDialog dialog{monitor, "Stop"};
    dialog.show();
    monitor->setStatus(Status{"Halfway", 50});
    EXPECT_EQ(dialog.message(), "Halfway");
    EXPECT_EQ(dialog.progressBar()->value(), 50);

    QSignalSpy spy{monitor.get(), &ProgressMonitor::stopRequested};
    QTest::mouseClick(dialog.stopButton(), Qt::LeftButton);
    EXPECT_TRUE(monitor->isStopped());
    EXPECT_FALSE(dialog.stopButton()->isEnabled());
    EXPECT_EQ(spy.count(), 1);
}

TEST(WaitingDialogTest, ClosingItAsksToStopAndWaitsForTheTask)
{
    auto monitor = std::make_shared<ProgressMonitor>();
    WaitingDialog dialog{monitor, "Stop"};
    dialog.show();
    dialog.close();
    EXPECT_TRUE(monitor->isStopped());
    EXPECT_TRUE(dialog.isVisible());
}

TEST(CameraLookTest, MatchesARawFileAndStopsWhenAsked)
{
    const std::filesystem::path file = std::filesystem::path{RAW_SAMPLES_DIR}
                                       / "Canon_EOS_5D_Mark_II_1.CR2";
    if (!std::filesystem::exists(file)) {
        GTEST_SKIP() << "No " << file << ": run modules/raw-converter/bin/fetch-samples.sh";
    }
    ProgressMonitor status;
    std::vector<Status> statuses;
    QObject::connect(&status, &ProgressMonitor::status, [&](const Status &s) {
        statuses.push_back(s);
        // A dozen trials are enough to see it work: stop then.
        if (s.message.contains("trial 12,")) {
            status.stop();
        }
    });
    const auto result = CameraLookUtils::matchCameraLook(
        {QString::fromStdString(file.string())},
        imageio::RawShotReader{},
        *core::makeStandardPipeline(),
        core::Settings{},
        status);
    ASSERT_TRUE(result.has_value()) << result.error().toStdString();
    EXPECT_TRUE(result->stopped);
    EXPECT_LT(result->errorAfter, result->errorBefore);
    ASSERT_FALSE(statuses.empty());
    EXPECT_TRUE(statuses.front().message.startsWith("Reading"));
    EXPECT_EQ(statuses.back().state, Status::State::Stopped);
}

TEST(CameraLookTest, AFileThatIsNotARawIsReported)
{
    ProgressMonitor status;
    const auto result = CameraLookUtils::matchCameraLook(
        {"/no/such/file.CR2"},
        imageio::RawShotReader{},
        *core::makeStandardPipeline(),
        core::Settings{},
        status);
    EXPECT_FALSE(result.has_value());
}
