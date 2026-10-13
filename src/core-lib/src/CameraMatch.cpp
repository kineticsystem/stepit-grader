// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "core/CameraMatch.h"
#include "core/Color.h"
#include "core/Parallel.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <numbers>
#include <numeric>
#include <random>
#include <stdexcept>

namespace core::match {

namespace {

/// A JPEG channel at 98% or more, in linear values, is taken for clipped.
constexpr float CLIPPED = 0.956f;
/// One step of the search, as a fraction of a slider's range: every slider moves about as much.
constexpr double UNIT_OF_RANGE = 1.0 / 40;
/// How far the first trials move each slider from the start, in steps.
constexpr double FIRST_MOVE = 3.0;
/// A stage ends when its trials lie within this many steps of each other, and their differences
/// within this much.
constexpr double STEPS_TOLERANCE = 0.2;
constexpr double ERROR_TOLERANCE = 0.005;
/// The seed of the pixels kept, so every run compares the same ones.
constexpr unsigned SEED = 0;

constexpr std::array<double, 3> D65_WHITE{0.95047, 1.0, 1.08883};

/// Thrown from a trial once `stopped` says yes, to leave the search at once.
struct Stop
{};

double degrees(double radians)
{
    return radians * 180 / std::numbers::pi;
}

double radians(double degrees)
{
    return degrees * std::numbers::pi / 180;
}

/// A slider as the search sees it.
struct Slider
{
    std::string id;
    double minimum = 0;
    double maximum = 0;
    double defaultValue = 0;
    int decimals = 0;
    double unit = 1;
};

/**
 * The Nelder-Mead simplex search (Nelder and Mead, 1965), with the usual coefficients, from
 * `start`, its first simplex `FIRST_MOVE` steps along each axis. Stops after `maxTrials` new
 * values, or once the simplex is within the tolerances.
 */
void nelderMead(
    const std::function<double(const std::vector<double> &)> &cost,
    const std::vector<double> &start,
    const std::function<int()> &trials,
    int maxTrials)
{
    const std::size_t n = start.size();
    std::vector<std::vector<double>> points{start};
    for (std::size_t i = 0; i < n; ++i) {
        auto point = start;
        point[i] += FIRST_MOVE;
        points.push_back(point);
    }
    std::vector<double> values;
    for (const auto &point : points) {
        values.push_back(cost(point));
    }
    const auto along =
        [n](const std::vector<double> &from, const std::vector<double> &to, double t) {
            std::vector<double> point(n);
            for (std::size_t i = 0; i < n; ++i) {
                point[i] = from[i] + t * (to[i] - from[i]);
            }
            return point;
        };

    while (trials() < maxTrials) {
        std::vector<std::size_t> order(n + 1);
        std::iota(order.begin(), order.end(), 0);
        std::ranges::sort(order, {}, [&values](std::size_t i) { return values[i]; });
        std::vector<std::vector<double>> sortedPoints;
        std::vector<double> sortedValues;
        for (const std::size_t i : order) {
            sortedPoints.push_back(points[i]);
            sortedValues.push_back(values[i]);
        }
        points = std::move(sortedPoints);
        values = std::move(sortedValues);

        double spread = 0;
        double errorSpread = 0;
        for (std::size_t i = 1; i <= n; ++i) {
            for (std::size_t j = 0; j < n; ++j) {
                spread = std::max(spread, std::abs(points[i][j] - points[0][j]));
            }
            errorSpread = std::max(errorSpread, std::abs(values[i] - values[0]));
        }
        if (spread <= STEPS_TOLERANCE && errorSpread <= ERROR_TOLERANCE) {
            return;
        }

        std::vector<double> centre(n, 0.0);
        for (std::size_t i = 0; i < n; ++i) {
            for (std::size_t j = 0; j < n; ++j) {
                centre[j] += points[i][j] / n;
            }
        }
        const auto reflected = along(centre, points[n], -1.0);
        const double reflectedValue = cost(reflected);
        if (reflectedValue < values[0]) {
            const auto expanded = along(centre, points[n], -2.0);
            const double expandedValue = cost(expanded);
            if (expandedValue < reflectedValue) {
                points[n] = expanded;
                values[n] = expandedValue;
            } else {
                points[n] = reflected;
                values[n] = reflectedValue;
            }
            continue;
        }
        if (reflectedValue < values[n - 1]) {
            points[n] = reflected;
            values[n] = reflectedValue;
            continue;
        }
        const bool outside = reflectedValue < values[n];
        const auto contracted = outside ? along(centre, reflected, 0.5)
                                        : along(centre, points[n], 0.5);
        const double contractedValue = cost(contracted);
        if (contractedValue < std::min(reflectedValue, values[n])) {
            points[n] = contracted;
            values[n] = contractedValue;
            continue;
        }
        // Shrink toward the best.
        for (std::size_t i = 1; i <= n; ++i) {
            points[i] = along(points[0], points[i], 0.5);
            values[i] = cost(points[i]);
        }
    }
}

/// The slider's value in the settings, or its default.
double valueOf(const Settings &settings, const Slider &slider)
{
    const auto it = settings.values().find(slider.id);
    return it == settings.values().end() ? slider.defaultValue : it->second;
}

/// The mean difference of the samples, graded with `settings`, from their JPEGs.
double errorOf(const std::vector<Sample> &samples, const Pipeline &pipeline, const Settings &settings)
{
    const Matrix3 &toSrgb = color::rec2020ToSrgb();
    double sum = 0;
    std::size_t count = 0;
    for (const Sample &sample : samples) {
        const auto graded = pipeline.render(sample.source, settings, RenderContext{sample.scale});
        std::vector<double> differences(sample.pixels.size());
        parallelForEach(sample.pixels.size(), [&](std::size_t i) {
            const float *p = &graded->data[static_cast<std::size_t>(sample.pixels[i]) * 3];
            float srgb[3];
            for (int row = 0; row < 3; ++row) {
                srgb[row] = static_cast<float>(
                    toSrgb[row * 3] * p[0] + toSrgb[row * 3 + 1] * p[1]
                    + toSrgb[row * 3 + 2] * p[2]);
            }
            differences[i] = deltaE2000(sample.target[i], labOfLinearSrgb(srgb));
        });
        sum += std::accumulate(differences.begin(), differences.end(), 0.0);
        count += differences.size();
    }
    return count > 0 ? sum / count : 0.0;
}

/// The exposure that brings the median brightness of the samples, graded with `settings`, to that
/// of their JPEGs.
double matchingExposure(
    const std::vector<Sample> &samples, const Pipeline &pipeline, const Settings &settings)
{
    const Matrix3 &toSrgb = color::rec2020ToSrgb();
    const Matrix3 &toXyz = color::srgbToXyz();
    std::vector<double> stops;
    for (const Sample &sample : samples) {
        const auto graded = pipeline.render(sample.source, settings, RenderContext{sample.scale});
        std::vector<double> ours;
        std::vector<double> theirs;
        for (std::size_t i = 0; i < sample.pixels.size(); ++i) {
            const float *p = &graded->data[static_cast<std::size_t>(sample.pixels[i]) * 3];
            // The luminance is the middle row of sRGB to XYZ applied to sRGB.
            double y = 0;
            for (int column = 0; column < 3; ++column) {
                const double srgb = toSrgb[column * 3] * p[0] + toSrgb[column * 3 + 1] * p[1]
                                    + toSrgb[column * 3 + 2] * p[2];
                y += toXyz[3 + column] * srgb;
            }
            ours.push_back(y);
            const double f = (sample.target[i][0] + 16) / 116;
            theirs.push_back(f * f * f);
        }
        if (ours.empty()) {
            continue;
        }
        const auto median = [](std::vector<double> &v) {
            std::ranges::nth_element(v, v.begin() + v.size() / 2);
            return v[v.size() / 2];
        };
        const double a = median(ours);
        const double b = median(theirs);
        if (a > 0 && b > 0) {
            stops.push_back(std::log2(b / a));
        }
    }
    if (stops.empty()) {
        return 0;
    }
    std::ranges::nth_element(stops, stops.begin() + stops.size() / 2);
    return stops[stops.size() / 2];
}

} // namespace

double deltaE2000(const Lab &a, const Lab &b)
{
    const double l1 = a[0], a1 = a[1], b1 = a[2];
    const double l2 = b[0], a2 = b[1], b2 = b[2];
    const double c1 = std::hypot(a1, b1);
    const double c2 = std::hypot(a2, b2);
    const double cMean7 = std::pow((c1 + c2) / 2, 7);
    const double g = 0.5 * (1 - std::sqrt(cMean7 / (cMean7 + std::pow(25.0, 7))));
    const double a1p = (1 + g) * a1;
    const double a2p = (1 + g) * a2;
    const double c1p = std::hypot(a1p, b1);
    const double c2p = std::hypot(a2p, b2);
    const auto hueOf = [](double b, double a) {
        if (a == 0 && b == 0) {
            return 0.0;
        }
        const double h = degrees(std::atan2(b, a));
        return h < 0 ? h + 360 : h;
    };
    const double h1p = hueOf(b1, a1p);
    const double h2p = hueOf(b2, a2p);

    const double dLp = l2 - l1;
    const double dCp = c2p - c1p;
    double dhp = 0;
    if (c1p * c2p != 0) {
        dhp = h2p - h1p;
        if (dhp > 180) {
            dhp -= 360;
        } else if (dhp < -180) {
            dhp += 360;
        }
    }
    const double dHp = 2 * std::sqrt(c1p * c2p) * std::sin(radians(dhp / 2));

    const double lMean = (l1 + l2) / 2;
    const double cMean = (c1p + c2p) / 2;
    double hMean = h1p + h2p;
    if (c1p * c2p != 0) {
        if (std::abs(h1p - h2p) <= 180) {
            hMean /= 2;
        } else {
            hMean = hMean < 360 ? (hMean + 360) / 2 : (hMean - 360) / 2;
        }
    }
    const double t = 1 - 0.17 * std::cos(radians(hMean - 30)) + 0.24 * std::cos(radians(2 * hMean))
                     + 0.32 * std::cos(radians(3 * hMean + 6))
                     - 0.20 * std::cos(radians(4 * hMean - 63));
    const double dTheta = 30 * std::exp(-std::pow((hMean - 275) / 25, 2));
    const double cMeanP7 = std::pow(cMean, 7);
    const double rc = 2 * std::sqrt(cMeanP7 / (cMeanP7 + std::pow(25.0, 7)));
    const double lOffset = (lMean - 50) * (lMean - 50);
    const double sl = 1 + 0.015 * lOffset / std::sqrt(20 + lOffset);
    const double sc = 1 + 0.045 * cMean;
    const double sh = 1 + 0.015 * cMean * t;
    const double rt = -std::sin(radians(2 * dTheta)) * rc;
    const double l = dLp / sl;
    const double c = dCp / sc;
    const double h = dHp / sh;
    return std::sqrt(l * l + c * c + h * h + rt * c * h);
}

Lab labOfLinearSrgb(const float *rgb)
{
    const Matrix3 &toXyz = color::srgbToXyz();
    double xyz[3]{};
    for (int row = 0; row < 3; ++row) {
        for (int column = 0; column < 3; ++column) {
            xyz[row] += toXyz[row * 3 + column] * std::clamp(rgb[column], 0.0f, 1.0f);
        }
        xyz[row] /= D65_WHITE[row];
    }
    constexpr double epsilon = 216.0 / 24389;
    constexpr double kappa = 24389.0 / 27;
    double f[3];
    for (int i = 0; i < 3; ++i) {
        f[i] = xyz[i] > epsilon ? std::cbrt(xyz[i]) : (kappa * xyz[i] + 16) / 116;
    }
    return {
        static_cast<float>(116 * f[1] - 16),
        static_cast<float>(500 * (f[0] - f[1])),
        static_cast<float>(200 * (f[1] - f[2]))};
}

Sample makeSample(Image source, const Image &jpegLinearSrgb, double scale, int maxPixels)
{
    if (source.width != jpegLinearSrgb.width || source.height != jpegLinearSrgb.height) {
        throw std::invalid_argument("The picture and its JPEG differ in size");
    }
    std::vector<int> kept;
    for (std::size_t i = 0; i < jpegLinearSrgb.pixelCount(); ++i) {
        const float *p = &jpegLinearSrgb.data[i * 3];
        if (std::max({p[0], p[1], p[2]}) < CLIPPED) {
            kept.push_back(static_cast<int>(i));
        }
    }
    Sample sample;
    std::ranges::sample(
        kept,
        std::back_inserter(sample.pixels),
        std::min<std::size_t>(kept.size(), static_cast<std::size_t>(std::max(0, maxPixels))),
        std::mt19937{SEED});
    for (const int i : sample.pixels) {
        sample.target.push_back(
            labOfLinearSrgb(&jpegLinearSrgb.data[static_cast<std::size_t>(i) * 3]));
    }
    sample.source = std::move(source);
    sample.scale = scale;
    return sample;
}

std::vector<Stage> standardStages()
{
    Stage tone{
        "Tone",
        {"Exposure2012",
         "Contrast2012",
         "Blacks2012",
         "Saturation",
         "Vibrance",
         "IncrementalTemperature",
         "IncrementalTint",
         "ParametricShadows",
         "ParametricDarks",
         "ParametricLights",
         "ParametricHighlights",
         "ParametricShadowSplit",
         "ParametricMidtoneSplit",
         "ParametricHighlightSplit"}};
    Stage colours{"Colours", {}};
    for (const char *kind : {"Hue", "Saturation", "Luminance"}) {
        for (const char *band :
             {"Red", "Orange", "Yellow", "Green", "Aqua", "Blue", "Purple", "Magenta"}) {
            colours.ids.push_back(std::string{kind} + "Adjustment" + band);
        }
    }
    return {tone, colours};
}

Result fit(const std::vector<Sample> &samples, const Pipeline &pipeline, const Options &options)
{
    std::map<std::string, Slider> sliders;
    for (const Parameter &parameter : pipeline.parameters()) {
        sliders[parameter.id] = Slider{
            parameter.id,
            parameter.minimum,
            parameter.maximum,
            parameter.defaultValue,
            parameter.decimals,
            (parameter.maximum - parameter.minimum) * UNIT_OF_RANGE};
    }
    for (const Stage &stage : options.stages) {
        for (const std::string &id : stage.ids) {
            if (!sliders.contains(id)) {
                throw std::invalid_argument("No slider named " + id);
            }
        }
    }

    Result result;
    result.errorBefore = errorOf(samples, pipeline, options.base);
    Settings best = options.base;
    for (const Stage &stage : options.stages) {
        for (const std::string &id : stage.ids) {
            best.setValue(id, sliders[id].defaultValue);
        }
    }
    if (best.has("Exposure2012")) {
        const Slider &exposure = sliders["Exposure2012"];
        const double stops = std::clamp(
            matchingExposure(samples, pipeline, best), exposure.minimum, exposure.maximum);
        best.setValue(exposure.id, std::round(stops * 100) / 100);
    }
    double bestError = errorOf(samples, pipeline, best);

    Progress progress;
    progress.stages = static_cast<int>(options.stages.size());
    try {
        for (std::size_t s = 0; s < options.stages.size(); ++s) {
            const Stage &stage = options.stages[s];
            progress.stage = static_cast<int>(s);
            progress.stageName = stage.name;
            std::map<std::vector<double>, double> tried;

            const auto settingsAt = [&](const std::vector<double> &steps) {
                Settings settings = best;
                for (std::size_t i = 0; i < stage.ids.size(); ++i) {
                    const Slider &slider = sliders[stage.ids[i]];
                    const double power = std::pow(10.0, slider.decimals);
                    const double value
                        = std::clamp(steps[i] * slider.unit, slider.minimum, slider.maximum);
                    settings.setValue(slider.id, std::round(value * power) / power);
                }
                return settings;
            };
            Settings stageBest = best;
            double stageBestError = bestError;
            const auto cost = [&](const std::vector<double> &steps) {
                const Settings settings = settingsAt(steps);
                std::vector<double> key;
                for (const std::string &id : stage.ids) {
                    key.push_back(valueOf(settings, sliders[id]));
                }
                if (const auto it = tried.find(key); it != tried.end()) {
                    return it->second;
                }
                if (options.stopped && options.stopped()) {
                    throw Stop{};
                }
                const double error = errorOf(samples, pipeline, settings);
                tried[key] = error;
                ++result.trials;
                if (error < stageBestError) {
                    stageBestError = error;
                    stageBest = settings;
                }
                if (stageBestError < bestError) {
                    best = stageBest;
                    bestError = stageBestError;
                }
                progress.trials = result.trials;
                progress.bestError = bestError;
                progress.fraction
                    = (static_cast<double>(s)
                       + std::min(1.0, static_cast<double>(tried.size()) / options.maxTrials))
                      / options.stages.size();
                if (options.progress) {
                    options.progress(progress);
                }
                return error;
            };

            std::vector<double> start;
            for (const std::string &id : stage.ids) {
                start.push_back(valueOf(best, sliders[id]) / sliders[id].unit);
            }
            nelderMead(
                cost, start, [&tried] { return static_cast<int>(tried.size()); }, options.maxTrials);
        }
    } catch (const Stop &) {
        result.stopped = true;
    }

    progress.fraction = 1;
    if (options.progress && !result.stopped) {
        options.progress(progress);
    }
    for (const Stage &stage : options.stages) {
        for (const std::string &id : stage.ids) {
            result.settings.setValue(id, valueOf(best, sliders[id]));
        }
    }
    result.errorAfter = bestError;
    return result;
}

} // namespace core::match
