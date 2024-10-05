// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "common/dsp/fadeable_object.h"
#include "common/utils/gtest_wrapper.h"

void
FadeableObject::copy_members_from (const FadeableObject &other)
{
  fade_in_pos_ = other.fade_in_pos_;
  fade_out_pos_ = other.fade_out_pos_;
  fade_in_opts_ = other.fade_in_opts_;
  fade_out_opts_ = other.fade_out_opts_;
}

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