// SPDX-FileCopyrightText: © 2018-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/backend/actions/arranger_selections_action.h"
#include "gui/backend/backend/actions/tracklist_selections_action.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"
#include "gui/backend/channel.h"
#include "gui/backend/io/file_import.h"
#include "gui/dsp/chord_track.h"
#include "gui/dsp/foldable_track.h"
#include "gui/dsp/marker_track.h"
#include "gui/dsp/master_track.h"
#include "gui/dsp/modulator_track.h"
#include "gui/dsp/router.h"
#include "gui/dsp/tempo_track.h"
#include "gui/dsp/track.h"
#include "gui/dsp/tracklist.h"
#include "utils/flags.h"
#include "utils/gtest_wrapper.h"
#include "utils/rt_thread_id.h"
#include "utils/string.h"

Tracklist::Tracklist (QObject * parent) : QAbstractListModel (parent) { }

Tracklist::Tracklist (
  Project                 &project,
  PortConnectionsManager * port_connections_manager)
    : QAbstractListModel (&project), project_ (&project),
      port_connections_manager_ (port_connections_manager)
{
}

Tracklist::Tracklist (
  SampleProcessor         &sample_processor,
  PortConnectionsManager * port_connections_manager)
    : sample_processor_ (&sample_processor),
      port_connections_manager_ (port_connections_manager)
{
}

void
Tracklist::init_loaded (Project * project, SampleProcessor * sample_processor)
{
  project_ = project;
  sample_processor_ = sample_processor;

  z_debug ("initializing loaded Tracklist...");
  for (auto &track : tracks_)
    {
      std::visit (
        [&] (auto &track) {
          using T = base_type<decltype (track)>;
          if constexpr (std::is_same_v<T, ChordTrack>)
            {
              chord_track_ = track;
            }
          else if constexpr (std::is_same_v<T, MarkerTrack>)
            {
              marker_track_ = track;
            }
          else if constexpr (std::is_same_v<T, MasterTrack>)
            {
              master_track_ = track;
            }
          else if constexpr (std::is_same_v<T, TempoTrack>)
            {
              tempo_track_ = track;
            }
          else if constexpr (std::is_same_v<T, ModulatorTrack>)
            {
              modulator_track_ = track;
            }
          track->setParent (this);
          track->tracklist_ = this;
          track->init_loaded ();
        },
        track);
    }
}

// ========================================================================
// QML Interface
// ========================================================================

QHash<int, QByteArray>
Tracklist::roleNames () const
{
  QHash<int, QByteArray> roles;
  roles[TrackPtrRole] = "track";
  roles[TrackNameRole] = "trackName";
  return roles;
}

int
Tracklist::rowCount (const QModelIndex &parent) const
{
  return static_cast<int> (tracks_.size ());
}

QVariant
Tracklist::data (const QModelIndex &index, int role) const
{
  if (!index.isValid ())
    return {};

  auto track = tracks_.at (index.row ());

  z_trace (
    "getting role {} for track {}", role,
    QString::fromStdString (
      Track::from_variant (tracks_.at (index.row ()))->name_));

  switch (role)
    {
    case TrackPtrRole:
      return QVariant::fromStdVariant (track);
    case TrackNameRole:
      return QString::fromStdString (Track::from_variant (track)->name_);
    default:
      return {};
    }
}

TempoTrack *
Tracklist::getTempoTrack () const
{
  return tempo_track_;
}

// ========================================================================

std::optional<TrackPtrVariant>
Tracklist::find_track_by_name (const std::string &name) const
{
  auto found = std::ranges::find_if (tracks_, [&] (auto &tr) {
    auto * track = Track::from_variant (tr);
    return track->name_ == name;
  });
  if (found != tracks_.end ())
    {
      return *found;
    }

  return std::nullopt;
}

std::optional<TrackPtrVariant>
Tracklist::find_track_by_name_hash (unsigned int hash) const
{
  // z_trace ("called for {}", hash);
  auto it = [&] () {
    if (
      is_in_active_project () && ROUTER && ROUTER->is_processing_thread ()
      && !is_auditioner ()) [[likely]]
      {
        return std::ranges::find_if (tracks_, [hash] (const auto &track) {
          return Track::from_variant (track)->name_hash_ == hash;
        });
      }
    else
      {
        return std::ranges::find_if (tracks_, [hash] (const auto &track) {
          return Track::from_variant (track)->get_name_hash () == hash;
        });
      }
  }();
  if (it == tracks_.end ())
    return std::nullopt;

  return *it;
}

void
Tracklist::select_all (bool select, bool fire_events)
{
  for (auto &track : tracks_)
    {
      std::visit (
        [&] (auto &&t) { t->select (select, false, fire_events); }, track);
    }

  if (!select)
    {
      std::visit (
        [&] (auto &&t) { t->select (true, true, fire_events); },
        tracks_.back ());
    }
}

std::optional<TrackPtrVariant>
Tracklist::get_last_track (const PinOption pin_opt, const bool visible_only) const
{
  int idx = get_last_pos (pin_opt, visible_only);
  return tracks_.at (idx);
}

void
Tracklist::get_visible_tracks (std::vector<Track *> visible_tracks) const
{
  for (const auto &track : tracks_)
    {
      std::visit (
        [&] (auto &&t) {
          if (t->should_be_visible ())
            {
              visible_tracks.push_back (t);
            }
        },
        track);
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
          std::visit (
            [&] (auto &&tr) {
              if (tr->should_be_visible ())
                {
                  count++;
                }
            },
            (*it));
        }
    }
  else if (src.pos_ > dest.pos_)
    {
      for (
        auto it = tracks_.begin () + dest.pos_;
        it != tracks_.begin () + src.pos_; ++it)
        {
          std::visit (
            [&] (auto &&tr) {
              if (tr->should_be_visible ())
                {
                  count--;
                }
            },
            (*it));
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
      std::visit (
        [&] (auto &&track) {
          if (track)
            {
              std::string                  parent_str;
              std::vector<FoldableTrack *> parents;
              track->add_folder_parents (parents, false);
              parent_str.append (parents.size () * 2, '-');
              if (!parents.empty ())
                parent_str += ' ';

              int fold_size = 1;
              if constexpr (
                std::is_same_v<base_type<decltype (track)>, FoldableTrack>)
                {
                  fold_size = track->size_;
                }

              z_info (
                "[{:03}] {}{} (pos {}, parents {}, size {})", i, parent_str,
                track->name_, track->pos_, parents.size (), fold_size);
            }
          else
            {
              z_info ("[{:03}] (null)", i);
            }
        },
        tracks_.at (i));
    }
  z_info ("------ end ------");
}

void
Tracklist::swap_tracks (const size_t index1, const size_t index2)
{
  z_return_if_fail (std::max (index1, index2) < tracks_.size ());
  swapping_tracks_ = true;

  {
    const auto &src_track = Track::from_variant (tracks_.at (index1));
    const auto &dest_track = Track::from_variant (tracks_.at (index2));
    z_debug (
      "swapping tracks {} [{}] and {} [{}]...",
      src_track ? src_track->name_ : "(none)", index1,
      dest_track ? dest_track->name_ : "(none)", index2);
  }

  std::iter_swap (tracks_.begin () + index1, tracks_.begin () + index2);

  {
    const auto &src_track = Track::from_variant (tracks_.at (index1));
    const auto &dest_track = Track::from_variant (tracks_.at (index2));

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
  AudioEngine         &engine,
  bool                 publish_events,
  bool                 recalc_graph)
{
  beginResetModel ();
  auto ret = [&] () -> T * {
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
    track->set_name (*this, track->name_, false);

    /* append the track at the end */
    T * added_track = std::get<T *> (tracks_.emplace_back (track.release ()));
    added_track->tracklist_ = this;
    added_track->setParent (this);

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
            port->id_->flags2_ |=
              dsp::PortIdentifier::Flags2::SampleProcessorTrack;
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
                auto port_var = PROJECT->find_port_by_id (*at->port_id_);
                z_return_val_if_fail (
                  port_var.has_value ()
                    && std::holds_alternative<ControlPort *> (port_var.value ()),
                  nullptr);
                auto port = std::get<ControlPort *> (port_var.value ());
                port->at_ = at;
              }
          }
      }

    if constexpr (std::derived_from<T, ChannelTrack>)
      {
        z_return_val_if_fail (port_connections_manager_, nullptr);
        added_track->channel_->connect (*port_connections_manager_, engine);
      }

    /* if audio output route to master */
    if constexpr (!std::is_same_v<T, MasterTrack>)
      {
        if (
          added_track->out_signal_type_ == dsp::PortType::Audio && master_track_)
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
  }();
  endResetModel ();
  return ret;
}

Track *
Tracklist::insert_track (
  std::unique_ptr<Track> &&track,
  int                      pos,
  AudioEngine             &engine,
  bool                     publish_events,
  bool                     recalc_graph)
{
  return std::visit (
    [&] (auto &&t) {
      using T = base_type<decltype (t)>;
      auto track_unique_ptr = std::unique_ptr<T> (t);
      return dynamic_cast<Track *> (insert_track<T> (
        std::move (track_unique_ptr), pos, engine, publish_events,
        recalc_graph));
    },
    convert_to_variant<TrackPtrVariant> (track.release ()));
}

Track *
Tracklist::append_track (
  std::unique_ptr<Track> &&track,
  AudioEngine             &engine,
  bool                     publish_events,
  bool                     recalc_graph)
{
  return std::visit (
    [&] (auto &&t) {
      using T = base_type<decltype (t)>;
      auto track_unique_ptr = std::unique_ptr<T> (t);
      return dynamic_cast<Track *> (append_track<T> (
        std::move (track_unique_ptr), engine, publish_events, recalc_graph));
    },
    convert_to_variant<TrackPtrVariant> (track.release ()));
}

ChordTrack *
Tracklist::get_chord_track () const
{
  return get_track_by_type<ChordTrack> ();
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
      auto * const track = Track::from_variant (tr);
      if (visible_only && !track->should_be_visible ())
        continue;

      bool ret = track->multiply_heights (multiplier, visible_only, check_only);

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
      return Track::from_variant (t) == &track;
    });
  z_return_val_if_fail (it != tracks_.cend (), -1);
  return static_cast<int> (std::distance (tracks_.cbegin (), it));
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
        // z_return_val_if_fail (track && track->is_in_active_project (), false);
        auto * tr = Track::from_variant (track);
        z_return_val_if_fail (tr, false);

        if (!tr->validate ())
          return false;

        if (tr->pos_ != get_track_pos (*tr))
          {
            return false;
          }

        /* validate size */
        int track_size = 1;
        if (const auto * foldable_track = dynamic_cast<FoldableTrack *> (tr))
          {
            track_size = foldable_track->size_;
          }
        z_return_val_if_fail (
          tr->pos_ + track_size <= (int) tracks_.size (), false);

        /* validate connections */
        if (const auto * channel_track = dynamic_cast<ChannelTrack *> (tr))
          {
            const auto &channel = channel_track->get_channel ();
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
      const auto &tr = Track::from_variant (tracks_[i]);

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
  return project_ == PROJECT
         || (sample_processor_ && sample_processor_->is_in_active_project ());
}

std::optional<TrackPtrVariant>
Tracklist::get_visible_track_after_delta (Track &track, int delta) const
{
  auto * vis_track = &track;
  while (delta != 0)
    {
      auto vis_after_delta =
        delta > 0
          ? get_next_visible_track (*vis_track)
          : get_prev_visible_track (*vis_track);
      if (!vis_after_delta)
        return std::nullopt;
      vis_track = Track::from_variant (vis_after_delta.value ());
      delta += delta > 0 ? -1 : 1;
    }
  return std::make_optional (convert_to_variant<TrackPtrVariant> (vis_track));
}

std::optional<TrackPtrVariant>
Tracklist::get_first_visible_track (const bool pinned) const
{
  auto it =
    std::find_if (tracks_.begin (), tracks_.end (), [pinned] (const auto &tr) {
      auto track = Track::from_variant (tr);
      return track->should_be_visible () && track->is_pinned () == pinned;
    });
  return it != tracks_.end () ? std::make_optional (*it) : std::nullopt;
}

std::optional<TrackPtrVariant>
Tracklist::get_prev_visible_track (const Track &track) const
{
  auto it = std::find_if (
    tracks_.rbegin (), tracks_.rend (), [&track] (const auto &tr) {
      auto cur_tr = Track::from_variant (tr);
      return cur_tr->pos_ < track.pos_ && cur_tr->should_be_visible ();
    });
  return it != tracks_.rend () ? std::make_optional (*it) : std::nullopt;
}

std::optional<TrackPtrVariant>
Tracklist::get_next_visible_track (const Track &track) const
{
  auto it =
    std::find_if (tracks_.begin (), tracks_.end (), [&track] (const auto &tr) {
      auto cur_tr = Track::from_variant (tr);
      return cur_tr->pos_ > track.pos_ && cur_tr->should_be_visible ();
    });
  return it != tracks_.end () ? std::make_optional (*it) : std::nullopt;
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

  beginRemoveRows ({}, track.pos_, track.pos_);

  std::optional<TrackPtrVariant> prev_visible = std::nullopt;
  std::optional<TrackPtrVariant> next_visible = std::nullopt;
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
  auto end_pos = std::ssize (tracks_) - 1;
  move_track (track, end_pos, false, false);

  if (!is_auditioner ())
    {
      TRACKLIST_SELECTIONS->remove_track (track, publish_events);
    }

  auto it =
    std::find_if (tracks_.begin (), tracks_.end (), [&track] (const auto &ptr) {
      auto ptr_track = Track::from_variant (ptr);
      return ptr_track == &track;
    });
  z_return_if_fail (it != tracks_.end ());
  auto removed_track = *it;
  tracks_.erase (it);

  if (is_in_active_project () && !is_auditioner ())
    {
      /* if it was the only track selected, select the next one */
      if (TRACKLIST_SELECTIONS->empty ())
        {
          auto track_to_select = next_visible ? next_visible : prev_visible;
          if (!track_to_select && !tracks_.empty ())
            {
              track_to_select = tracks_.at (0);
            }
          if (track_to_select)
            {
              std::visit (
                [] (auto &&tr) { TRACKLIST_SELECTIONS->add_track (*tr); },
                *track_to_select);
            }
        }
    }

  std::visit (
    [free_track] (auto &&tr) {
      tr->pos_ = -1;

      if (free_track)
        {
          tr->deleteLater ();
        }
    },
    removed_track);

  if (recalc_graph)
    {
      ROUTER->recalc_graph (false);
    }

  endRemoveRows ();

  z_debug ("done removing track");
}

void
Tracklist::
  move_track (Track &track, int pos, bool always_before_pos, bool recalc_graph)
{
  z_debug ("moving track: {} from {} to {}", track.get_name (), track.pos_, pos);

  if (pos == track.pos_)
    return;

  beginMoveRows ({}, track.pos_, track.pos_, {}, pos);

  bool move_higher = pos < track.pos_;

  auto prev_visible = get_prev_visible_track (track);
  auto next_visible = get_next_visible_track (track);

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
      auto region_var = CLIP_EDITOR->get_region ();
      if (region_var)
        {
          std::visit (
            [&] (auto &&region) {
              auto region_track_var = region->get_track ();
              std::visit (
                [&] (auto &&region_track) {
                  if (region_track == &track)
                    {
                      CLIP_EDITOR->set_region (std::nullopt, true);
                    }
                },
                region_track_var);
            },
            region_var.value ());
        }

      /* deselect all objects */
      track.unselect_all ();

      TRACKLIST_SELECTIONS->remove_track (track, true);

      /* if it was the only track selected, select the next one */
      if (TRACKLIST_SELECTIONS->empty () && (prev_visible || next_visible))
        {
          auto track_to_add = next_visible ? *next_visible : *prev_visible;
          std::visit (
            [] (auto &&tr) { TRACKLIST_SELECTIONS->add_track (*tr); },
            track_to_add);
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
      TRACKLIST_SELECTIONS->select_single (track, true);
    }

  if (recalc_graph)
    {
      ROUTER->recalc_graph (false);
    }

  endMoveRows ();

  z_debug ("finished moving track");
}

bool
Tracklist::track_name_is_unique (const std::string &name, Track * track_to_skip)
  const
{
  return std::none_of (
    tracks_.begin (), tracks_.end (), [&name, track_to_skip] (const auto &tr) {
      auto track = Track::from_variant (tr);
      return track->get_name () == name && track != track_to_skip;
    });
}

bool
Tracklist::has_soloed () const
{
  return std::any_of (tracks_.begin (), tracks_.end (), [] (const auto &tr) {
    auto track = Track::from_variant (tr);
    return track->has_channel () && track->get_soloed ();
  });
}

bool
Tracklist::has_listened () const
{
  return std::any_of (tracks_.begin (), tracks_.end (), [] (const auto &tr) {
    auto track = Track::from_variant (tr);
    return track->has_channel () && track->get_listened ();
  });
}

int
Tracklist::get_num_muted_tracks () const
{
  return std::count_if (tracks_.begin (), tracks_.end (), [] (const auto &tr) {
    auto track = Track::from_variant (tr);
    return track->has_channel () && track->get_muted ();
  });
}

int
Tracklist::get_num_soloed_tracks () const
{
  return std::count_if (tracks_.begin (), tracks_.end (), [] (const auto &tr) {
    auto track = Track::from_variant (tr);
    return track->has_channel () && track->get_soloed ();
  });
}

int
Tracklist::get_num_listened_tracks () const
{
  return std::count_if (tracks_.begin (), tracks_.end (), [] (const auto &tr) {
    auto track = Track::from_variant (tr);
    return track->has_channel () && track->get_listened ();
  });
}

void
Tracklist::get_plugins (
  std::vector<zrythm::gui::old_dsp::plugins::Plugin *> &arr) const
{
  std::ranges::for_each (tracks_, [&arr] (const auto &tr) {
    auto track = Track::from_variant (tr);
    track->get_plugins (arr);
  });
}

void
Tracklist::activate_all_plugins (bool activate)
{
  std::ranges::for_each (tracks_, [&activate] (const auto &tr) {
    auto track = Track::from_variant (tr);
    track->activate_all_plugins (activate);
  });
}

int
Tracklist::get_num_visible_tracks (bool visible) const
{
  return std::count_if (
    tracks_.begin (), tracks_.end (), [visible] (const auto &tr) {
      auto track = Track::from_variant (tr);
      return track->should_be_visible () == visible;
    });
}

void
Tracklist::expose_ports_to_backend (AudioEngine &engine)
{
  for (auto * track : tracks_ | type_is<ChannelTrack> ())
    {
      track->channel_->expose_ports_to_backend (engine);
    }
}

void
Tracklist::import_regions (
  std::vector<std::vector<std::shared_ptr<Region>>> &region_arrays,
  const FileImportInfo *                             import_info,
  TracksReadyCallback                                ready_cb)
{
// TODO
#if 0
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
                  auto tmp = get_track (index);
                  std::visit ([&track] (auto * t) { track = t; }, tmp);
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
#endif
}

void
Tracklist::import_files (
  const StringArray *    uri_list,
  const FileDescriptor * orig_file,
  const Track *          track,
  const TrackLane *      lane,
  int                    index,
  const dsp::Position *  pos,
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
      throw ZrythmException (QObject::tr ("No file was found"));
    }
  else if (track && file_arr.size () > 1)
    {
      throw ZrythmException (
        QObject::tr ("Can only drop 1 file at a time on existing tracks"));
    }

  for (const auto &file : file_arr)
    {
      if (file.is_supported () && file.is_audio ())
        {
          if (track && !track->is_audio ())
            {
              throw ZrythmException (
                QObject::tr ("Can only drop audio files on audio tracks"));
            }
        }
      else if (file.is_midi ())
        {
          if (track && !track->has_piano_roll ())
            {
              throw ZrythmException (QObject::tr (
                "Can only drop MIDI files on MIDI/instrument tracks"));
            }
        }
      else
        {
          auto descr = FileDescriptor::get_type_description (file.type_);
          throw ZrythmException (
            format_qstr (QObject::tr ("Unsupported file type {}"), descr));
        }
    }

  StringArray filepaths;
  for (const auto &file : file_arr)
    {
      filepaths.add (file.abs_path_);
    }

    // TODO
#if 0
  auto nfo = FileImportInfo (
    track ? track->name_hash_ : 0, lane ? lane->pos_ : 0,
    pos ? *pos : Position (),
    track ? track->pos_ : (index >= 0 ? index : TRACKLIST->tracks_.size ()));

  if (ZRYTHM_TESTING || ZRYTHM_BENCHMARKING)
    {
      for (const auto &filepath : filepaths)
        {

          auto     fi = file_import_new (filepath.toStdString (), &nfo);
          GError * err = nullptr;
          auto     regions = file_import_sync (fi, &err);
          if (err != nullptr)
            {
              throw ZrythmException (format_str (
                QObject::tr ("Failed to import file {}: {}"),
                filepath.toStdString (), err->message));
            }
          std::vector<std::vector<std::shared_ptr<Region>>> region_arrays;
          region_arrays.push_back (regions);
          import_regions (region_arrays, &nfo, ready_cb);

        }
    }
  else
    {
      auto filepaths_null_terminated = filepaths.getNullTerminated ();
      FileImportProgressDialog * dialog = file_import_progress_dialog_new (
        (const char **) filepaths_null_terminated, &nfo, ready_cb,
        UI_ACTIVE_WINDOW_OR_NULL);
      file_import_progress_dialog_run (dialog);
      g_strfreev (filepaths_null_terminated);
    }
#endif
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
      UNDO_MANAGER->perform (new gui::actions::MoveTracksAction (
        after_tls,
        lowest_cloned_track_pos + diff_between_track_below_and_parent));
    }
  catch (const ZrythmException &e)
    {
      e.handle (QObject::tr (
        "Failed to move tracks after copying or moving inside folder"));
      return;
    }

  auto ua_opt = UNDO_MANAGER->get_last_action ();
  z_return_if_fail (ua_opt.has_value ());
  std::visit ([&] (auto &&ua) { ua->num_actions_ = 2; }, ua_opt.value ());
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
              e.handle (QObject::tr ("Failed to copy tracks inside"));
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
                      e.handle (QObject::tr ("Failed to copy track inside"));
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
                          e.handle (QObject::tr ("Failed to clone/add track"));
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
                  e.handle (QObject::tr ("Failed to copy tracks"));
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
                    QObject::tr ("Error"), QObject::tr ("Cannot drag folder into itself"));
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
                  e.handle (QObject::tr ("Failed to move track inside folder"));
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
                      e.handle (QObject::tr ("Failed to move track inside folder"));
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
                          e.handle (QObject::tr ("Failed to clone track"));
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
                  e.handle (QObject::tr ("Failed to move tracks"));
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
  std::ranges::for_each (tracks_, [bounce] (auto &tr) {
    auto track = Track::from_variant (tr);
    track->mark_for_bounce (bounce, true, false, false);
  });
}

int
Tracklist::get_total_bars (const Transport &transport, int total_bars) const
{
  std::ranges::for_each (tracks_, [&] (auto &tr) {
    auto track = Track::from_variant (tr);
    total_bars = track->get_total_bars (transport, total_bars);
  });
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
  std::ranges::for_each (tracks_, [types] (auto &tr) {
    auto track = Track::from_variant (tr);
    track->set_caches (types);
  });
}

void
Tracklist::init_after_cloning (const Tracklist &other)
{
  pinned_tracks_cutoff_ = other.pinned_tracks_cutoff_;

  tracks_.clear ();
  tracks_.reserve (other.tracks_.size ());
  for (const auto &track : other.tracks_)
    {
      std::visit (
        [&] (auto &tr) {
          auto raw_ptr = tr->clone_raw_ptr ();
          raw_ptr->setParent (this);
          tracks_.push_back (raw_ptr);
        },
        track);
    }
}

Tracklist::~Tracklist ()
{
  z_debug ("freeing tracklist...");

  // Disconnect all signals to prevent access during destruction
  disconnect ();

  // Schedule tempo track for later deletion because it might be used when
  // printing positions
  std::ranges::for_each (children (), [] (QObject * child) {
    if (dynamic_cast<TempoTrack *> (child) != nullptr)
      {
        child->setParent (nullptr);
        child->deleteLater ();
      }
  });
}
