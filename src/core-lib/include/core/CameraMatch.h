// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#pragma once

#include "core/Image.h"
#include "core/Pipeline.h"
#include "core/Settings.h"

#include <array>
#include <functional>
#include <string>
#include <vector>

/**
 * Finds the slider values that make a raw picture, graded, look like the JPEG its camera made of
 * it: the camera's own look, as a grade. The pictures are compared as the screen shows them, by
 * the CIEDE2000 colour difference. The sliders are searched in stages, first the tones, then the
 * colours: in each, Levenberg-Marquardt gets close and a Nelder-Mead search polishes.
 */
namespace core::match {

/// CIE L*a*b*, D65.
using Lab = std::array<float, 3>;

/// The CIEDE2000 difference of two colours (Sharma et al., 2005): below 1 it cannot be seen.
double deltaE2000(const Lab &a, const Lab &b);

/// A linear sRGB colour as the screen shows it, clipped to [0, 1], in CIE L*a*b*.
Lab labOfLinearSrgb(const float *rgb);

/// One picture to match: the raw, small, and the camera's JPEG at a sample of its pixels.
struct Sample
{
    /// The raw picture in the working space, linear Rec. 2020, reduced.
    Image source;
    /// The size of the reduced picture against the full one, for the effects measured in pixels.
    double scale = 1.0;
    /// The pixels compared, as indices into `source`, and the JPEG's colour at each.
    std::vector<int> pixels;
    std::vector<Lab> target;
};

/// The JPEG pixels compared per picture: as many as fit the camera's look as well as 100,000 do,
/// a quarter of the work per trial.
constexpr int SAMPLE_PIXELS = 25'000;

/**
 * A sample from a raw picture and its camera JPEG, both of the same size: the raw in the working
 * space, the JPEG in linear sRGB. The JPEG's clipped pixels, a channel at 98% or more, say
 * nothing of the camera's curve and are left out; of the others, at most `maxPixels` are kept, the
 * same for every run.
 */
Sample makeSample(
    Image source, const Image &jpegLinearSrgb, double scale, int maxPixels = SAMPLE_PIXELS);

/// A group of sliders searched together, the others kept at their best values so far.
struct Stage
{
    std::string name;
    std::vector<std::string> ids;
};

/**
 * The stages that fit the camera's look with the tools that act the same on every picture: the
 * exposure, the contrast, the blacks, the saturation, the vibrance, the white balance and the
 * tone curve, then the colour mixer. The local tools, e.g. the shadows, are left alone: they would
 * fit one picture and do something else on the next.
 */
std::vector<Stage> standardStages();

/// How the search is going, for a progress bar.
struct Progress
{
    /// From 0 at the start to 1 at the end.
    double fraction = 0;
    int stage = 0;
    int stages = 0;
    std::string stageName;
    int trials = 0;
    /// The best mean difference so far.
    double bestError = 0;
};

struct Options
{
    std::vector<Stage> stages = standardStages();
    /**
     * The settings the grade found goes over, e.g. the picture's own: the sliders not searched keep
     * their values here through the search, so the grade fits the picture as it will be shown.
     * The sliders searched start from their defaults whatever their values here.
     */
    Settings base;
    /// The most trials of each stage.
    int maxTrials = 1500;
    /// Called after each trial, from the searching thread.
    std::function<void(const Progress &)> progress;
    /// Asked before each trial: once it says yes, the search stops and keeps its best so far.
    std::function<bool()> stopped;
};

struct Result
{
    /// The values of the sliders searched; the others are left out, as in `Options::base`.
    Settings settings;
    /// The mean difference from the JPEGs, with `Options::base`, and with the settings found over
    /// it.
    double errorBefore = 0;
    double errorAfter = 0;
    int trials = 0;
    /// Stopped before the search was done: the settings are the best found by then.
    bool stopped = false;
};

/**
 * Searches the sliders of `options.stages` so the samples, graded by `pipeline`, look like their
 * JPEGs. A trial renders only the pixels compared when every effect it runs is pointwise, and
 * renders the effects before a stage's first slider once for the whole stage.
 */
Result fit(const std::vector<Sample> &samples, const Pipeline &pipeline, const Options &options);

} // namespace core::match
