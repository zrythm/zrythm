// SPDX-FileCopyrightText: © 2018-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <ranges>

#include "common/dsp/channel.h"
#include "common/dsp/chord_track.h"
#include "common/dsp/foldable_track.h"
#include "common/dsp/marker_track.h"
#include "common/dsp/master_track.h"
#include "common/dsp/modulator_track.h"
#include "common/dsp/router.h"
#include "common/dsp/tempo_track.h"
#include "common/dsp/track.h"
#include "common/dsp/tracklist.h"
#include "common/io/file_import.h"
#include "common/utils/flags.h"
#include "common/utils/gtest_wrapper.h"
#include "common/utils/rt_thread_id.h"
#include "common/utils/string.h"
#include "gui/backend/backend/actions/arranger_selections.h"
#include "gui/backend/backend/actions/tracklist_selections.h"
#include "gui/backend/backend/project.h"

#include <glib/gi18n.h>

void
Tracklist::init_loaded (Project * project, SampleProcessor * sample_processor)
{
  project_ = project;
  sample_processor_ = sample_processor;

  z_debug ("initializing loaded Tracklist...");
  for (auto &track : tracks_)
    {
      if (track->is_chord ())
        chord_track_ = dynamic_cast<ChordTrack *> (track.get ());
      else if (track->is_marker ())
        marker_track_ = dynamic_cast<MarkerTrack *> (track.get ());
      else if (track->is_master ())
        master_track_ = dynamic_cast<MasterTrack *> (track.get ());
      else if (track->is_tempo ())
        tempo_track_ = dynamic_cast<TempoTrack *> (track.get ());
      else if (track->is_modulator ())
        modulator_track_ = dynamic_cast<ModulatorTrack *> (track.get ());
      track->tracklist_ = this;
      track->init_loaded ();
    }
}

void
Tracklist::select_all (bool select, bool fire_events)
{
  for (auto &track : tracks_)
    {
      track->select (select, false, fire_events);
    }

  if (!select)
    tracks_.back ()->select (true, true, fire_events);
}

void
Tracklist::get_visible_tracks (std::vector<Track *> visible_tracks) const
{
  for (const auto &track : TRACKLIST->tracks_)
    {
      if (track->should_be_visible ())
        visible_tracks.push_back (track.get ());
    }
}

int
Tracklist::get_visible_track_diff (const Track &src, const Track &dest) const
{
  int count = 0;
  if (src.pos_ < dest.pos_)
    {
      for (
        auto it = tracks_.begin () + src.pos_;
        it != tracks_.begin () + dest.pos_; ++it)
        {
          if ((*it)->should_be_visible ())
            {
              count++;
            }
        }
    }
  else if (src.pos_ > dest.pos_)
    {
      for (
        auto it = tracks_.begin () + dest.pos_;
        it != tracks_.begin () + src.pos_; ++it)
        {
          if ((*it)->should_be_visible ())
            {
              count--;
            }
        }
    }

  return count;
}

void
Tracklist::print_tracks () const
{
  z_info ("----- tracklist tracks ------");
  for (size_t i = 0; i < tracks_.size (); i++)
    {
      const auto &track = tracks_[i];
      if (track)
        {
          std::string                  parent_str;
          std::vector<FoldableTrack *> parents;
          track->add_folder_parents (parents, false);
          parent_str.append (parents.size () * 2, '-');
          if (!parents.empty ())
            parent_str += ' ';

          z_info (
            "[{:03}] {}{} (pos {}, parents {}, size {})", i, parent_str,
            track->name_, track->pos_, parents.size (),
            track->is_foldable ()
              ? dynamic_cast<FoldableTrack *> (track.get ())->size_
              : 1);
        }
      else
        {
          z_info ("[{:03}] (null)", i);
        }
    }
  z_info ("------ end ------");
}

void
Tracklist::swap_tracks (const size_t index1, const size_t index2)
{
  z_return_if_fail (std::max (index1, index2) < tracks_.size ());
  swapping_tracks_ = true;

  {
    const auto &src_track = tracks_[index1];
    const auto &dest_track = tracks_[index2];
    z_debug (
      "swapping tracks {} [{}] and {} [{}]...",
      src_track ? src_track->name_ : "(none)", index1,
      dest_track ? dest_track->name_ : "(none)", index2);
  }

  std::iter_swap (tracks_.begin () + index1, tracks_.begin () + index2);

  {
    const auto &src_track = tracks_[index1];
    const auto &dest_track = tracks_[index2];

    if (src_track)
      src_track->pos_ = index1;
    if (dest_track)
      dest_track->pos_ = index2;
  }

  swapping_tracks_ = false;
  z_debug ("tracks swapped");
}

template <FinalTrackSubclass T>
T *
Tracklist::insert_track (
  std::unique_ptr<T> &&track,
  int                  pos,
  bool                 publish_events,
  bool                 recalc_graph)
{
  z_info ("inserting {} at {}", track->name_, pos);

  /* throw error if attempted to add a special track (like master) when it
   * already exists */
  if (
    !Track::type_is_deletable (Track::get_type_for_class<T> ())
    && contains_track_type<T> ())
    {
      z_error (
        "cannot add track of type {} when it already exists",
        Track::get_type_for_class<T> ());
      return nullptr;
    }

  /* set to -1 so other logic knows it is a new track */
  track->pos_ = -1;
  if constexpr (std::derived_from<T, ChannelTrack>)
    {
      track->channel_->track_pos_ = -1;
    }

  /* this needs to be called before appending the track to the tracklist */
  track->set_name (track->name_, false);

  /* append the track at the end */
  auto added_track =
    dynamic_cast<T *> (tracks_.emplace_back (std::move (track)).get ());
  added_track->tracklist_ = this;

  /* remember important tracks */
  if constexpr (std::is_same_v<T, MasterTrack>)
    master_track_ = added_track;
  else if constexpr (std::is_same_v<T, ChordTrack>)
    chord_track_ = added_track;
  else if constexpr (std::is_same_v<T, MarkerTrack>)
    marker_track_ = added_track;
  else if constexpr (std::is_same_v<T, TempoTrack>)
    tempo_track_ = added_track;
  else if constexpr (std::is_same_v<T, ModulatorTrack>)
    modulator_track_ = added_track;

  /* add flags for auditioner track ports */
  if (is_auditioner ())
    {
      std::vector<Port *> ports;
      added_track->append_ports (ports, true);
      for (auto * port : ports)
        {
          port->id_.flags2_ |= PortIdentifier::Flags2::SampleProcessorTrack;
        }
    }

  /* if inserting it, swap until it reaches its position */
  if (static_cast<size_t> (pos) != tracks_.size () - 1)
    {
      for (int i = static_cast<int> (tracks_.size ()) - 1; i > pos; --i)
        {
          swap_tracks (i, i - 1);
        }
    }

  added_track->pos_ = pos;

  if (
    is_in_active_project ()
    /* auditioner doesn't need automation */
    && !is_auditioner ())
    {
      /* make the track the only selected track */
      TRACKLIST_SELECTIONS->select_single (*added_track, publish_events);

      /* set automation track on ports */
      if constexpr (std::derived_from<T, AutomatableTrack>)
        {
          const auto &atl = added_track->get_automation_tracklist ();
          for (const auto &at : atl.ats_)
            {
              auto port = Port::find_from_identifier<ControlPort> (at->port_id_);
              z_return_val_if_fail (port, nullptr);
              port->at_ = at.get ();
            }
        }
    }

  if constexpr (std::derived_from<T, ChannelTrack>)
    {
      added_track->channel_->connect ();
    }

  /* if audio output route to master */
  if constexpr (!std::is_same_v<T, MasterTrack>)
    {
      if (added_track->out_signal_type_ == PortType::Audio && master_track_)
        {
          master_track_->add_child (
            added_track->get_name_hash (), true, false, false);
        }
    }

  if (is_in_active_project ())
    {
      added_track->activate_all_plugins (true);
    }

  if (!is_auditioner ())
    {
      /* verify */
      z_return_val_if_fail (added_track->validate (), nullptr);
    }

  if (ZRYTHM_TESTING)
    {
      for (auto * cur_track : tracks_ | type_is<ChannelTrack> ())
        {
          auto ch = cur_track->channel_;
          if (ch->has_output_)
            {
              z_return_val_if_fail (
                ch->output_name_hash_ != cur_track->get_name_hash (), nullptr);
            }
        }
    }

  if (ZRYTHM_HAVE_UI && !is_auditioner ())
    {
      /* generate track widget */
      // added_track->widget_ = track_widget_new (added_track);
    }

  if (recalc_graph)
    {
      ROUTER->recalc_graph (false);
    }

  if (publish_events)
    {
      // EVENTS_PUSH (EventType::ET_TRACK_ADDED, added_track);
    }

  z_debug (
    "done - inserted track '{}' ({}) at {}", added_track->name_,
    added_track->get_name_hash (), pos);

  return added_track;
}

Track *
Tracklist::insert_track (
  std::unique_ptr<Track> &&track,
  int                      pos,
  bool                     publish_events,
  bool                     recalc_graph)
{
  return std::visit (
    [&] (auto &&t) {
      using T = base_type<decltype (t)>;
      auto track_unique_ptr = std::unique_ptr<T> (t);
      return dynamic_cast<Track *> (insert_track<T> (
        std::move (track_unique_ptr), pos, publish_events, recalc_graph));
    },
    convert_to_variant<TrackPtrVariant> (track.release ()));
}

Track *
Tracklist::append_track (
  std::unique_ptr<Track> &&track,
  bool                     publish_events,
  bool                     recalc_graph)
{
  return std::visit (
    [&] (auto &&t) {
      using T = base_type<decltype (t)>;
      auto track_unique_ptr = std::unique_ptr<T> (t);
      return dynamic_cast<Track *> (append_track<T> (
        std::move (track_unique_ptr), publish_events, recalc_graph));
    },
    convert_to_variant<TrackPtrVariant> (track.release ()));
}

ChordTrack *
Tracklist::get_chord_track () const
{
  return get_track_by_type<ChordTrack> (Track::Type::Chord);
}

template <typename T>
T *
Tracklist::find_track_by_name_hash (unsigned int hash) const
{
  // z_trace ("called for {}", hash);
  static_assert (TrackSubclass<T>);
  if (
    is_in_active_project () && ROUTER && ROUTER->is_processing_thread ()
    && !is_auditioner ()) [[likely]]
    {
      auto found = std::ranges::find_if (tracks_, [hash] (const auto &track) {
        return track->name_hash_ == hash;
      });
      if (found != tracks_.end ())
        return dynamic_cast<T *> (found->get ());

      return nullptr;
    }
  else
    {
      for (const auto &track : tracks_)
        {
          if (ZRYTHM_TESTING)
            {
              z_return_val_if_fail (track, nullptr);
            }
          if (track->get_name_hash () == hash)
            return dynamic_cast<T *> (track.get ());
        }
    }
  return nullptr;
}

bool
Tracklist::multiply_track_heights (
  double multiplier,
  bool   visible_only,
  bool   check_only,
  bool   fire_events)
{
  for (auto &tr : tracks_)
    {
      if (visible_only && !tr->should_be_visible ())
        continue;

      bool ret = tr->multiply_heights (multiplier, visible_only, check_only);

      if (!ret)
        {
          return false;
        }

      if (!check_only && fire_events)
        {
          /* FIXME should be event */
          // track_widget_update_size (tr->widget_);
        }
    }

  return true;
}

int
Tracklist::get_track_pos (Track &track) const
{
  auto it =
    std::find_if (tracks_.cbegin (), tracks_.cend (), [&track] (const auto &t) {
      return t.get () == &track;
    });
  z_return_val_if_fail (it != tracks_.cend (), -1);
  return std::distance (tracks_.cbegin (), it);
}

bool
Tracklist::validate () const
{
  /* this validates tracks in parallel */
  std::vector<std::future<bool>> ret_vals;
  ret_vals.reserve (tracks_.size ());
  for (const auto &track : tracks_)
    {
      ret_vals.emplace_back (std::async ([this, &track] () {
        z_return_val_if_fail (track && track->is_in_active_project (), false);

        if (!track->validate ())
          return false;

        if (track->pos_ != get_track_pos (*track))
          {
            return false;
          }

        /* validate size */
        int track_size = 1;
        if (auto foldable_track = dynamic_cast<FoldableTrack *> (track.get ()))
          {
            track_size = foldable_track->size_;
          }
        z_return_val_if_fail (
          track->pos_ + track_size <= (int) tracks_.size (), false);

        /* validate connections */
        if (auto channel_track = dynamic_cast<ChannelTrack *> (track.get ()))
          {
            auto &channel = channel_track->get_channel ();
            for (const auto &send : channel->sends_)
              {
                send->validate ();
              }
          }
        return true;
      }));
    }

  return std::all_of (ret_vals.begin (), ret_vals.end (), [] (auto &t) {
    return t.get ();
  });
}

int
Tracklist::get_last_pos (const PinOption pin_opt, const bool visible_only) const
{
  for (int i = tracks_.size () - 1; i >= 0; i--)
    {
      const auto &tr = tracks_[i];

      if (pin_opt == PinOption::PinnedOnly && !tr->is_pinned ())
        continue;
      if (pin_opt == PinOption::UnpinnedOnly && tr->is_pinned ())
        continue;
      if (visible_only && !tr->should_be_visible ())
        continue;

      return i;
    }

  /* no track with given options found, select the last */
  return tracks_.size () - 1;
}

bool
Tracklist::is_in_active_project () const
{
  return project_ == PROJECT.get ()
         || (sample_processor_ && sample_processor_->is_in_active_project ());
}

Track *
Tracklist::get_visible_track_after_delta (Track &track, int delta) const
{
  auto vis_track = &track;
  while (delta != 0)
    {
      vis_track =
        delta > 0
          ? get_next_visible_track (*vis_track)
          : get_prev_visible_track (*vis_track);
      if (!vis_track)
        return nullptr;
      delta += delta > 0 ? -1 : 1;
    }
  return vis_track;
}

Track *
Tracklist::get_first_visible_track (const bool pinned) const
{
  auto it =
    std::find_if (tracks_.begin (), tracks_.end (), [pinned] (const auto &tr) {
      return tr->should_be_visible () && tr->is_pinned () == pinned;
    });
  return it != tracks_.end () ? it->get () : nullptr;
}

Track *
Tracklist::get_prev_visible_track (const Track &track) const
{
  auto it = std::find_if (
    tracks_.rbegin (), tracks_.rend (), [&track] (const auto &tr) {
      return tr->pos_ < track.pos_ && tr->should_be_visible ();
    });
  return it != tracks_.rend () ? it->get () : nullptr;
}

Track *
Tracklist::get_next_visible_track (const Track &track) const
{
  auto it =
    std::find_if (tracks_.begin (), tracks_.end (), [&track] (const auto &tr) {
      return tr->pos_ > track.pos_ && tr->should_be_visible ();
    });
  return it != tracks_.end () ? it->get () : nullptr;
}

void
Tracklist::remove_track (
  Track &track,
  bool   rm_pl,
  bool   free_track,
  bool   publish_events,
  bool   recalc_graph)
{
  z_debug (
    "removing [{}] {} - remove plugins {} - "
    "free track {} - pub events {} - "
    "recalc graph {} - "
    "num tracks before deletion: {}",
    track.pos_, track.get_name (), rm_pl, free_track, publish_events,
    recalc_graph, tracks_.size ());

  Track * prev_visible = nullptr;
  Track * next_visible = nullptr;
  if (!is_auditioner ())
    {
      prev_visible = get_prev_visible_track (track);
      next_visible = get_next_visible_track (track);
    }

  /* remove/deselect all objects */
  track.clear_objects ();

  int idx = get_track_pos (track);
  z_return_if_fail (track.pos_ == idx);

  track.disconnect (rm_pl, false);

  /* move track to the end */
  int end_pos = tracks_.size () - 1;
  move_track (track, end_pos, false, false, false);

  if (!is_auditioner ())
    {
      TRACKLIST_SELECTIONS->remove_track (track, publish_events);
    }

  auto it =
    std::find_if (tracks_.begin (), tracks_.end (), [&track] (const auto &ptr) {
      return ptr.get () == &track;
    });
  z_return_if_fail (it != tracks_.end ());
  std::unique_ptr<Track> removed_track = std::move (*it);
  tracks_.erase (it);

  if (is_in_active_project () && !is_auditioner ())
    {
      /* if it was the only track selected, select the next one */
      if (TRACKLIST_SELECTIONS->empty ())
        {
          Track * track_to_select = next_visible ? next_visible : prev_visible;
          if (!track_to_select && !tracks_.empty ())
            {
              track_to_select = tracks_[0].get ();
            }
          if (track_to_select)
            {
              TRACKLIST_SELECTIONS->add_track (*track_to_select, publish_events);
            }
        }
    }

  removed_track->pos_ = -1;

  if (free_track)
    {
      removed_track.reset ();
    }

  if (recalc_graph)
    {
      ROUTER->recalc_graph (false);
    }

  if (publish_events)
    {
      // EVENTS_PUSH (EventType::ET_TRACKS_REMOVED, nullptr);
    }

  z_debug ("done removing track");
}

void
Tracklist::move_track (
  Track &track,
  int    pos,
  bool   always_before_pos,
  bool   publish_events,
  bool   recalc_graph)
{
  z_debug ("moving track: {} from {} to {}", track.get_name (), track.pos_, pos);

  if (pos == track.pos_)
    return;

  bool move_higher = pos < track.pos_;

  Track * prev_visible = get_prev_visible_track (track);
  Track * next_visible = get_next_visible_track (track);

  int idx = get_track_pos (track);
  z_return_if_fail (track.pos_ == idx);

  /* the current implementation currently moves some tracks to tracks.size() + 1
   * temporarily, so we expand the vector here and resize it back at the end */
  bool expanded = false;
  if (pos >= static_cast<int> (tracks_.size ()))
    {
      tracks_.resize (pos + 1);
      expanded = true;
    }

  if (is_in_active_project () && !is_auditioner ())
    {
      /* clear the editor region if it exists and belongs to this track */
      auto region = CLIP_EDITOR->get_region ();
      if (region && region->get_track () == &track)
        {
          CLIP_EDITOR->set_region (nullptr, publish_events);
        }

      /* deselect all objects */
      track.unselect_all ();

      TRACKLIST_SELECTIONS->remove_track (track, publish_events);

      /* if it was the only track selected, select the next one */
      if (TRACKLIST_SELECTIONS->empty () && (prev_visible || next_visible))
        {
          TRACKLIST_SELECTIONS->add_track (
            next_visible ? *next_visible : *prev_visible, publish_events);
        }
    }

  if (move_higher)
    {
      /* move all other tracks 1 track further */
      for (int i = track.pos_; i > pos; i--)
        {
          swap_tracks (i, i - 1);
        }
    }
  else
    {
      /* move all other tracks 1 track earlier */
      for (int i = track.pos_; i < pos; i++)
        {
          swap_tracks (i, i + 1);
        }

      if (always_before_pos && pos > 0)
        {
          /* swap with previous track */
          swap_tracks (pos, pos - 1);
        }
    }

  if (expanded)
    {
      /* resize back */
      tracks_.resize (tracks_.size () - 1);
    }

  if (is_in_active_project () && !is_auditioner ())
    {
      /* make the track the only selected track */
      TRACKLIST_SELECTIONS->select_single (track, publish_events);
    }

  if (recalc_graph)
    {
      ROUTER->recalc_graph (false);
    }

  if (publish_events)
    {
      // EVENTS_PUSH (EventType::ET_TRACKS_MOVED, nullptr);
    }

  z_debug ("finished moving track");
}

bool
Tracklist::track_name_is_unique (const std::string &name, Track * track_to_skip)
  const
{
  return std::none_of (
    tracks_.begin (), tracks_.end (), [&name, track_to_skip] (const auto &track) {
      return track->get_name () == name && track.get () != track_to_skip;
    });
}

bool
Tracklist::has_soloed () const
{
  return std::any_of (tracks_.begin (), tracks_.end (), [] (const auto &track) {
    return track->has_channel () && track->get_soloed ();
  });
}

bool
Tracklist::has_listened () const
{
  return std::any_of (tracks_.begin (), tracks_.end (), [] (const auto &track) {
    return track->has_channel () && track->get_listened ();
  });
}

int
Tracklist::get_num_muted_tracks () const
{
  return std::count_if (tracks_.begin (), tracks_.end (), [] (const auto &track) {
    return track->has_channel () && track->get_muted ();
  });
}

int
Tracklist::get_num_soloed_tracks () const
{
  return std::count_if (tracks_.begin (), tracks_.end (), [] (const auto &track) {
    return track->has_channel () && track->get_soloed ();
  });
}

int
Tracklist::get_num_listened_tracks () const
{
  return std::count_if (tracks_.begin (), tracks_.end (), [] (const auto &track) {
    return track->has_channel () && track->get_listened ();
  });
}

void
Tracklist::get_plugins (std::vector<zrythm::plugins::Plugin *> &arr) const
{
  for (auto &track : tracks_)
    {
      track->get_plugins (arr);
    }
}

void
Tracklist::activate_all_plugins (bool activate)
{
  for (auto &track : tracks_)
    {
      track->activate_all_plugins (activate);
    }
}

int
Tracklist::get_num_visible_tracks (bool visible) const
{
  return std::count_if (
    tracks_.begin (), tracks_.end (), [visible] (const auto &track) {
      return track->should_be_visible () == visible;
    });
}

void
Tracklist::expose_ports_to_backend ()
{
  for (auto track : tracks_ | type_is<ChannelTrack> ())
    {
      track->channel_->expose_ports_to_backend ();
    }
}

void
Tracklist::import_regions (
  std::vector<std::vector<std::shared_ptr<Region>>> &region_arrays,
  const FileImportInfo *                             import_info,
  TracksReadyCallback                                ready_cb)
{
  z_debug ("Adding regions into the project...");

  AudioEngine::State state{};
  AUDIO_ENGINE->wait_for_pause (state, false, true);
  int executed_actions = 0;
  try
    {
      for (auto regions : region_arrays)
        {
          z_debug ("REGION ARRAY ({} elements)", regions.size ());
          int i = 0;
          while (!regions.empty ())
            {
              int iter = i++;
              z_debug ("REGION {}", iter);
              auto r = regions.back ();
              regions.pop_back ();
              Track::Type track_type = Track::Type::Audio;
              bool        gen_name = true;
              if (r->is_midi ())
                {
                  track_type = Track::Type::Midi;
                  if (!r->name_.empty ())
                    gen_name = false;
                }
              else if (!r->is_audio ())
                {
                  z_warning ("Unknown region type");
                  continue;
                }

              Track * track = nullptr;
              if (import_info->track_name_hash_)
                {
                  track =
                    find_track_by_name_hash (import_info->track_name_hash_);
                }
              else
                {
                  int index = import_info->track_idx_ + iter;
                  Track::create_empty_at_idx_with_action (track_type, index);
                  track = get_track (index);
                  executed_actions++;
                }
              z_return_if_fail (track);
              track->add_region_plain (r, nullptr, 0, gen_name, false);
              r->select (true, false, true);
              UNDO_MANAGER->perform (
                std::make_unique<CreateArrangerSelectionsAction> (
                  *TL_SELECTIONS));
              ++executed_actions;
            }
        }
    }
  catch (const ZrythmException &e)
    {
      z_warning ("Failed to import regions: {}", e.what ());

      /* undo any performed actions */
      while (executed_actions > 0)
        {
          UNDO_MANAGER->undo ();
          --executed_actions;
        }

      /* rethrow the exception */
      throw;
    }

  if (executed_actions > 0)
    {
      auto last_action = UNDO_MANAGER->get_last_action ();
      last_action->num_actions_ = executed_actions;
    }

  AUDIO_ENGINE->resume (state);

  if (ready_cb)
    {
      ready_cb (import_info);
    }
}

void
Tracklist::import_files (
  const StringArray *    uri_list,
  const FileDescriptor * orig_file,
  const Track *          track,
  const TrackLane *      lane,
  int                    index,
  const Position *       pos,
  TracksReadyCallback    ready_cb)
{
  std::vector<FileDescriptor> file_arr;
  if (orig_file)
    {
      file_arr.push_back (*orig_file);
    }
  else
    {
      for (const auto &uri : *uri_list)
        {
          if (!uri.contains ("file://"))
            continue;

          auto file = FileDescriptor::new_from_uri (uri.toStdString ());
          file_arr.push_back (*file);
        }
    }

  if (file_arr.empty ())
    {
      throw ZrythmException (_ ("No file was found"));
    }
  else if (track && file_arr.size () > 1)
    {
      throw ZrythmException (
        _ ("Can only drop 1 file at a time on existing tracks"));
    }

  for (const auto &file : file_arr)
    {
      if (file.is_supported () && file.is_audio ())
        {
          if (track && !track->is_audio ())
            {
              throw ZrythmException (
                _ ("Can only drop audio files on audio tracks"));
            }
        }
      else if (file.is_midi ())
        {
          if (track && !track->has_piano_roll ())
            {
              throw ZrythmException (
                _ ("Can only drop MIDI files on MIDI/instrument tracks"));
            }
        }
      else
        {
          auto descr = FileDescriptor::get_type_description (file.type_);
          throw ZrythmException (
            format_str (_ ("Unsupported file type {}"), descr));
        }
    }

  StringArray filepaths;
  for (const auto &file : file_arr)
    {
      filepaths.add (file.abs_path_);
    }

  auto nfo = FileImportInfo (
    track ? track->name_hash_ : 0, lane ? lane->pos_ : 0,
    pos ? *pos : Position (),
    track ? track->pos_ : (index >= 0 ? index : TRACKLIST->tracks_.size ()));

  if (ZRYTHM_TESTING || ZRYTHM_BENCHMARKING)
    {
      for (const auto &filepath : filepaths)
        {

          auto fi = file_import_new (filepath.toStdString ().c_str (), &nfo);
          GError * err = nullptr;
          auto     regions = file_import_sync (fi, &err);
          if (err != nullptr)
            {
              throw ZrythmException (format_str (
                _ ("Failed to import file {}: {}"), filepath.toStdString (),
                err->message));
            }
          std::vector<std::vector<std::shared_ptr<Region>>> region_arrays;
          region_arrays.push_back (regions);
          import_regions (region_arrays, &nfo, ready_cb);
        }
    }
  else
    {
#if 0
      auto filepaths_null_terminated = filepaths.getNullTerminated ();
      FileImportProgressDialog * dialog = file_import_progress_dialog_new (
        (const char **) filepaths_null_terminated, &nfo, ready_cb,
        UI_ACTIVE_WINDOW_OR_NULL);
      file_import_progress_dialog_run (dialog);
      g_strfreev (filepaths_null_terminated);
#endif
    }
}

void
Tracklist::move_after_copying_or_moving_inside (
  TracklistSelections &after_tls,
  int                  diff_between_track_below_and_parent)
{
  const auto &lowest_cloned_track = *(std::max_element (
    after_tls.tracks_.begin (), after_tls.tracks_.end (),
    [] (const auto &lhs, const auto &rhs) { return lhs->pos_ < rhs->pos_; }));
  auto        lowest_cloned_track_pos = lowest_cloned_track->pos_;

  try
    {
      UNDO_MANAGER->perform (std::make_unique<MoveTracksAction> (
        after_tls,
        lowest_cloned_track_pos + diff_between_track_below_and_parent));
    }
  catch (const ZrythmException &e)
    {
      e.handle (
        _ ("Failed to move tracks after copying or moving inside folder"));
      return;
    }

  auto ua = UNDO_MANAGER->get_last_action ();
  ua->num_actions_ = 2;
}

#if 0
void
Tracklist::handle_move_or_copy (
  Track               &this_track,
  TrackWidgetHighlight location,
  GdkDragAction        action)
{
  z_debug (
    "this track '{}' - location {} - action {}", this_track.name_,
    track_widget_highlight_to_str (location),
    action == GDK_ACTION_COPY ? "copy" : "move");

  int pos = -1;
  if (location == TrackWidgetHighlight::TRACK_WIDGET_HIGHLIGHT_TOP)
    {
      pos = this_track.pos_;
    }
  else
    {
      auto next = get_next_visible_track (this_track);
      if (next)
        pos = next->pos_;
      /* else if last track, move to end */
      else if (this_track.pos_ == static_cast<int> (tracks_.size ()) - 1)
        pos = tracks_.size ();
      /* else if last visible track but not last track */
      else
        pos = this_track.pos_ + 1;
    }

  if (pos == -1)
    return;

  TRACKLIST_SELECTIONS->select_foldable_children ();

  if (action == GDK_ACTION_COPY)
    {
      if (TRACKLIST_SELECTIONS->contains_uncopyable_track ())
        {
          z_warning ("cannot copy - track selection contains uncopyable track");
          return;
        }

      if (location == TrackWidgetHighlight::TRACK_WIDGET_HIGHLIGHT_INSIDE)
        {
          try
            {
              UNDO_MANAGER->perform (
                std::make_unique<CopyTracksInsideFoldableTrackAction> (
                  *TRACKLIST_SELECTIONS->gen_tracklist_selections (),
                  *PORT_CONNECTIONS_MGR, this_track.pos_));
            }
          catch (const ZrythmException &e)
            {
              e.handle (_ ("Failed to copy tracks inside"));
              return;
            }
        }
      /* else if not highlighted inside */
      else
        {
          auto                                &tls = *TRACKLIST_SELECTIONS;
          int                                  num_tls = tls.get_num_tracks ();
          std::unique_ptr<TracklistSelections> after_tls;
          int  diff_between_track_below_and_parent = 0;
          bool copied_inside = false;
          if (static_cast<size_t> (pos) < tracks_.size ())
            {
              auto &track_below = *tracks_[pos];
              auto track_below_parent = track_below.get_direct_folder_parent ();
              tls.sort ();
              auto cur_parent = find_track_by_name (tls.track_names_[0]);

              if (track_below_parent)
                {
                  diff_between_track_below_and_parent =
                    track_below.pos_ - track_below_parent->pos_;
                }

              /* first copy inside new parent */
              if (track_below_parent && track_below_parent != cur_parent)
                {
                  try
                    {
                      UNDO_MANAGER->perform (
                        std::make_unique<CopyTracksInsideFoldableTrackAction> (
                          *tls.gen_tracklist_selections (),
                          *PORT_CONNECTIONS_MGR, track_below_parent->pos_));
                    }
                  catch (const ZrythmException &e)
                    {
                      e.handle (_ ("Failed to copy track inside"));
                      return;
                    }

                  after_tls = std::make_unique<TracklistSelections> ();
                  for (int j = 1; j <= num_tls; j++)
                    {
                      try
                        {
                          after_tls->add_track (
                            clone_unique_with_variant<TrackVariant> (
                              tracks_[track_below_parent->pos_ + j].get ()));
                        }
                      catch (const ZrythmException &e)
                        {
                          e.handle (_ ("Failed to clone/add track"));
                          return;
                        }
                    }

                  copied_inside = true;
                }
            }

          /* if not copied inside, copy normally */
          if (!copied_inside)
            {
              try
                {
                  UNDO_MANAGER->perform (std::make_unique<CopyTracksAction> (
                    *tls.gen_tracklist_selections (), *PORT_CONNECTIONS_MGR,
                    pos));
                }
              catch (const ZrythmException &e)
                {
                  e.handle (_ ("Failed to copy tracks"));
                  return;
                }
            }
          /* else if copied inside and there is a track difference, also move */
          else if (diff_between_track_below_and_parent != 0)
            {
              move_after_copying_or_moving_inside (
                *after_tls, diff_between_track_below_and_parent);
            }
        }
    }
  else if (action == GDK_ACTION_MOVE)
    {
      if (location == TrackWidgetHighlight::TRACK_WIDGET_HIGHLIGHT_INSIDE)
        {
          if (TRACKLIST_SELECTIONS->contains_track (this_track))
            {
              if (!ZRYTHM_TESTING && !ZRYTHM_BENCHMARKING)
                {
                  ui_show_error_message (
                    _ ("Error"), _ ("Cannot drag folder into itself"));
                }
              return;
            }
          /* else if selections do not contain the track dragged into */
          else
            {
              try
                {
                  UNDO_MANAGER->perform (
                    std::make_unique<MoveTracksInsideFoldableTrackAction> (
                      *TRACKLIST_SELECTIONS->gen_tracklist_selections (),
                      this_track.pos_));
                }
              catch (const ZrythmException &e)
                {
                  e.handle (_ ("Failed to move track inside folder"));
                  return;
                }
            }
        }
      /* else if not highlighted inside */
      else
        {
          auto                                &tls = *TRACKLIST_SELECTIONS;
          int                                  num_tls = tls.get_num_tracks ();
          std::unique_ptr<TracklistSelections> after_tls;
          int  diff_between_track_below_and_parent = 0;
          bool moved_inside = false;
          if (static_cast<size_t> (pos) < tracks_.size ())
            {
              auto &track_below = *tracks_[pos];
              auto track_below_parent = track_below.get_direct_folder_parent ();
              tls.sort ();
              auto cur_parent = find_track_by_name (tls.track_names_[0]);

              if (track_below_parent)
                {
                  diff_between_track_below_and_parent =
                    track_below.pos_ - track_below_parent->pos_;
                }

              /* first move inside new parent */
              if (track_below_parent && track_below_parent != cur_parent)
                {
                  try
                    {
                      UNDO_MANAGER->perform (
                        std::make_unique<MoveTracksInsideFoldableTrackAction> (
                          *tls.gen_tracklist_selections (),
                          track_below_parent->pos_));
                    }
                  catch (const ZrythmException &e)
                    {
                      e.handle (_ ("Failed to move track inside folder"));
                      return;
                    }

                  after_tls = std::make_unique<TracklistSelections> ();
                  for (int j = 1; j <= num_tls; j++)
                    {
                      try
                        {
                          const auto &cur_track =
                            tracks_[track_below_parent->pos_ + j];
                          std::visit (
                            [&] (auto &&cur_track_casted) {
                              after_tls->add_track (
                                cur_track_casted->clone_unique ());
                            },
                            convert_to_variant<TrackPtrVariant> (
                              cur_track.get ()));
                        }
                      catch (const ZrythmException &e)
                        {
                          e.handle (_ ("Failed to clone track"));
                          return;
                        }
                    }

                  moved_inside = true;
                }
            } /* endif moved to an existing track */

          /* if not moved inside, move normally */
          if (!moved_inside)
            {
              try
                {
                  UNDO_MANAGER->perform (std::make_unique<MoveTracksAction> (
                    *tls.gen_tracklist_selections (), pos));
                }
              catch (const ZrythmException &e)
                {
                  e.handle (_ ("Failed to move tracks"));
                  return;
                }
            }
          /* else if moved inside and there is a track difference, also move */
          else if (diff_between_track_below_and_parent != 0)
            {
              move_after_copying_or_moving_inside (
                *after_tls, diff_between_track_below_and_parent);
            }
        }
    } /* endif action is MOVE */
}
#endif

void
Tracklist::mark_all_tracks_for_bounce (bool bounce)
{
  for (auto &track : tracks_)
    {
      track->mark_for_bounce (bounce, true, false, false);
    }
}

int
Tracklist::get_total_bars (int total_bars) const
{
  for (const auto &track : tracks_)
    {
      total_bars = track->get_total_bars (total_bars);
    }
  return total_bars;
}

bool
Tracklist::contains_master_track () const
{
  return contains_track_type<MasterTrack> ();
}

bool
Tracklist::contains_chord_track () const
{
  return contains_track_type<ChordTrack> ();
}

void
Tracklist::set_caches (CacheType types)
{
  for (auto &track : tracks_)
    {
      track->set_caches (types);
    }
}

void
Tracklist::init_after_cloning (const Tracklist &other)
{
  pinned_tracks_cutoff_ = other.pinned_tracks_cutoff_;
  clone_variant_container<TrackVariant> (tracks_, other.tracks_);
}

Tracklist::Tracklist (Project &project) : project_ (&project) { }

Tracklist::Tracklist (SampleProcessor &sample_processor)
    : sample_processor_ (&sample_processor)
{
}

Tracklist::~Tracklist ()
{
  z_debug ("freeing tracklist...");

  auto new_end =
    std::remove_if (tracks_.begin (), tracks_.end (), [this] (auto &track) {
      return track.get () != tempo_track_;
    });
  tracks_.erase (new_end, tracks_.end ());

#if 0
  for (auto &track : std::ranges::reverse_view (tracks_))
    {
      if (track.get () == tempo_track_)
        continue;

      remove_track (*track, true, true, false, false);
    }
#endif

  /* remove tempo track last (used when printing positions) */
  tracks_.clear ();
}

template ProcessableTrack *
Tracklist::find_track_by_name_hash (unsigned int) const;
template ChannelTrack *
Tracklist::find_track_by_name_hash (unsigned int) const;
template FoldableTrack *
Tracklist::find_track_by_name_hash (unsigned int) const;
template RecordableTrack *
Tracklist::find_track_by_name_hash (unsigned int) const;
template AutomatableTrack *
Tracklist::find_track_by_name_hash (unsigned int) const;
template LanedTrackImpl<AudioRegion> *
Tracklist::find_track_by_name_hash (unsigned int) const;
template LanedTrackImpl<MidiRegion> *
Tracklist::find_track_by_name_hash (unsigned int) const;
template GroupTargetTrack *
Tracklist::find_track_by_name_hash (unsigned int) const;
template ModulatorTrack *
Tracklist::find_track_by_name_hash (unsigned int) const;
template ChordTrack *
Tracklist::find_track_by_name_hash (unsigned int) const;