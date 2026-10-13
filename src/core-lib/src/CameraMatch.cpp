// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "core/CameraMatch.h"
#include "core/Color.h"
#include "core/Parallel.h"

#include <algorithm>
#include <cmath>
#include <iterator>
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
/// The step of each slider for the slopes of Levenberg-Marquardt, in steps: big enough that the
/// rounding to the slider's decimals moves the slope little.
constexpr double SLOPE_STEP = 1.0;
/// The most iterations of Levenberg-Marquardt in a stage; it usually ends in fewer than 20.
constexpr int MAX_ITERATIONS = 40;
/// The tries of each iteration at a smaller move, once a move makes the differences worse.
constexpr int MAX_DAMPED_TRIES = 8;
/// The damping of the first iteration, then how it shrinks after a better move and grows after a
/// worse one: from a step along the slopes to a short one down the steepest descent.
constexpr double FIRST_DAMPING = 1e-2;
constexpr double DAMPING_DOWN = 3.0;
constexpr double DAMPING_UP = 4.0;
/// Levenberg-Marquardt ends once an iteration improves the squared differences by less than this
/// share, or moves every slider by less than this many steps.
constexpr double MIN_GAIN = 1e-4;
constexpr double MIN_MOVE = 0.05;
/// How far the Nelder-Mead polish's first trials move each slider, in steps: near, as it starts
/// where Levenberg-Marquardt ends.
constexpr double POLISH_MOVE = 1.0;
/// The most trials of the polish in a stage.
constexpr int POLISH_TRIALS = 300;
/// The polish ends when its trials lie within this many steps of each other, and their
/// differences within this much.
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

/// The solution of `a` x = `b`, `a` an n by n matrix by rows, by Gaussian elimination with
/// partial pivoting.
std::vector<double> solve(std::vector<double> a, std::vector<double> b)
{
    const std::size_t n = b.size();
    for (std::size_t column = 0; column < n; ++column) {
        std::size_t pivot = column;
        for (std::size_t row = column + 1; row < n; ++row) {
            if (std::abs(a[row * n + column]) > std::abs(a[pivot * n + column])) {
                pivot = row;
            }
        }
        for (std::size_t k = 0; k < n; ++k) {
            std::swap(a[column * n + k], a[pivot * n + k]);
        }
        std::swap(b[column], b[pivot]);
        for (std::size_t row = column + 1; row < n; ++row) {
            const double factor = a[row * n + column] / a[column * n + column];
            for (std::size_t k = column; k < n; ++k) {
                a[row * n + k] -= factor * a[column * n + k];
            }
            b[row] -= factor * b[column];
        }
    }
    std::vector<double> x(n);
    for (std::size_t row = n; row-- > 0;) {
        double sum = b[row];
        for (std::size_t k = row + 1; k < n; ++k) {
            sum -= a[row * n + k] * x[k];
        }
        x[row] = sum / a[row * n + row];
    }
    return x;
}

double sumOfSquares(const std::vector<float> &values)
{
    double sum = 0;
    for (const float value : values) {
        sum += static_cast<double>(value) * value;
    }
    return sum;
}

/**
 * Levenberg-Marquardt (Levenberg, 1944; Marquardt, 1963) from `start`, within [`low`, `high`]:
 * it moves every slider at once along the slopes of each pixel's difference, measured one slider
 * at a time, so it needs far fewer trials than a search that sees only the mean difference.
 * `residuals` gives the differences at a point. Returns where it ends, early once `exhausted`
 * says so.
 */
std::vector<double> levenbergMarquardt(
    const std::function<std::vector<float>(const std::vector<double> &)> &residuals,
    std::vector<double> x,
    const std::vector<double> &low,
    const std::vector<double> &high,
    const std::function<bool()> &exhausted)
{
    const std::size_t n = x.size();
    std::vector<float> r = residuals(x);
    double squares = sumOfSquares(r);
    double damping = FIRST_DAMPING;
    for (int iteration = 0; iteration < MAX_ITERATIONS && !exhausted(); ++iteration) {
        // The slopes, forward, or backward at the top of a slider's range.
        std::vector<std::vector<float>> slopes(n);
        for (std::size_t i = 0; i < n; ++i) {
            auto moved = x;
            const double step = x[i] + SLOPE_STEP <= high[i] ? SLOPE_STEP : -SLOPE_STEP;
            moved[i] += step;
            slopes[i] = residuals(moved);
            for (std::size_t k = 0; k < r.size(); ++k) {
                slopes[i][k] = static_cast<float>((slopes[i][k] - r[k]) / step);
            }
        }
        // The normal equations: (JᵀJ + damping · diag(JᵀJ)) move = -Jᵀr.
        std::vector<double> jtj(n * n);
        std::vector<double> jtr(n);
        parallelForEach(n, [&](std::size_t i) {
            for (std::size_t j = 0; j <= i; ++j) {
                double sum = 0;
                for (std::size_t k = 0; k < r.size(); ++k) {
                    sum += static_cast<double>(slopes[i][k]) * slopes[j][k];
                }
                jtj[i * n + j] = sum;
            }
            double sum = 0;
            for (std::size_t k = 0; k < r.size(); ++k) {
                sum += static_cast<double>(slopes[i][k]) * r[k];
            }
            jtr[i] = -sum;
        });
        for (std::size_t i = 0; i < n; ++i) {
            for (std::size_t j = i + 1; j < n; ++j) {
                jtj[i * n + j] = jtj[j * n + i];
            }
        }

        bool better = false;
        double longest = 0;
        for (int attempt = 0; attempt < MAX_DAMPED_TRIES && !better; ++attempt) {
            auto damped = jtj;
            for (std::size_t i = 0; i < n; ++i) {
                // A slider that changes nothing still gets a little damping, so the matrix solves.
                damped[i * n + i] += damping * std::max(jtj[i * n + i], 1e-9);
            }
            const std::vector<double> move = solve(damped, jtr);
            auto next = x;
            longest = 0;
            for (std::size_t i = 0; i < n; ++i) {
                next[i] = std::clamp(x[i] + move[i], low[i], high[i]);
                longest = std::max(longest, std::abs(next[i] - x[i]));
            }
            std::vector<float> nextR = residuals(next);
            const double nextSquares = sumOfSquares(nextR);
            if (nextSquares < squares) {
                better = true;
                const double gain = (squares - nextSquares) / squares;
                x = std::move(next);
                r = std::move(nextR);
                squares = nextSquares;
                damping /= DAMPING_DOWN;
                if (gain < MIN_GAIN) {
                    return x;
                }
            } else {
                damping *= DAMPING_UP;
            }
        }
        if (!better || longest < MIN_MOVE) {
            break;
        }
    }
    return x;
}

/**
 * The Nelder-Mead simplex search (Nelder and Mead, 1965), with the usual coefficients, from
 * `start`, its first simplex `POLISH_MOVE` steps along each axis. Stops after `maxTrials` new
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
        point[i] += POLISH_MOVE;
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

/**
 * The mean difference of the samples, graded with `settings`, from their JPEGs. With `residuals`,
 * also each pixel's difference in L*a*b*, three numbers per pixel, for Levenberg-Marquardt: unlike
 * CIEDE2000, it is smooth, so its slopes say where to go.
 */
double errorOf(
    const std::vector<Sample> &samples,
    const Pipeline &pipeline,
    const Settings &settings,
    std::vector<float> *residuals = nullptr)
{
    if (residuals) {
        residuals->clear();
    }
    const Matrix3 &toSrgb = color::rec2020ToSrgb();
    double sum = 0;
    std::size_t count = 0;
    for (const Sample &sample : samples) {
        const auto graded = pipeline.render(sample.source, settings, RenderContext{sample.scale});
        std::vector<double> differences(sample.pixels.size());
        const std::size_t at = residuals ? residuals->size() : 0;
        if (residuals) {
            residuals->resize(at + sample.pixels.size() * 3);
        }
        parallelForEach(sample.pixels.size(), [&](std::size_t i) {
            const float *p = &graded->data[static_cast<std::size_t>(sample.pixels[i]) * 3];
            float srgb[3];
            for (int row = 0; row < 3; ++row) {
                srgb[row] = static_cast<float>(
                    toSrgb[row * 3] * p[0] + toSrgb[row * 3 + 1] * p[1]
                    + toSrgb[row * 3 + 2] * p[2]);
            }
            const Lab lab = labOfLinearSrgb(srgb);
            differences[i] = deltaE2000(sample.target[i], lab);
            if (residuals) {
                for (int c = 0; c < 3; ++c) {
                    (*residuals)[at + i * 3 + c] = lab[c] - sample.target[i][c];
                }
            }
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

/// Whether every effect a trial may run works pixel by pixel: those of the sliders searched, and
/// those the base settings turn on. The samples then need only their compared pixels rendered.
bool trialsArePointwise(const Pipeline &pipeline, const Options &options)
{
    for (const auto &effect : pipeline.effects()) {
        if (effect->isPointwise()) {
            continue;
        }
        if (!effect->isIdentity(options.base)) {
            return false;
        }
        for (const Parameter &parameter : effect->parameters()) {
            for (const Stage &stage : options.stages) {
                if (std::ranges::find(stage.ids, parameter.id) != stage.ids.end()) {
                    return false;
                }
            }
        }
    }
    return true;
}

/// The samples with their compared pixels alone, in a row, in the order they are compared.
std::vector<Sample> packed(const std::vector<Sample> &samples)
{
    std::vector<Sample> rows;
    for (const Sample &sample : samples) {
        Sample row;
        row.source = Image{static_cast<int>(sample.pixels.size()), 1};
        for (std::size_t i = 0; i < sample.pixels.size(); ++i) {
            std::copy_n(
                &sample.source.data[static_cast<std::size_t>(sample.pixels[i]) * 3],
                3,
                &row.source.data[i * 3]);
        }
        row.pixels.resize(sample.pixels.size());
        std::iota(row.pixels.begin(), row.pixels.end(), 0);
        row.target = sample.target;
        row.scale = sample.scale;
        rows.push_back(std::move(row));
    }
    return rows;
}

/// The index of the first effect with a slider of `stage`: the effects before it do the same in
/// every trial of the stage.
std::size_t firstEffectOf(const Pipeline &pipeline, const Stage &stage)
{
    const auto &effects = pipeline.effects();
    for (std::size_t i = 0; i < effects.size(); ++i) {
        for (const Parameter &parameter : effects[i]->parameters()) {
            if (std::ranges::find(stage.ids, parameter.id) != stage.ids.end()) {
                return i;
            }
        }
    }
    return effects.size();
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

    // A trial renders only the pixels it compares, unless an effect looks at their neighbours.
    const std::vector<Sample> rows = trialsArePointwise(pipeline, options) ? packed(samples)
                                                                           : std::vector<Sample>{};
    const std::vector<Sample> &work = rows.empty() ? samples : rows;

    Result result;
    result.errorBefore = errorOf(work, pipeline, options.base);
    Settings best = options.base;
    for (const Stage &stage : options.stages) {
        for (const std::string &id : stage.ids) {
            best.setValue(id, sliders[id].defaultValue);
        }
    }
    if (best.has("Exposure2012")) {
        const Slider &exposure = sliders["Exposure2012"];
        const double stops
            = std::clamp(matchingExposure(work, pipeline, best), exposure.minimum, exposure.maximum);
        best.setValue(exposure.id, std::round(stops * 100) / 100);
    }
    double bestError = errorOf(work, pipeline, best);

    Progress progress;
    progress.stages = static_cast<int>(options.stages.size());
    try {
        for (std::size_t s = 0; s < options.stages.size(); ++s) {
            const Stage &stage = options.stages[s];
            progress.stage = static_cast<int>(s);
            progress.stageName = stage.name;
            std::map<std::vector<double>, double> tried;

            // The effects before the stage's first do the same in every trial: the samples go
            // through them once, and each trial renders the rest.
            const auto &effects = pipeline.effects();
            const auto first = effects.begin()
                               + static_cast<std::ptrdiff_t>(firstEffectOf(pipeline, stage));
            const Pipeline head{std::vector<std::shared_ptr<const Effect>>{effects.begin(), first}};
            const Pipeline tail{std::vector<std::shared_ptr<const Effect>>{first, effects.end()}};
            std::vector<Sample> stageSamples = work;
            for (Sample &sample : stageSamples) {
                sample.source
                    = *head.render(std::move(sample.source), best, RenderContext{sample.scale});
            }

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
            // A trial at `steps`; with `residuals`, also its differences per pixel, so graded
            // again even if tried before.
            const auto trial = [&](const std::vector<double> &steps, std::vector<float> *residuals) {
                const Settings settings = settingsAt(steps);
                std::vector<double> key;
                for (const std::string &id : stage.ids) {
                    key.push_back(valueOf(settings, sliders[id]));
                }
                if (const auto it = tried.find(key); it != tried.end() && !residuals) {
                    return it->second;
                }
                if (options.stopped && options.stopped()) {
                    throw Stop{};
                }
                const double error = errorOf(stageSamples, tail, settings, residuals);
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
            std::vector<double> low;
            std::vector<double> high;
            for (const std::string &id : stage.ids) {
                const Slider &slider = sliders[id];
                start.push_back(valueOf(best, slider) / slider.unit);
                low.push_back(slider.minimum / slider.unit);
                high.push_back(slider.maximum / slider.unit);
            }
            // Levenberg-Marquardt gets close in few trials, on the smooth L*a*b* differences;
            // Nelder-Mead then polishes the CIEDE2000 difference, the one the user is shown.
            const auto end = levenbergMarquardt(
                [&](const std::vector<double> &steps) {
                    std::vector<float> residuals;
                    trial(steps, &residuals);
                    return residuals;
                },
                start,
                low,
                high,
                [&] { return static_cast<int>(tried.size()) >= options.maxTrials; });
            const int polishFrom = static_cast<int>(tried.size());
            nelderMead(
                [&](const std::vector<double> &steps) { return trial(steps, nullptr); },
                end,
                [&tried, polishFrom] { return static_cast<int>(tried.size()) - polishFrom; },
                std::min(POLISH_TRIALS, std::max(0, options.maxTrials - polishFrom)));
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
