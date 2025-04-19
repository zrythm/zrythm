// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/dsp/fadeable_object.h"

#include "utils/gtest_wrapper.h"

void
FadeableObject::copy_members_from (
  const FadeableObject &other,
  ObjectCloneType       clone_type)
{
  fade_in_pos_ = other.fade_in_pos_;
  fade_out_pos_ = other.fade_out_pos_;
  fade_in_opts_ = other.fade_in_opts_;
  fade_out_opts_ = other.fade_out_opts_;
}

bool
FadeableObject::are_members_valid (bool is_project, double frames_per_tick) const
{
  if (!is_position_valid (
        fade_in_pos_, PositionType::FadeIn, 1.0 / frames_per_tick))
    {
      if (ZRYTHM_TESTING)
        {
          z_info ("invalid fade in pos {}", fade_in_pos_);
        }
      return false;
    }
  if (!is_position_valid (
        fade_out_pos_, PositionType::FadeOut, 1.0 / frames_per_tick))
    {
      if (ZRYTHM_TESTING)
        {
          z_info ("invalid fade out pos {}", fade_out_pos_);
        }
      return false;
    }
  return true;
}
