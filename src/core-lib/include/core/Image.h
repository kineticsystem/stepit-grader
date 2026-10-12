// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#pragma once

#include <cstddef>
#include <memory>
#include <vector>

namespace core {

/**
 * An allocator that leaves new values uninitialised, as `new float[n]` does. A picture's planes
 * are always written whole before they are read, and filling 80 MB with zeros first takes longer
 * than the filter that writes them.
 */
template<typename T>
struct UninitialisedAllocator : std::allocator<T>
{
    template<typename U>
    struct rebind
    {
        using other = UninitialisedAllocator<U>;
    };

    UninitialisedAllocator() = default;
    template<typename U>
    UninitialisedAllocator(const UninitialisedAllocator<U> &) noexcept
    {}

    template<typename U>
    void construct(U *p) noexcept
    {
        ::new (static_cast<void *>(p)) U;
    }
    template<typename U, typename... Args>
    void construct(U *p, Args &&...args)
    {
        ::new (static_cast<void *>(p)) U(std::forward<Args>(args)...);
    }
};

/// The samples of an image or a plane.
using Samples = std::vector<float, UninitialisedAllocator<float>>;

/**
 * An RGB image in the working colour space: linear Rec. 2020, D65, 32-bit float per sample,
 * interleaved. 1.0 is the white the input could record; values above it are kept, values below
 * 0 can appear in saturated colours and are kept too, until the display or the output clips them.
 * A new image's values are undefined until written.
 */
struct Image
{
    int width = 0;
    int height = 0;
    Samples data;

    Image() = default;
    Image(int width, int height)
        : width{width}
        , height{height}
        , data(static_cast<std::size_t>(width) * height * 3)
    {}

    bool isNull() const { return width <= 0 || height <= 0; }
    std::size_t pixelCount() const { return static_cast<std::size_t>(width) * height; }

    float *pixel(int x, int y) { return &data[(static_cast<std::size_t>(y) * width + x) * 3]; }
    const float *pixel(int x, int y) const
    {
        return &data[(static_cast<std::size_t>(y) * width + x) * 3];
    }

    bool operator==(const Image &) const = default;
};

/// One channel of float values, e.g. the luminance of an Image. Undefined until written.
struct Plane
{
    int width = 0;
    int height = 0;
    Samples data;

    Plane() = default;
    Plane(int width, int height)
        : width{width}
        , height{height}
        , data(static_cast<std::size_t>(width) * height)
    {}
    Plane(int width, int height, float value)
        : width{width}
        , height{height}
        , data(static_cast<std::size_t>(width) * height, value)
    {}

    float &at(int x, int y) { return data[static_cast<std::size_t>(y) * width + x]; }
    float at(int x, int y) const { return data[static_cast<std::size_t>(y) * width + x]; }
};

} // namespace core
