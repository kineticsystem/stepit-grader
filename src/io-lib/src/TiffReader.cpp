// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "imageio/TiffReader.h"
#include "imageio/Error.h"

#include "ColourProfiles.h"

#include "core/Geometry.h"
#include "core/Parallel.h"

#include <lcms2.h>
#include <tiffio.h>

#include <cstdint>
#include <cstring>
#include <memory>
#include <vector>

namespace imageio {

namespace {

using profiles::Profile;
using profiles::Transform;

struct TiffCloser
{
    void operator()(TIFF *tiff) const { TIFFClose(tiff); }
};
TIFF *open(const std::filesystem::path &path)
{
#ifdef _WIN32
    return TIFFOpenW(path.wstring().c_str(), "r");
#else
    return TIFFOpen(path.string().c_str(), "r");
#endif
}

/// The profile a file without one is taken to have.
Profile defaultProfile(bool grey, bool floating)
{
    if (floating) {
        return profiles::workingSpace();
    }
    if (grey) {
        // The grey of sRGB: its white and, near enough, its curve.
        cmsToneCurve *curve = cmsBuildGamma(nullptr, 2.2);
        Profile profile{cmsCreateGrayProfile(cmsD50_xyY(), curve)};
        cmsFreeToneCurve(curve);
        return profile;
    }
    return Profile{cmsCreate_sRGBProfile()};
}

cmsUInt32Number inputFormat(bool grey, int bits)
{
    if (bits == 32) {
        return grey ? TYPE_GRAY_FLT : TYPE_RGB_FLT;
    }
    if (bits == 16) {
        return grey ? TYPE_GRAY_16 : TYPE_RGB_16;
    }
    return grey ? TYPE_GRAY_8 : TYPE_RGB_8;
}

} // namespace

core::Image TiffReader::read(const std::filesystem::path &path) const
{
    const std::string name = path.string();
    // Said here rather than by libtiff, which prints its own line on the error output.
    if (!std::filesystem::exists(path)) {
        throw Error{"Cannot read " + name + ": no such file"};
    }
    TIFFSetWarningHandler(nullptr);
    std::unique_ptr<TIFF, TiffCloser> tiff{open(path)};
    if (!tiff) {
        throw Error{"Cannot open " + name};
    }
    TIFF *tif = tiff.get();

    uint32_t width = 0, height = 0;
    uint16_t samples = 1, bits = 8, format = SAMPLEFORMAT_UINT, photometric = PHOTOMETRIC_RGB;
    uint16_t planar = PLANARCONFIG_CONTIG, orientation = ORIENTATION_TOPLEFT;
    TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &width);
    TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &height);
    TIFFGetFieldDefaulted(tif, TIFFTAG_SAMPLESPERPIXEL, &samples);
    TIFFGetFieldDefaulted(tif, TIFFTAG_BITSPERSAMPLE, &bits);
    TIFFGetFieldDefaulted(tif, TIFFTAG_SAMPLEFORMAT, &format);
    TIFFGetFieldDefaulted(tif, TIFFTAG_PLANARCONFIG, &planar);
    TIFFGetFieldDefaulted(tif, TIFFTAG_ORIENTATION, &orientation);
    TIFFGetField(tif, TIFFTAG_PHOTOMETRIC, &photometric);

    const bool grey = photometric == PHOTOMETRIC_MINISBLACK;
    const bool floating = format == SAMPLEFORMAT_IEEEFP;
    if (width == 0 || height == 0) {
        throw Error{"Cannot read " + name + ": no image"};
    }
    if (!grey && !(photometric == PHOTOMETRIC_RGB && samples >= 3)) {
        throw Error{"Cannot read " + name + ": only RGB and grey TIFFs are supported"};
    }
    if (!(bits == 8 || bits == 16 || (bits == 32 && floating)) || (floating && bits != 32)
        || (!floating && format != SAMPLEFORMAT_UINT)) {
        throw Error{
            "Cannot read " + name + ": " + std::to_string(bits)
            + "-bit samples of this kind are not supported"};
    }

    // The channels kept, packed: 1 for grey, 3 for RGB, the others dropped.
    const int channels = grey ? 1 : 3;
    const std::size_t bytes = bits / 8;
    const std::size_t packedRow = static_cast<std::size_t>(width) * channels * bytes;
    std::vector<uint8_t> packed(packedRow * height);
    std::vector<uint8_t> line(TIFFScanlineSize(tif));
    for (uint32_t y = 0; y < height; ++y) {
        uint8_t *out = &packed[packedRow * y];
        if (planar == PLANARCONFIG_CONTIG) {
            if (TIFFReadScanline(tif, line.data(), y, 0) < 0) {
                throw Error{"Cannot read " + name + ": row " + std::to_string(y) + " is damaged"};
            }
            for (uint32_t x = 0; x < width; ++x) {
                std::memcpy(out + x * channels * bytes, &line[x * samples * bytes], channels * bytes);
            }
        } else {
            for (int c = 0; c < channels; ++c) {
                if (TIFFReadScanline(tif, line.data(), y, static_cast<uint16_t>(c)) < 0) {
                    throw Error{
                        "Cannot read " + name + ": row " + std::to_string(y) + " is damaged"};
                }
                for (uint32_t x = 0; x < width; ++x) {
                    std::memcpy(out + (x * channels + c) * bytes, &line[x * bytes], bytes);
                }
            }
        }
    }

    uint32_t iccSize = 0;
    void *iccData = nullptr;
    Profile input;
    if (TIFFGetField(tif, TIFFTAG_ICCPROFILE, &iccSize, &iccData) && iccSize > 0) {
        input.reset(cmsOpenProfileFromMem(iccData, iccSize));
        if (input && cmsGetColorSpace(input.get()) != (grey ? cmsSigGrayData : cmsSigRgbData)) {
            input.reset();
        }
    }
    if (!input) {
        input = defaultProfile(grey, floating);
    }
    const Profile output = profiles::workingSpace();
    // Without a cache, one transform can be used by several threads at once.
    const Transform transform{cmsCreateTransform(
        input.get(),
        inputFormat(grey, bits),
        output.get(),
        TYPE_RGB_FLT,
        INTENT_RELATIVE_COLORIMETRIC,
        cmsFLAGS_NOCACHE)};
    if (!transform) {
        throw Error{"Cannot read " + name + ": its colour profile cannot be used"};
    }

    core::Image image{static_cast<int>(width), static_cast<int>(height)};
    core::parallelFor(0, image.height, [&](int begin, int end) {
        for (int y = begin; y < end; ++y) {
            cmsDoTransform(transform.get(), &packed[packedRow * y], image.pixel(0, y), width);
        }
    });
    return core::geometry::orient(image, orientation);
}

} // namespace imageio
