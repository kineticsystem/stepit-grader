// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "editor/ImageExportUtils.h"
#include "editor/ImageConversionUtils.h"
#include "imageio/Error.h"
#include "imageio/TiffWriter.h"

#include <QColorSpace>
#include <QImageWriter>

namespace ImageExportUtils {

namespace {

/// The quality of a JPEG, 0 to 100: high, so it is a picture to keep, not a preview.
constexpr int JPEG_QUALITY = 95;

} // namespace

const std::vector<ImageFormat> &imageFormats()
{
    static const std::vector<ImageFormat> formats{
        {QObject::tr("TIFF, 16 bits, sRGB (*.tif *.tiff)"), "tif", false, imageio::ColourSpace::Srgb},
        {QObject::tr("TIFF, 16 bits, Adobe RGB (*.tif *.tiff)"),
         "tif",
         false,
         imageio::ColourSpace::AdobeRgb},
        {QObject::tr("TIFF, 16 bits, ProPhoto RGB (*.tif *.tiff)"),
         "tif",
         false,
         imageio::ColourSpace::ProPhoto},
        {QObject::tr("JPEG, 8 bits, sRGB (*.jpg *.jpeg)"), "jpg", true, imageio::ColourSpace::Srgb},
    };
    return formats;
}

std::expected<void, QString> saveImage(
    const core::Image &picture,
    const core::Settings &settings,
    const core::Pipeline &pipeline,
    const QString &path,
    const ImageFormat &format)
{
    const auto graded = pipeline.render(picture, settings);
    if (!graded) {
        return std::unexpected(QObject::tr("The picture could not be graded."));
    }
    if (format.jpeg) {
        QImage image = ImageConversionUtils::toQImage(*graded);
        image.setColorSpace(QColorSpace::SRgb);
        QImageWriter writer{path, "JPEG"};
        writer.setQuality(JPEG_QUALITY);
        if (!writer.write(image)) {
            return std::unexpected(
                QObject::tr("Cannot write %1: %2").arg(path, writer.errorString()));
        }
        return {};
    }
    try {
        imageio::TiffWriter{{format.colourSpace, true}}
            .write(*graded, std::filesystem::path{path.toStdU16String()});
    } catch (const imageio::Error &e) {
        return std::unexpected(QString::fromStdString(e.what()));
    }
    return {};
}

} // namespace ImageExportUtils
