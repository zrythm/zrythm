// SPDX-FileCopyrightText: © 2018-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/tracks/track_all.h"
#include "structure/tracks/tracklist.h"

namespace zrythm::structure::tracks
{

Tracklist::Tracklist (
  dsp::PortRegistry               &port_registry,
  dsp::ProcessorParameterRegistry &param_registry,
  TrackRegistry                   &track_registry,
  QObject *                        parent)
    : QAbstractListModel (parent), track_registry_ (track_registry),
      port_registry_ (port_registry), param_registry_ (param_registry),
      track_routing_ (
        utils::make_qobject_unique<TrackRouting> (track_registry, this)),
      track_selection_manager_ (
        std::make_unique<TrackSelectionManager> (
          selected_tracks_,
          track_registry_,
          [this] () { Q_EMIT selectedTracksChanged (); })),
      singleton_tracks_ (utils::make_qobject_unique<SingletonTracks> (this))
{
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
    "getting role {} for track {}", role, TrackSpan::name_projection (track));

  switch (role)
    {
    case TrackPtrRole:
      return QVariant::fromStdVariant (track);
    case TrackNameRole:
      return TrackSpan::name_projection (track).to_qstring ();
    default:
      return {};
    }
}

QVariant
Tracklist::selectedTrack () const
{
  if (selected_tracks_.empty ())
    return {};
  return QVariant::fromStdVariant (
    track_registry_.find_by_id_or_throw (*selected_tracks_.begin ()));
}

void
Tracklist::setExclusivelySelectedTrack (QVariant track)
{
  auto track_var = qvariantToStdVariant<TrackPtrVariant> (track);
  get_selection_manager ().select_unique (
    TrackSpan::uuid_projection (track_var));
}

// ========================================================================

std::optional<TrackUuidReference>
Tracklist::get_track_for_plugin (const plugins::Plugin::Uuid &plugin_id) const
{
  for (const auto &tr_ref : tracks_)
    {
      auto * tr = tracks::from_variant (tr_ref.get_object ());
      auto * channel = tr->channel ();
      if (channel == nullptr)
        continue;

      std::vector<plugins::PluginPtrVariant> plugins;
      channel->get_plugins (plugins);
      if (
        std::ranges::contains (
          plugins | std::views::transform (&plugins::plugin_ptr_variant_to_base),
          plugin_id, &plugins::Plugin::get_uuid))
        {
          return tr_ref;
        }
    }

  return std::nullopt;
}

void
Tracklist::mark_track_for_bounce (
  TrackPtrVariant track_var,
  bool            bounce,
  bool            mark_regions,
  bool            mark_children,
  bool            mark_parents)
{
// TODO
#if 0
  std::visit (
    [&] (auto &&track) {
      using TrackT = base_type<decltype (track)>;
      if constexpr (!std::derived_from<TrackT, ChannelTrack>)
        return;

      z_debug (
        "marking {} for bounce {}, mark regions {}", track->get_name (), bounce,
        mark_regions);

      track->bounce_ = bounce;

      if (mark_regions)
        {
          if constexpr (std::derived_from<TrackT, LanedTrack>)
            {
              for (auto &lane_var : track->lanes_)
                {
                  using LaneT = TrackT::TrackLaneType;
                  auto lane = std::get<LaneT *> (lane_var);
                  for (auto * region : lane->get_children_view ())
                    {
                      region->bounce_ = bounce;
                    }
                }
            }

          if constexpr (std::is_same_v<TrackT, ChordTrack>)
            for (
              auto * region :
              track->arrangement::template ArrangerObjectOwner<
                arrangement::ChordRegion>::get_children_view ())
              {
                region->bounce_ = bounce;
              }
        }

      if constexpr (std::derived_from<TrackT, ChannelTrack>)
        {
          auto * direct_out = track->channel ()->get_output_track ();
          if (direct_out && mark_parents)
            {
              std::visit (
                [&] (auto &&direct_out_derived) {
                  mark_track_for_bounce (
                    direct_out_derived, bounce, false, false, true);
                },
                convert_to_variant<TrackPtrVariant> (direct_out));
            }
        }

      if (mark_children)
        {
          if constexpr (std::derived_from<TrackT, GroupTargetTrack>)
            {
              for (auto child_id : track->children_)
                {
                  if (
                    auto child_var =
                      track->get_tracklist ()->get_track (child_id))
                    {
                      std::visit (
                        [&] (auto &&c) {
                          c->bounce_to_master_ = track->bounce_to_master_;
                          mark_track_for_bounce (
                            c, bounce, mark_regions, true, false);
                        },
                        *child_var);
                    }
                }
            }
        }
    },
    track_var);
#endif
}

int
Tracklist::get_visible_track_diff (Track::Uuid src_track, Track::Uuid dest_track)
  const
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
    span.begin () + lower, span.begin () + upper, [this] (const auto &track_var) {
      return should_be_visible (TrackSpan::uuid_projection (track_var));
    });

  // Apply direction modifier
  return sign * static_cast<int> (count);
}

void
Tracklist::swap_tracks (const size_t index1, const size_t index2)
{
  z_return_if_fail (std::max (index1, index2) < tracks_.size ());
  AtomicBoolRAII raii{ swapping_tracks_ };

  {
    auto        span = get_track_span ();
    const auto &src_track = from_variant (span.at (index1));
    const auto &dest_track = from_variant (span.at (index2));
    z_debug (
      "swapping tracks {} [{}] and {} [{}]...",
      src_track ? src_track->get_name () : u8"(none)", index1,
      dest_track ? dest_track->get_name () : u8"(none)", index2);
  }

  std::iter_swap (
    std::ranges::next (tracks_.begin (), static_cast<int> (index1)),
    std::ranges::next (tracks_.begin (), static_cast<int> (index2)));

  {
    // auto        span = get_track_span ();
    // const auto &src_track = Track::from_variant (span.at (index1));
    // const auto &dest_track = Track::from_variant (span.at (index2));

    // if (src_track)
    //   src_track->set_index (index1);
    // if (dest_track)
    //   dest_track->set_index (index2);
  }

  z_debug ("tracks swapped");
}

std::optional<arrangement::ArrangerObjectPtrVariant>
Tracklist::get_region_at_pos (
  signed_frame_t            pos_samples,
  Track *                   track,
  tracks::AutomationTrack * at,
  bool                      include_region_end)
{
  auto is_at_pos = [&] (const auto &region_var) -> bool {
    return std::visit (
      [&] (auto &&region) -> bool {
        using RegionT = base_type<decltype (region)>;
        if constexpr (arrangement::RegionObject<RegionT>)
          {
            const auto region_pos = region->position ()->samples ();
            const auto region_end_pos =
              region->regionMixin ()->bounds ()->get_end_position_samples (true);
            return region_pos <= pos_samples
                   && (include_region_end ? region_end_pos >= pos_samples : region_end_pos > pos_samples);
          }
        else
          {
            throw std::runtime_error ("expected region");
          }
      },
      region_var.get_object ());
  };

  if (track != nullptr)
    {
      return std::visit (
        [&] (auto &&track_derived) -> std::optional<ArrangerObjectPtrVariant> {
          using TrackT = base_type<decltype (track_derived)>;
          if (track_derived->lanes ())
            {
// TODO
#if 0
              for (const auto &lane : track_derived->lanes ()->lanes_view ())
                {
                  auto ret_var =
                    arrangement::ArrangerObjectSpan{ lane->get_children_vector () }
                      .get_bounded_object_at_position (
                        pos_samples, include_region_end);
                  if (ret_var)
                    return std::get<typename TrackLaneT::RegionT *> (*ret_var);

                  auto region_vars = lane->get_children_vector ();
                  auto it = std::ranges::find_if (region_vars, is_at_pos);
                  if (it != region_vars.end ())
                    {
                      return (*it).get_object ();
                    }
                  }
#endif
            }
          if constexpr (std::is_same_v<TrackT, tracks::ChordTrack>)
            {
              auto region_vars = track_derived->template ArrangerObjectOwner<
                arrangement::ChordRegion>::get_children_vector ();
              auto it = std::ranges::find_if (region_vars, is_at_pos);
              if (it != region_vars.end ())
                {
                  return (*it).get_object ();
                }
            }
          return std::nullopt;
        },
        convert_to_variant<TrackPtrVariant> (track));
    }
  if (at != nullptr)
    {
      auto region_vars = at->get_children_vector ();
      auto it = std::ranges::find_if (region_vars, is_at_pos);
      if (it != region_vars.end ())
        {
          return (*it).get_object ();
        }
      return std::nullopt;
    }
  z_return_val_if_reached (std::nullopt);
}

TrackPtrVariant
Tracklist::insert_track (const TrackUuidReference &track_id, int pos)
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
      // track->set_index (-1);

      /* this needs to be called before appending the track to the tracklist */
      track->setName (
        get_unique_name_for_track (track->get_uuid (), track->get_name ())
          .to_qstring ());

      /* append the track at the end */
      tracks_.emplace_back (track_id);
      track->set_selection_status_getter ([&] (const TrackUuid &id) {
        return TrackSelectionManager{ selected_tracks_, track_registry_ }
          .is_selected (id);
      });
      // track->tracklist_ = this;

      /* remember important tracks */
      if constexpr (std::is_same_v<TrackT, MasterTrack>)
        singleton_tracks_->master_track_ = track;
      else if constexpr (std::is_same_v<TrackT, ChordTrack>)
        singleton_tracks_->chord_track_ = track;
      else if constexpr (std::is_same_v<TrackT, MarkerTrack>)
        singleton_tracks_->marker_track_ = track;
      else if constexpr (std::is_same_v<TrackT, ModulatorTrack>)
        singleton_tracks_->modulator_track_ = track;

      /* if inserting it, swap until it reaches its position */
      if (static_cast<size_t> (pos) != tracks_.size () - 1)
        {
          for (int i = static_cast<int> (tracks_.size ()) - 1; i > pos; --i)
            {
              swap_tracks (i, i - 1);
            }
        }

      // track->set_index (pos);

      /* make the track the only selected track */
      get_selection_manager ().select_unique (track->get_uuid ());

      /* if audio output route to master */
      if constexpr (!std::is_same_v<TrackT, MasterTrack>)
        {
          if (
            track->get_output_signal_type () == dsp::PortType::Audio
            && singleton_tracks_->masterTrack ())
            {
              track_routing_->add_or_replace_route (
                track->get_uuid (),
                singleton_tracks_->masterTrack ()->get_uuid ());
            }
        }

      z_debug (
        "done - inserted track '{}' ({}) at {}", track->get_name (),
        track->get_uuid (), pos);
    },
    track_var);
  endResetModel ();
  return track_var;
}

#if 0
ChordTrack *
Tracklist::get_chord_track () const
{
  auto span = get_track_span ();
  return std::get<ChordTrack *> (
    *std::ranges::find_if (span, TrackSpan::type_projection<ChordTrack>));
}
#endif

bool
Tracklist::should_be_visible (const Track::Uuid &track_id) const
{
  auto track = from_variant (get_track (track_id).value ());
  if (!track->visible ())
    return false;

  return true;
// TODO
#if 0
  std::vector<FoldableTrack *> parents;
  add_folder_parents (track_id, parents, false);
  return std::ranges::all_of (parents, [] (const auto &parent) {
    return parent->visible_ && !parent->folded_;
  });
#endif
}

// TODO
#if 0
void
Tracklist::add_folder_parents (
  const Track::Uuid            &track_id,
  std::vector<FoldableTrack *> &parents,
  bool                          prepend) const
{
  for (
    const auto &cur_track_var :
    get_track_span ()
      | std::views::filter (
        TrackSpan::derived_from_type_projection<FoldableTrack>))
    {
      std::visit (
        [&] (const auto &cur_track) {
          using TrackT = base_type<decltype (cur_track)>;
          if constexpr (std::derived_from<TrackT, FoldableTrack>)
            {
              /* last position covered by the foldable track cur_track */
              int last_covered_pos = cur_track->pos_ + (cur_track->size_ - 1);

              if (cur_track->pos_ < pos_ && pos_ <= last_covered_pos)
                {
                  if (prepend)
                    {
                      parents.insert (parents.begin (), cur_track);
                    }
                  else
                    {
                      parents.push_back (cur_track);
                    }
                }
            }
        },
        cur_track_var);
    }
  }

void
Tracklist::remove_from_folder_parents (const Track::Uuid &track_id)
{
  std::vector<FoldableTrack *> parents;
  add_folder_parents (track_id, parents, false);
  for (auto parent : parents)
    {
      parent->size_--;
    }
}
#endif

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
          if (visible_only && !should_be_visible (track->get_uuid ()))
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

std::optional<TrackPtrVariant>
Tracklist::get_visible_track_after_delta (Track::Uuid track_id, int delta) const
{
  // TODO
  return std::nullopt;
#if 0
  auto span =
    get_track_span () | std::views::filter (TrackSpan::visible_projection);
  auto current_it =
    std::ranges::find (span, track_id, TrackSpan::uuid_projection);
  auto found = std::ranges::next (current_it, delta);
  if (found == span.end ())
    return std::nullopt;
  return *(found);
#endif
}

std::optional<TrackPtrVariant>
Tracklist::get_first_visible_track (const bool pinned) const
{
  // TODO
  return std::nullopt;
#if 0
  auto span = get_track_span ().get_visible_tracks ();
  auto it = std::ranges::find_if (span, [&] (const auto &track_var) {
    return std::visit (
      [&] (auto &&track) {
        return is_track_pinned (track->get_uuid ()) == pinned;
      },
      track_var);
  });
  return it != span.end () ? std::make_optional (*it) : std::nullopt;
#endif
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

utils::Utf8String
Tracklist::get_unique_name_for_track (
  const Track::Uuid       &track_to_skip,
  const utils::Utf8String &name) const
{
  auto new_name = name;
  while (!track_name_is_unique (new_name, track_to_skip))
    {
      auto [ending_num, name_without_num] = new_name.get_int_after_last_space ();
      if (ending_num == -1)
        {
          new_name += u8" 1";
        }
      else
        {
          new_name = utils::Utf8String::from_utf8_encoded_string (
            fmt::format ("{} {}", name_without_num, ending_num + 1));
        }
    }
  return new_name;
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
      z_debug (
        "removing [{}] {} - num tracks before deletion: {}", track_index,
        track->get_name (), tracks_.size ());

      beginRemoveRows (
        {}, static_cast<int> (track_index), static_cast<int> (track_index));

      std::optional<TrackPtrVariant> prev_visible = std::nullopt;
      std::optional<TrackPtrVariant> next_visible = std::nullopt;
      prev_visible = get_prev_visible_track (track_id);
      next_visible = get_next_visible_track (track_id);

      /* move track to the end */
      const auto end_pos = static_cast<int> (std::ssize (tracks_) - 1);
      move_track (track_id, end_pos, false);

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

      z_debug ("done removing track {}", track->name ());
    },
    track_var);
}

void
Tracklist::clear_selections_for_object_siblings (
  const ArrangerObject::Uuid &object_id)
{
// TODO
#if 0
  auto obj_var = PROJECT->find_arranger_object_by_id (object_id);
  std::visit (
    [&] (auto &&obj) {
      using ObjT = base_type<decltype (obj)>;
      if constexpr (std::derived_from<ObjT, arrangement::RegionOwnedObject>)
        {
          auto region = obj->get_region ();
          for (auto * child : region->get_children_view ())
            {
              auto selection_mgr =
                arrangement::ArrangerObjectFactory::get_instance ()
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
                    arrangement::ArrangerObjectFactory::get_instance ()
                      ->get_selection_manager_for_object (*tl_obj);
                  selection_mgr.remove_from_selection (tl_obj->get_uuid ());
                },
                tl_obj_var);
            }
        }
    },
    *obj_var);
#endif
}

std::vector<arrangement::ArrangerObjectPtrVariant>
Tracklist::get_timeline_objects () const
{
  std::vector<ArrangerObjectPtrVariant> ret;
// TODO
#if 0
  for (const auto &track : get_track_span ())
    {
      std::vector<ArrangerObjectPtrVariant> objs;
      std::visit (
        [&] (auto &&tr) {
          tr->collect_timeline_objects (objs);

          for (auto &obj_var : objs)
            {
              std::visit (
                [&] (auto &&o) {
                  using ObjT = base_type<decltype (o)>;
                  if constexpr (arrangement::BoundedObject<ObjT>)
                    {
                      if (arrangement::ArrangerObjectSpan::bounds_projection (o)
                            ->is_hit_by_range (range->first, range->second))
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
#endif
  return ret;
}

void
Tracklist::move_track (const TrackUuid track_id, int pos, bool always_before_pos)
{
  auto       track_var = get_track (track_id);
  const auto track_index = get_track_index (track_id);

  std::visit (
    [&] (auto &&track) {
      z_debug (
        "moving track: {} from {} to {}", track->get_name (), track_index, pos);

      if (pos == track_index)
        return;

      beginMoveRows (
        {}, static_cast<int> (track_index), static_cast<int> (track_index), {},
        pos);

      bool move_higher = pos < track_index;

      auto prev_visible = get_prev_visible_track (track_id);
      auto next_visible = get_next_visible_track (track_id);

      {
        /* clear the editor region if it exists and belongs to this track */
        // CLIP_EDITOR->unset_region_if_belongs_to_track (track_id);

        /* deselect all objects */
        // TODO
        // track->Track::unselect_all ();

        get_selection_manager ().remove_from_selection (track->get_uuid ());

        /* if it was the only track selected, select the next one */
        if (get_selection_manager ().empty () && (prev_visible || next_visible))
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
          tracks_.emplace_back (track_registry_);
          // tracks_.resize (pos + 1);
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
          tracks_.erase (tracks_.end () - 1);
        }

      /* make the track the only selected track */
      get_selection_manager ().select_unique (track_id);

      endMoveRows ();

      z_debug ("finished moving track");
    },
    *track_var);
}

bool
Tracklist::track_name_is_unique (
  const utils::Utf8String &name,
  const TrackUuid          track_to_skip) const
{
  auto track_ids_to_check = std::ranges::to<std::vector> (std::views::filter (
    tracks_, [&] (const auto &id) { return id.id () != track_to_skip; }));
  return !TrackSpan{ track_ids_to_check }.contains_track_name (name);
}

void
Tracklist::move_region_to_track (
  ArrangerObjectPtrVariant region_var,
  const Track::Uuid       &to_track_id,
  int                      lane_or_at_index,
  int                      index)
{
#if 0
  // TODO
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
#  if 0
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
#  endif

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
                  structure::arrangement::ArrangerObjectFactory::get_instance ()
                    ->get_selection_manager_for_object (*region)
                    .append_to_selection (region->get_uuid ());
                }

              // z_debug ("after: {}", get_derived ());
            },
            from_track_var);
        }
    },
    *to_track_var, region_var);
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
init_from (
  Tracklist             &obj,
  const Tracklist       &other,
  utils::ObjectCloneType clone_type)
{
  obj.pinned_tracks_cutoff_ = other.pinned_tracks_cutoff_;

  if (clone_type == utils::ObjectCloneType::Snapshot)
    {
      obj.tracks_ = other.tracks_;
      obj.selected_tracks_ = other.selected_tracks_;
    }
  else if (clone_type == utils::ObjectCloneType::NewIdentity)
    {
// TODO/delete
#if 0
      obj.tracks_.clear ();
      obj.tracks_.reserve (other.tracks_.size ());
      auto span = other.get_track_span ();
      for (const auto &track_var : span)
        {
          std::visit (
            [&] (auto &tr) {
              auto id_ref = obj.track_registry_->clone_object (
                *tr, PROJECT->get_file_audio_source_registry (),
                *obj.track_registry_, PROJECT->get_plugin_registry (),
                PROJECT->get_port_registry (), PROJECT->get_param_registry (),
                PROJECT->get_arranger_object_registry (), true);
              obj.tracks_.push_back (id_ref);
            },
            track_var);
        }
#endif
    }
}

void
Tracklist::handle_click (TrackUuid track_id, bool ctrl, bool shift, bool dragged)
{
  const auto track_var_opt = get_track (track_id);
  z_return_if_fail (track_var_opt.has_value ());
  auto span = get_track_span ();
  auto selected_tracks =
    std::views::filter (span, TrackSpan::selected_projection)
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
// TODO
#if 0
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
#endif
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
from_json (const nlohmann::json &j, Tracklist &t)
{
  j.at (Tracklist::kPinnedTracksCutoffKey).get_to (t.pinned_tracks_cutoff_);
  // TODO
  // j.at (Tracklist::kTracksKey).get_to (t.tracks_);
  j.at (Tracklist::kSelectedTracksKey).get_to (t.selected_tracks_);
}
}
