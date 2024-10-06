// SPDX-FileCopyrightText: © 2018-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-FileCopyrightText: © 2022 Robert Panovics <robert.panovics at gmail dot com>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
/*
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 * ---
 *
 *  AUTHOR: Steven Goodwin (StevenGoodwin@gmail.com)
 *      Copyright 2010, Steven Goodwin.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 3 of
 *  the License,or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  ---
 */

#include "common/dsp/midi_event.h"
#include "common/dsp/midi_note.h"
#include "common/dsp/midi_region.h"
#include "common/dsp/piano_roll_track.h"
#include "common/dsp/region.h"
#include "common/dsp/tempo_track.h"
#include "common/dsp/tracklist.h"
#include "common/io/midi_file.h"
#include "common/utils/logger.h"
#include "common/utils/math.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"
#include "gui/backend/gtk_widgets/arranger_wrapper.h"
#include "gui/backend/gtk_widgets/bot_dock_edge.h"
#include "gui/backend/gtk_widgets/center_dock.h"
#include "gui/backend/gtk_widgets/clip_editor.h"
#include "gui/backend/gtk_widgets/clip_editor_inner.h"
#include "gui/backend/gtk_widgets/midi_arranger.h"
#include "gui/backend/gtk_widgets/midi_editor_space.h"
#include "gui/backend/gtk_widgets/zrythm_app.h"

#include "midilib/src/midifile.h"
#include "midilib/src/midiutil.h"

MidiRegion::MidiRegion ()
{
  unended_notes_.reserve (12000);
}

MidiRegion::MidiRegion (
  const Position &start_pos,
  const Position &end_pos,
  unsigned int    track_name_hash,
  int             lane_pos,
  int             idx_inside_lane)
    : MidiRegion ()
{
  id_.type_ = RegionType::Midi;

  init (start_pos, end_pos, track_name_hash, lane_pos, idx_inside_lane);
}
MidiRegion::MidiRegion (
  const Position  &pos,
  ChordDescriptor &descr,
  unsigned int     track_name_hash,
  int              lane_pos,
  int              idx_inside_lane)
    : MidiRegion ()
{
  int r_length_ticks = SNAP_GRID_TIMELINE->get_default_ticks ();
  int mn_length_ticks = SNAP_GRID_EDITOR->get_default_ticks ();

  /* get region end pos */
  Position r_end_pos = pos;
  r_end_pos.add_ticks (r_length_ticks);

  /* create region */
  id_.type_ = RegionType::Midi;
  init (pos, r_end_pos, track_name_hash, lane_pos, idx_inside_lane);

  /* get midi note positions */
  Position mn_pos, mn_end_pos;
  mn_end_pos.add_ticks (mn_length_ticks);

  /* create midi notes */
  for (int i = 0; i < CHORD_DESCRIPTOR_MAX_NOTES; i++)
    {
      if (descr.notes_[i])
        {
          append_object (
            std::make_shared<MidiNote> (
              id_, mn_pos, mn_end_pos, i + 36, VELOCITY_DEFAULT),
            false);
        }
    }
}

void
MidiRegion::init_loaded ()
{
  ArrangerObject::init_loaded_base ();
  NameableObject::init_loaded_base ();
  for (auto &note : midi_notes_)
    {
      note->init_loaded ();
    }
}

void
MidiRegion::print_midi_notes () const
{
  for (const auto &mn : midi_notes_)
    {
      z_info ("Note");
      mn->print ();
    }
}

std::string
MidiRegion::print_to_str () const
{
  // TODO
  return "";
}

MidiNote *
MidiRegion::pop_unended_note (int pitch)
{
  auto it = std::find_if (
    unended_notes_.begin (), unended_notes_.end (),
    [pitch] (MidiNote * mn) { return pitch == -1 || mn->val_ == pitch; });

  if (it != unended_notes_.end ())
    {
      MidiNote * mn = *it;
      unended_notes_.erase (it);
      return mn;
    }

  return nullptr;
}

MidiNote *
MidiRegion::get_first_midi_note ()
{
  return midi_notes_.empty () ? nullptr : midi_notes_.front ().get ();
}

MidiNote *
MidiRegion::get_last_midi_note ()
{
  return (*std::max_element (
            midi_notes_.begin (), midi_notes_.end (),
            [] (const auto &a, const auto &b) {
              return a->end_pos_.ticks_ < b->end_pos_.ticks_;
            }))
    .get ();
}

MidiNote *
MidiRegion::get_highest_midi_note ()
{
  return (*std::max_element (
            midi_notes_.begin (), midi_notes_.end (),
            [] (const auto &a, const auto &b) { return a->val_ < b->val_; }))
    .get ();
}

MidiNote *
MidiRegion::get_lowest_midi_note ()
{
  return (*std::min_element (
            midi_notes_.begin (), midi_notes_.end (),
            [] (const auto &a, const auto &b) { return a->val_ < b->val_; }))
    .get ();
}

MidiRegion::MidiRegion (
  const Position    &start_pos,
  const std::string &abs_path,
  unsigned int       track_name_hash,
  int                lane_pos,
  int                idx_inside_lane,
  int                midi_track_idx)
{
  z_debug ("reading from {}...", abs_path);

  id_.type_ = RegionType::Midi;

  Position end_pos = start_pos;
  end_pos.add_ticks (1);
  init (start_pos, end_pos, track_name_hash, lane_pos, idx_inside_lane);

  MidiFile mf (abs_path);
  mf.into_region (*this, *TRANSPORT, midi_track_idx);

  if (pos_ >= end_pos_)
    {
      throw ZrythmException (fmt::format (
        "Invalid positions: start {} end {}", pos_.to_string (),
        end_pos_.to_string ()));
    }

  z_info ("done ~ {} MIDI notes read", midi_notes_.size ());
}

void
MidiRegion::start_unended_note (
  Position * start_pos,
  Position * _end_pos,
  int        pitch,
  int        vel,
  bool       pub_events)
{
  z_return_if_fail (start_pos);

  /* set end pos */
  Position end_pos;
  if (_end_pos)
    {
      end_pos = *_end_pos;
    }
  else
    {
      end_pos = *start_pos;
      end_pos.add_ticks (1);
    }

  auto mn = append_object (
    std::make_shared<MidiNote> (id_, *start_pos, end_pos, pitch, vel),
    pub_events);

  /* add to unended notes */
  unended_notes_.push_back (mn.get ());
}

void
MidiRegion::write_to_midi_file (
  MIDI_FILE * mf,
  const bool  add_region_start,
  bool        export_full) const
{
  MidiEventVector events;
  add_events (events, nullptr, nullptr, add_region_start, export_full);

  midiFileSetTracksDefaultChannel (mf, 1, MIDI_CHANNEL_1);
  midiTrackAddText (mf, 1, textTrackName, name_.c_str ());

  events.write_to_midi_file (mf, 1);
}

void
MidiRegion::export_to_midi_file (
  const std::string &full_path,
  int                midi_version,
  const bool         export_full) const
{
  MIDI_FILE * mf;

  if ((mf = midiFileCreate (full_path.c_str (), TRUE)))
    {
      /* Write tempo information out to track 1 */
      midiSongAddTempo (mf, 1, (int) P_TEMPO_TRACK->get_current_bpm ());

      /* All data is written out to _tracks_ not channels. We therefore set the
      current channel before writing data out. Channel assignments can change
      any number of times during the file, and affect all
      ** tracks messages until it is changed. */
      midiFileSetTracksDefaultChannel (mf, 1, MIDI_CHANNEL_1);

      midiFileSetPPQN (mf, TICKS_PER_QUARTER_NOTE);

      midiFileSetVersion (mf, midi_version);

      /* common time: 4 crochet beats, per bar */
      int beats_per_bar = P_TEMPO_TRACK->get_beats_per_bar ();
      midiSongAddSimpleTimeSig (
        mf, 1, beats_per_bar,
        math_round_double_to_signed_32 (TRANSPORT->ticks_per_beat_));

      write_to_midi_file (mf, false, export_full);

      midiFileClose (mf);
    }
}

uint8_t
MidiRegion::get_midi_ch () const
{
  uint8_t ret;
  auto    lane = get_lane ();
  z_return_val_if_fail (lane, 1);
  if (lane->midi_ch_ > 0)
    ret = lane->midi_ch_;
  else
    {
      auto track = lane->get_track ();
      z_return_val_if_fail (track, 1);
      auto piano_roll_track = dynamic_cast<PianoRollTrack *> (track);
      ret = piano_roll_track->midi_ch_;
    }

  z_return_val_if_fail (ret > 0, 1);

  return ret;
}

bool
MidiRegion::is_note_playable (const MidiNote &midi_note) const
{
  if (midi_note.get_muted (false))
    {
      return false;
    }

  if (
    !midi_note.pos_.is_between_excl_2nd (loop_start_pos_, loop_end_pos_)
    && !midi_note.pos_.is_between_excl_2nd (clip_start_pos_, loop_start_pos_))
    {
      return false;
    }

  return true;
}

void
MidiRegion::get_note_positions_in_export (
  Position * start_pos,
  Position * end_pos,
  int        repeat_index) const
{
  double   loop_length_in_ticks = get_loop_length_in_ticks ();
  Position export_start_pos;
  Position export_end_pos{ get_length_in_ticks () };

  *end_pos = Position::get_min (loop_end_pos_, *end_pos);

  if (*start_pos < clip_start_pos_)
    {
      ++repeat_index;
    }

  start_pos->add_ticks (
    loop_length_in_ticks * repeat_index - clip_start_pos_.ticks_);
  end_pos->add_ticks (
    loop_length_in_ticks * repeat_index - clip_start_pos_.ticks_);
  *start_pos = Position::get_max (*start_pos, export_start_pos);
  *end_pos = Position::get_min (*end_pos, export_end_pos);
}

bool
MidiRegion::is_note_export_start_pos_in_full_region (
  const Position start_pos) const
{
  Position export_start_pos;
  Position export_end_pos{ get_length_in_ticks () };
  return start_pos.is_between_excl_2nd (export_start_pos, export_end_pos);
}

void
MidiRegion::add_events (
  MidiEventVector &events,
  const Position * start,
  const Position * end,
  const bool       add_region_start,
  const bool       full) const
{
  double region_start = 0;
  if (add_region_start)
    region_start = pos_.ticks_;

  double loop_length_in_ticks = get_loop_length_in_ticks ();
  int    number_of_loop_repeats = (int) ceil (
    (get_length_in_ticks () - loop_start_pos_.ticks_ + clip_start_pos_.ticks_)
    / loop_length_in_ticks);

  for (auto &mn : midi_notes_)
    {
      if (full && !is_note_playable (*mn))
        {
          continue;
        }

      int  repeat_counter = 0;
      bool write_only_once = true;

      do
        {
          Position mn_pos = mn->pos_;
          Position mn_end_pos = mn->end_pos_;

          if (full)
            {
              if (mn->pos_.is_between_excl_2nd (loop_start_pos_, loop_end_pos_))
                {
                  write_only_once = false;
                }

              get_note_positions_in_export (
                &mn_pos, &mn_end_pos, repeat_counter);

              if (!is_note_export_start_pos_in_full_region (mn_pos))
                {
                  continue;
                }
            }

          double note_global_start_ticks = mn_pos.ticks_ + region_start;
          double note_global_end_ticks = mn_end_pos.ticks_ + region_start;
          bool   write_note = true;
          if (start && note_global_end_ticks < start->ticks_)
            write_note = false;
          if (end && note_global_start_ticks > end->ticks_)
            write_note = false;

          if (write_note)
            {
              if (start)
                {
                  note_global_start_ticks -= start->ticks_;
                  note_global_end_ticks -= start->ticks_;
                }
              events.add_note_on (
                1, mn->val_, mn->vel_->vel_,
                (midi_time_t) (note_global_start_ticks));
              events.add_note_off (
                1, mn->val_, (midi_time_t) (note_global_end_ticks));
            }
        }
      while (++repeat_counter < number_of_loop_repeats && !write_only_once);
    }

  events.sort ();
}

void
MidiRegion::get_velocities_in_range (
  const Position *         start_pos,
  const Position *         end_pos,
  std::vector<Velocity *> &velocities,
  bool                     inside)
{
  Position global_start_pos;
  for (auto &mn : midi_notes_)
    {
      mn->get_global_start_pos (global_start_pos);

      if (
        inside && global_start_pos >= *start_pos && global_start_pos <= *end_pos)
        {
          velocities.push_back (mn->vel_.get ());
        }
      else if (
        !inside
        && ((global_start_pos < *start_pos) || global_start_pos > *end_pos))
        {
          velocities.push_back (mn->vel_.get ());
        }
    }
}

bool
MidiRegion::validate (bool is_project, double frames_per_tick) const
{
  if (
    !Region::are_members_valid (is_project)
    || !TimelineObject::are_members_valid (is_project)
    || !NameableObject::are_members_valid (is_project)
    || !LoopableObject::are_members_valid (is_project)
    || !MuteableObject::are_members_valid (is_project)
    || !LengthableObject::are_members_valid (is_project)
    || !ColoredObject::are_members_valid (is_project)
    || !ArrangerObject::are_members_valid (is_project))
    {
      return false;
    }
  return true;
}

ArrangerSelections *
MidiRegion::get_arranger_selections () const
{
  return MIDI_SELECTIONS.get ();
}

ArrangerWidget *
MidiRegion::get_arranger_for_children () const
{
  return MW_MIDI_ARRANGER;
}