// SPDX-FileCopyrightText: © 2019-2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <ranges>

#include "common/dsp/engine.h"
#include "common/dsp/foldable_track.h"
#include "common/dsp/master_track.h"
#include "common/dsp/position.h"
#include "common/dsp/recordable_track.h"
#include "common/dsp/track.h"
#include "common/dsp/tracklist.h"
#include "common/dsp/transport.h"
#include "common/utils/rt_thread_id.h"
#include "gui/backend/backend/actions/tracklist_selections.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/settings_manager.h"
#include "gui/backend/backend/tracklist_selections.h"
#include "gui/backend/backend/zrythm.h"

using namespace zrythm;

TrackPtrVariant
SimpleTracklistSelections::get_highest_track () const
{
  auto it =
    std::ranges::min_element (track_names_, {}, [this] (const auto &name) {
      auto track = tracklist_->find_track_by_name (name);
      return std::visit ([] (const auto &t) { return t->pos_; }, *track);
    });

  return *tracklist_->find_track_by_name (*it);
}

TrackPtrVariant
SimpleTracklistSelections::get_lowest_track () const
{
  auto it =
    std::ranges::max_element (track_names_, {}, [this] (const auto &name) {
      auto track = tracklist_->find_track_by_name (name);
      return std::visit ([] (const auto &t) { return t->pos_; }, *track);
    });

  return *tracklist_->find_track_by_name (*it);
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
SimpleTracklistSelections::add_track (Track &tr, bool fire_events)
{
  std::visit (
    [&] (auto &&track) {
      using TrackT = base_type<decltype (track)>;
      if (!contains_track (*track))
        {
          track_names_.push_back (track->name_);

          if (fire_events)
            {
              Q_EMIT track->selectedChanged (true);
              /* EVENTS_PUSH (EventType::ET_TRACK_CHANGED, &track); */
              /* EVENTS_PUSH (EventType::ET_TRACKLIST_SELECTIONS_CHANGED,
               * nullptr); */
            }
        }

      if constexpr (std::derived_from<TrackT, RecordableTrack>)
        {
          z_debug (
            "{} currently recording: {}, have channel: {}", track->name_,
            track->get_recording (), track->has_channel ());

          /* if recording is not already on, auto-arm */
          if (
            ZRYTHM_HAVE_UI
            && zrythm::gui::SettingsManager::get_instance ()->get_trackAutoArm ()
            && !track->get_recording () && track->has_channel ())
            {
              track->set_recording (true, fire_events);
              track->record_set_automatically_ = true;
            }
        }
    },
    convert_to_variant<TrackPtrVariant> (&tr));
}

void
SimpleTracklistSelections::
  add_tracks_in_range (int min_pos, int max_pos, bool fire_events)
{
  z_debug ("selecting tracks from {} to {}...", min_pos, max_pos);

  clear (fire_events);

  for (int i = min_pos; i <= max_pos; i++)
    {
      Track * track = Track::from_variant (tracklist_->get_track (i));
      add_track (*track, fire_events);
    }

  z_debug ("done");
}

void
SimpleTracklistSelections::clear (const bool fire_events)
{
  z_debug ("clearing tracklist selections...");

  z_return_if_fail (tracklist_);
  for (auto &track_name : std::ranges::reverse_view (track_names_))
    {
      auto track = tracklist_->find_track_by_name (track_name);
      z_return_if_fail (track);

      std::visit (
        [&] (auto &&tr) {
          remove_track (*tr, fire_events);

          if (tr->is_in_active_project ())
            {
#if 0
          /* process now because the track might get deleted after this */
          if (track->widget_ && GTK_IS_WIDGET (track->widget_))
            {
              gtk_widget_set_visible (
                GTK_WIDGET (track->widget_), track->visible_);
            }
#endif
            }
        },
        *track);
    }

  if (fire_events && ZRYTHM_HAVE_UI && PROJECT->loaded_)
    {
      /* EVENTS_PUSH (EventType::ET_TRACKLIST_SELECTIONS_CHANGED, nullptr); */
    }

  z_debug ("done clearing tracklist selections");
}

void
SimpleTracklistSelections::select_foldable_children ()
{
  int num_tracklist_sel = track_names_.size ();
  for (int i = 0; i < num_tracklist_sel; i++)
    {
      auto cur_track = tracklist_->find_track_by_name (track_names_[i]);
      z_return_if_fail (cur_track);
      std::visit (
        [&] (auto &&cur_t) {
          using TrackT = base_type<decltype (cur_t)>;
          if constexpr (std::derived_from<TrackT, FoldableTrack>)
            {
              for (int j = 1; j < cur_t->size_; ++j)
                {
                  Track * child_track = Track::from_variant (
                    TRACKLIST->get_track (j + cur_t->pos_));
                  if (!contains_track (*child_track))
                    {
                      child_track->select (true, false, false);
                    }
                }
            }
        },
        *cur_track);
    }
}

void
SimpleTracklistSelections::sort ()
{
  std::sort (
    track_names_.begin (), track_names_.end (),
    [&] (const auto &a, const auto &b) {
      auto track_a_var = tracklist_->find_track_by_name (a);
      auto track_b_var = tracklist_->find_track_by_name (b);
      return std::visit (
        [&] (auto &&track_a, auto &&track_b) {
          return track_a->pos_ < track_b->pos_;
        },
        *track_a_var, *track_b_var);
    });
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
              auto highest_var = get_highest_track ();
              auto lowest_var = get_lowest_track ();
              std::visit (
                [&] (auto &&highest, auto &&lowest) {
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
                },
                highest_var, lowest_var);
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
  for (auto &tr : tracklist_->tracks_)
    {
      std::visit (
        [&] (auto &&track) {
          if (track->visible_ || !visible_only)
            {
              add_track (*track, false);
            }
        },
        tr);
    }

  /* EVENTS_PUSH (EventType::ET_TRACKLIST_SELECTIONS_CHANGED, nullptr); */
}

void
SimpleTracklistSelections::remove_track (Track &track, int fire_events)
{
  std::visit (
    [&] (auto &&tr) {
      using TrackT = base_type<decltype (tr)>;
      auto it =
        std::find (track_names_.begin (), track_names_.end (), track.name_);
      if (it == track_names_.end ())
        {
          if (fire_events)
            {
              /* EVENTS_PUSH (EventType::ET_TRACK_CHANGED, &track); */
              /* EVENTS_PUSH (EventType::ET_TRACKLIST_SELECTIONS_CHANGED,
               * nullptr); */
            }
          return;
        }

      /* if record mode was set automatically when the track was selected, turn
       * record off - unless currently recording */
      if constexpr (std::derived_from<TrackT, RecordableTrack>)
        {
          if (
            tr->record_set_automatically_
            && !(TRANSPORT->is_recording () && TRANSPORT->isRolling ()))
            {
              tr->set_recording (false, fire_events);
              tr->record_set_automatically_ = false;
            }
        }

      track_names_.erase (it);

      if (fire_events)
        {
          Q_EMIT tr->selectedChanged (false);
          /* EVENTS_PUSH (EventType::ET_TRACKLIST_SELECTIONS_CHANGED, nullptr);
           */
        }
    },
    convert_to_variant<TrackPtrVariant> (&track));
}
bool
SimpleTracklistSelections::contains_uninstantiated_plugin () const
{
  // collect all tracks
  std::vector<std::optional<TrackPtrVariant>> track_ptrs;
  for (const auto &track_name : track_names_)
    {
      track_ptrs.push_back (tracklist_->find_track_by_name (track_name));
    }

  return std::any_of (
    track_ptrs.begin (), track_ptrs.end (), [] (const auto &track_ptr) {
      return track_ptr
             && std::visit (
               [&] (auto &&tr) { return tr->contains_uninstantiated_plugin (); },
               *track_ptr);
    });
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
      if (track)
        {
          return predicate (
            *dynamic_cast<DerivedTrackType *> (Track::from_variant (*track)));
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
  clear (fire_events);
  add_track (track, fire_events);

  if (fire_events)
    {
      /* EVENTS_PUSH (EventType::ET_TRACKLIST_SELECTIONS_CHANGED, nullptr); */
    }
}

void
SimpleTracklistSelections::select_last_visible ()
{
  auto track = tracklist_->get_last_track (Tracklist::PinOption::Both, true);
  z_return_if_fail (track);
  select_single (*Track::from_variant (*track), true);
}

void
TracklistSelections::get_plugins (std::vector<zrythm::plugins::Plugin *> &arr)
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
          std::visit ([&] (auto &&tr) { tr->visible_ = !tr->visible_; }, *track);
        }
    }

  /* EVENTS_PUSH (EventType::ET_TRACK_VISIBILITY_CHANGED, nullptr); */
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
          std::visit (
            [&] (auto &&tr) {
              if (!with_parents)
                {
                  tr->bounce_to_master_ = true;
                }
              tr->mark_for_bounce (true, true, true, with_parents);
            },
            *track);
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
        [&] (auto &&t) { ret->add_track (t->clone_unique ()); }, *track);
    }
  return ret;
}

void
TracklistSelections::init_after_cloning (const TracklistSelections &other)
{
  for (const auto &track : other.tracks_)
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
      e.handle (QObject::tr ("Failed to paste tracks"));
    }
}