// SPDX-FileCopyrightText: © 2019-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <typeinfo>

# include "gui/dsp/chord_track.h"
# include "gui/dsp/engine.h"
# include "gui/dsp/marker_track.h"
# include "gui/dsp/midi_event.h"
# include "dsp/position.h"
# include "gui/dsp/tempo_track.h"
# include "gui/dsp/track.h"
# include "gui/dsp/tracklist.h"
# include "gui/dsp/transport.h"
#include "utils/dsp.h"
#include "utils/math.h"
#include "utils/rt_thread_id.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/timeline_selections.h"
#include "gui/backend/backend/zrythm.h"

TimelineSelections::TimelineSelections (QObject * parent)
    : QObject (parent), ArrangerSelections (Type::Timeline)
{
}

TimelineSelections::TimelineSelections (
  const Position &start_pos,
  const Position &end_pos)
    : ArrangerSelections (ArrangerSelections::Type::Timeline)
{
  for (auto &track : TRACKLIST->tracks_)
    {
      std::vector<ArrangerObject *> objs;
      std::visit (
        [&] (auto &&tr) {
          tr->append_objects (objs);

          for (auto * obj : objs)
            {
              std::visit (
                [&] (auto &&o) {
                  using ObjT = base_type<decltype (o)>;
                  if constexpr (std::derived_from<ObjT, LengthableObject>)
                    {
                      if (o->is_hit_by_range (start_pos, end_pos))
                        {
                          add_object_owned (o->clone_unique ());
                        }
                    }
                  else
                    {
                      if (o->is_start_hit_by_range (start_pos, end_pos))
                        {
                          add_object_owned (o->clone_unique ());
                        }
                    }
                },
                convert_to_variant<ArrangerObjectWithoutVelocityPtrVariant> (
                  obj));
            }
        },
        track);
    }
}

void
TimelineSelections::sort_by_indices (bool desc)
{
  auto sort_regions = [] (const auto &a, const auto &b) {
    using AType = base_type<decltype (a)>;
    using BType = base_type<decltype (b)>;
    auto at_var = TRACKLIST->find_track_by_name_hash (a.id_.track_name_hash_);
    auto bt_var = TRACKLIST->find_track_by_name_hash (b.id_.track_name_hash_);
    z_return_val_if_fail (at_var, false);
    z_return_val_if_fail (bt_var, false);
    auto track_pos_compare_result = std::visit (
      [&] (auto &&track_a, auto &&track_b) -> std::optional<bool> {
        if (track_a->pos_ < track_b->pos_)
          return true;
        if (track_a->pos_ > track_b->pos_)
          return false;
        return std::nullopt;
      },
      *at_var, *bt_var);
    if (track_pos_compare_result)
      {
        return *track_pos_compare_result;
      }

    /* if one of the objects is laned and the other isn't, just return - order
     * doesn't matter in this case */
    if constexpr (
      std::derived_from<AType, LaneOwnedObject>
      != std::derived_from<BType, LaneOwnedObject>)
      return true;

    if constexpr (std::derived_from<AType, LaneOwnedObject>)
      {
        if (a.id_.lane_pos_ < b.id_.lane_pos_)
          {
            return true;
          }
        if (a.id_.lane_pos_ > b.id_.lane_pos_)
          {
            return false;
          }
      }
    else if constexpr (
      std::is_same_v<AType, AutomationRegion>
      && std::is_same_v<BType, AutomationRegion>)
      {
        if (a.id_.at_idx_ < b.id_.at_idx_)
          {
            return true;
          }
        if (a.id_.at_idx_ > b.id_.at_idx_)
          {
            return false;
          }
      }

    return a.id_.idx_ < b.id_.idx_;
  };

  std::sort (
    objects_.begin (), objects_.end (),
    [desc, sort_regions] (const auto &a_var, const auto &b_var) {
      bool ret = false;
      return std::visit (
        [&] (auto &&a, auto &&b) {
          using AType = base_type<decltype (a)>;
          using BType = base_type<decltype (b)>;
          if constexpr (std::is_same_v<AType, BType>)
            {
              if constexpr (std::derived_from<AType, Region>)
                {
                  ret = sort_regions (*a, *b);
                }
              else if constexpr (std::is_same_v<AType, ScaleObject>)
                {
                  ret = a->index_in_chord_track_ < b->index_in_chord_track_;
                }
              else if constexpr (std::is_same_v<AType, Marker>)
                {
                  ret = a->marker_track_index_ < b->marker_track_index_;
                }
            }
          else
            {
              ret =
                std::type_index (typeid (*a)) < std::type_index (typeid (*b));
            }
          return desc ? !ret : ret;
        },
        a_var, b_var);
    });
}

bool
TimelineSelections::contains_looped () const
{
  return std::any_of (
    objects_.begin (), objects_.end (), [] (const auto &obj_var) {
      return std::visit (
        [&] (auto &&obj) {
          using ObjT = base_type<decltype (obj)>;
          if constexpr (std::derived_from<ObjT, LoopableObject>)
            {
              return obj->is_looped ();
            }
          return false;
        },
        obj_var);
    });
}

bool
TimelineSelections::all_on_same_lane () const
{
  if (objects_.empty ())
    return true;

  auto first_obj_var = objects_.front ();
  return std::visit (
    [&] (auto &&first) {
      using FirstObjT = base_type<decltype (first)>;
      if constexpr (std::derived_from<FirstObjT, Region>)
        {
          const auto &id = first->id_;
          return std::all_of (
            objects_.begin () + 1, objects_.end (), [&] (const auto &obj_var) {
              return std::visit (
                [&] (auto &&curr) {
                  using CurObjT = base_type<decltype (curr)>;
                  if constexpr (std::derived_from<CurObjT, Region>)
                    {
                      if (id.type_ != curr->id_.type_)
                        return false;

                      if constexpr (std::derived_from<CurObjT, LaneOwnedObject>)
                        return id.track_name_hash_ == curr->id_.track_name_hash_
                               && id.lane_pos_ == curr->id_.lane_pos_;
                      else if constexpr (
                        std::is_same_v<CurObjT, AutomationRegion>)
                        return id.track_name_hash_ == curr->id_.track_name_hash_
                               && id.at_idx_ == curr->id_.at_idx_;
                      else
                        return true;
                    }
                  else
                    {
                      return false;
                    }
                },
                obj_var);
            });
        }
      else
        {
          return false;
        }
    },
    first_obj_var);
}

bool
TimelineSelections::can_be_merged () const
{
  return objects_.size () > 1 && all_on_same_lane () && !contains_looped ();
}

void
TimelineSelections::merge ()
{
  if (!all_on_same_lane ())
    {
      z_warning ("selections not on same lane");
      return;
    }

  auto ticks_length = get_length_in_ticks ();
  auto num_frames = static_cast<unsigned_frame_t> (
    ceil (AUDIO_ENGINE->frames_per_tick_ * ticks_length));
  auto [first_obj, pos] = get_first_object_and_pos (true);
  Position end_pos{ pos.ticks_ + ticks_length, AUDIO_ENGINE->frames_per_tick_ };

  z_return_if_fail (!first_obj || !first_obj->is_region ());

  std::visit (
    [&] (auto &&first_r) {
      using RegionT = base_type<decltype (first_r)>;

      RegionT * new_r;
      if constexpr (std::is_same_v<RegionT, MidiRegion>)
        {
          new_r = new MidiRegion (
            pos, end_pos, first_r->id_.track_name_hash_, first_r->id_.lane_pos_,
            first_r->id_.idx_);
          for (auto &obj : objects_)
            {
              auto * r = std::get<MidiRegion *> (obj);
              double ticks_diff = r->pos_->ticks_ - first_obj->pos_->ticks_;

              for (auto &mn : r->midi_notes_)
                {
                  auto new_mn = mn->clone_raw_ptr ();
                  new_mn->move (ticks_diff);
                  new_r->append_object (new_mn, false);
                }
            }
        }
      else if constexpr (std::is_same_v<RegionT, AudioRegion>)
        {
          auto               first_r_clip = first_r->get_clip ();
          utils::audio::AudioBuffer frames{
            first_r_clip->get_num_channels (), static_cast<int> (num_frames)
          };
          auto               max_depth = first_r_clip->get_bit_depth ();
          z_return_if_fail (!first_r_clip->get_name ().empty ());
          for (auto &obj : objects_)
            {
              auto * r = std::get<AudioRegion *> (obj);
              long   frames_diff = r->pos_->frames_ - first_obj->pos_->frames_;
              long   r_frames_length = r->get_length_in_frames ();

              auto * clip = r->get_clip ();
              for (int i = 0; i < frames.getNumChannels (); ++i)
                {
                  utils::float_ranges::add2 (
                    &frames.getWritePointer (i)[frames_diff],
                    clip->get_samples ().getReadPointer (i),
                    static_cast<size_t> (r_frames_length));
                }

              max_depth = std::max (max_depth, clip->get_bit_depth ());
            }

          new_r = new AudioRegion (
            frames, true, first_r_clip->get_name (), max_depth, pos,
            first_r->id_.track_name_hash_, first_r->id_.lane_pos_,
            first_r->id_.idx_);
        }
      else if constexpr (std::is_same_v<RegionT, ChordRegion>)
        {
          new_r = new ChordRegion (pos, end_pos, first_r->id_.idx_);
          for (auto &obj : objects_)
            {
              auto * r = std::get<ChordRegion *> (obj);
              double ticks_diff = r->pos_->ticks_ - first_obj->pos_->ticks_;

              for (auto &co : r->chord_objects_)
                {
                  auto new_co = co->clone_raw_ptr ();
                  new_co->move (ticks_diff);
                  new_r->append_object (new_co, false);
                }
            }
        }
      else if constexpr (std::is_same_v<RegionT, AutomationRegion>)
        {
          new_r = new AutomationRegion (
            pos, end_pos, first_r->id_.track_name_hash_, first_r->id_.at_idx_,
            first_r->id_.idx_);
          for (auto &obj : objects_)
            {
              auto * r = std::get<AutomationRegion *> (obj);
              double ticks_diff = r->pos_->ticks_ - first_obj->pos_->ticks_;

              for (auto &ap : r->aps_)
                {
                  auto * new_ap = ap->clone_raw_ptr ();
                  new_ap->move (ticks_diff);
                  new_r->append_object (new_ap, false);
                }
            }
        }

      new_r->gen_name (first_r->name_.c_str (), nullptr, nullptr);

      clear (false);
      add_object_owned (std::unique_ptr<RegionT> (new_r));
    },
    convert_to_variant<RegionPtrVariant> (first_obj));
}

OptionalTrackPtrVariant
TimelineSelections::get_first_track () const
{
  auto it = std::min_element (
    objects_.begin (), objects_.end (),
    [] (const auto &a_var, const auto &b_var) {
      return std::visit (
        [&] (auto &&a, auto &&b) {
          auto track_a_var = a->get_track ();
          auto track_b_var = b->get_track ();
          return std::visit (
            [&] (auto &&track_a, auto &&track_b) {
              return track_a && (!track_b || track_a->pos_ < track_b->pos_);
            },
            track_a_var, track_b_var);
        },
        a_var, b_var);
    });
  if (it == objects_.end ())
    return std::nullopt;
  return std::visit ([&] (auto &&obj) { return obj->get_track (); }, *it);
}

OptionalTrackPtrVariant
TimelineSelections::get_last_track () const
{
  auto it = std::max_element (
    objects_.begin (), objects_.end (),
    [] (const auto &a_var, const auto &b_var) {
      return std::visit (
        [&] (auto &&a, auto &&b) {
          auto track_a_var = a->get_track ();
          auto track_b_var = b->get_track ();
          return std::visit (
            [&] (auto &&track_a, auto &&track_b) {
              return !track_a || (track_b && track_a->pos_ < track_b->pos_);
            },
            track_a_var, track_b_var);
        },
        a_var, b_var);
    });
  if (it == objects_.end ())
    return std::nullopt;

  return std::visit ([] (auto &&obj) { return obj->get_track (); }, *it);
}

void
TimelineSelections::set_vis_track_indices ()
{
  auto highest_tr_var_opt = get_first_track ();
  if (!highest_tr_var_opt)
    return;

  std::ranges::for_each (objects_, [&] (auto &obj_var) {
    std::visit (
      [&] (auto &&obj, auto &&highest_tr) {
        using ObjT = base_type<decltype (obj)>;
        if constexpr (std::derived_from<ObjT, Region>)
          {
            auto region_track = obj->get_track ();
            std::visit (
              [&] (auto &&track) {
                region_track_vis_index_ =
                  TRACKLIST->get_visible_track_diff (*highest_tr, *track);
              },
              region_track);
          }
        else if constexpr (std::is_same_v<ObjT, Marker>)
          {
            marker_track_vis_index_ =
              TRACKLIST->get_visible_track_diff (*highest_tr, *P_MARKER_TRACK);
          }
        else if constexpr (std::is_same_v<ObjT, ChordObject>)
          {
            chord_track_vis_index_ =
              TRACKLIST->get_visible_track_diff (*highest_tr, *P_CHORD_TRACK);
          }
      },
      obj_var, *highest_tr_var_opt);
  });
}

bool
TimelineSelections::can_be_pasted_at_impl (const Position pos, const int idx) const
{
  auto tr_var =
    idx >= 0
      ? TRACKLIST->get_track (idx)
      : TRACKLIST_SELECTIONS->get_highest_track ();

  return std::ranges::all_of (objects_, [&] (auto &&obj_var) {
    return std::visit (
      [&] (auto &&obj, auto &&tr) {
        using ObjT = base_type<decltype (obj)>;
        using TrackT = base_type<decltype (tr)>;
        if constexpr (std::derived_from<ObjT, Region>)
          {
            // automation regions can't be copy-pasted this way
            if constexpr (std::is_same_v<ObjT, AutomationRegion>)
              return false;

            // check if this track can host this region
            if (!Track::type_can_host_region_type (tr->type_, obj->get_type ()))
              {
                z_info (
                  "track {} can't host region type {}", tr->get_name (),
                  obj->get_type ());
                return false;
              }
          }
        else if constexpr (
          std::is_same_v<ObjT, Marker> && !std::is_same_v<TrackT, MarkerTrack>)
          return false;
        else if constexpr (
          std::is_same_v<ObjT, ChordObject>
          && !std::is_same_v<TrackT, ChordTrack>)
          return false;
        return true;
      },
      obj_var, tr_var);
  });
}

void
TimelineSelections::mark_for_bounce (bool with_parents)
{
  AUDIO_ENGINE->reset_bounce_mode ();

  for (const auto &obj_var : objects_)
    {
      std::visit (
        [&] (auto &&obj) {
          using ObjT = base_type<decltype (obj)>;
          auto track_var = obj->get_track ();

          std::visit (
            [&] (auto &&track) {
              if (!with_parents)
                {
                  track->bounce_to_master_ = true;
                }
              track->mark_for_bounce (true, false, true, with_parents);
            },
            track_var);

          if constexpr (std::derived_from<ObjT, Region>)
            {
              obj->bounce_ = true;
            }
        },
        obj_var);
    }
}

bool
TimelineSelections::contains_clip (const AudioClip &clip) const
{
  return std::ranges::any_of (objects_, [&clip] (const auto &obj_var) {
    return std::visit (
      [&] (auto &&obj) {
        using ObjT = base_type<decltype (obj)>;
        if constexpr (std::is_same_v<ObjT, AudioRegion>)
          {
            return obj->pool_id_ == clip.get_pool_id ();
          }
        return false;
      },
      obj_var);
  });
}

bool
TimelineSelections::move_regions_to_new_lanes_or_tracks_or_ats (
  const int vis_track_diff,
  const int lane_diff,
  const int vis_at_diff)
{
  /* if nothing to do return */
  if (vis_track_diff == 0 && lane_diff == 0 && vis_at_diff == 0)
    return false;

  /* only 1 operation supported at once */
  if (vis_track_diff != 0)
    {
      z_return_val_if_fail (lane_diff == 0 && vis_at_diff == 0, false);
    }
  if (lane_diff != 0)
    {
      z_return_val_if_fail (vis_track_diff == 0 && vis_at_diff == 0, false);
    }
  if (vis_at_diff != 0)
    {
      z_return_val_if_fail (lane_diff == 0 && vis_track_diff == 0, false);
    }

  /* if there are objects other than regions, moving is not supported */
  if (std::ranges::any_of (objects_, [] (const auto &obj_var) {
        return std::visit (
          [&] (auto &&obj) { return !obj->is_region (); }, obj_var);
      }))
    {
      z_debug (
        "selection contains non-regions - skipping moving to another track/lane");
      return false;
    }

  /* if there are no objects do nothing */
  if (objects_.empty ())
    return false;

  sort_by_indices (false);

  /* store selected regions because they will be deselected during moving */
  std::vector<RegionPtrVariant> regions_arr;
  for (const auto &obj_var : objects_)
    {
      std::visit (
        [&] (auto &&obj) {
          using ObjT = base_type<decltype (obj)>;
          if constexpr (std::derived_from<ObjT, Region>)
            {
              regions_arr.push_back (obj);
            }
        },
        obj_var);
    }

  /*
   * for tracks, check that:
   * - all regions can be moved to a compatible track
   * for lanes, check that:
   * - all regions are in the same track
   * - only lane regions are selected
   * - the lane bounds are not exceeded
   */
  bool compatible = std::ranges::all_of (regions_arr, [&] (auto &&region_var) {
    return std::visit (
      [&] (auto &&region) {
        using RegionT = base_type<decltype (region)>;
        auto track_var = region->get_track ();
        return std::visit (
          [&] (auto &&track) {
            using TrackT = base_type<decltype (track)>;
            if (vis_track_diff != 0)
              {
                auto visible = TRACKLIST->get_visible_track_after_delta (
                  *track, vis_track_diff);
                if (!visible)
                  {
                    return false;
                  }

                bool track_incompatible = std::visit (
                  [&] (auto &&visible_tr) {
                    if (
                      !Track::type_is_compatible_for_moving (
                        track->type_, visible_tr->type_)
                      ||
                      /* do not allow moving automation tracks to other tracks
                       * for now
                       */
                      region->is_automation ())
                      {
                        return true;
                      }

                    return false;
                  },
                  *visible);
                if (track_incompatible)
                  return false;
              }
            else if (lane_diff != 0)
              {
                if constexpr (std::derived_from<TrackT, LanedTrack>)
                  {
                    track->block_auto_creation_and_deletion_ = true;
                    if (region->id_.lane_pos_ + lane_diff < 0)
                      {
                        return false;
                      }

                    /* don't create more than 1 extra lanes */
                    if constexpr (std::derived_from<RegionT, LaneOwnedObject>)
                      {
                        auto lane = region->get_lane ();
                        z_return_val_if_fail (region && lane, false);
                        int new_lane_pos = lane->pos_ + lane_diff;
                        z_return_val_if_fail (new_lane_pos >= 0, false);
                        if (new_lane_pos >= (int) track->lanes_.size ())
                          {
                            z_debug (
                              "new lane position %d is >= the number of lanes in the track (%d)",
                              new_lane_pos, track->lanes_.size ());
                            return false;
                          }
                        if (
                          new_lane_pos > track->last_lane_created_
                          && track->last_lane_created_ > 0 && lane_diff > 0)
                          {
                            z_debug (
                              "already created a new lane at %d, skipping new lane for %d",
                              track->last_lane_created_, new_lane_pos);
                            return false;
                          }

                        return true;
                      }
                    else
                      {
                        return false;
                      }
                  }
                else
                  {
                    return false;
                  }
              }
            else if (vis_at_diff != 0)
              {
                if (!region->is_automation ())
                  {
                    return false;
                  }

                /* don't allow moving automation regions -- too error prone */
                return false;
              }
            return true;
          },
          track_var);
      },
      region_var);
  });
  if (!compatible)
    {
      return false;
    }

  /* new positions are all compatible, move the regions */
  for (auto region_var : regions_arr)
    {
      auto success = std::visit (
        [&] (auto &&region) {
          using RegionT = base_type<decltype (region)>;
          if (vis_track_diff != 0)
            {
              auto region_track_var = region->get_track ();
              bool moved_successfully = std::visit (
                [&] (auto &&region_track) -> bool {
                  auto track_to_move_to =
                    TRACKLIST->get_visible_track_after_delta (
                      *region_track, vis_track_diff);
                  z_return_val_if_fail (track_to_move_to, false);
                  std::visit (
                    [&] (auto &&tr_to_move_to) {
                      region->move_to_track (tr_to_move_to, -1, -1);
                    },
                    *track_to_move_to);
                  return true;
                },
                region_track_var);
              if (!moved_successfully)
                {
                  return false;
                }
            }
          else if (lane_diff != 0)
            {
              if constexpr (std::derived_from<RegionT, LaneOwnedObject>)
                {
                  auto lane = region->get_lane ();
                  z_return_val_if_fail (lane, false);

                  int new_lane_pos = lane->pos_ + lane_diff;
                  z_return_val_if_fail (new_lane_pos >= 0, false);
                  auto laned_track = lane->get_track ();
                  bool new_lanes_created =
                    laned_track->create_missing_lanes (new_lane_pos);
                  if (new_lanes_created)
                    {
                      laned_track->last_lane_created_ = new_lane_pos;
                    }
                  region->move_to_track (laned_track, new_lane_pos, -1);
                }
            }
          else if (vis_at_diff != 0)
            {
              if constexpr (std::is_same_v<RegionT, AutomationRegion>)
                {
                  auto at = region->get_automation_track ();
                  z_return_val_if_fail (at, false);
                  auto              atl = at->get_automation_tracklist ();
                  AutomationTrack * new_at =
                    atl->get_visible_at_after_delta (*at, vis_at_diff);

                  if (at != new_at)
                    {
                      /* TODO */
                      z_warning ("!MOVING!");
                      /*automation_track_remove_region (at, region);*/
                      /*automation_track_add_region (new_at, region);*/
                    }
                }
            }
          return true;
        },
        region_var);

      if (!success)
        {
          z_warning (
            "failed to move region {}",
            std::visit (
              [&] (auto &&region) { return region->get_name (); }, region_var));
          return false;
        }
    }

  /* EVENTS_PUSH (EventType::ET_TRACK_LANES_VISIBILITY_CHANGED, nullptr); */

  return true;
}

bool
TimelineSelections::move_regions_to_new_ats (const int vis_at_diff)
{
  return move_regions_to_new_lanes_or_tracks_or_ats (0, 0, vis_at_diff);
}

bool
TimelineSelections::move_regions_to_new_lanes (const int diff)
{
  return move_regions_to_new_lanes_or_tracks_or_ats (0, diff, 0);
}

bool
TimelineSelections::move_regions_to_new_tracks (const int vis_track_diff)
{
  return move_regions_to_new_lanes_or_tracks_or_ats (vis_track_diff, 0, 0);
}

void
TimelineSelections::set_index_in_prev_lane ()
{
  for (auto &obj_var : objects_)
    {
      std::visit (
        [&] (auto &&obj) {
          using ObjT = base_type<decltype (obj)>;
          if constexpr (std::derived_from<ObjT, LaneOwnedObject>)
            obj->index_in_prev_lane_ = obj->id_.idx_;
        },
        obj_var);
    }
}

bool
TimelineSelections::contains_only_regions () const
{
  return std::all_of (
    objects_.begin (), objects_.end (), [] (const auto &obj_var) {
      return std::visit (
        [&] (auto &&obj) {
          using ObjT = base_type<decltype (obj)>;
          if constexpr (std::derived_from<ObjT, Region>)
            return true;
          return false;
        },
        obj_var);
    });
}

bool
TimelineSelections::contains_only_region_types (RegionType type) const
{
  return std::all_of (
    objects_.begin (), objects_.end (), [type] (const auto &obj_var) {
      return std::visit (
        [&] (auto &&obj) {
          using ObjT = base_type<decltype (obj)>;
          if constexpr (std::derived_from<ObjT, Region>)
            return obj->get_type () == type;
          return false;
        },
        obj_var);
    });
}

void
TimelineSelections::export_to_midi_file (
  const char * full_path,
  int          midi_version,
  bool         export_full_regions,
  bool         lanes_as_tracks) const
{
  auto mf = midiFileCreate (full_path, true);
  if (!mf)
    throw ZrythmException ("Failed to create MIDI file");

  /* write tempo info to track 1 */
  auto tempo_track = TRACKLIST->tempo_track_;
  midiSongAddTempo (mf, 1, static_cast<int> (tempo_track->get_current_bpm ()));

  /* all data is written out to tracks (not channels). we therefore set the
  current channel before writing data out. channel assignments can change any
  number of times during the file and affect all track messages until changed */
  midiFileSetTracksDefaultChannel (mf, 1, MIDI_CHANNEL_1);
  midiFileSetPPQN (mf, PositionProxy::TICKS_PER_QUARTER_NOTE);
  midiFileSetVersion (mf, midi_version);

  int beats_per_bar = tempo_track->get_beats_per_bar ();
  midiSongAddSimpleTimeSig (
    mf, 1, beats_per_bar,
    math_round_double_to_signed_32 (TRANSPORT->ticks_per_beat_));

  auto sel_clone = clone_unique ();
  sel_clone->sort_by_indices (false);

  int                              last_midi_track_pos = -1;
  std::unique_ptr<MidiEventVector> events;

  for (const auto &obj_var : sel_clone->objects_)
    {
      std::visit (
        [&] (auto &&obj) {
          using ObjT = base_type<decltype (obj)>;
          if constexpr (std::derived_from<ObjT, MidiRegion>)
            {

              int midi_track_pos = 1;
              if (midi_version > 0)
                {
                  auto track_var = obj->get_track ();

                  std::visit (
                    [&] (auto &&track) {
                      using TrackT = base_type<decltype (track)>;
                      if constexpr (std::derived_from<TrackT, PianoRollTrack>)
                        {
                          std::string midi_track_name;
                          if (lanes_as_tracks)
                            {
                              auto lane = obj->get_lane ();
                              z_return_if_fail (lane);
                              midi_track_name = fmt::format (
                                "{} - {}", track->name_, lane->name_);
                              midi_track_pos = lane->calculate_lane_idx ();
                            }
                          else
                            {
                              midi_track_name = track->name_;
                              midi_track_pos = track->pos_;
                            }
                          midiTrackAddText (
                            mf, midi_track_pos, textTrackName,
                            midi_track_name.c_str ());
                        }
                      else
                        {
                          throw ZrythmException ("Expected PianoRollTrack");
                        }
                    },
                    track_var);
                }

              if (last_midi_track_pos != midi_track_pos)
                {
                  /* finish prev events (if any) */
                  if (events)
                    {
                      events->write_to_midi_file (mf, last_midi_track_pos);
                    }

                  /* start new events */
                  events = std::make_unique<MidiEventVector> ();
                }

              /* append to the current events */
              obj->add_events (
                *events, nullptr, nullptr, true, export_full_regions);
              last_midi_track_pos = midi_track_pos;
            }
        },
        obj_var);
    }

  /* finish prev events (if any) again */
  if (events)
    {
      events->write_to_midi_file (mf, last_midi_track_pos);
    }

  midiFileClose (mf);
}

#if 0
/**
 * Gets index of the lowest track in the selections.
 *
 * Used during pasting.
 */
static int
get_lowest_track_pos (TimelineSelections * ts)
{
  int track_pos = INT_MAX;

#  define CHECK_POS(id) \
    { \
    }

  for (int i = 0; i < ts->num_regions; i++)
    {
      Region * r = ts->regions[i];
      Track *   tr = tracklist_find_track_by_name_hash (
          TRACKLIST, r->id.track_name_hash);
      if (tr->pos < track_pos)
        {
          track_pos = tr->pos;
        }
    }

  if (ts->num_scale_objects > 0)
    {
      if (ts->chord_track_vis_index < track_pos)
        track_pos = ts->chord_track_vis_index;
    }
  if (ts->num_markers > 0)
    {
      if (ts->marker_track_vis_index < track_pos)
        track_pos = ts->marker_track_vis_index;
    }

  return track_pos;
}
#endif
