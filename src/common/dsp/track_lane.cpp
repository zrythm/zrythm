// SPDX-FileCopyrightText: © 2019-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "common/dsp/midi_event.h"
#include "common/dsp/track_lane.h"
#include "common/dsp/tracklist.h"
#include "common/utils/rt_thread_id.h"
#include "gui/backend/backend/actions/tracklist_selections.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"

#include "midilib/src/midifile.h"
#include "midilib/src/midiinfo.h"
#include <fmt/printf.h>

TrackLane::TrackLane (int pos, std::string name)
    : pos_ (pos), name_ (std::move (name))
{
}

template <typename RegionT>
TrackLaneImpl<RegionT>::TrackLaneImpl (LanedTrackT * track, int pos)
    : TrackLane (pos, format_str (QObject::tr ("Lane {}").toStdString (), pos + 1)),
      track_ (track)
{
}

template <typename RegionT>
void
TrackLaneImpl<RegionT>::init_loaded (LanedTrackT * track)
{
  track_ = track;
  for (auto &region_var : this->region_list_->regions_)
    {
      auto region = std::get<RegionT *> (region_var);
      region->set_lane (*dynamic_cast<TrackLaneT *> (this));
      region->init_loaded ();
    }
}

template <typename RegionT>
void
TrackLaneImpl<RegionT>::rename_with_action (const std::string &new_name)
{
  rename (new_name, true);
}

template <typename RegionT>
void
TrackLaneImpl<RegionT>::unselect_all ()
{
  for (auto &region_var : this->region_list_->regions_)
    {
      auto region = std::get<RegionT *> (region_var);
      region->select (false, false, false);
    }
}

template <typename RegionT>
void
TrackLaneImpl<RegionT>::rename (const std::string &new_name, bool with_action)
{
  if (with_action)
    {
      try
        {
          UNDO_MANAGER->perform (
            std::make_unique<RenameTrackLaneAction> (*this, new_name));
        }
      catch (const ZrythmException &e)
        {
          e.handle (QObject::tr ("Failed to rename lane"));
          return;
        }
      // EVENTS_PUSH (EventType::ET_TRACK_LANES_VISIBILITY_CHANGED, nullptr);
    }
  else
    {
      name_ = new_name;
    }
}

template <typename RegionT>
void
TrackLaneImpl<
  RegionT>::set_soloed (bool solo, bool trigger_undo, bool fire_events)
{
  if (trigger_undo)
    {
      try
        {
          UNDO_MANAGER->perform (
            std::make_unique<SoloTrackLaneAction> (*this, solo));
        }
      catch (const ZrythmException &e)
        {
          e.handle (QObject::tr ("Cannot set track lane soloed"));
          return;
        }
    }
  else
    {
      z_debug ("setting lane '{}' soloed to {}", name_, solo);
      solo_ = solo;
    }

  if (fire_events)
    {
      /* TODO use more specific event */
      // EVENTS_PUSH (EventType::ET_TRACK_LANES_VISIBILITY_CHANGED, nullptr);
    }
}

template <typename RegionT>
void
TrackLaneImpl<RegionT>::set_muted (bool mute, bool trigger_undo, bool fire_events)
{
  if (trigger_undo)
    {
      try
        {
          UNDO_MANAGER->perform (
            std::make_unique<MuteTrackLaneAction> (*this, mute));
        }
      catch (const ZrythmException &e)
        {
          e.handle (QObject::tr ("Could not mute/unmute track lane"));
          return;
        }
    }
  else
    {
      z_debug ("setting lane '{}' muted to {}", name_, mute);
      mute_ = mute;
    }

  if (fire_events)
    {
      /* TODO use more specific event */
      // EVENTS_PUSH (EventType::ET_TRACK_LANES_VISIBILITY_CHANGED, nullptr);
    }
}

template <typename RegionT>
void
TrackLaneImpl<RegionT>::after_remove_region ()
{
  auto track = get_track ();
  z_return_if_fail (track);
  if (
    !RegionOwnerImpl<RegionT>::clearing_
    && !track->block_auto_creation_and_deletion_)
    {
      track->remove_empty_last_lanes ();
    }
}

template <typename RegionT>
bool
TrackLaneImpl<RegionT>::is_effectively_muted () const
{
  if (get_muted ())
    return true;

  /* if lane is non-soloed while other soloed lanes exist, this should
   * be muted */
  auto track = get_track ();
  z_return_val_if_fail (track, true);
  if (track->has_soloed_lanes () && !get_soloed ())
    return true;

  return false;
}

template <typename RegionT>
bool
TrackLaneImpl<RegionT>::is_in_active_project () const
{
  return track_ != nullptr && track_->is_in_active_project ();
}

template <typename RegionT>
void
TrackLaneImpl<RegionT>::update_track_name_hash ()
{
  z_return_if_fail (track_);

  for (auto &region_var : this->region_list_->regions_)
    {
      auto region = std::get<RegionT *> (region_var);
      region->id_.track_name_hash_ = track_->get_name_hash ();
      region->id_.lane_pos_ = pos_;
      region->update_identifier ();
    }
}

template <typename RegionT>
bool
TrackLaneImpl<RegionT>::is_auditioner () const
{
  return track_ && track_->is_auditioner ();
}

template <typename RegionT>
Tracklist *
TrackLaneImpl<RegionT>::get_tracklist () const
{
  if (is_auditioner ())
    return SAMPLE_PROCESSOR->tracklist_.get ();
  return TRACKLIST;
}

/**
 * Calculates a unique index for this lane.
 */
template <typename RegionT>
int
TrackLaneImpl<RegionT>::calculate_lane_idx () const
{
  auto        track = get_track ();
  Tracklist * tracklist = get_tracklist ();
  int         pos = 1;
  for (auto cur_track : tracklist->tracks_ | type_is<LanedTrackT> ())
    {
      if (cur_track == track)
        {
          pos += pos_;
          break;
        }

      pos += static_cast<int> (cur_track->lanes_.size ());
    }

  return pos;
}

template <typename RegionT>
void
TrackLaneImpl<RegionT>::write_to_midi_file (
  MIDI_FILE *       mf,
  MidiEventVector * events,
  const Position *  start,
  const Position *  end,
  bool              lanes_as_tracks,
  bool              use_track_or_lane_pos)
  requires std::derived_from<MidiRegion, RegionT>
{
  auto track = get_track ();
  z_return_if_fail (track);
  int                              midi_track_pos = track->pos_;
  std::unique_ptr<MidiEventVector> own_events;
  if (lanes_as_tracks)
    {
      z_return_if_fail (!events);
      midi_track_pos = calculate_lane_idx ();
      own_events = std::make_unique<MidiEventVector> ();
    }
  else if (!use_track_or_lane_pos)
    {
      z_return_if_fail (events);
      midi_track_pos = 1;
    }
  /* else if using track positions */
  else
    {
      z_return_if_fail (events);
    }

  /* All data is written out to tracks not channels. We therefore set the
   * current channel before writing data out. Channel assignments can change any
   * number of times during the file, and affect all tracks messages until it is
   * changed. */
  midiFileSetTracksDefaultChannel (mf, midi_track_pos, MIDI_CHANNEL_1);

  /* add track name */
  if (lanes_as_tracks && use_track_or_lane_pos)
    {
      char midi_track_name[1000];
      sprintf (
        midi_track_name, "%s - %s", track->name_.c_str (), name_.c_str ());
      midiTrackAddText (mf, midi_track_pos, textTrackName, midi_track_name);
    }

  for (auto &region_var : this->region_list_->regions_)
    {
      auto region = std::get<RegionT *> (region_var);
      /* skip regions not inside the given range */
      if (start)
        {
          if (*region->end_pos_ < *start)
            continue;
        }
      if (end)
        {
          if (*region->pos_ > *end)
            continue;
        }

      region->add_events (
        own_events ? *own_events : *events, start, end, true, true);
    }

  if (own_events)
    {
      own_events->write_to_midi_file (mf, midi_track_pos);
    }
}

template <typename RegionT>
void
TrackLaneImpl<RegionT>::copy_members_from (const TrackLaneImpl &other)
{
  pos_ = other.pos_;
  name_ = other.name_;
  // y_ = other.y_;
  height_ = other.height_;
  mute_ = other.mute_;
  solo_ = other.solo_;
  for (auto &region_var : this->region_list_->regions_)
    {
      auto region = std::get<RegionT *> (region_var);
      region->is_auditioner_ = is_auditioner ();
      region->owner_lane_ = dynamic_cast<TrackLaneT *> (this);
      region->gen_name (region->name_.c_str (), nullptr, nullptr);
    }
}

template <typename RegionT>
std::unique_ptr<typename TrackLaneImpl<RegionT>::TrackLaneT>
TrackLaneImpl<RegionT>::gen_snapshot () const
{
  auto ret = dynamic_cast<const TrackLaneT *> (this)->clone_unique ();
  ret->track_ = track_;

  /* clone_unique above creates the regions in `regions_` but we want them in
   * `region_snapshots_`... */
  for (auto &region_var : this->region_list_->regions_)
    {
      auto region = std::get<RegionT *> (region_var);
      ret->region_snapshots_.emplace_back (region->clone_unique ());
    }
  ret->region_list_->clear ();
  return ret;
}

template class TrackLaneImpl<MidiRegion>;
template class TrackLaneImpl<AudioRegion>;