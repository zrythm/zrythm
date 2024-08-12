// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/curve.h"
#include "dsp/position.h"
#include "utils/color.h"

void
CurveOptions::define_fields (const Context &ctx)
{
  serialize_fields (
    ctx, make_field ("algorithm", algo_), make_field ("curviness", curviness_));
}

void
Position::define_fields (const Context &ctx)
{
  serialize_fields (
    ctx, make_field ("ticks", ticks_), make_field ("frames", frames_));
}

void
Color::define_fields (const Context &ctx)
{
  serialize_fields (
    ctx, make_field ("red", red_), make_field ("green", green_),
    make_field ("blue", blue_), make_field ("alpha", alpha_));
}