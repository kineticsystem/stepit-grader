// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#pragma once

#include "imageio/SettingsStore.h"

#include <string>

namespace imageio {

/**
 * The settings in an XMP sidecar, as Lightroom and Camera Raw write them: next to the image, with
 * its name and the extension .xmp, e.g. IMG_0001.xmp for IMG_0001.CR2, and each parameter as the
 * attribute of the Camera Raw namespace named by its id, e.g. crs:Exposure2012="+0.50".
 *
 * Saving keeps everything else the file holds, e.g. a rating or keywords written by another
 * program, and replaces only the values of the parameters.
 */
class XmpSidecar : public SettingsStore
{
public:
    /**
     * The settings in an XMP file of any name, e.g. one saved for a picture and applied to
     * others. Throws imageio::Error when it cannot be read.
     */
    core::Settings loadFile(
        const std::filesystem::path &xmp,
        const std::vector<core::Parameter> &parameters) const override;

    std::string extension() const override { return "xmp"; }

    std::filesystem::path location(const std::filesystem::path &image) const override;
    std::optional<core::Settings> load(
        const std::filesystem::path &image,
        const std::vector<core::Parameter> &parameters) const override;
    void save(
        const std::filesystem::path &image,
        const core::Settings &settings,
        const std::vector<core::Parameter> &parameters) const override;
};

namespace xmp {

/// The namespace of the Camera Raw settings.
inline constexpr const char *CRS_NAMESPACE = "http://ns.adobe.com/camera-raw-settings/1.0/";

/// The settings found in an XMP document; those it does not have are left out.
core::Settings parse(const std::string &document, const std::vector<core::Parameter> &parameters);

/**
 * The document with the settings written into it, or a new document when `document` is empty.
 * Throws imageio::Error when `document` is not XML.
 */
std::string write(
    const std::string &document,
    const core::Settings &settings,
    const std::vector<core::Parameter> &parameters);

/// A value as Lightroom writes it: "+0.50", "-12", "0", "40".
std::string format(const core::Parameter &parameter, double value);

} // namespace xmp
} // namespace imageio
