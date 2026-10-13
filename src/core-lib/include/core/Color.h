// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#pragma once

#include <array>

namespace core {

/// A 3 x 3 matrix, row-major.
using Matrix3 = std::array<double, 9>;

Matrix3 multiply(const Matrix3 &a, const Matrix3 &b);
Matrix3 inverse(const Matrix3 &m);

/// CIE xy chromaticity.
struct Chromaticity
{
    double x = 0;
    double y = 0;
};

namespace color {

/// The luminance weights of linear Rec. 2020, ITU-R BT.2020.
constexpr float LUMA_R = 0.2627f;
constexpr float LUMA_G = 0.6780f;
constexpr float LUMA_B = 0.0593f;

/// Middle grey, in linear values: the pivot of the tone controls.
constexpr float MIDDLE_GREY = 0.18f;

inline float luminance(const float *rgb)
{
    return LUMA_R * rgb[0] + LUMA_G * rgb[1] + LUMA_B * rgb[2];
}

/// The white of D65, the white of the working space.
constexpr Chromaticity D65{0.31271, 0.32902};

/// Linear Rec. 2020 to CIE XYZ, D65.
const Matrix3 &rec2020ToXyz();

/// Linear Rec. 2020 to linear sRGB, both D65.
const Matrix3 &rec2020ToSrgb();

/// Linear sRGB to CIE XYZ, D65: the inverse of the matrix rec2020ToSrgb() goes through.
const Matrix3 &srgbToXyz();

/**
 * The sRGB curve: a linear value to the tone a screen shows, 0 to 0 and 1 to 1, straight near
 * black. Not clipped: a value outside [0, 1] gives a tone outside it.
 */
double srgbEncode(double linear);

/// The inverse of srgbEncode(): a tone to its linear value.
double srgbDecode(double tone);

/**
 * The chromatic adaptation of linear Rec. 2020 values from a scene lit by `source` to one lit by
 * D65, with the Bradford transform: what was white under `source` becomes white.
 */
Matrix3 adaptToD65(Chromaticity source);

/// The chromaticity of a black body at `kelvin`, 1667 to 25000 K (Kim et al., 2002).
Chromaticity planckian(double kelvin);

/// CIE 1960 UCS (u, v), where distances between colours of light are about even.
std::array<double, 2> toUv(Chromaticity xy);
Chromaticity fromUv(double u, double v);

/// Linear Rec. 2020 to Oklab (Ottosson, 2020): L, a, b. And back.
void rec2020ToOklab(const float *rgb, float *lab);
void oklabToRec2020(const float *lab, float *rgb);

} // namespace color
} // namespace core
