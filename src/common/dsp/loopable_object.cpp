// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "common/dsp/loopable_object.h"
#include "common/utils/debug.h"
#include "common/utils/gtest_wrapper.h"
#include "gui/backend/backend/zrythm.h"

void
LoopableObject::copy_members_from (const LoopableObject &other)
{
  clip_start_pos_ = other.clip_start_pos_;
  loop_start_pos_ = other.loop_start_pos_;
  loop_end_pos_ = other.loop_end_pos_;
}

void
LoopableObject::init_loaded_base ()
{
}

int
LoopableObject::get_num_loops (bool count_incomplete) const
{
  int  i = 0;
  long loop_size = get_loop_length_in_frames ();
  z_return_val_if_fail_cmp (loop_size, >, 0, 0);
  long full_size = get_length_in_frames ();
  long loop_start = loop_start_pos_.frames_ - clip_start_pos_.frames_;
  long curr_frames = loop_start;

  while (curr_frames < full_size)
    {
      i++;
      curr_frames += loop_size;
    }

  if (!count_incomplete)
    i--;

  return i;
}

bool
LoopableObject::are_members_valid (bool is_project) const
{
  if (!is_position_valid (loop_start_pos_, PositionType::LoopStart))
    {
      if (ZRYTHM_TESTING)
        {
          z_info ("invalid loop start pos {}", loop_start_pos_);
        }
      return false;
    }
  if (!is_position_valid (loop_end_pos_, PositionType::LoopEnd))
    {
      if (ZRYTHM_TESTING)
        {
          z_info ("invalid loop end pos {}", loop_end_pos_);
        }
      return false;
    }
  if (!is_position_valid (clip_start_pos_, PositionType::ClipStart))
    {
      if (ZRYTHM_TESTING)
        {
          z_info ("invalid clip start pos {}", clip_start_pos_);
        }
      return false;
    }

  return true;
}