// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/arrangement/fadeable_object.h"
#include "utils/gtest_wrapper.h"

namespace zrythm::structure::arrangement
{
void
init_from (
  FadeableObject        &obj,
  const FadeableObject  &other,
  utils::ObjectCloneType clone_type)
{
  obj.fade_in_pos_ = other.fade_in_pos_;
  obj.fade_out_pos_ = other.fade_out_pos_;
  obj.fade_in_opts_ = other.fade_in_opts_;
  obj.fade_out_opts_ = other.fade_out_opts_;
}

bool
FadeableObject::are_members_valid (
  bool               is_project,
  dsp::FramesPerTick frames_per_tick) const
{
  const auto ticks_per_frame = dsp::to_ticks_per_frame (frames_per_tick);
  if (!is_position_valid (fade_in_pos_, PositionType::FadeIn, ticks_per_frame))
    {
      if (ZRYTHM_TESTING)
        {
          z_info ("invalid fade in pos {}", fade_in_pos_);
        }
      return false;
    }
  if (!is_position_valid (fade_out_pos_, PositionType::FadeOut, ticks_per_frame))
    {
      if (ZRYTHM_TESTING)
        {
          z_info ("invalid fade out pos {}", fade_out_pos_);
        }
      return false;
    }
  return true;
}
}
