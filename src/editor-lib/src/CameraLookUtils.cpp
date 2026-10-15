// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "editor/CameraLookUtils.h"
#include "core/Color.h"
#include "core/Geometry.h"
#include "imageio/Error.h"

#include <QFileInfo>

#include <cmath>

namespace CameraLookUtils {

namespace {

/// The share of the progress bar for reading the raw files; the search has the rest.
constexpr int READING_SHARE = 10;
/// How far the JPEG's shape may differ from the raw's, as a fraction: a camera may leave a few
/// pixels out of its JPEG, never a strip of the picture.
constexpr double SHAPE_TOLERANCE = 0.02;

} // namespace

std::expected<core::Image, QString> cameraJpegImage(
    const std::vector<std::uint8_t> &jpeg, int orientation, int width, int height)
{
    QImage decoded = QImage::fromData(jpeg.data(), static_cast<int>(jpeg.size()), "JPEG");
    if (decoded.isNull()) {
        return std::unexpected(QObject::tr("its camera JPEG cannot be read"));
    }
    // The JPEG is stored as the sensor saw it, as the raw is: its size before turning it upright.
    const bool turned = orientation >= 5 && orientation <= 8;
    const QSize stored = turned ? QSize{height, width} : QSize{width, height};
    if (decoded.size() != stored) {
        const double ours = static_cast<double>(stored.width()) / stored.height();
        const double theirs = static_cast<double>(decoded.width()) / decoded.height();
        if (std::abs(ours / theirs - 1) > SHAPE_TOLERANCE) {
            return std::unexpected(
                QObject::tr("its camera JPEG is not the same shape as the picture"));
        }
        decoded = decoded.scaled(stored, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }
    decoded = decoded.convertToFormat(QImage::Format_RGB888);

    std::array<float, 256> table{};
    for (int i = 0; i < 256; ++i) {
        table[i] = static_cast<float>(core::color::srgbDecode(i / 255.0));
    }
    core::Image image{decoded.width(), decoded.height()};
    for (int y = 0; y < decoded.height(); ++y) {
        const uchar *line = decoded.constScanLine(y);
        float *out = image.pixel(0, y);
        for (int x = 0; x < decoded.width() * 3; ++x) {
            out[x] = table[line[x]];
        }
    }
    return core::geometry::orient(image, orientation);
}

std::expected<core::match::Result, QString> matchCameraLook(
    const QStringList &raws,
    const imageio::CameraShotReader &reader,
    const core::Pipeline &pipeline,
    const core::Settings &base,
    ProgressMonitor &status)
{
    std::vector<core::match::Sample> samples;
    for (int i = 0; i < raws.size(); ++i) {
        if (status.isStopped()) {
            return std::unexpected(QObject::tr("Stopped before the search began."));
        }
        const QString name = QFileInfo{raws[i]}.fileName();
        status.setStatus(Status{
            QObject::tr("Reading %1 (%2 of %3)…").arg(name).arg(i + 1).arg(raws.size()),
            READING_SHARE * i / static_cast<int>(raws.size())});
        try {
            imageio::CameraShot shot = reader.read(std::filesystem::path{raws[i].toStdU16String()});
            if (shot.cameraJpeg.empty()) {
                return std::unexpected(QObject::tr("%1 has no camera JPEG.").arg(name));
            }
            auto jpeg = cameraJpegImage(
                shot.cameraJpeg, shot.orientation, shot.image.width, shot.image.height);
            if (!jpeg) {
                return std::unexpected(QObject::tr("%1: %2.").arg(name, jpeg.error()));
            }
            const core::Image source = core::geometry::downscale(shot.image, CAMERA_LOOK_SIZE);
            const core::Image target = core::geometry::downscale(*jpeg, CAMERA_LOOK_SIZE);
            const double scale = static_cast<double>(source.width) / shot.image.width;
            samples.push_back(core::match::makeSample(source, target, scale));
        } catch (const imageio::Error &e) {
            return std::unexpected(QString::fromStdString(e.what()));
        }
    }

    core::match::Options options;
    options.base = base;
    options.stopped = [&status] { return status.isStopped(); };
    options.progress = [&status](const core::match::Progress &progress) {
        status.setStatus(Status{
            QObject::tr("%1, stage %2 of %3: trial %4, difference %5")
                .arg(QString::fromStdString(progress.stageName))
                .arg(progress.stage + 1)
                .arg(progress.stages)
                .arg(progress.trials)
                .arg(progress.bestError, 0, 'f', 2),
            READING_SHARE
                + static_cast<int>(std::lround(progress.fraction * (100 - READING_SHARE)))});
    };
    core::match::Result result = core::match::fit(samples, pipeline, options);
    status.setStatus(Status{
        result.stopped ? QObject::tr("Stopped.") : QObject::tr("Done."),
        100,
        result.stopped ? Status::State::Stopped : Status::State::Finished});
    return result;
}

} // namespace CameraLookUtils
