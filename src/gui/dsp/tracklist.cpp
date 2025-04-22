// SPDX-FileCopyrightText: © 2018-2025 Alexandros Theodotou <alex@zrythm.org>
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
#include "utils/gtest_wrapper.h"
#include "utils/rt_thread_id.h"
#include "utils/string.h"

Tracklist::Tracklist (QObject * parent) : QAbstractListModel (parent) { }

Tracklist::Tracklist (
  Project                 &project,
  PortRegistry            &port_registry,
  TrackRegistry           &track_registry,
  PortConnectionsManager * port_connections_manager)
    : QAbstractListModel (&project), track_registry_ (track_registry),
      port_registry_ (port_registry), project_ (&project),
      port_connections_manager_ (port_connections_manager)
{
}

Tracklist::Tracklist (
  SampleProcessor         &sample_processor,
  PortRegistry            &port_registry,
  TrackRegistry           &track_registry,
  PortConnectionsManager * port_connections_manager)
    : track_registry_ (track_registry), port_registry_ (port_registry),
      sample_processor_ (&sample_processor),
      port_connections_manager_ (port_connections_manager)
{
}

void
Tracklist::init_loaded (
  PortRegistry     &port_registry,
  Project *         project,
  SampleProcessor * sample_processor)
{
  port_registry_ = port_registry;
  project_ = project;
  sample_processor_ = sample_processor;

  z_debug ("initializing loaded Tracklist...");
  for (auto &track_var : get_track_span ())
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
          track->tracklist_ = this;
          track->init_loaded (*plugin_registry_, *port_registry_);
        },
        track_var);
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

  auto track_id = tracks_.at (index.row ());
  auto track = track_id.get_object ();

  z_trace (
    "getting role {} for track {}", role,
    QString::fromStdString (TrackSpan::name_projection (track)));

  switch (role)
    {
    case TrackPtrRole:
      return QVariant::fromStdVariant (track);
    case TrackNameRole:
      return QString::fromStdString (TrackSpan::name_projection (track));
    default:
      return {};
    }
}

TempoTrack *
Tracklist::getTempoTrack () const
{
  return tempo_track_;
}

void
Tracklist::setExclusivelySelectedTrack (QVariant track)
{
  auto track_var = qvariantToStdVariant<TrackPtrVariant> (track);
  get_selection_manager ().select_unique (
    TrackSpan::uuid_projection (track_var));
}

// ========================================================================

int
Tracklist::get_visible_track_diff (
  Track::TrackUuid src_track,
  Track::TrackUuid dest_track) const
{
  const auto src_track_index = std::distance (
    tracks_.begin (),
    std::ranges::find (tracks_, src_track, &TrackUuidReference::id));
  const auto dest_track_index = std::distance (
    tracks_.begin (),
    std::ranges::find (tracks_, dest_track, &TrackUuidReference::id));

  // Determine range boundaries and direction
  const auto [lower, upper] = std::minmax (src_track_index, dest_track_index);
  const int sign = src_track_index < dest_track_index ? 1 : -1;

  // Count visible tracks in the range (exclusive upper bound)
  auto       span = get_track_span ();
  const auto count = std::ranges::count_if (
    span.begin () + lower, span.begin () + upper,
    TrackRegistrySpan::visible_projection);

  // Apply direction modifier
  return sign * count;
}

void
Tracklist::swap_tracks (const size_t index1, const size_t index2)
{
  z_return_if_fail (std::max (index1, index2) < tracks_.size ());
  AtomicBoolRAII raii{ swapping_tracks_ };

  {
    auto        span = get_track_span ();
    const auto &src_track = Track::from_variant (span.at (index1));
    const auto &dest_track = Track::from_variant (span.at (index2));
    z_debug (
      "swapping tracks {} [{}] and {} [{}]...",
      src_track ? src_track->get_name () : "(none)", index1,
      dest_track ? dest_track->get_name () : "(none)", index2);
  }

  std::iter_swap (tracks_.begin () + index1, tracks_.begin () + index2);

  {
    auto        span = get_track_span ();
    const auto &src_track = Track::from_variant (span.at (index1));
    const auto &dest_track = Track::from_variant (span.at (index2));

    if (src_track)
      src_track->set_index (index1);
    if (dest_track)
      dest_track->set_index (index2);
  }

  z_debug ("tracks swapped");
}

TrackPtrVariant
Tracklist::insert_track (
  const TrackUuidReference &track_id,
  int                       pos,
  AudioEngine              &engine,
  bool                      publish_events,
  bool                      recalc_graph)
{
  auto track_var = track_id.get_object ();

  beginResetModel ();
  std::visit (
    [&] (auto &&track) {
      using TrackT = base_type<decltype (track)>;
      z_info ("inserting {} at {}", track->get_name (), pos);

      /* throw error if attempted to add a special track (like master) when it
       * already exists */
      if (
        !Track::type_is_deletable (Track::get_type_for_class<TrackT> ())
        && get_track_span ().contains_type<TrackT> ())
        {
          z_error (
            "cannot add track of type {} when it already exists",
            Track::get_type_for_class<TrackT> ());
          return;
        }

      /* set to -1 so other logic knows it is a new track */
      track->set_index (-1);

      /* this needs to be called before appending the track to the tracklist */
      track->set_name (*this, track->get_name (), false);

      /* append the track at the end */
      tracks_.emplace_back (track_id);
      track->set_selection_status_getter ([&] (const TrackUuid &id) {
        return TrackSelectionManager{ selected_tracks_, *track_registry_ }
          .is_selected (id);
      });
      track->tracklist_ = this;

      /* remember important tracks */
      if constexpr (std::is_same_v<TrackT, MasterTrack>)
        master_track_ = track;
      else if constexpr (std::is_same_v<TrackT, ChordTrack>)
        chord_track_ = track;
      else if constexpr (std::is_same_v<TrackT, MarkerTrack>)
        marker_track_ = track;
      else if constexpr (std::is_same_v<TrackT, TempoTrack>)
        tempo_track_ = track;
      else if constexpr (std::is_same_v<TrackT, ModulatorTrack>)
        modulator_track_ = track;

      /* add flags for auditioner track ports */
      if (is_auditioner ())
        {
          std::vector<Port *> ports;
          track->append_ports (ports, true);
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

      track->set_index (pos);

      if (
        is_in_active_project ()
        /* auditioner doesn't need automation */
        && !is_auditioner ())
        {
          /* make the track the only selected track */
          get_selection_manager ().select_unique (track->get_uuid ());

          /* set automation track on ports */
          if constexpr (std::derived_from<TrackT, AutomatableTrack>)
            {
              const auto &atl = track->get_automation_tracklist ();
              for (auto * at : atl.get_automation_tracks ())
                {
                  auto &port = at->get_port ();
                  port.at_ = at;
                }
            }
        }

      if constexpr (std::derived_from<TrackT, ChannelTrack>)
        {
          z_return_if_fail (port_connections_manager_);
          track->channel_->connect_channel (*port_connections_manager_, engine);
        }

      /* if audio output route to master */
      if constexpr (!std::is_same_v<TrackT, MasterTrack>)
        {
          if (
            track->get_output_signal_type () == dsp::PortType::Audio
            && master_track_)
            {
              master_track_->add_child (track->get_uuid (), true, false, false);
            }
        }

      if (is_in_active_project ())
        {
          track->activate_all_plugins (true);
        }

      if (!is_auditioner ())
        {
          /* verify */
          if (!track->validate ())
            {
              throw ZrythmException (
                fmt::format ("{} is invalid", track->get_name ()));
            }
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
        "done - inserted track '{}' ({}) at {}", track->get_name (),
        track->get_uuid (), pos);
    },
    track_var);
  endResetModel ();
  return track_var;
}

ChordTrack *
Tracklist::get_chord_track () const
{
  auto span = get_track_span ();
  return std::get<ChordTrack *> (
    *std::ranges::find_if (span, TrackSpan::type_projection<ChordTrack>));
}

bool
Tracklist::multiply_track_heights (
  double multiplier,
  bool   visible_only,
  bool   check_only,
  bool   fire_events)
{
  auto span = get_track_span ();
  for (const auto &track_var : span)
    {
      bool ret = std::visit (
        [&] (auto &&track) {
          if (visible_only && !track->should_be_visible ())
            return true;

          if (!track->multiply_heights (multiplier, visible_only, check_only))
            {
              return false;
            }

          if (!check_only && fire_events)
            {
              /* FIXME should be event */
              // track_widget_update_size (tr->widget_);
            }

          return true;
        },
        track_var);
      if (!ret)
        {
          return false;
        }
    }

  return true;
}

bool
Tracklist::validate () const
{
  /* this validates tracks in parallel */
  std::vector<std::future<bool>> ret_vals;
  ret_vals.reserve (tracks_.size ());
  auto span = get_track_span ();
  for (const auto [index, track_var] : std::views::enumerate (span))
    {
      ret_vals.emplace_back (std::async ([this, index, &track_var] () {
        // z_return_val_if_fail (track && track->is_in_active_project (), false);
        auto * tr = Track::from_variant (track_var);
        z_return_val_if_fail (tr, false);

        if (!tr->validate ())
          return false;

        if (tr->get_index () != index)
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
          tr->get_index () + track_size <= (int) tracks_.size (), false);

        /* validate connections */
        if (const auto * channel_track = dynamic_cast<ChannelTrack *> (tr))
          {
            const auto &channel = channel_track->get_channel ();
            for (const auto &send : channel->get_sends ())
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
  auto span =

    std::views::filter (
      get_track_span (),
      [&] (const auto &track_var) {
        const auto &tr = Track::from_variant (track_var);
        const bool  is_pinned = is_track_pinned (tr->get_index ());
        if (pin_opt == PinOption::PinnedOnly && !is_pinned)
          return false;
        if (pin_opt == PinOption::UnpinnedOnly && is_pinned)
          return false;
        if (visible_only && !tr->should_be_visible ())
          return false;

        return true;
      })
    | std::views::reverse | std::views::take (1);

  /* no track with given options found, select the last */
  if (span.empty ())
    return tracks_.size () - 1;

  return std::visit (
    [&] (const auto &track) { return track->get_index (); }, span.front ());
}

bool
Tracklist::is_in_active_project () const
{
  return project_ == PROJECT
         || (sample_processor_ && sample_processor_->is_in_active_project ());
}

std::optional<TrackPtrVariant>
Tracklist::get_visible_track_after_delta (Track::TrackUuid track_id, int delta)
  const
{
  auto span =
    get_track_span ()
    | std::views::filter (TrackRegistrySpan::visible_projection);
  auto current_it =
    std::ranges::find (span, track_id, TrackRegistrySpan::uuid_projection);
  auto found = std::ranges::next (current_it, delta);
  if (found == span.end ())
    return std::nullopt;
  return *(found);
}

std::optional<TrackPtrVariant>
Tracklist::get_first_visible_track (const bool pinned) const
{
  auto span = get_track_span ().get_visible_tracks ();
  auto it = std::ranges::find_if (span, [&] (const auto &track_var) {
    return std::visit (
      [&] (auto &&track) {
        return is_track_pinned (track->get_uuid ()) == pinned;
      },
      track_var);
  });
  return it != span.end () ? std::make_optional (*it) : std::nullopt;
}

std::optional<TrackPtrVariant>
Tracklist::get_prev_visible_track (TrackUuid track_id) const
{
  return get_visible_track_after_delta (track_id, -1);
}

std::optional<TrackPtrVariant>
Tracklist::get_next_visible_track (TrackUuid track_id) const
{
  return get_visible_track_after_delta (track_id, 1);
}

void
Tracklist::remove_track (const TrackUuid &track_id)
{
  auto track_it = std::ranges::find (tracks_, track_id, &TrackUuidReference::id);
  z_return_if_fail (track_it != tracks_.end ());
  const auto track_index = std::distance (tracks_.begin (), track_it);
  auto       span = get_track_span ();
  auto       track_var = span.at (track_index);
  std::visit (
    [&] (auto &&track) {
      z_return_if_fail (track->get_index () == track_index);
      z_debug (
        "removing [{}] {} - num tracks before deletion: {}", track_index,
        track->get_name (), tracks_.size ());

      beginRemoveRows ({}, track_index, track_index);

      std::optional<TrackPtrVariant> prev_visible = std::nullopt;
      std::optional<TrackPtrVariant> next_visible = std::nullopt;
      if (!is_auditioner ())
        {
          prev_visible = get_prev_visible_track (track_id);
          next_visible = get_next_visible_track (track_id);
        }

      /* remove/deselect all objects */
      track->clear_objects ();

      track->disconnect_track ();

      /* move track to the end */
      auto end_pos = std::ssize (tracks_) - 1;
      move_track (track_id, end_pos, false, std::nullopt);

      get_selection_manager ().remove_from_selection (track->get_uuid ());

      track_it = std::ranges::find (tracks_, track_id, &TrackUuidReference::id);
      z_return_if_fail (track_it != tracks_.end ());
      tracks_.erase (track_it);
      track->unset_selection_status_getter ();

      // recreate the span because underlying vector changed
      span = get_track_span ();

      /* if it was the only track selected, select the next one */
      if (get_selection_manager ().empty ())
        {
          auto track_to_select = next_visible ? next_visible : prev_visible;
          if (!track_to_select && !tracks_.empty ())
            {
              track_to_select = span.at (0);
            }
          if (track_to_select)
            {
              get_selection_manager ().append_to_selection (
                TrackSpan::uuid_projection (*track_to_select));
            }
        }

      endRemoveRows ();

      z_debug ("done removing track {}", track->getName ());
    },
    track_var);
}

void
Tracklist::clear_selections_for_object_siblings (
  const ArrangerObject::Uuid &object_id)
{
  auto obj_var = PROJECT->find_arranger_object_by_id (object_id);
  std::visit (
    [&] (auto &&obj) {
      using ObjT = base_type<decltype (obj)>;
      if constexpr (std::derived_from<ObjT, RegionOwnedObject>)
        {
          auto region = obj->get_region ();
          for (auto * child : region->get_children_view ())
            {
              auto selection_mgr =
                ArrangerObjectFactory::get_instance ()
                  ->get_selection_manager_for_object (*child);
              selection_mgr.remove_from_selection (child->get_uuid ());
            }
        }
      else
        {
          auto tl_objs = get_timeline_objects_in_range ();
          for (const auto &tl_obj_var : tl_objs)
            {
              std::visit (
                [&] (auto &&tl_obj) {
                  auto selection_mgr =
                    ArrangerObjectFactory::get_instance ()
                      ->get_selection_manager_for_object (*tl_obj);
                  selection_mgr.remove_from_selection (tl_obj->get_uuid ());
                },
                tl_obj_var);
            }
        }
    },
    *obj_var);
}

std::vector<ArrangerObjectPtrVariant>
Tracklist::get_timeline_objects_in_range (
  std::optional<std::pair<dsp::Position, dsp::Position>> range) const
{
  if (!range)
    {
      dsp::Position pos;
      pos.set_to_bar (
        dsp::Position::POSITION_MAX_BAR,
        PROJECT->getTransport ()->ticks_per_bar_,
        AUDIO_ENGINE->frames_per_tick_);
      range.emplace (dsp::Position{}, pos);
    }
  std::vector<ArrangerObjectPtrVariant> ret;
  for (const auto &track : get_track_span ())
    {
      std::vector<ArrangerObjectPtrVariant> objs;
      std::visit (
        [&] (auto &&tr) {
          tr->append_objects (objs);

          for (auto &obj_var : objs)
            {
              std::visit (
                [&] (auto &&o) {
                  using ObjT = base_type<decltype (o)>;
                  if constexpr (std::derived_from<ObjT, BoundedObject>)
                    {
                      if (o->is_hit_by_range (range->first, range->second))
                        {
                          ret.push_back (o);
                        }
                    }
                  else
                    {
                      if (o->is_start_hit_by_range (range->first, range->second))
                        {
                          ret.push_back (o);
                        }
                    }
                },
                obj_var);
            }
        },
        track);
    }
  return ret;
}

void
Tracklist::move_track (
  const TrackUuid                               track_id,
  int                                           pos,
  bool                                          always_before_pos,
  std::optional<std::reference_wrapper<Router>> router)
{
  auto       track_var = get_track (track_id);
  const auto track_index = get_track_index (track_id);

  std::visit (
    [&] (auto &&track) {
      z_return_if_fail (track_index == track->get_index ());
      z_debug (
        "moving track: {} from {} to {}", track->get_name (), track_index, pos);

      if (pos == track_index)
        return;

      beginMoveRows ({}, track_index, track_index, {}, pos);

      bool move_higher = pos < track_index;

      auto prev_visible = get_prev_visible_track (track_id);
      auto next_visible = get_next_visible_track (track_id);

      if (is_in_active_project () && !is_auditioner ())
        {
          /* clear the editor region if it exists and belongs to this track */
          CLIP_EDITOR->unset_region_if_belongs_to_track (track_id);

          /* deselect all objects */
          track->Track::unselect_all ();

          get_selection_manager ().remove_from_selection (track->get_uuid ());

          /* if it was the only track selected, select the next one */
          if (
            get_selection_manager ().empty () && (prev_visible || next_visible))
            {
              auto track_to_add = next_visible ? *next_visible : *prev_visible;
              get_selection_manager ().append_to_selection (
                TrackSpan::uuid_projection (track_to_add));
            }
        }

      /* the current implementation currently moves some tracks to tracks.size()
       * + 1 temporarily, so we expand the vector here and resize it back at the
       * end */
      bool expanded = false;
      if (pos >= static_cast<int> (tracks_.size ()))
        {
          tracks_.resize (pos + 1);
          expanded = true;
        }

      if (move_higher)
        {
          /* move all other tracks 1 track further */
          for (int i = track_index; i > pos; i--)
            {
              swap_tracks (i, i - 1);
            }
        }
      else
        {
          /* move all other tracks 1 track earlier */
          for (int i = track_index; i < pos; i++)
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

      /* make the track the only selected track */
      get_selection_manager ().select_unique (track_id);

      if (router)
        {
          router->get ().recalc_graph (false);
        }

      endMoveRows ();

      z_debug ("finished moving track");
    },
    *track_var);
}

bool
Tracklist::track_name_is_unique (
  const std::string &name,
  const TrackUuid    track_to_skip) const
{
  auto track_ids_to_check = std::ranges::to<std::vector> (std::views::filter (
    tracks_, [&] (const auto &id) { return id.id () != track_to_skip; }));
  return !TrackUuidReferenceSpan{ track_ids_to_check }.contains_track_name (
    name);
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
Tracklist::move_region_to_track (
  ArrangerObjectPtrVariant region_var,
  const Track::Uuid       &to_track_id,
  int                      lane_or_at_index,
  int                      index)
{
  auto to_track_var = get_track (to_track_id);
  assert (to_track_var);

  std::visit (
    [&] (auto &&to_track, auto &&region) {
      using RegionT = base_type<decltype (region)>;
      if constexpr (std::derived_from<RegionT, Region>)
        {
          auto from_track_var = *get_track (*region->get_track_id ());
          std::visit (
            [&] (auto &&region_track) {
              z_debug (
                "moving region {} to track {}", region->get_name (),
                to_track->get_name ());

              const bool selected = region->getSelected ();
              auto       clip_editor_region = CLIP_EDITOR->get_region ();

          // TODO remove region from owner

// TODO
#if 0
      int lane_pos = lane_or_at_index >= 0 ? lane_or_at_index : id_.lane_pos_;
      int at_pos = lane_or_at_index >= 0 ? lane_or_at_index : id_.at_idx_;

      if constexpr (is_automation ())
        {
          auto  automatable_track = dynamic_cast<AutomatableTrack *> (track);
          auto &at = automatable_track->automation_tracklist_->ats_[at_pos];

          /* convert the automation points to match the new automatable */
          auto port_var = PROJECT->find_port_by_id (at->port_id_);
          z_return_if_fail (
            port_var.has_value ()
            && std::holds_alternative<ControlPort *> (port_var.value ()));
          auto * port = std::get<ControlPort *> (port_var.value ());
          z_return_if_fail (port);
          for (auto &ap : get_derived ().aps_)
            {
              ap->fvalue_ = port->normalized_val_to_real (ap->normalized_val_);
            }

          /* add the region to its new track */
          try
            {
              if (index >= 0)
                {
                  track->insert_region (
                    shared_this, at, -1, index, false, false);
                }
              else
                {
                  track->add_region (shared_this, at, -1, false, false);
                }
            }
          catch (const ZrythmException &e)
            {
              e.handle ("Failed to add region to track");
              return;
            }

          z_warn_if_fail (id_.at_idx_ == at->index_);
          get_derived ().set_automation_track (*at);
        }
      else
        {
          if constexpr (is_laned ())
            {
              /* create lanes if they don't exist */
              auto laned_track = dynamic_cast<LanedTrackImpl<
                typename LaneOwnedObject<RegionT>::TrackLaneT> *> (track);
              z_return_if_fail (laned_track);
              laned_track->create_missing_lanes (lane_pos);
            }

          /* add the region to its new track */
          try
            {
              if (index >= 0)
                {
                  track->insert_region (
                    shared_this, nullptr, lane_pos, index, false, false);
                }
              else
                {
                  track->add_region (
                    shared_this, nullptr, lane_pos, false, false);
                }
            }
          catch (const ZrythmException &e)
            {
              e.handle ("Failed to add region to track");
              return;
            }

          z_return_if_fail (id_.lane_pos_ == lane_pos);

          if constexpr (is_laned ())
            {
              using TrackLaneT = typename LaneOwnedObject<RegionT>::TrackLaneT;
              auto laned_track =
                dynamic_cast<LanedTrackImpl<TrackLaneT> *> (track);
              auto lane =
                std::get<TrackLaneT *> (laned_track->lanes_.at (lane_pos));
              z_return_if_fail (
                !lane->region_list_->regions_.empty ()
                && std::get<RegionT *> (lane->region_list_->regions_.at (id_.idx_))
                     == dynamic_cast<RegionT *> (this));
              get_derived ().set_lane (*lane);

              laned_track->create_missing_lanes (lane_pos);

              if (region_track)
                {
                  /* remove empty lanes if the region was the last on its track
                   * lane
                   */
                  auto region_laned_track = dynamic_cast<LanedTrackImpl<
                    typename LaneOwnedObject<RegionT>::TrackLaneT> *> (
                    region_track);
                  region_laned_track->remove_empty_last_lanes ();
                }
            }

          if (link_group)
            {
              link_group->add_region (*this);
            }
        }
#endif

              /* reset the clip editor region because track_remove_region clears
               * it */
              if (
                clip_editor_region.has_value ()
                && std::holds_alternative<RegionT *> (
                  clip_editor_region.value ()))
                {
                  if (
                    std::get<RegionT *> (clip_editor_region.value ())
                    == dynamic_cast<RegionT *> (this))
                    {
                      {
                        CLIP_EDITOR->set_region (region->get_uuid ());
                      }
                    }
                }

              /* reselect if necessary */
              if (selected)
                {
                  ArrangerObjectFactory::get_instance ()
                    ->get_selection_manager_for_object (*region)
                    .append_to_selection (region->get_uuid ());
                }

              // z_debug ("after: {}", get_derived ());

              if (ZRYTHM_TESTING)
                {
                  REGION_LINK_GROUP_MANAGER.validate ();
                }
            },
            from_track_var);
        }
    },
    *to_track_var, region_var);
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
Tracklist::init_after_cloning (const Tracklist &other, ObjectCloneType clone_type)
{
  pinned_tracks_cutoff_ = other.pinned_tracks_cutoff_;
  track_registry_ = other.track_registry_;

  if (clone_type == ObjectCloneType::Snapshot)
    {
      tracks_ = other.tracks_;
      selected_tracks_ = other.selected_tracks_;
    }
  else if (clone_type == ObjectCloneType::NewIdentity)
    {
      tracks_.clear ();
      tracks_.reserve (other.tracks_.size ());
      auto span = other.get_track_span ();
      for (const auto &track_var : span)
        {
          std::visit (
            [&] (auto &tr) {
              auto id_ref = track_registry_->clone_object (
                *tr, *track_registry_, PROJECT->get_plugin_registry (),
                PROJECT->get_port_registry (),
                PROJECT->get_arranger_object_registry (), true);
              tracks_.push_back (id_ref);
            },
            track_var);
        }
    }
}

void
Tracklist::handle_click (TrackUuid track_id, bool ctrl, bool shift, bool dragged)
{
  const auto track_var_opt = get_track (track_id);
  z_return_if_fail (track_var_opt.has_value ());
  auto span = get_track_span ();
  auto selected_tracks =
    std::views::filter (span, TrackRegistrySpan::selected_projection)
    | std::ranges::to<std::vector> ();
  bool is_selected = get_selection_manager ().is_selected (track_id);
  if (is_selected)
    {
      if ((ctrl || shift) && !dragged)
        {
          if (get_selection_manager ().size () > 1)
            {
              get_selection_manager ().remove_from_selection (track_id);
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
          if (!selected_tracks.empty ())
            {
              TrackSpan selected_tracks_span{ selected_tracks };
              auto      highest_var = selected_tracks_span.get_first_track ();
              auto      lowest_var = selected_tracks_span.get_last_track ();
              std::visit (
                [&] (auto &&highest, auto &&lowest) {
                  const auto track_index = get_track_index (track_id);
                  const auto highest_index =
                    get_track_index (highest->get_uuid ());
                  const auto lowest_index =
                    get_track_index (lowest->get_uuid ());

                  if (track_index > highest_index)
                    {
                      /* select all tracks in between */
                      auto tracks_to_select = std::span (
                        tracks_.begin () + highest_index,
                        tracks_.begin () + track_index);
                      get_selection_manager ().select_only_these (
                        tracks_to_select
                        | std::views::transform ([&] (const auto &id_ref) {
                            return id_ref.id ();
                          }));
                    }
                  else if (track_index < lowest_index)
                    {
                      /* select all tracks in between */
                      auto tracks_to_select = std::span (
                        tracks_.begin () + track_index,
                        tracks_.begin () + lowest_index);
                      get_selection_manager ().select_only_these (
                        tracks_to_select
                        | std::views::transform ([&] (const auto &id_ref) {
                            return id_ref.id ();
                          }));
                    }
                },
                highest_var, lowest_var);
            }
        }
      else if (ctrl)
        {
          // append to selections
          get_selection_manager ().append_to_selection (track_id);
        }
      else
        {
          // select exclusively
          get_selection_manager ().select_unique (track_id);
        }
    }
}

void
Tracklist::define_fields (const utils::serialization::Context &ctx)
{
  using T = ISerializable<Tracklist>;
  T::serialize_fields (
    ctx, T::make_field ("pinnedTracksCutoff", pinned_tracks_cutoff_),
    T::make_field ("tracks", tracks_),
    T::make_field ("selectedTracks", selected_tracks_));
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
