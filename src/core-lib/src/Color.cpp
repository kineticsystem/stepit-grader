// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#include "core/Color.h"

#include <algorithm>
#include <cmath>

namespace core {

Matrix3 multiply(const Matrix3 &a, const Matrix3 &b)
{
    Matrix3 r{};
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            for (int k = 0; k < 3; ++k) {
                r[i * 3 + j] += a[i * 3 + k] * b[k * 3 + j];
            }
        }
    }
    return r;
}

Matrix3 inverse(const Matrix3 &m)
{
    const double det = m[0] * (m[4] * m[8] - m[5] * m[7]) - m[1] * (m[3] * m[8] - m[5] * m[6])
                       + m[2] * (m[3] * m[7] - m[4] * m[6]);
    const double d = 1.0 / det;
    return {
        (m[4] * m[8] - m[5] * m[7]) * d,
        (m[2] * m[7] - m[1] * m[8]) * d,
        (m[1] * m[5] - m[2] * m[4]) * d,
        (m[5] * m[6] - m[3] * m[8]) * d,
        (m[0] * m[8] - m[2] * m[6]) * d,
        (m[2] * m[3] - m[0] * m[5]) * d,
        (m[3] * m[7] - m[4] * m[6]) * d,
        (m[1] * m[6] - m[0] * m[7]) * d,
        (m[0] * m[4] - m[1] * m[3]) * d};
}

namespace color {

namespace {

constexpr Matrix3 XYZ_TO_SRGB{
    3.2404542,
    -1.5371385,
    -0.4985314,
    -0.9692660,
    1.8760108,
    0.0415560,
    0.0556434,
    -0.2040259,
    1.0572252};

constexpr Matrix3 BRADFORD{0.8951, 0.2664, -0.1614, -0.7502, 1.7135, 0.0367, 0.0389, -0.0685, 1.0296};

// Oklab's matrices: XYZ (D65) to LMS, and the cube roots of LMS to Lab.
constexpr Matrix3 OKLAB_M1{
    0.8189330101,
    0.3618667424,
    -0.1288597137,
    0.0329845436,
    0.9293118715,
    0.0361456387,
    0.0482003018,
    0.2643662691,
    0.6338517070};
constexpr Matrix3 OKLAB_M2{
    0.2104542553,
    0.7936177850,
    -0.0040720468,
    1.9779984951,
    -2.4285922050,
    0.4505937099,
    0.0259040371,
    0.7827717662,
    -0.8086757660};

std::array<double, 3> toXyz(Chromaticity c)
{
    return {c.x / c.y, 1.0, (1.0 - c.x - c.y) / c.y};
}

std::array<double, 3> apply(const Matrix3 &m, const std::array<double, 3> &v)
{
    return {
        m[0] * v[0] + m[1] * v[1] + m[2] * v[2],
        m[3] * v[0] + m[4] * v[1] + m[5] * v[2],
        m[6] * v[0] + m[7] * v[1] + m[8] * v[2]};
}

struct OklabMatrices
{
    std::array<float, 9> toLms;
    std::array<float, 9> fromLms;
    std::array<float, 9> toLab;
    std::array<float, 9> fromLab;
};

std::array<float, 9> toFloat(const Matrix3 &m)
{
    std::array<float, 9> f{};
    for (int i = 0; i < 9; ++i) {
        f[i] = static_cast<float>(m[i]);
    }
    return f;
}

const OklabMatrices &oklab()
{
    static const OklabMatrices matrices = [] {
        Matrix3 toLms = multiply(OKLAB_M1, rec2020ToXyz());
        // Oklab's matrix is made for D65 rounded differently: each row is scaled so that the
        // working space's white is exactly neutral, and a grey has no colour at all.
        for (int row = 0; row < 3; ++row) {
            const double sum = toLms[row * 3] + toLms[row * 3 + 1] + toLms[row * 3 + 2];
            for (int col = 0; col < 3; ++col) {
                toLms[row * 3 + col] /= sum;
            }
        }
        return OklabMatrices{
            toFloat(toLms), toFloat(inverse(toLms)), toFloat(OKLAB_M2), toFloat(inverse(OKLAB_M2))};
    }();
    return matrices;
}

inline void apply(const std::array<float, 9> &m, const float *in, float *out)
{
    const float a = in[0], b = in[1], c = in[2];
    out[0] = m[0] * a + m[1] * b + m[2] * c;
    out[1] = m[3] * a + m[4] * b + m[5] * c;
    out[2] = m[6] * a + m[7] * b + m[8] * c;
}

} // namespace

const Matrix3 &rec2020ToXyz()
{
    static const Matrix3
        m{0.6369580,
          0.1446169,
          0.1688810,
          0.2627002,
          0.6779981,
          0.0593017,
          0.0000000,
          0.0280727,
          1.0609851};
    return m;
}

const Matrix3 &rec2020ToSrgb()
{
    static const Matrix3 m = multiply(XYZ_TO_SRGB, rec2020ToXyz());
    return m;
}

Matrix3 adaptToD65(Chromaticity source)
{
    const auto from = apply(BRADFORD, toXyz(source));
    const auto to = apply(BRADFORD, toXyz(D65));
    const Matrix3 scale{to[0] / from[0], 0, 0, 0, to[1] / from[1], 0, 0, 0, to[2] / from[2]};
    const Matrix3 adaptXyz = multiply(inverse(BRADFORD), multiply(scale, BRADFORD));
    return multiply(inverse(rec2020ToXyz()), multiply(adaptXyz, rec2020ToXyz()));
}

Chromaticity planckian(double kelvin)
{
    const double t = std::clamp(kelvin, 1667.0, 25000.0);
    const double t1 = 1e3 / t, t2 = t1 * t1, t3 = t2 * t1;
    const double x = t <= 4000 ? -0.2661239 * t3 - 0.2343589 * t2 + 0.8776956 * t1 + 0.179910
                               : -3.0258469 * t3 + 2.1070379 * t2 + 0.2226347 * t1 + 0.240390;
    const double x2 = x * x, x3 = x2 * x;
    double y;
    if (t <= 2222) {
        y = -1.1063814 * x3 - 1.34811020 * x2 + 2.18555832 * x - 0.20219683;
    } else if (t <= 4000) {
        y = -0.9549476 * x3 - 1.37418593 * x2 + 2.09137015 * x - 0.16748867;
    } else {
        y = 3.0817580 * x3 - 5.87338670 * x2 + 3.75112997 * x - 0.37001483;
    }
    return {x, y};
}

std::array<double, 2> toUv(Chromaticity c)
{
    const double d = -2.0 * c.x + 12.0 * c.y + 3.0;
    return {4.0 * c.x / d, 6.0 * c.y / d};
}

Chromaticity fromUv(double u, double v)
{
    const double d = 2.0 * u - 8.0 * v + 4.0;
    return {3.0 * u / d, 2.0 * v / d};
}

void rec2020ToOklab(const float *rgb, float *lab)
{
    const auto &m = oklab();
    float lms[3];
    apply(m.toLms, rgb, lms);
    for (float &v : lms) {
        v = std::cbrt(v);
    }
    apply(m.toLab, lms, lab);
}

void oklabToRec2020(const float *lab, float *rgb)
{
    const auto &m = oklab();
    float lms[3];
    apply(m.fromLab, lab, lms);
    for (float &v : lms) {
        v = v * v * v;
    }
    apply(m.fromLms, lms, rgb);
}

} // namespace color
} // namespace core
