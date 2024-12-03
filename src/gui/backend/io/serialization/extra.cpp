// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "utils/color.h"

using namespace zrythm;

void
utils::Color::define_fields (const Context &ctx)
{
  serialize_fields (
    ctx, make_field ("red", red_), make_field ("green", green_),
    make_field ("blue", blue_), make_field ("alpha", alpha_));
}
