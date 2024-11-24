// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/dsp/colored_object.h"

void
ColoredObject::copy_members_from (const ColoredObject &other)
{
  color_ = other.color_;
  use_color_ = other.use_color_;
}

void
ColoredObject::init_loaded_base ()
{
}
