// SPDX-FileCopyrightText: © 2019-2021, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/position.h"
#include "dsp/snap_grid.h"
#include "dsp/transport.h"
#include "gui/dsp/quantize_options.h"
#include "utils/algorithms.h"
#include "utils/pcg_rand.h"

namespace zrythm::gui::old_dsp
{

void
QuantizeOptions::update_quantize_points (const dsp::Transport &transport)
{
// TODO
#if 0
  auto    &audio_engine = transport.project_->audio_engine_;
  Position tmp, end_pos;
  end_pos.set_to_bar (
    transport.total_bars_ + 1, transport.ticks_per_bar_,
    audio_engine->frames_per_tick_);
  q_points_.clear ();
  q_points_.push_back (tmp);
  double ticks = dsp::SnapGrid::get_ticks_from_length_and_type (
    note_length_, note_type_, transport.ticks_per_bar_,
    transport.ticks_per_beat_);
  double swing_offset = (swing_ / 100.0) * ticks / 2.0;
  while (tmp < end_pos)
    {
      tmp.add_ticks (ticks, audio_engine->frames_per_tick_);

      /* delay every second point by swing */
      if ((q_points_.size () + 1) % 2 == 0)
        {
          tmp.add_ticks (swing_offset, audio_engine->frames_per_tick_);
        }

      q_points_.push_back (tmp);
    }
#endif
}

void
QuantizeOptions::init (NoteLength note_length)
{
  note_length_ = note_length;
  q_points_.clear ();
  note_type_ = NoteType::Normal;
  amount_ = 100.f;
  adjust_start_ = true;
  adjust_end_ = false;
  swing_ = 0.f;
  randomization_ticks_ = 0.0;
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
  return (float) randomization_ticks_;
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
  randomization_ticks_ = (double) randomization;
}

utils::Utf8String
QuantizeOptions::to_string (NoteLength note_length, NoteType note_type)
{
  return utils::Utf8String::from_qstring (
    dsp::SnapGrid::stringize_length_and_type (note_length, note_type));
}

units::precise_tick_t
QuantizeOptions::get_prev_point (units::precise_tick_t pos) const
{
  assert (pos > units::ticks (0));

  auto result = utils::algorithms::binary_search_nearby (
    pos, std::span (q_points_), true, true);
  return result ? result->get () : units::ticks (0);
  // Position * prev_point = (Position *) algorithms_binary_search_nearby (
  //   pos, q_points_.data (), q_points_.size (), sizeof (Position),
  //   Position::compare_frames_cmpfunc, true, true);

  // return prev_point;
}

units::precise_tick_t
QuantizeOptions::get_next_point (units::precise_tick_t pos) const
{
  assert (pos > units::ticks (0));

  auto result = utils::algorithms::binary_search_nearby (
    pos, std::span (q_points_), false, true);
  return result ? result->get () : units::ticks (0);
}

std::pair<units::precise_tick_t, double>
QuantizeOptions::quantize_position (units::precise_tick_t pos)
{
  auto prev_point = get_prev_point (pos);
  auto next_point = get_next_point (pos);

  const double upper = randomization_ticks_;
  const double lower = -randomization_ticks_;
  double       rand_double = (double) rand_.u32 ();
  double       rand_ticks = fmod (rand_double, (upper - lower + 1.0)) + lower;

  /* if previous point is closer */
  units::precise_tick_t diff;
  if (pos - prev_point <= next_point - pos)
    {
      diff = prev_point - pos;
    }
  /* if next point is closer */
  else
    {
      diff = next_point - pos;
    }

  /* multiply by amount */
  diff = (diff * (double) (amount_ / 100.f));

  /* add random ticks */
  diff += units::ticks (rand_ticks);

  /* quantize position */
  auto ret_pos = pos + diff;

  return { ret_pos, diff.in (units::ticks) };
}

} // namespace zrythm::gui::old_dsp
