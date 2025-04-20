// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/dsp/colored_object.h"
#include "gui/dsp/track_all.h"

void
ColoredObject::copy_members_from (
  const ColoredObject &other,
  ObjectCloneType      clone_type)
{
  color_ = other.color_;
  use_color_ = other.use_color_;
}

QColor
ColoredObject::get_effective_color () const
{
  if (use_color_)
    {
      return color_.to_qcolor ();
    }

  auto track_var = get_track ();
  return std::visit (
    [&] (auto &&track) { return track->getColor (); }, track_var);
}