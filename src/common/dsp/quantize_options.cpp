// SPDX-FileCopyrightText: © 2019-2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "common/dsp/position.h"
#include "common/dsp/quantize_options.h"
#include "common/dsp/snap_grid.h"
#include "common/dsp/transport.h"
#include "common/utils/algorithms.h"
#include "common/utils/pcg_rand.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"
#include "gui/backend/gtk_widgets/gtk_wrapper.h"

void
QuantizeOptions::update_quantize_points ()
{
  Position tmp, end_pos;
  end_pos.set_to_bar (TRANSPORT->total_bars_ + 1);
  q_points_.clear ();
  q_points_.push_back (tmp);
  double ticks =
    SnapGrid::get_ticks_from_length_and_type (note_length_, note_type_);
  double swing_offset = (swing_ / 100.0) * ticks / 2.0;
  while (tmp < end_pos)
    {
      tmp.add_ticks (ticks);

      /* delay every second point by swing */
      if ((q_points_.size () + 1) % 2 == 0)
        {
          tmp.add_ticks (swing_offset);
        }

      q_points_.push_back (tmp);
    }
}

void
QuantizeOptions::init (NoteLength note_length)
{
  note_length_ = note_length;
  q_points_.clear ();
  note_type_ = NoteType::NOTE_TYPE_NORMAL;
  amount_ = 100.f;
  adj_start_ = true;
  adj_end_ = false;
  swing_ = 0.f;
  rand_ticks_ = 0.0;
}

float
QuantizeOptions::get_swing () const
{
  return swing_;
}

float
QuantizeOptions::get_amount () const
{
  return amount_;
}

float
QuantizeOptions::get_randomization () const
{
  return (float) rand_ticks_;
}

void
QuantizeOptions::set_swing (float swing)
{
  swing_ = swing;
}

void
QuantizeOptions::set_amount (float amount)
{
  amount_ = amount;
}

void
QuantizeOptions::set_randomization (float randomization)
{
  rand_ticks_ = (double) randomization;
}

std::string
QuantizeOptions::to_string (NoteLength note_length, NoteType note_type)
{
  return SnapGrid::stringize_length_and_type (note_length, note_type);
}

const Position *
QuantizeOptions::get_prev_point (Position * pos) const
{
  z_return_val_if_fail (pos->is_positive (), nullptr);

  Position * prev_point = (Position *) algorithms_binary_search_nearby (
    pos, q_points_.data (), q_points_.size (), sizeof (Position),
    Position::compare_frames_cmpfunc, true, true);

  return prev_point;
}

const Position *
QuantizeOptions::get_next_point (Position * pos) const
{
  z_return_val_if_fail (pos->is_positive (), nullptr);

  Position * next_point = (Position *) algorithms_binary_search_nearby (
    pos, q_points_.data (), q_points_.size (), sizeof (Position),
    Position::compare_frames_cmpfunc, false, true);

  return next_point;
}

double
QuantizeOptions::quantize_position (Position * pos)
{
  auto prev_point = get_prev_point (pos);
  auto next_point = get_next_point (pos);
  z_return_val_if_fail (prev_point && next_point, 0);

  const double upper = rand_ticks_;
  const double lower = -rand_ticks_;
  auto         rand = PCGRand::getInstance ();
  double       rand_double = (double) rand->u32 ();
  double       rand_ticks = fmod (rand_double, (upper - lower + 1.0)) + lower;

  /* if previous point is closer */
  double diff;
  if (pos->ticks_ - prev_point->ticks_ <= next_point->ticks_ - pos->ticks_)
    {
      diff = prev_point->ticks_ - pos->ticks_;
    }
  /* if next point is closer */
  else
    {
      diff = next_point->ticks_ - pos->ticks_;
    }

  /* multiply by amount */
  diff = (diff * (double) (amount_ / 100.f));

  /* add random ticks */
  diff += rand_ticks;

  /* quantize position */
  pos->add_ticks (diff);

  return diff;
}