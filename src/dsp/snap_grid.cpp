// SPDX-FileCopyrightText: © 2019-2022, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <cmath>
#include <utility>

#include "dsp/snap_grid.h"
#include "utils/algorithms.h"

namespace zrythm::dsp
{

SnapGrid::SnapGrid (
  const TempoMap          &tempo_map,
  utils::NoteLength        default_note_length,
  LastObjectLengthCallback last_object_length_callback,
  QObject *                parent)
    : QObject (parent), default_note_length_ (default_note_length),
      tempo_map_ (tempo_map),
      last_object_length_callback_ (std::move (last_object_length_callback))
{
}

int
SnapGrid::get_ticks_from_length_and_type (
  utils::NoteLength length,
  utils::NoteType   type,
  int               ticks_per_bar,
  int               ticks_per_beat)
{
  int ticks = 0;
  switch (length)
    {
    case utils::NoteLength::Bar:
      z_return_val_if_fail (ticks_per_bar > 0, -1);
      ticks = ticks_per_bar;
      break;
    case utils::NoteLength::Beat:
      z_return_val_if_fail (ticks_per_beat > 0, -1);
      ticks = ticks_per_beat;
      break;
    case utils::NoteLength::Note_2_1:
      ticks = 8 * TempoMap::get_ppq ();
      break;
    case utils::NoteLength::Note_1_1:
      ticks = 4 * TempoMap::get_ppq ();
      break;
    case utils::NoteLength::Note_1_2:
      ticks = 2 * TempoMap::get_ppq ();
      break;
    case utils::NoteLength::Note_1_4:
      ticks = TempoMap::get_ppq ();
      break;
    case utils::NoteLength::Note_1_8:
      ticks = TempoMap::get_ppq () / 2;
      break;
    case utils::NoteLength::Note_1_16:
      ticks = TempoMap::get_ppq () / 4;
      break;
    case utils::NoteLength::Note_1_32:
      ticks = TempoMap::get_ppq () / 8;
      break;
    case utils::NoteLength::Note_1_64:
      ticks = TempoMap::get_ppq () / 16;
      break;
    case utils::NoteLength::Note_1_128:
      ticks = TempoMap::get_ppq () / 32;
      break;
    default:
      z_return_val_if_reached (-1);
    }

  switch (type)
    {
    case utils::NoteType::Normal:
      break;
    case utils::NoteType::Dotted:
      ticks = 3 * ticks;
      z_return_val_if_fail (ticks % 2 == 0, -1);
      ticks = ticks / 2;
      break;
    case utils::NoteType::Triplet:
      ticks = 2 * ticks;
      z_return_val_if_fail (ticks % 3 == 0, -1);
      ticks = ticks / 3;
      break;
    }

  return ticks;
}

double
SnapGrid::snapTicks (int64_t ticks) const
{
  if (snap_adaptive_)
    {
      if (sixteenths_visible_)
        {
          return get_ticks_from_length_and_type (
            utils::NoteLength::Note_1_16, snap_note_type_,
            TempoMap::get_ppq () * 4, TempoMap::get_ppq ());
        }
      if (beats_visible_)
        {
          return get_ticks_from_length_and_type (
            utils::NoteLength::Beat, snap_note_type_, TempoMap::get_ppq () * 4,
            TempoMap::get_ppq ());
        }

      return get_ticks_from_length_and_type (
        utils::NoteLength::Bar, snap_note_type_, TempoMap::get_ppq () * 4,
        TempoMap::get_ppq ());
    }

  auto time_sig = tempo_map_.time_signature_at_tick (units::ticks (ticks));
  int  ticks_per_bar =
    (time_sig.numerator * TempoMap::get_ppq () * 4) / time_sig.denominator;
  int ticks_per_beat = ticks_per_bar / time_sig.numerator;

  return get_ticks_from_length_and_type (
    snap_note_length_, snap_note_type_, ticks_per_bar, ticks_per_beat);
}

double
SnapGrid::defaultTicks (int64_t ticks) const
{
  if (length_type_ == NoteLengthType::Link)
    {
      return snapTicks (ticks);
    }
  if (length_type_ == NoteLengthType::LastObject)
    {
      return last_object_length_callback_ ();
    }

  auto time_sig = tempo_map_.time_signature_at_tick (units::ticks (ticks));
  int  ticks_per_bar =
    (time_sig.numerator * TempoMap::get_ppq () * 4) / time_sig.denominator;
  int ticks_per_beat = ticks_per_bar / time_sig.numerator;

  return get_ticks_from_length_and_type (
    default_note_length_, utils::NoteType::Normal, ticks_per_bar,
    ticks_per_beat);
}

static auto
get_note_type_short_str (utils::NoteType type)
{
  static constexpr std::array<std::string_view, 3> note_type_short_strings = {
    "",
    ".",
    "t",
  };
  return note_type_short_strings.at (ENUM_VALUE_TO_INT (type));
}

QString
SnapGrid::stringize_length_and_type (
  utils::NoteLength note_length,
  utils::NoteType   note_type)
{
  const auto c = get_note_type_short_str (note_type);
  const auto first_part = utils::note_length_to_str (note_length);

  return QString::fromStdString (fmt::format ("{}{}", first_part, c));
}

QString
SnapGrid::snapString () const
{
  if (snap_adaptive_)
    {
      return tr ("Adaptive");
    }

  return stringize_length_and_type (snap_note_length_, snap_note_type_);
}

bool
SnapGrid::get_prev_or_next_snap_point (
  double  pivot_ticks,
  double &out_ticks,
  bool    get_prev_point) const
{
  out_ticks = pivot_ticks;
  if (pivot_ticks < 0)
    {
      /* negative not supported, set to same position */
      return false;
    }

  bool snapped = false;

  // Grid snapping
  if (snap_to_grid_)
    {
      double snap_ticks = snapTicks (static_cast<int64_t> (out_ticks));
      double ticks_from_prev = std::fmod (pivot_ticks, snap_ticks);
      if (get_prev_point)
        {
          out_ticks = pivot_ticks - ticks_from_prev;
        }
      else
        {
          out_ticks = pivot_ticks + (snap_ticks - ticks_from_prev);
        }
      snapped = true;
    }

  // Event snapping
  if (snap_to_events_ && event_callback_.has_value ())
    {
      const auto snap_ticks = snapTicks (static_cast<int64_t> (pivot_ticks));
      const auto allowable_event_range =
        std::make_pair (pivot_ticks - snap_ticks, pivot_ticks + snap_ticks);
      auto event_points = get_event_snap_points (
        allowable_event_range.first, allowable_event_range.second);

      // callback should only return points in the range but this is not a hard
      // requirement. enforce the range here
      std::erase_if (event_points, [allowable_event_range] (const auto &point) {
        return point < allowable_event_range.first
               || point >= allowable_event_range.second;
      });

      auto it = utils::algorithms::find_closest (event_points, pivot_ticks);
      if (it != event_points.end ())
        {
          // out_ticks could already be snapped to the grid from above - only
          // snap further if this point is closer
          if (
            !snapped
            || std::abs (*it - pivot_ticks) < std::abs (out_ticks - pivot_ticks))
            {
              out_ticks = *it;
            }
          snapped = true;
        }
    }

  /* if no point to snap to, set to same position */
  if (!snapped)
    {
      out_ticks = pivot_ticks;
    }

  return snapped;
}

std::vector<double>
SnapGrid::get_event_snap_points (double start_ticks, double end_ticks) const
{
  if (event_callback_.has_value () && snap_to_events_)
    {
      return event_callback_.value () (start_ticks, end_ticks);
    }
  return {};
}

double
SnapGrid::nextSnapPoint (double ticks)
{
  double result{};
  get_prev_or_next_snap_point (ticks, result, false);
  return result;
}

double
SnapGrid::prevSnapPoint (double ticks)
{
  double result{};
  get_prev_or_next_snap_point (ticks, result, true);
  return result;
}

double
SnapGrid::closestSnapPoint (double ticks)
{
  double prev = prevSnapPoint (ticks);
  double next = nextSnapPoint (ticks);

  return (std::abs (ticks - prev) <= std::abs (ticks - next)) ? prev : next;
}

double
SnapGrid::snap (double ticks, std::optional<double> start_ticks)
{
  /* this should only be called if snap is on. the check should be done before
   * calling */
  assert (snap_to_grid_ || snap_to_events_);

  /* position must be positive - only global positions allowed */
  if (ticks < 0)
    return 0;

  if (snap_to_grid_keep_offset_)
    {
      assert (start_ticks.has_value ());

      /* get previous snap point from start position */
      double prev_sp_from_start = prevSnapPoint (start_ticks.value ());

      /* get diff from previous snap point */
      double ticks_delta = start_ticks.value () - prev_sp_from_start;

      /* subtract offset and find closest snap point */
      // double temp_ticks = ticks - ticks_delta;
      double closest_sp = closestSnapPoint (ticks);

      /* re-add offset */
      return closest_sp + ticks_delta;
    }

  return closestSnapPoint (ticks);
}

void
SnapGrid::setSnapAdaptive (bool adaptive)
{
  if (snap_adaptive_ != adaptive)
    {
      snap_adaptive_ = adaptive;
      Q_EMIT snapAdaptiveChanged ();
      Q_EMIT snapChanged ();
    }
}

void
SnapGrid::setSnapToGrid (bool snap)
{
  if (snap_to_grid_ != snap)
    {
      snap_to_grid_ = snap;
      Q_EMIT snapToGridChanged ();
      Q_EMIT snapChanged ();
    }
}

void
SnapGrid::setSnapToEvents (bool snap)
{
  if (snap_to_events_ != snap)
    {
      snap_to_events_ = snap;
      Q_EMIT snapToEventsChanged ();
      Q_EMIT snapChanged ();
    }
}

void
SnapGrid::setKeepOffset (bool keep)
{
  if (snap_to_grid_keep_offset_ != keep)
    {
      snap_to_grid_keep_offset_ = keep;
      Q_EMIT keepOffsetChanged ();
      Q_EMIT snapChanged ();
    }
}

void
SnapGrid::setSixteenthsVisible (bool visible)
{
  if (sixteenths_visible_ != visible)
    {
      sixteenths_visible_ = visible;
      Q_EMIT sixteenthsVisibleChanged ();
      Q_EMIT snapChanged ();
    }
}

void
SnapGrid::setBeatsVisible (bool visible)
{
  if (beats_visible_ != visible)
    {
      beats_visible_ = visible;
      Q_EMIT beatsVisibleChanged ();
      Q_EMIT snapChanged ();
    }
}

void
SnapGrid::setSnapNoteLength (utils::NoteLength length)
{
  if (length == snap_note_length_)
    return;

  snap_note_length_ = length;
  Q_EMIT snapNoteLengthChanged ();
  Q_EMIT snapChanged ();
}

void
SnapGrid::setSnapNoteType (utils::NoteType type)
{
  if (type == snap_note_type_)
    return;

  snap_note_type_ = type;
  Q_EMIT snapNoteTypeChanged ();
  Q_EMIT snapChanged ();
}

void
SnapGrid::setLengthType (NoteLengthType type)
{
  if (type == length_type_)
    return;

  length_type_ = type;
  Q_EMIT lengthTypeChanged ();
  Q_EMIT snapChanged ();
}

void
SnapGrid::set_event_callback (SnapEventCallback callback)
{
  event_callback_ = callback;
}

void
SnapGrid::clear_callbacks ()
{
  event_callback_.reset ();
  last_object_length_callback_ = [] () { return 0.0; };
}

} // namespace zrythm::dsp
