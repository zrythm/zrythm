// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/fadeable_object.h"
#include "zrythm.h"

bool
FadeableObject::are_members_valid (bool is_project) const
{
  if (!is_position_valid (fade_in_pos_, PositionType::FadeIn))
    {
      if (ZRYTHM_TESTING)
        {
          z_info ("invalid fade in pos {}", fade_in_pos_);
        }
      return false;
    }
  if (!is_position_valid (fade_out_pos_, PositionType::FadeOut))
    {
      if (ZRYTHM_TESTING)
        {
          z_info ("invalid fade out pos {}", fade_out_pos_);
        }
      return false;
    }
  return true;
}