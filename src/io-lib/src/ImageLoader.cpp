// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "imageio/ImageLoader.h"
#include "imageio/Error.h"
#include "imageio/RawReader.h"
#include "imageio/TiffReader.h"

namespace imageio {

ImageLoader::ImageLoader(std::vector<std::shared_ptr<const ImageReader>> readers)
    : m_readers{std::move(readers)}
{}

std::vector<std::string> ImageLoader::extensions() const
{
    std::vector<std::string> all;
    for (const auto &reader : m_readers) {
        for (std::string &ext : reader->extensions()) {
            all.push_back(std::move(ext));
        }
    }
    return all;
}

bool ImageLoader::canRead(const std::filesystem::path &path) const
{
    for (const auto &reader : m_readers) {
        if (reader->canRead(path)) {
            return true;
        }
    }
    return false;
}

core::Image ImageLoader::read(const std::filesystem::path &path) const
{
    for (const auto &reader : m_readers) {
        if (reader->canRead(path)) {
            return reader->read(path);
        }
    }
    throw Error{"Cannot read " + path.string() + ": not a TIFF, DNG or camera raw file"};
}

std::shared_ptr<const ImageReader> makeStandardImageReader()
{
    return std::make_shared<const ImageLoader>(std::vector<std::shared_ptr<const ImageReader>>{
        std::make_shared<TiffReader>(), std::make_shared<RawReader>()});
}

} // namespace imageio
