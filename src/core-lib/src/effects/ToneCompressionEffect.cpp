// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "core/effects/ToneCompressionEffect.h"
#include "core/Color.h"
#include "core/Filters.h"
#include "core/Parallel.h"

#include <algorithm>
#include <cmath>

namespace core {

namespace {

/// The most stops above white brought into the picture, at 100.
constexpr float MAX_STOPS = 4.0f;
/// The compression point's 0 and 100, in linear values: from middle grey to just below white.
constexpr float LOWEST_KNEE = color::MIDDLE_GREY;
constexpr float HIGHEST_KNEE = 0.9f;
/// The areas compressed together: a hundredth of the image's diagonal, without crossing the
/// edges where the brightness changes by more than about half a stop.
constexpr double AREA_OF_DIAGONAL = 0.01;
constexpr float EDGE_STOPS = 0.5f;
/// Darker than this, a pixel is left alone: the curve starts well above it.
constexpr float MIN_VALUE = 1e-6f;

/**
 * The scale of the exponential shoulder A (1 - exp(-x / A)) that has a slope of 1 at 0 and
 * reaches `out` at `in`, found by bisection: it rises with A, from 0 to `in`.
 */
double shoulderScale(double in, double out)
{
    double low = 1e-6, high = 1e6;
    for (int i = 0; i < 80; ++i) {
        const double middle = std::sqrt(low * high);
        (middle * (1.0 - std::exp(-in / middle)) < out ? low : high) = middle;
    }
    return std::sqrt(low * high);
}

/// The curve, made once per render: its scale takes a bisection.
class Shoulder
{
public:
    Shoulder(float knee, float white)
        : m_knee{knee}
        , m_linear{white <= 1.0f}
        , m_scale{m_linear ? 0.0 : shoulderScale(white - knee, 1.0 - knee)}
    {}

    float operator()(float value) const
    {
        if (m_linear || value <= m_knee) {
            return value;
        }
        return static_cast<float>(m_knee + m_scale * (1.0 - std::exp(-(value - m_knee) / m_scale)));
    }

private:
    float m_knee;
    bool m_linear;
    double m_scale;
};

} // namespace

float ToneCompressionEffect::compress(float value, float knee, float white)
{
    return Shoulder{knee, white}(value);
}

bool ToneCompressionEffect::isIdentity(const Settings &settings) const
{
    return settings.value(AMOUNT) <= 0;
}

void ToneCompressionEffect::apply(Image &image, const Settings &settings, const RenderContext &) const
{
    const float amount = static_cast<float>(settings.value(AMOUNT) / 100.0);
    if (amount <= 0) {
        return;
    }
    const float white = std::exp2(amount * MAX_STOPS);
    const float point = static_cast<float>(settings.value(POINT) / 100.0);
    const float knee = std::exp2(
        std::log2(LOWEST_KNEE) + point * (std::log2(HIGHEST_KNEE) - std::log2(LOWEST_KNEE)));
    const float local = static_cast<float>(settings.value(LOCAL) / 100.0);
    const Shoulder curve{knee, white};

    // Each pixel's brightness, as its brightest channel, in stops.
    Plane stops{image.width, image.height};
    parallelFor(0, image.height, [&](int begin, int end) {
        for (int y = begin; y < end; ++y) {
            for (int x = 0; x < image.width; ++x) {
                const float *p = image.pixel(x, y);
                stops.at(x, y) = std::log2(std::max({p[0], p[1], p[2], MIN_VALUE}));
            }
        }
    });

    // The part compressed: the pixel's own brightness, or, with local contrast, its area's. The
    // rest, the texture within the area, is kept as it is.
    Plane area;
    if (local > 0) {
        const int radius
            = std::max(1, static_cast<int>(std::hypot(image.width, image.height) * AREA_OF_DIAGONAL));
        area = filters::guidedFilter(stops, radius, EDGE_STOPS * EDGE_STOPS);
    }

    parallelFor(0, image.height, [&](int begin, int end) {
        for (int y = begin; y < end; ++y) {
            for (int x = 0; x < image.width; ++x) {
                const float own = stops.at(x, y);
                const float base = local > 0 ? own + local * (area.at(x, y) - own) : own;
                const float detail = own - base;
                const float compressed = curve(std::exp2(base)) * std::exp2(detail);
                const float gain = compressed / std::exp2(own);
                float *p = image.pixel(x, y);
                p[0] *= gain;
                p[1] *= gain;
                p[2] *= gain;
            }
        }
    });
}

} // namespace core
