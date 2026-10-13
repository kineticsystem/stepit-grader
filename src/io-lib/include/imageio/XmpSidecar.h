// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#pragma once

#include "imageio/SettingsStore.h"

#include <string>

namespace imageio {

/**
 * The settings in an XMP sidecar, as most raw editors write them: next to the image, with
 * its name and the extension .xmp, e.g. IMG_0001.xmp for IMG_0001.CR2, and each parameter as the
 * attribute of the `crs` namespace named by its id, e.g. crs:Exposure2012="+0.50".
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

    /**
     * `<name>.xmp` for a camera raw file or a DNG, as raw editors name it; for any other image,
     * e.g. a TIFF, its whole name with `.xmp`, `<name>.tif.xmp`, so a raw file and a TIFF of the
     * same name in one folder each keep their own.
     */
    std::filesystem::path location(const std::filesystem::path &image) const override;
    /**
     * Where a sidecar of an image that is not raw was saved until it took its whole name,
     * `<name>.xmp`, read when the new one is missing; nothing when a raw file of the same name
     * beside it owns that file, or the image is raw itself.
     */
    std::optional<std::filesystem::path> formerLocation(const std::filesystem::path &image) const;
    /// The settings of the image's sidecar, or of its former one when it has none yet.
    std::optional<core::Settings> load(
        const std::filesystem::path &image,
        const std::vector<core::Parameter> &parameters) const override;
    /**
     * Updates the sidecar, keeping what else it holds. One that is not XMP is replaced by a new
     * one, and kept beside it, as damagedCopy() names it.
     */
    void save(
        const std::filesystem::path &image,
        const core::Settings &settings,
        const std::vector<core::Parameter> &parameters) const override;
};

/// Where a sidecar that could not be updated is kept, e.g. `IMG_0001.xmp.damaged`.
std::filesystem::path damagedCopy(const std::filesystem::path &sidecar);

namespace xmp {

/// The namespace of the settings, `crs`, the one most raw editors use.
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

/// A value as raw editors write it: "+0.50", "-12", "0", "40".
std::string format(const core::Parameter &parameter, double value);

} // namespace xmp
} // namespace imageio
