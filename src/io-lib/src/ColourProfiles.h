// Copyright (c) 2026 Giovanni Remigi
// SPDX-License-Identifier: MIT

#pragma once

// The ICC profiles the readers and the writers share: Little CMS, private to io-lib.

#include "imageio/ColourSpace.h"

#include <lcms2.h>

#include <memory>

namespace imageio::profiles {

struct ProfileCloser
{
    void operator()(void *profile) const { cmsCloseProfile(profile); }
};
struct TransformDeleter
{
    void operator()(void *transform) const { cmsDeleteTransform(transform); }
};
using Profile = std::unique_ptr<void, ProfileCloser>;
using Transform = std::unique_ptr<void, TransformDeleter>;

/// The working space: Rec. 2020 primaries, D65 white, linear.
Profile workingSpace();

/// The profile of an output colour space, named as a program shows it, e.g. "sRGB".
Profile of(ColourSpace space);

} // namespace imageio::profiles
