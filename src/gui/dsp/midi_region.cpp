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

#include <algorithm>

#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"
#include "gui/backend/io/midi_file.h"
#include "gui/dsp/midi_event.h"
#include "gui/dsp/midi_note.h"
#include "gui/dsp/midi_region.h"
#include "gui/dsp/piano_roll_track.h"
#include "gui/dsp/region.h"
#include "gui/dsp/tempo_track.h"
#include "gui/dsp/tracklist.h"
#include "utils/logger.h"
#include "utils/math.h"

#include "midilib/src/midifile.h"
#include "midilib/src/midiutil.h"

MidiRegion::MidiRegion (QObject * parent)
    : ArrangerObject (Type::Region), QAbstractListModel (parent)
{
  id_.type_ = RegionType::Midi;
  ArrangerObject::parent_base_qproperties (*this);
  BoundedObject::parent_base_qproperties (*this);
  init_colored_object ();
  unended_notes_.reserve (12000);
}

MidiRegion::MidiRegion (
  const Position                &start_pos,
  const Position                &end_pos,
  dsp::PortIdentifier::TrackUuid track_uuid,
  int                            lane_pos,
  int                            idx_inside_lane,
  QObject *                      parent)
    : MidiRegion (parent)
{
  init (
    start_pos, end_pos, track_uuid, lane_pos, idx_inside_lane,
    AUDIO_ENGINE->ticks_per_frame_);
}
MidiRegion::MidiRegion (
  const Position                &pos,
  ChordDescriptor               &descr,
  dsp::PortIdentifier::TrackUuid track_uuid,
  int                            lane_pos,
  int                            idx_inside_lane,
  QObject *                      parent)
    : MidiRegion (parent)
{
  int r_length_ticks = SNAP_GRID_TIMELINE->get_default_ticks ();
  int mn_length_ticks = SNAP_GRID_EDITOR->get_default_ticks ();

  /* get region end pos */
  Position r_end_pos = pos;
  r_end_pos.add_ticks (r_length_ticks, AUDIO_ENGINE->frames_per_tick_);

  /* create region */
  init (
    pos, r_end_pos, track_uuid, lane_pos, idx_inside_lane,
    AUDIO_ENGINE->ticks_per_frame_);

  /* get midi note positions */
  Position mn_pos, mn_end_pos;
  mn_end_pos.add_ticks (mn_length_ticks, AUDIO_ENGINE->frames_per_tick_);

  /* create midi notes */
  for (size_t i = 0; i < ChordDescriptor::MAX_NOTES; i++)
    {
      if (descr.notes_[i])
        {
          append_object (
            new MidiNote (
              id_, mn_pos, mn_end_pos, i + 36, VELOCITY_DEFAULT, this),
            false);
        }
    }
}

MidiRegion::MidiRegion (
  const Position                &start_pos,
  const std::string             &abs_path,
  dsp::PortIdentifier::TrackUuid track_uuid,
  int                            lane_pos,
  int                            idx_inside_lane,
  int                            midi_track_idx,
  QObject *                      parent)
    : MidiRegion (parent)
{
  z_debug ("reading from {}...", abs_path);

  Position end_pos = start_pos;
  end_pos.add_ticks (1, AUDIO_ENGINE->frames_per_tick_);
  init (
    start_pos, end_pos, track_uuid, lane_pos, idx_inside_lane,
    AUDIO_ENGINE->ticks_per_frame_);

  MidiFile mf (abs_path);
  mf.into_region (*this, *TRANSPORT, midi_track_idx);

  if (*pos_ >= end_pos_)
    {
      throw ZrythmException (
        fmt::format ("Invalid positions: start {} end {}", *pos_, *end_pos_));
    }

  z_info ("done ~ {} MIDI notes read", midi_notes_.size ());
}

void
MidiRegion::init_loaded ()
{
  ArrangerObject::init_loaded_base ();
  NamedObject::init_loaded_base ();
  for (auto &note : midi_notes_)
    {
      note->init_loaded ();
    }
}

void
MidiRegion::init_after_cloning (
  const MidiRegion &other,
  ObjectCloneType   clone_type)
{
  midi_notes_.reserve (other.midi_notes_.size ());
  for (const auto &note : other.midi_notes_)
    {
      auto * clone = note->clone_raw_ptr ();
      clone->setParent (this);
      midi_notes_.push_back (clone);
    }
  LaneOwnedObjectImpl::copy_members_from (other, clone_type);
  Region::copy_members_from (other, clone_type);
  TimelineObject::copy_members_from (other, clone_type);
  NamedObject::copy_members_from (other, clone_type);
  LoopableObject::copy_members_from (other, clone_type);
  MuteableObject::copy_members_from (other, clone_type);
  BoundedObject::copy_members_from (other, clone_type);
  ColoredObject::copy_members_from (other, clone_type);
  ArrangerObject::copy_members_from (other, clone_type);
}

// ========================================================================
// QML Interface
// ========================================================================

QHash<int, QByteArray>
MidiRegion::roleNames () const
{
  QHash<int, QByteArray> roles;
  roles[MidiNotePtrRole] = "midiNote";
  return roles;
}
int
MidiRegion::rowCount (const QModelIndex &parent) const
{
  return static_cast<int> (midi_notes_.size ());
}

QVariant
MidiRegion::data (const QModelIndex &index, int role) const
{
  if (index.row () < 0 || index.row () >= static_cast<int> (midi_notes_.size ()))
    return {};

  if (role == MidiNotePtrRole)
    {
      return QVariant::fromValue (midi_notes_.at (index.row ()));
    }
  return {};
}

// ========================================================================

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
    [pitch] (MidiNote * mn) { return pitch == -1 || mn->pitch_ == pitch; });

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
  return midi_notes_.empty () ? nullptr : midi_notes_.front ();
}

MidiNote *
MidiRegion::get_last_midi_note ()
{
  return (*std::ranges::max_element (midi_notes_,{},  &MidiNote::end_pos_));
}

MidiNote *
MidiRegion::get_highest_midi_note ()
{
  return (
    *std::ranges::max_element (midi_notes_, [] (const auto &a, const auto &b) {
      return a->pitch_ < b->pitch_;
    }));
}

MidiNote *
MidiRegion::get_lowest_midi_note ()
{
  return (
    *std::ranges::min_element (midi_notes_, [] (const auto &a, const auto &b) {
      return a->pitch_ < b->pitch_;
    }));
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
      end_pos.add_ticks (1, AUDIO_ENGINE->frames_per_tick_);
    }

  auto * mn = new MidiNote (id_, *start_pos, end_pos, pitch, vel, this);

  append_object (mn, pub_events);

  /* add to unended notes */
  unended_notes_.push_back (mn);
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

      midiFileSetPPQN (mf, MidiRegion::Position::TICKS_PER_QUARTER_NOTE);

      midiFileSetVersion (mf, midi_version);

      /* common time: 4 crochet beats, per bar */
      int beats_per_bar = P_TEMPO_TRACK->get_beats_per_bar ();
      midiSongAddSimpleTimeSig (
        mf, 1, beats_per_bar, TRANSPORT->ticks_per_beat_);

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
    !midi_note.pos_->is_between_excl_2nd (loop_start_pos_, loop_end_pos_)
    && !midi_note.pos_->is_between_excl_2nd (clip_start_pos_, loop_start_pos_))
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
  Position export_end_pos{
    get_length_in_ticks (), AUDIO_ENGINE->frames_per_tick_
  };

  *end_pos = Position::get_min (loop_end_pos_, *end_pos);

  if (*start_pos < clip_start_pos_)
    {
      ++repeat_index;
    }

  start_pos->add_ticks (
    loop_length_in_ticks * repeat_index - clip_start_pos_.ticks_,
    AUDIO_ENGINE->frames_per_tick_);
  end_pos->add_ticks (
    loop_length_in_ticks * repeat_index - clip_start_pos_.ticks_,
    AUDIO_ENGINE->frames_per_tick_);
  *start_pos = Position::get_max (*start_pos, export_start_pos);
  *end_pos = Position::get_min (*end_pos, export_end_pos);
}

bool
MidiRegion::is_note_export_start_pos_in_full_region (
  const Position start_pos) const
{
  Position export_start_pos;
  Position export_end_pos{
    get_length_in_ticks (), AUDIO_ENGINE->frames_per_tick_
  };
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
    region_start = pos_->ticks_;

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
          Position mn_pos = *static_cast<Position *> (mn->pos_);
          Position mn_end_pos = *mn->end_pos_;

          if (full)
            {
              if (mn->pos_->is_between_excl_2nd (loop_start_pos_, loop_end_pos_))
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
                1, mn->pitch_, mn->vel_->vel_,
                (midi_time_t) (note_global_start_ticks));
              events.add_note_off (
                1, mn->pitch_, (midi_time_t) (note_global_end_ticks));
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
          velocities.push_back (mn->vel_);
        }
      else if (
        !inside
        && ((global_start_pos < *start_pos) || global_start_pos > *end_pos))
        {
          velocities.push_back (mn->vel_);
        }
    }
}

bool
MidiRegion::validate (bool is_project, double frames_per_tick) const
{
  if (
    !Region::are_members_valid (is_project)
    || !TimelineObject::are_members_valid (is_project)
    || !NamedObject::are_members_valid (is_project)
    || !LoopableObject::are_members_valid (is_project)
    || !MuteableObject::are_members_valid (is_project)
    || !BoundedObject::are_members_valid (is_project)
    || !ColoredObject::are_members_valid (is_project)
    || !ArrangerObject::are_members_valid (is_project))
    {
      return false;
    }
  return true;
}
