// SPDX-FileCopyrightText: © 2019-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "engine/device_io/engine.h"
#include "engine/session/transport.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/settings_manager.h"
#include "gui/backend/backend/zrythm.h"
#include "gui/dsp/snap_grid.h"
#include "utils/gtest_wrapper.h"

#include <fmt/printf.h>

namespace zrythm::gui
{

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
      ticks = 8 * Position::TICKS_PER_QUARTER_NOTE;
      break;
    case utils::NoteLength::Note_1_1:
      ticks = 4 * Position::TICKS_PER_QUARTER_NOTE;
      break;
    case utils::NoteLength::Note_1_2:
      ticks = 2 * Position::TICKS_PER_QUARTER_NOTE;
      break;
    case utils::NoteLength::Note_1_4:
      ticks = Position::TICKS_PER_QUARTER_NOTE;
      break;
    case utils::NoteLength::Note_1_8:
      ticks = Position::TICKS_PER_QUARTER_NOTE / 2;
      break;
    case utils::NoteLength::Note_1_16:
      ticks = Position::TICKS_PER_QUARTER_NOTE / 4;
      break;
    case utils::NoteLength::Note_1_32:
      ticks = Position::TICKS_PER_QUARTER_NOTE / 8;
      break;
    case utils::NoteLength::Note_1_64:
      ticks = Position::TICKS_PER_QUARTER_NOTE / 16;
      break;
    case utils::NoteLength::Note_1_128:
      ticks = Position::TICKS_PER_QUARTER_NOTE / 32;
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

int
SnapGrid::get_snap_ticks () const
{
  if (snap_adaptive_)
    {
      if (!ZRYTHM_HAVE_UI || ZRYTHM_TESTING || ZRYTHM_BENCHMARKING)
        {
          z_error ("can only be used with UI");
          return -1;
        }

      return -1;

#if 0
      RulerWidget * ruler = NULL;
      if (type_ == Type::Timeline)
        {
          ruler = MW_RULER;
        }
      else
        {
          ruler = EDITOR_RULER;
        }

      /* get intervals used in drawing */
      int sixteenth_interval = ruler_widget_get_sixteenth_interval (ruler);
      int beat_interval = ruler_widget_get_beat_interval (ruler);
      int bar_interval =
        (int) MAX ((RW_PX_TO_HIDE_BEATS) / (double) ruler->px_per_bar, 1.0);

      /* attempt to snap at smallest interval */
      if (sixteenth_interval > 0)
        {
          return sixteenth_interval
                 * get_ticks_from_length_and_type (
                   NoteLength::NOTE_LENGTH_1_16, snap_note_type_);
        }
      else if (beat_interval > 0)
        {
          return beat_interval
                 * get_ticks_from_length_and_type (
                   NoteLength::NOTE_LENGTH_BEAT, snap_note_type_);
        }
      else
        {
          return bar_interval
                 * get_ticks_from_length_and_type (
                   NoteLength::NOTE_LENGTH_BAR, snap_note_type_);
        }
#endif
    }
  else
    {
      return SnapGrid::get_ticks_from_length_and_type (
        snap_note_length_, snap_note_type_, ticks_per_bar_provider_ (),
        ticks_per_beat_provider_ ());
    }
}

double
SnapGrid::get_snap_frames () const
{
  int snap_ticks = get_snap_ticks ();
  return type_safe::get (AUDIO_ENGINE->frames_per_tick_)
         * static_cast<double> (snap_ticks);
}

int
SnapGrid::get_default_ticks () const
{
  if (length_type_ == NoteLengthType::NOTE_LENGTH_LINK)
    {
      return get_snap_ticks ();
    }
  if (length_type_ == NoteLengthType::NOTE_LENGTH_LAST_OBJECT)
    {
      double last_obj_length = 0.0;
      if (type_ == SnapGrid::Type::Timeline)
        {
          last_obj_length =
            gui::SettingsManager::timelineLastCreatedObjectLengthInTicks ();
        }
      else if (type_ == SnapGrid::Type::Editor)
        {
          last_obj_length =
            gui::SettingsManager::editorLastCreatedObjectLengthInTicks ();
        }
      return (int) last_obj_length;
    }

  return SnapGrid::get_ticks_from_length_and_type (
    default_note_length_, default_note_type_, ticks_per_bar_provider_ (),
    ticks_per_beat_provider_ ());
}

static const char *
get_note_type_short_str (utils::NoteType type)
{
  static const char * note_type_short_strings[] = {
    "",
    ".",
    "t",
  };
  return note_type_short_strings[ENUM_VALUE_TO_INT (type)];
}

utils::Utf8String
SnapGrid::stringize_length_and_type (
  utils::NoteLength note_length,
  utils::NoteType   note_type)
{
  const char * c = get_note_type_short_str (note_type);
  const auto   first_part = utils::note_length_to_str (note_length);

  return utils::Utf8String::from_utf8_encoded_string (
    fmt::format ("{}{}", first_part, c));
}

utils::Utf8String
SnapGrid::stringize () const
{
  if (snap_adaptive_)
    {
      return utils::Utf8String::from_qstring (QObject::tr ("Adaptive"));
    }
  else
    {
      return stringize_length_and_type (snap_note_length_, snap_note_type_);
    }
}

bool
SnapGrid::get_nearby_snap_point (
  Position  &ret_pos,
  double     ticks,
  const bool return_prev)
{
  assert (ticks >= 0);

  ret_pos.from_ticks (ticks, AUDIO_ENGINE->frames_per_tick_);
  double snap_ticks = get_snap_ticks ();
  double ticks_from_prev = std::fmod (ticks, snap_ticks);
  if (return_prev)
    {
      ret_pos.add_ticks (-ticks_from_prev, AUDIO_ENGINE->frames_per_tick_);
    }
  else
    {
      ret_pos.add_ticks (
        snap_ticks - ticks_from_prev, AUDIO_ENGINE->frames_per_tick_);
    }

  return true;
}

zrythm::dsp::Position
SnapGrid::get_snapped_end_position (const Position &start_pos) const
{
  Position end_pos (start_pos);
  end_pos.add_ticks (get_default_ticks (), AUDIO_ENGINE->frames_per_tick_);
  return end_pos;
}

bool
SnapGrid::get_prev_or_next_snap_point (
  const Position                     &pivot_pos,
  Track *                             track,
  std::optional<ArrangerObject::Uuid> region,
  Position                           &out_pos,
  bool                                get_prev_point) const
{
  out_pos = pivot_pos;
  if (!pivot_pos.is_positive ())
    {
      /* negative not supported, set to same position */
      return false;
    }

  bool snapped = false;
  auto snap_point = pivot_pos;

  if (snap_to_grid_)
    {
      double snap_ticks = get_snap_ticks ();
      double ticks_from_prev = fmod (pivot_pos.ticks_, snap_ticks);
      if (get_prev_point)
        {
          snap_point.add_ticks (
            -ticks_from_prev, AUDIO_ENGINE->frames_per_tick_);
        }
      else
        {
          snap_point.add_ticks (
            snap_ticks - ticks_from_prev, AUDIO_ENGINE->frames_per_tick_);
        }
      out_pos = snap_point;
      snapped = true;
    }

  if (track != nullptr)
    {
// TODO
#if 0
      std::visit (
        [&] (auto &&t) {
          using TrackT = base_type<decltype (t)>;
          if constexpr (std::derived_from<TrackT, structure::tracks::LanedTrack>)
            {
              for (auto &lane_var : t->lanes_)
                {
                  using TrackLaneT = TrackT::LanedTrackImpl::TrackLaneType;
                  auto lane = std::get<TrackLaneT *> (lane_var);
                  for (auto * r : lane->get_children_view ())
                    {
                      snap_point = r->get_position ();
                      if (
                        get_prev_point
                          ? (snap_point <= pivot_pos && snap_point > out_pos)
                          : (snap_point > pivot_pos && snap_point < out_pos))
                        {
                          out_pos = snap_point;
                          snapped = true;
                        }
                      snap_point = r->get_end_position ();
                      if (
                        get_prev_point
                          ? (snap_point <= pivot_pos && snap_point > out_pos)
                          : (snap_point > pivot_pos && snap_point < out_pos))
                        {
                          out_pos = snap_point;
                          snapped = true;
                        }
                    }
                }
            }
        },
        convert_to_variant<structure::tracks::TrackPtrVariant> (track));
#endif
    }
  else if (region.has_value ())
    {
      /* TODO */
    }

  /* if no point to snap to, set to same position */
  if (!snapped)
    {
      out_pos = pivot_pos;
    }

  return snapped;
}

bool
SnapGrid::get_prev_snap_point (
  const Position                     &pivot_pos,
  Track *                             track,
  std::optional<ArrangerObject::Uuid> region,
  Position                           &prev_sp) const
{
  return get_prev_or_next_snap_point (pivot_pos, track, region, prev_sp, true);
}

bool
SnapGrid::get_next_snap_point (
  const Position                     &pivot_pos,
  Track *                             track,
  std::optional<ArrangerObject::Uuid> region,
  Position                           &next_sp) const
{
  return get_prev_or_next_snap_point (pivot_pos, track, region, next_sp, false);
}

bool
SnapGrid::get_closest_snap_point (
  const Position                     &pivot_pos,
  Track *                             track,
  std::optional<ArrangerObject::Uuid> region,
  Position                           &closest_sp) const
{
  /* get closest snap point */
  Position prev_sp;
  Position next_sp;
  bool prev_snapped = get_prev_snap_point (pivot_pos, track, region, prev_sp);
  bool next_snapped = get_next_snap_point (pivot_pos, track, region, next_sp);
  if (prev_snapped && next_snapped)
    {
      closest_sp = pivot_pos.get_closest_position (prev_sp, next_sp);
      return true;
    }
  if (prev_snapped)
    {
      closest_sp = prev_sp;
      return true;
    }
  if (next_snapped)
    {
      closest_sp = next_sp;
      return true;
    }

  closest_sp = pivot_pos;
  return false;
}

SnapGrid::Position
SnapGrid::snap (
  const Position                     &pivot_pos,
  const Position *                    start_pos,
  Track *                             track,
  std::optional<ArrangerObject::Uuid> region) const
{
  /* this should only be called if snap is on. the check should be done before
   * calling */
  z_warn_if_fail (any_snap ());

  /* position must be positive - only global positions allowed */
  z_return_val_if_fail (pivot_pos.is_positive (), {});

  if (!snap_to_events_)
    {
      region.reset ();
      track = nullptr;
    }

  Position ret (pivot_pos);

  /* snap to grid with offset */
  if (snap_to_grid_keep_offset_)
    {
      /* get previous snap point from start pos */
      z_return_val_if_fail (start_pos, {});
      Position prev_sp_from_start_pos;
      get_prev_snap_point (*start_pos, track, region, prev_sp_from_start_pos);

      /* get diff from previous snap point */
      double ticks_delta = start_pos->ticks_ - prev_sp_from_start_pos.ticks_;

      /* add ticks and check the closest snap point */
      ret.add_ticks (-ticks_delta, AUDIO_ENGINE->frames_per_tick_);

      /* get closest snap point */
      Position closest_sp;
      bool     have_closest_sp =
        get_closest_snap_point (ret, track, region, closest_sp);
      if (have_closest_sp)
        {
          /* move to closest snap point */
          ret = closest_sp;
        }

      /* readd ticks */
      ret.add_ticks (ticks_delta, AUDIO_ENGINE->frames_per_tick_);
    }
  /* else if snap to grid without offset */
  else
    {
      /* get closest snap point */
      Position closest_sp;
      get_closest_snap_point (ret, track, region, closest_sp);

      /* move to closest snap point */
      ret = closest_sp;
    }

  return ret;
}

}; // namespace zrythm::gui
