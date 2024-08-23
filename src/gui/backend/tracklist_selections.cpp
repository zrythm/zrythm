// SPDX-FileCopyrightText: © 2019-2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "actions/tracklist_selections.h"
#include "dsp/engine.h"
#include "dsp/foldable_track.h"
#include "dsp/master_track.h"
#include "dsp/position.h"
#include "dsp/recordable_track.h"
#include "dsp/track.h"
#include "dsp/tracklist.h"
#include "dsp/transport.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/backend/tracklist_selections.h"
#include "gui/widgets/track.h"
#include "project.h"
#include "settings/g_settings_manager.h"
#include "utils/rt_thread_id.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

#include "gtk_wrapper.h"

Track *
SimpleTracklistSelections::get_highest_track () const
{
  int     min_pos = 1000;
  Track * min_track = nullptr;
  for (const auto &track_name : track_names_)
    {
      Track * track = Track::find_by_name (track_name);
      if (track && track->pos_ < min_pos)
        {
          min_pos = track->pos_;
          min_track = track;
        }
    }

  return min_track;
}

Track *
SimpleTracklistSelections::get_lowest_track () const
{
  int     max_pos = -1;
  Track * max_track = nullptr;
  for (const auto &track_name : track_names_)
    {
      Track * track = Track::find_by_name (track_name);
      if (track && track->pos_ > max_pos)
        {
          max_pos = track->pos_;
          max_track = track;
        }
    }

  return max_track;
}

Track *
TracklistSelections::get_lowest_track () const
{
  if (tracks_.empty ())
    {
      return nullptr;
    }

  return std::max_element (
           tracks_.begin (), tracks_.end (),
           [] (const auto &a, const auto &b) { return a->pos_ < b->pos_; })
    ->get ();
}

void
SimpleTracklistSelections::add_track (Track &track, bool fire_events)
{
  if (!contains_track (track))
    {
      track_names_.push_back (track.name_);

      if (fire_events)
        {
          EVENTS_PUSH (EventType::ET_TRACK_CHANGED, &track);
          EVENTS_PUSH (EventType::ET_TRACKLIST_SELECTIONS_CHANGED, nullptr);
        }
    }

  if (track.can_record ())
    {
      auto &rec_track = dynamic_cast<RecordableTrack &> (track);

      z_debug (
        "%s currently recording: %d, have channel: %d", track.name_,
        rec_track.get_recording (), track.has_channel ());

      /* if recording is not already on, auto-arm */
      if (
        ZRYTHM_HAVE_UI && g_settings_get_boolean (S_UI, "track-autoarm")
        && !rec_track.get_recording () && track.has_channel ())
        {
          rec_track.set_recording (true, fire_events);
          rec_track.record_set_automatically_ = true;
        }
    }
}

void
SimpleTracklistSelections::
  add_tracks_in_range (int min_pos, int max_pos, bool fire_events)
{
  z_info ("selecting tracks from {} to {}...", min_pos, max_pos);

  clear (fire_events);

  for (int i = min_pos; i <= max_pos; i++)
    {
      Track * track = tracklist_->get_track (i);
      add_track (*track, fire_events);
    }

  z_info ("done");
}

void
SimpleTracklistSelections::clear (const bool fire_events)
{
  z_info ("clearing tracklist selections...");

  z_return_if_fail (tracklist_);
  for (auto it = track_names_.rbegin (); it != track_names_.rend (); ++it)
    {
      Track * track = tracklist_->find_track_by_name (*it);
      remove_track (*track, false);

      if (track->is_in_active_project ())
        {
          /* process now because the track might get deleted after this */
          if (track->widget_ && GTK_IS_WIDGET (track->widget_))
            {
              gtk_widget_set_visible (
                GTK_WIDGET (track->widget_), track->visible_);
            }
        }
    }

  if (fire_events && ZRYTHM_HAVE_UI && PROJECT->loaded_)
    {
      EVENTS_PUSH (EventType::ET_TRACKLIST_SELECTIONS_CHANGED, nullptr);
    }

  z_info ("done");
}

void
SimpleTracklistSelections::select_foldable_children ()
{
  int num_tracklist_sel = track_names_.size ();
  for (int i = 0; i < num_tracklist_sel; i++)
    {
      Track * cur_track = tracklist_->find_track_by_name (track_names_[i]);
      if (cur_track->is_foldable ())
        {
          auto * foldable_track = dynamic_cast<FoldableTrack *> (cur_track);
          for (int j = 1; j < foldable_track->size_; ++j)
            {
              Track * child_track = TRACKLIST->get_track (j + cur_track->pos_);
              if (!contains_track (*child_track))
                {
                  child_track->select (true, false, false);
                }
            }
        }
    }
}

void
SimpleTracklistSelections::
  handle_click (Track &track, bool ctrl, bool shift, bool dragged)
{
  bool is_selected = contains_track (track);
  if (is_selected)
    {
      if ((ctrl || shift) && !dragged)
        {
          if (track_names_.size () > 1)
            {
              track.select (false, false, true);
            }
        }
      else
        {
          /* do nothing */
        }
    }
  else /* not selected */
    {
      if (shift)
        {
          if (track_names_.size () > 0)
            {
              Track * highest = get_highest_track ();
              Track * lowest = get_lowest_track ();
              z_return_if_fail (
                IS_TRACK_AND_NONNULL (highest) && IS_TRACK_AND_NONNULL (lowest));
              if (track.pos_ > highest->pos_)
                {
                  /* select all tracks in between */
                  add_tracks_in_range (highest->pos_, track.pos_, true);
                }
              else if (track.pos_ < lowest->pos_)
                {
                  /* select all tracks in between */
                  add_tracks_in_range (track.pos_, lowest->pos_, true);
                }
            }
        }
      else if (ctrl)
        {
          track.select (true, false, true);
        }
      else
        {
          track.select (true, true, true);
        }
    }
}

void
SimpleTracklistSelections::select_all (bool visible_only)
{
  for (auto &track : tracklist_->tracks_)
    {
      if (track->visible_ || !visible_only)
        {
          add_track (*track, false);
        }
    }

  EVENTS_PUSH (EventType::ET_TRACKLIST_SELECTIONS_CHANGED, nullptr);
}

void
SimpleTracklistSelections::remove_track (Track &track, int fire_events)
{
  auto it = std::find (track_names_.begin (), track_names_.end (), track.name_);
  if (it == track_names_.end ())
    {
      if (fire_events)
        {
          EVENTS_PUSH (EventType::ET_TRACK_CHANGED, &track);
          EVENTS_PUSH (EventType::ET_TRACKLIST_SELECTIONS_CHANGED, nullptr);
        }
      return;
    }

  /* if record mode was set automatically when the track was selected, turn
   * record off - unless currently recording */
  RecordableTrack * recordable_track =
    dynamic_cast<RecordableTrack *> (const_cast<Track *> (&track));
  if (
    recordable_track && recordable_track->record_set_automatically_
    && !(TRANSPORT->is_recording () && TRANSPORT->is_rolling ()))
    {
      recordable_track->set_recording (false, fire_events);
      recordable_track->record_set_automatically_ = false;
    }

  track_names_.erase (it);

  if (fire_events)
    {
      EVENTS_PUSH (EventType::ET_TRACKLIST_SELECTIONS_CHANGED, nullptr);
    }
}
bool
SimpleTracklistSelections::contains_uninstantiated_plugin () const
{
  for (auto &track_name : track_names_)
    {
      auto track = tracklist_->find_track_by_name (track_name);
      if (track->contains_uninstantiated_plugin ())
        {
          return true;
        }
    }

  return false;
}

template <typename DerivedTrackType, typename Predicate>
bool
TracklistSelections::contains_track_matching (Predicate predicate) const
{
  return std::any_of (
    tracks_.begin (), tracks_.end (), [&predicate] (const auto &track) {
      if (auto derived_track = dynamic_cast<DerivedTrackType *> (track.get ()))
        {
          return predicate (*derived_track);
        }
      return false;
    });
}

template <typename DerivedTrackType, typename Predicate>
bool
SimpleTracklistSelections::contains_track_matching (Predicate predicate) const
{
  return std::any_of (
    track_names_.begin (), track_names_.end (),
    [this, &predicate] (const auto &track_name) {
      auto track = tracklist_->find_track_by_name (track_name);
      if (auto derived_track = dynamic_cast<DerivedTrackType *> (track))
        {
          return predicate (*derived_track);
        }
      return false;
    });
}

template <typename T>
bool
contains_undeletable_track_impl (const T &selections)
{
  return selections.template contains_track_matching<ChannelTrack> (
    [] (const ChannelTrack &track) { return !track.is_copyable (); });
}

bool
TracklistSelections::contains_undeletable_track () const
{
  return contains_undeletable_track_impl (*this);
}

bool
SimpleTracklistSelections::contains_undeletable_track () const
{
  return contains_undeletable_track_impl (*this);
}

template <typename T>
bool
contains_uncopyable_track_impl (const T &selections)
{
  return selections.template contains_track_matching<ChannelTrack> (
    [] (const ChannelTrack &track) { return !track.is_copyable (); });
}

bool
TracklistSelections::contains_uncopyable_track () const
{
  return contains_uncopyable_track_impl (*this);
}

bool
SimpleTracklistSelections::contains_uncopyable_track () const
{
  return contains_uncopyable_track_impl (*this);
}

bool
SimpleTracklistSelections::contains_non_automatable_track () const
{
  return contains_track_matching<ChannelTrack> ([] (const ChannelTrack &track) {
    return !track.has_automation ();
  });
}

bool
SimpleTracklistSelections::contains_soloed_track (bool soloed) const
{
  return contains_track_matching<ChannelTrack> (
    [soloed] (const ChannelTrack &track) {
      return track.get_soloed () == soloed;
    });
}

bool
SimpleTracklistSelections::contains_muted_track (bool muted) const
{
  return contains_track_matching<ChannelTrack> (
    [muted] (const ChannelTrack &track) { return track.get_muted () == muted; });
}

bool
SimpleTracklistSelections::contains_listened_track (bool listened) const
{
  return contains_track_matching<ChannelTrack> (
    [listened] (const ChannelTrack &track) {
      return track.get_listened () == listened;
    });
}

bool
SimpleTracklistSelections::contains_enabled_track (bool enabled) const
{
  return contains_track_matching<ChannelTrack> (
    [enabled] (const ChannelTrack &track) {
      return track.is_enabled () == enabled;
    });
}
bool
SimpleTracklistSelections::contains_track (const Track &track)
{
  return std::find (track_names_.begin (), track_names_.end (), track.name_)
         != track_names_.end ();
}

bool
TracklistSelections::contains_track_index (int track_idx) const
{
  return std::any_of (
    tracks_.begin (), tracks_.end (),
    [track_idx] (const auto &track) { return track->pos_ == track_idx; });
}

void
SimpleTracklistSelections::select_single (Track &track, bool fire_events)
{
  clear (false);
  add_track (track, false);

  if (fire_events)
    {
      EVENTS_PUSH (EventType::ET_TRACKLIST_SELECTIONS_CHANGED, nullptr);
    }
}

void
SimpleTracklistSelections::select_last_visible ()
{
  Track * track = tracklist_->get_last_track (Tracklist::PinOption::Both, true);
  z_warn_if_fail (track);
  select_single (*track, true);
}

void
TracklistSelections::get_plugins (std::vector<Plugin *> &arr)
{
  for (auto &track : tracks_)
    {
      track->get_plugins (arr);
    }
}

void
SimpleTracklistSelections::toggle_visibility ()
{
  for (auto &track_name : track_names_)
    {
      auto track = tracklist_->find_track_by_name (track_name);
      if (track)
        {
          track->visible_ = !track->visible_;
        }
    }

  EVENTS_PUSH (EventType::ET_TRACK_VISIBILITY_CHANGED, nullptr);
}

void
SimpleTracklistSelections::paste_to_pos (int pos)
{
  gen_tracklist_selections ()->paste_to_pos (pos);
}

void
SimpleTracklistSelections::mark_for_bounce (bool with_parents, bool mark_master)
{
  AUDIO_ENGINE->reset_bounce_mode ();

  for (auto &track_name : track_names_)
    {
      auto track = tracklist_->find_track_by_name (track_name);
      if (track)
        {
          if (!with_parents)
            {
              track->bounce_to_master_ = true;
            }
          track->mark_for_bounce (true, true, true, with_parents);
        }
    }

  if (mark_master)
    {
      tracklist_->master_track_->mark_for_bounce (true, false, false, false);
    }
}

std::unique_ptr<TracklistSelections>
SimpleTracklistSelections::gen_tracklist_selections () const
{
  auto ret = std::make_unique<TracklistSelections> ();
  for (auto &track_name : track_names_)
    {
      auto track = tracklist_->find_track_by_name (track_name);
      if (!track)
        continue;

      std::visit (
        [&] (auto &&t) { ret->add_track (t->clone_unique ()); },
        convert_to_variant<TrackPtrVariant> (track));
    }
  return ret;
}

void
TracklistSelections::init_after_cloning (const TracklistSelections &other)
{
  for (auto &track : other.tracks_)
    {
      tracks_.emplace_back (
        clone_unique_with_variant<TrackVariant> (track.get ()));
    }
}

void
TracklistSelections::add_track (std::unique_ptr<Track> &&track)
{
  tracks_.emplace_back (std::move (track));
  sort ();
}

void
TracklistSelections::paste_to_pos (int pos)
{
  try
    {
      UNDO_MANAGER->perform (
        std::make_unique<CopyTracksAction> (*this, *PORT_CONNECTIONS_MGR, pos));
    }
  catch (const ZrythmException &e)
    {
      e.handle (_ ("Failed to paste tracks"));
    }
}