// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "imageio/TiffWriter.h"
#include "imageio/Error.h"
#include "imageio/Files.h"

#include "ColourProfiles.h"

#include "core/Parallel.h"

#include <tiffio.h>

#include <cstdint>
#include <vector>

namespace imageio {

namespace {

TIFF *open(const std::filesystem::path &path)
{
#ifdef _WIN32
    return TIFFOpenW(path.wstring().c_str(), "w");
#else
    return TIFFOpen(path.string().c_str(), "w");
#endif
}

std::vector<std::uint8_t> iccBytes(void *profile)
{
    cmsUInt32Number size = 0;
    cmsSaveProfileToMem(profile, nullptr, &size);
    std::vector<std::uint8_t> bytes(size);
    cmsSaveProfileToMem(profile, bytes.data(), &size);
    return bytes;
}

} // namespace

TiffWriter::TiffWriter(TiffOptions options)
    : m_options{std::move(options)}
{}

void TiffWriter::write(const core::Image &image, const std::filesystem::path &path) const
{
    const std::string name = path.string();
    if (image.isNull()) {
        throw Error{"Cannot write " + name + ": no image"};
    }

    // To the colour space, in 16 bits, several rows at a time.
    const profiles::Profile input = profiles::workingSpace();
    const profiles::Profile output = profiles::of(m_options.colourSpace);
    const profiles::Transform transform{cmsCreateTransform(
        input.get(),
        TYPE_RGB_FLT,
        output.get(),
        TYPE_RGB_16,
        INTENT_RELATIVE_COLORIMETRIC,
        cmsFLAGS_NOCACHE)};
    if (!transform) {
        throw Error{"Cannot write " + name + ": the colour conversion failed"};
    }
    const std::size_t row = static_cast<std::size_t>(image.width) * 3;
    std::vector<std::uint16_t> samples(row * image.height);
    core::parallelFor(0, image.height, [&](int begin, int end) {
        for (int y = begin; y < end; ++y) {
            cmsDoTransform(transform.get(), image.pixel(0, y), &samples[row * y], image.width);
        }
    });
    const std::vector<std::uint8_t> icc = iccBytes(output.get());

    const std::filesystem::path temporary = files::temporaryFor(path);
    TIFF *tif = open(temporary);
    if (!tif) {
        throw Error{"Cannot write " + temporary.string()};
    }
    TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, static_cast<uint32_t>(image.width));
    TIFFSetField(tif, TIFFTAG_IMAGELENGTH, static_cast<uint32_t>(image.height));
    TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, 3);
    TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 16);
    TIFFSetField(tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT);
    TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
    TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
    TIFFSetField(tif, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
    TIFFSetField(tif, TIFFTAG_SOFTWARE, m_options.software.c_str());
    TIFFSetField(tif, TIFFTAG_ICCPROFILE, static_cast<uint32_t>(icc.size()), icc.data());
    if (m_options.compress) {
        TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_ADOBE_DEFLATE);
        TIFFSetField(tif, TIFFTAG_PREDICTOR, PREDICTOR_HORIZONTAL);
    } else {
        TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_NONE);
    }
    TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, TIFFDefaultStripSize(tif, 0));

    bool written = true;
    for (int y = 0; y < image.height && written; ++y) {
        written = TIFFWriteScanline(tif, &samples[row * y], static_cast<uint32_t>(y), 0) >= 0;
    }
    TIFFClose(tif);

    if (!written) {
        std::error_code ignored;
        std::filesystem::remove(temporary, ignored);
        throw Error{"Cannot write " + name};
    }
    files::replace(temporary, path);
}

} // namespace imageio
