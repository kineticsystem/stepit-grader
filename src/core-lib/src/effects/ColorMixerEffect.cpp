// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "core/effects/ColorMixerEffect.h"
#include "core/Color.h"
#include "core/Luminance.h"
#include "core/Parallel.h"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <string>

namespace core {

namespace {

/// The middle of each band, as the hue of an sRGB colour on the HSV wheel, in degrees.
constexpr std::array<double, ColorMixerEffect::BANDS> SRGB_HUES{0, 30, 60, 120, 180, 240, 270, 300};
/// How far a hue turns at +100 or -100, in degrees of Oklab hue.
constexpr float MAX_HUE_TURN = 30.0f;
/// How far a band's luminance moves at +100 or -100, in stops.
constexpr float MAX_LUMINANCE_STOPS = 1.0f;
/// Below this Oklab chroma a colour counts as grey, and the bands leave it alone; they reach their
/// full effect at twice that.
constexpr float GREY_CHROMA = 0.02f;

std::array<Parameter, ColorMixerEffect::BANDS> makeParameters(
    const std::string &idPrefix, const std::string &labelSuffix)
{
    std::array<Parameter, ColorMixerEffect::BANDS> parameters;
    for (int band = 0; band < ColorMixerEffect::BANDS; ++band) {
        const std::string name = ColorMixerEffect::BAND_NAMES[band];
        parameters[band] = Parameter{idPrefix + name, name + " " + labelSuffix, -100, 100, 0, 0};
    }
    return parameters;
}

/// The linear sRGB colour of a hue of the HSV wheel, at full saturation and value.
std::array<float, 3> srgbOfHue(double degrees)
{
    const auto channel = [degrees](double n) {
        const double k = std::fmod(n + degrees / 60.0, 6.0);
        return static_cast<float>(1.0 - std::max(0.0, std::min({k, 4.0 - k, 1.0})));
    };
    return {channel(5), channel(3), channel(1)};
}

/**
 * The two bands a hue falls between, and the weight of the second: 0 at the first band's middle,
 * 1 at the second's, eased so that the change is smooth across each middle.
 */
void bandsOf(
    float hue,
    const std::array<double, ColorMixerEffect::BANDS> &middles,
    int &first,
    int &second,
    float &weight)
{
    constexpr int n = ColorMixerEffect::BANDS;
    for (int band = 0; band < n; ++band) {
        const double from = middles[band];
        double to = middles[(band + 1) % n];
        if (to <= from) {
            to += 360;
        }
        double h = hue;
        if (h < from) {
            h += 360;
        }
        if (h < to) {
            first = band;
            second = (band + 1) % n;
            const float t = static_cast<float>((h - from) / (to - from));
            weight = t * t * (3 - 2 * t);
            return;
        }
    }
    first = second = 0;
    weight = 0;
}

} // namespace

const std::array<Parameter, ColorMixerEffect::BANDS> &ColorMixerEffect::hues()
{
    static const auto parameters = makeParameters("HueAdjustment", "Hue");
    return parameters;
}

const std::array<Parameter, ColorMixerEffect::BANDS> &ColorMixerEffect::saturations()
{
    static const auto parameters = makeParameters("SaturationAdjustment", "Saturation");
    return parameters;
}

const std::array<Parameter, ColorMixerEffect::BANDS> &ColorMixerEffect::luminances()
{
    static const auto parameters = makeParameters("LuminanceAdjustment", "Luminance");
    return parameters;
}

std::vector<Parameter> ColorMixerEffect::parameters() const
{
    std::vector<Parameter> all;
    for (const auto *kind : {&hues(), &saturations(), &luminances()}) {
        all.insert(all.end(), kind->begin(), kind->end());
    }
    return all;
}

const std::array<double, ColorMixerEffect::BANDS> &ColorMixerEffect::bandHues()
{
    static const std::array<double, BANDS> middles = [] {
        const Matrix3 toRec2020 = inverse(color::rec2020ToSrgb());
        std::array<double, BANDS> result{};
        for (int band = 0; band < BANDS; ++band) {
            const auto srgb = srgbOfHue(SRGB_HUES[band]);
            float rgb[3];
            for (int row = 0; row < 3; ++row) {
                rgb[row] = static_cast<float>(
                    toRec2020[row * 3] * srgb[0] + toRec2020[row * 3 + 1] * srgb[1]
                    + toRec2020[row * 3 + 2] * srgb[2]);
            }
            float lab[3];
            color::rec2020ToOklab(rgb, lab);
            const double degrees = std::atan2(lab[2], lab[1]) * 180 / std::numbers::pi;
            result[band] = degrees < 0 ? degrees + 360 : degrees;
        }
        return result;
    }();
    return middles;
}

void ColorMixerEffect::apply(Image &image, const Settings &settings, const RenderContext &) const
{
    std::array<float, BANDS> turns{};
    std::array<float, BANDS> chromaChanges{};
    std::array<float, BANDS> stops{};
    for (int band = 0; band < BANDS; ++band) {
        turns[band] = static_cast<float>(settings.value(hues()[band]) / 100.0) * MAX_HUE_TURN;
        chromaChanges[band] = static_cast<float>(settings.value(saturations()[band]) / 100.0);
        stops[band] = static_cast<float>(settings.value(luminances()[band]) / 100.0)
                      * MAX_LUMINANCE_STOPS;
    }
    const auto &middles = bandHues();
    constexpr float degreesToRadians = std::numbers::pi_v<float> / 180;

    parallelForEach(image.pixelCount(), [&](std::size_t i) {
        float *p = &image.data[i * 3];
        float lab[3];
        color::rec2020ToOklab(p, lab);
        const float chroma = std::hypot(lab[1], lab[2]);
        const float colourful = luminance::smoothstep(GREY_CHROMA, 2 * GREY_CHROMA, chroma);
        if (colourful <= 0) {
            return;
        }
        float hue = std::atan2(lab[2], lab[1]) / degreesToRadians;
        if (hue < 0) {
            hue += 360;
        }
        int first = 0;
        int second = 0;
        float weight = 0;
        bandsOf(hue, middles, first, second, weight);
        const auto mix = [&](const std::array<float, BANDS> &values) {
            return colourful * (values[first] + (values[second] - values[first]) * weight);
        };

        const float angle = (hue + mix(turns)) * degreesToRadians;
        const float newChroma = chroma * std::max(0.0f, 1.0f + mix(chromaChanges));
        lab[1] = newChroma * std::cos(angle);
        lab[2] = newChroma * std::sin(angle);
        color::oklabToRec2020(lab, p);

        const float gain = std::exp2(mix(stops));
        p[0] *= gain;
        p[1] *= gain;
        p[2] *= gain;
    });
}

} // namespace core
