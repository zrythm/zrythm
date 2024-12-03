// SPDX-FileCopyrightText: © 2018-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/stretcher.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"
#include "gui/backend/channel.h"
#include "gui/dsp/audio_lane.h"
#include "gui/dsp/audio_region.h"
#include "gui/dsp/automation_region.h"
#include "gui/dsp/chord_region.h"
#include "gui/dsp/chord_track.h"
#include "gui/dsp/clip.h"
#include "gui/dsp/control_port.h"
#include "gui/dsp/instrument_track.h"
#include "gui/dsp/lane_owned_object.h"
#include "gui/dsp/midi_lane.h"
#include "gui/dsp/midi_note.h"
#include "gui/dsp/midi_region.h"
#include "gui/dsp/pool.h"
#include "gui/dsp/recording_manager.h"
#include "gui/dsp/region.h"
#include "gui/dsp/region_link_group_manager.h"
#include "gui/dsp/router.h"
#include "gui/dsp/track.h"
#include "gui/dsp/tracklist.h"
#include "utils/debug.h"
#include "utils/gtest_wrapper.h"
#include "utils/logger.h"
#include "utils/rt_thread_id.h"

using namespace zrythm;

void
Region::copy_members_from (const Region &other)
{
  id_ = other.id_;
  bounce_ = other.bounce_;
}

void
Region::init (
  const dsp::Position &start_pos,
  const dsp::Position &end_pos,
  unsigned int         track_name_hash,
  int                  lane_pos_or_at_idx,
  int                  idx_inside_lane_or_at,
  double               ticks_per_frame)
{
  type_ = Type::Region;

  id_.track_name_hash_ = track_name_hash;
  if (is_automation ())
    {
      id_.at_idx_ = lane_pos_or_at_idx;
    }
  else if (region_type_has_lane (id_.type_))
    {
      id_.lane_pos_ = lane_pos_or_at_idx;
    }
  id_.idx_ = idx_inside_lane_or_at;

  *static_cast<Position *> (pos_) = start_pos;
  *static_cast<Position *> (end_pos_) = end_pos;
  long length = get_length_in_frames ();
  z_return_if_fail (length > 0);
  loop_end_pos_.from_frames (length, ticks_per_frame);

  if (region_type_can_fade (id_.type_))
    {
      auto fo = dynamic_cast<FadeableObject *> (this);
      /* set fade positions to start/end */
      fo->fade_out_pos_.from_frames (length, ticks_per_frame);
    }
}

void
Region::gen_name (const char * base_name, AutomationTrack * at, Track * track)
{
  /* Name to try to assign */
  std::string orig_name;
  if (base_name)
    orig_name = base_name;
  else if (at)
    orig_name =
      fmt::format ("{} - {}", track->name_, at->port_id_->get_label ());
  else
    orig_name = track->name_;

  set_name (orig_name, false);
}

void
Region::post_deserialize_children ()
{
  if (is_midi ())
    {
      auto mr = dynamic_cast<MidiRegion *> (this);
      for (auto &mn : mr->midi_notes_)
        {
          mn->post_deserialize ();
        }
    }
  else if (is_automation ())
    {
      auto ar = dynamic_cast<AutomationRegion *> (this);
      for (auto &ap : ar->aps_)
        {
          ap->post_deserialize ();
        }
    }
  else if (is_chord ())
    {
      auto cr = dynamic_cast<ChordRegion *> (this);
      for (auto &co : cr->chord_objects_)
        {
          co->post_deserialize ();
        }
    }
}

template <typename RegionT>
void
RegionImpl<RegionT>::send_note_offs (
  MidiEventVector            &midi_events,
  const EngineProcessTimeInfo time_nfo,
  bool                        is_note_off_for_loop_or_region_end) const
  requires RegionTypeWithMidiEvents<RegionT>
{
  auto        region = dynamic_cast<const RegionT *> (this);
  midi_byte_t channel = 1;
  if constexpr (is_midi ())
    {
      channel = region->get_midi_ch ();
    }
  else if constexpr (is_chord ())
    {
      /* FIXME set channel */
      // auto cr = dynamic_cast<ChordRegion *> (this);
    }

  /* -1 to send event 1 sample before the end point */
  const auto midi_time_for_note_off =
    (midi_time_t) ((time_nfo.local_offset_ + time_nfo.nframes_) - 1);
  const signed_frame_t frame_for_note_off =
    (signed_frame_t) (time_nfo.g_start_frame_w_offset_ + time_nfo.nframes_) - 1;
  if (is_note_off_for_loop_or_region_end && is_midi ())
    {
      if constexpr (is_midi ())
        {
          for (const auto &mn : region->midi_notes_)
            {
              if (mn->is_hit (frame_for_note_off))
                {
                  midi_events.add_note_off (
                    channel, mn->val_, midi_time_for_note_off);
                }
            }
        }
    }
  else
    {
      midi_events.add_all_notes_off (channel, midi_time_for_note_off, true);
    }
}

template <typename RegionT>
void
RegionImpl<RegionT>::fill_midi_events (
  const EngineProcessTimeInfo &time_nfo,
  bool                         note_off_at_end,
  bool                         is_note_off_for_loop_or_region_end,
  MidiEventVector             &midi_events) const
  requires RegionTypeWithMidiEvents<RegionT>
{
  std::visit (
    [&] (auto &&track) {
      auto r = dynamic_cast<const RegionT *> (this);

      /* send note offs if needed */
      if (note_off_at_end)
        {
          send_note_offs (
            midi_events, time_nfo, is_note_off_for_loop_or_region_end);
        }

      const auto r_local_pos = timeline_frames_to_local (
        (signed_frame_t) time_nfo.g_start_frame_w_offset_, true);

      auto process_object = [&]<typename ObjectType> (const ObjectType &obj) {
        if (obj.get_muted (false))
          return;

        ChordDescriptor * descr = nullptr;
        if constexpr (std::is_same_v<ObjectType, ChordObject>)
          {
            descr = obj.get_chord_descriptor ();
          }

        /* if object starts inside the current range */
        if (
          obj.pos_->frames_ >= 0 && obj.pos_->frames_ >= r_local_pos
          && obj.pos_->frames_ < r_local_pos + (signed_frame_t) time_nfo.nframes_)
          {
            auto _time =
              (midi_time_t) (time_nfo.local_offset_
                             + (obj.pos_->frames_ - r_local_pos));

            if constexpr (std::is_same_v<RegionT, MidiRegion>)
              {
                midi_events.add_note_on (
                  r->get_midi_ch (), obj.val_, obj.vel_->vel_, _time);
              }
            else if constexpr (std::is_same_v<ObjectType, ChordObject>)
              {
                midi_events.add_note_ons_from_chord_descr (
                  *descr, 1, VELOCITY_DEFAULT, _time);
              }
          }

        signed_frame_t obj_end_frames = 0;
        if constexpr (std::is_same_v<ObjectType, MidiNote>)
          {
            obj_end_frames = obj.end_pos_->frames_;
          }
        else if constexpr (std::is_same_v<ObjectType, ChordObject>)
          {
            obj_end_frames = math_round_double_to_signed_frame_t (
              obj.pos_->frames_
              + TRANSPORT->ticks_per_beat_ * AUDIO_ENGINE->frames_per_tick_);
          }

        /* if note ends within the cycle */
        if (
          obj_end_frames >= r_local_pos
          && (obj_end_frames <= (r_local_pos + time_nfo.nframes_)))
          {
            auto _time =
              (midi_time_t) (time_nfo.local_offset_
                             + (obj_end_frames - r_local_pos));

            /* note actually ends 1 frame before the end point, not at the end
             * point */
            if (_time > 0)
              {
                _time--;
              }

            if constexpr (std::is_same_v<RegionT, MidiRegion>)
              {
                midi_events.add_note_off (r->get_midi_ch (), obj.val_, _time);
              }
            else if constexpr (std::is_same_v<ObjectType, ChordObject>)
              {
                for (int l = 0; l < CHORD_DESCRIPTOR_MAX_NOTES; l++)
                  {
                    if (descr->notes_[l])
                      {
                        midi_events.add_note_off (1, l + 36, _time);
                      }
                  }
              }
          }
      };

      /* process each object */
      if constexpr (std::is_same_v<RegionT, MidiRegion>)
        {
          const auto &mr = dynamic_cast<const MidiRegion &> (*this);
          for (const auto &mn : mr.midi_notes_)
            {
              process_object (*mn);
            }
        }
      else if constexpr (std::is_same_v<RegionT, ChordRegion>)
        {
          auto cr = dynamic_cast<const ChordRegion *> (this);
          for (const auto &co : cr->chord_objects_)
            {
              process_object (*co);
            }
        }
    },
    get_track ());
}

template <typename RegionT>
bool
RegionImpl<RegionT>::get_muted (bool check_parent) const
{
  if (check_parent)
    {
      if constexpr (is_laned ())
        {
          auto lane_owned_obj =
            dynamic_cast<const LaneOwnedObjectImpl<RegionT> *> (this);
          auto lane = lane_owned_obj->get_lane ();
          z_return_val_if_fail (lane, true);
          if (lane->is_effectively_muted ())
            return true;
        }
    }
  return muted_;
}

template <typename RegionT>
std::span<typename RegionImpl<RegionT>::ChildTPtr>
RegionImpl<RegionT>::get_objects ()
  requires RegionWithChildren<RegionT>
{
  using SharedPtrType = ChildTPtr;
  using SpanType = std::span<SharedPtrType>;
  if constexpr (std::is_same_v<RegionT, AutomationRegion>)
    {
      auto * ar = dynamic_cast<AutomationRegion *> (this);
      return SpanType (ar->aps_.data (), ar->aps_.size ());
    }
  else if constexpr (std::is_same_v<RegionT, ChordRegion>)
    {
      auto * cr = dynamic_cast<ChordRegion *> (this);
      return SpanType (cr->chord_objects_.data (), cr->chord_objects_.size ());
    }
  else if constexpr (std::is_same_v<RegionT, MidiRegion>)
    {
      auto * mr = dynamic_cast<MidiRegion *> (this);
      return SpanType (mr->midi_notes_.data (), mr->midi_notes_.size ());
    }
  return SpanType ();
}

template <typename RegionT>
std::vector<typename RegionImpl<RegionT>::ChildTPtr> &
RegionImpl<RegionT>::get_objects_vector ()
  requires RegionWithChildren<RegionT>
{
  if constexpr (is_automation ())
    {
      return static_cast<AutomationRegion *> (this)->aps_;
    }
  else if constexpr (is_chord ())
    {
      return static_cast<ChordRegion *> (this)->chord_objects_;
    }
  else if constexpr (is_midi ())
    {
      return static_cast<MidiRegion *> (this)->midi_notes_;
    }
  else
    {
      [[maybe_unused]] typedef typename RegionT::something_made_up X;
    }
}

template <typename RegionT>
void
RegionImpl<RegionT>::insert_object (ChildTPtr obj, int index, bool fire_events)
  requires RegionWithChildren<RegionT>
{
  z_debug ("inserting {} at index {}", *obj, index);
  auto &objects = get_objects_vector ();

  dynamic_cast<RegionT *> (this)->beginInsertRows ({}, index, index);
  // don't allow duplicates
  z_return_if_fail (
    std::find (objects.begin (), objects.end (), obj) == objects.end ());

  obj->setParent (dynamic_cast<RegionT *> (this));
  objects.insert (objects.begin () + index, obj);
  for (size_t i = index; i < objects.size (); ++i)
    {
      objects[i]->set_region_and_index (*this, i);
    }

  dynamic_cast<RegionT *> (this)->endInsertRows ();

  if (fire_events)
    {
      // EVENTS_PUSH (EventType::ET_ARRANGER_OBJECT_CREATED, ret.get ());
    }
}

template <typename RegionT>
RegionT *
RegionImpl<RegionT>::insert_clone_to_project_at_index (
  int  index,
  bool fire_events) const
{
  auto track = TRACKLIST->find_track_by_name_hash (id_.track_name_hash_);
  z_return_val_if_fail (track, nullptr);

  RegionT * ret = static_cast<const RegionT *> (this)->clone_raw_ptr ();
  std::visit (
    [&] (auto &&t) {
      if constexpr (is_automation ())
        {
          auto automatable_track = dynamic_cast<AutomatableTrack *> (t);
          z_return_if_fail (automatable_track);
          auto &at =
            automatable_track->get_automation_tracklist ().ats_[id_.at_idx_];
          t->Track::insert_region (ret, at, -1, index, true, fire_events);
        }
      else if constexpr (is_chord ())
        {
          auto chord_track = dynamic_cast<ChordTrack *> (t);
          z_return_if_fail (chord_track);
          t->Track::insert_region (ret, nullptr, -1, index, true, fire_events);
        }
      else if constexpr (is_laned ())
        {
          auto laned_track = dynamic_cast<LanedTrackImpl<
            typename LaneOwnedObjectImpl<RegionT>::TrackLaneT> *> (t);
          z_return_if_fail (laned_track);

          t->Track::insert_region (
            ret, nullptr, id_.lane_pos_, index, true, fire_events);
        }
    },
    *track);

  /* also set is as the clip editor region */
  CLIP_EDITOR->set_region (ret, true);
  return ret;
}

template <typename RegionT>
void
RegionImpl<
  RegionT>::move_to_track (Track * track, int lane_or_at_index, int index)
{
  z_return_if_fail (track);

  std::visit (
    [&] (auto &&region_track) {
      z_debug ("moving region {} to track {}", name_, track->name_);
      z_debug ("before: {}", print_to_str ());

      auto * self = dynamic_cast<RegionT *> (this);

      RegionLinkGroup * link_group = NULL;
      if (has_link_group ())
        {
          link_group = get_link_group ();
          z_return_if_fail (link_group);
          link_group->remove_region (*this, false, true);
        }

      bool selected = is_selected ();
      auto clip_editor_region = CLIP_EDITOR->get_region ();

      /* keep alive while moving*/
      auto * shared_this = dynamic_cast<RegionT *> (this);
      z_return_if_fail (shared_this);

      if (region_track)
        {
          /* remove the region from its old track */
          remove_from_project (false);
        }

      int lane_pos = lane_or_at_index >= 0 ? lane_or_at_index : id_.lane_pos_;
      int at_pos = lane_or_at_index >= 0 ? lane_or_at_index : id_.at_idx_;

      if constexpr (is_automation ())
        {
          auto  automatable_track = dynamic_cast<AutomatableTrack *> (track);
          auto &at = automatable_track->automation_tracklist_->ats_[at_pos];

          /* convert the automation points to match the new automatable */
          auto port = Port::find_from_identifier<ControlPort> (*at->port_id_);
          z_return_if_fail (port);
          for (auto &ap : derived_.aps_)
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
          derived_.set_automation_track (*at);
        }
      else
        {
          if constexpr (is_laned ())
            {
              /* create lanes if they don't exist */
              auto laned_track = dynamic_cast<LanedTrackImpl<
                typename LaneOwnedObjectImpl<RegionT>::TrackLaneT> *> (track);
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
              using TrackLaneT =
                typename LaneOwnedObjectImpl<RegionT>::TrackLaneT;
              auto laned_track =
                dynamic_cast<LanedTrackImpl<TrackLaneT> *> (track);
              auto lane =
                std::get<TrackLaneT *> (laned_track->lanes_.at (lane_pos));
              z_return_if_fail (
                !lane->region_list_->regions_.empty ()
                && std::get<RegionT *> (lane->region_list_->regions_.at (id_.idx_))
                     == dynamic_cast<RegionT *> (this));
              derived_.set_lane (*lane);

              laned_track->create_missing_lanes (lane_pos);

              if (region_track)
                {
                  /* remove empty lanes if the region was the last on its track
                   * lane
                   */
                  auto region_laned_track = dynamic_cast<LanedTrackImpl<
                    typename LaneOwnedObjectImpl<RegionT>::TrackLaneT> *> (
                    region_track);
                  region_laned_track->remove_empty_last_lanes ();
                }
            }

          if (link_group)
            {
              link_group->add_region (*this);
            }
        }

      /* reset the clip editor region because track_remove_region clears it */
      if (
        clip_editor_region.has_value ()
        && std::holds_alternative<RegionT *> (clip_editor_region.value ()))
        {
          if (
            std::get<RegionT *> (clip_editor_region.value ())
            == dynamic_cast<RegionT *> (this))
            {
              {
                CLIP_EDITOR->set_region (self, true);
              }
            }
        }

      /* reselect if necessary */
      select (selected, true, false);

      z_debug ("after: {}", print_to_str ());

      if (ZRYTHM_TESTING)
        {
          REGION_LINK_GROUP_MANAGER.validate ();
        }
    },
    get_track ());
}

template <typename RegionT>
void
RegionImpl<RegionT>::stretch (double ratio)
{
  z_debug ("stretching region {} (ratio {:f})", fmt::ptr (this), ratio);

  stretching_ = true;

  if constexpr (is_midi () || is_automation () || is_chord ())
    {
      auto objs = get_objects ();
      for (auto &obj : objs)
        {
          /* set start pos */
          double        before_ticks = obj->pos_->ticks_;
          double        new_ticks = before_ticks * ratio;
          dsp::Position tmp (new_ticks, AUDIO_ENGINE->frames_per_tick_);
          obj->pos_setter (&tmp);

          if (obj->has_length ())
            {
              /* set end pos */
              auto lengthable_obj = dynamic_cast<LengthableObject *> (obj);
              before_ticks = lengthable_obj->end_pos_->ticks_;
              new_ticks = before_ticks * ratio;
              tmp = dsp::Position (new_ticks, AUDIO_ENGINE->frames_per_tick_);
              lengthable_obj->end_pos_setter (&tmp);
            }
        }
    }
  else if constexpr (is_audio ())
    {
      auto * clip = derived_.get_clip ();
      int new_clip_id = AUDIO_POOL->duplicate_clip (clip->get_pool_id (), false);
      z_return_if_fail (new_clip_id >= 0);
      auto * new_clip = AUDIO_POOL->get_clip (new_clip_id);
      derived_.set_clip_id (new_clip->get_pool_id ());

      auto stretcher = dsp::Stretcher::create_rubberband (
        AUDIO_ENGINE->sample_rate_, new_clip->get_num_channels (), ratio, 1.0,
        false);

      auto buf = new_clip->get_samples ();
      buf.interleave_samples ();
      auto stretched_buf = stretcher->stretch_interleaved (buf);
      stretched_buf.deinterleave_samples (new_clip->get_num_channels ());
      new_clip->clear_frames ();
      new_clip->expand_with_frames (stretched_buf);
      auto num_frames_per_channel = new_clip->get_num_frames ();
      z_return_if_fail (num_frames_per_channel > 0);

      new_clip->write_to_pool (false, false);

      /* readjust end position to match the number of frames exactly */
      dsp::Position new_end_pos (
        static_cast<signed_frame_t> (num_frames_per_channel),
        AUDIO_ENGINE->ticks_per_frame_);
      set_position (&new_end_pos, ArrangerObject::PositionType::LoopEnd, false);
      new_end_pos.add_frames (pos_->frames_, AUDIO_ENGINE->ticks_per_frame_);
      set_position (&new_end_pos, ArrangerObject::PositionType::End, false);
    }
  else
    {
      [[maybe_unused]] typedef typename RegionT::something_made_up X;
    }

  stretching_ = false;
}

template <typename RegionT>
bool
RegionImpl<RegionT>::are_members_valid (bool is_project) const
{
  if (!id_.validate ())
    {
      return false;
    }

  if (is_project)
    {
      const auto &found = find (id_);
      if (found != this)
        {
          return false;
        }
    }

  z_return_val_if_fail (loop_start_pos_ < loop_end_pos_, false);

  return true;
}

/**
 * Returns the region's link group.
 */
RegionLinkGroup *
Region::get_link_group ()
{
  z_return_val_if_fail (
    id_.link_group_ >= 0
      && REGION_LINK_GROUP_MANAGER.groups_.size ()
           > static_cast<size_t> (id_.link_group_),
    nullptr);
  auto group = REGION_LINK_GROUP_MANAGER.get_group (id_.link_group_);
  return group;
}

template <typename RegionT>
void
RegionImpl<RegionT>::set_link_group (int group_idx, bool update_identifier)
{
  if (ENUM_BITSET_TEST (Flags, flags_, Flags::NonProject))
    {
      id_.link_group_ = group_idx;
      return;
    }

  if (id_.link_group_ >= 0 && id_.link_group_ != group_idx)
    {
      RegionLinkGroup * link_group = get_link_group ();
      z_return_if_fail (link_group);
      link_group->remove_region (*this, true, update_identifier);
    }
  if (group_idx >= 0)
    {
      RegionLinkGroup * group = REGION_LINK_GROUP_MANAGER.get_group (group_idx);
      z_return_if_fail (group);
      group->add_region (*this);
    }

  z_return_if_fail (group_idx == id_.link_group_);

  if (update_identifier)
    this->update_identifier ();
}

template <typename RegionT>
void
RegionImpl<RegionT>::create_link_group_if_none ()
{
  if (ENUM_BITSET_TEST (Flags, flags_, Flags::NonProject))
    return;

  if (id_.link_group_ < 0)
    {
      z_debug ("creating link group for region: {}", print_to_str ());
      int new_group = REGION_LINK_GROUP_MANAGER.add_group ();
      set_link_group (new_group, true);

      z_debug ("after link group ({}): {}", new_group, print_to_str ());
    }
}

template <typename RegionT>
void
RegionImpl<RegionT>::unlink ()
{
  if (ENUM_BITSET_TEST (
        ArrangerObject::Flags, flags_, ArrangerObject::Flags::NonProject))
    {
      id_.link_group_ = -1;
    }
  else if (id_.link_group_ >= 0)
    {
      RegionLinkGroup * group =
        REGION_LINK_GROUP_MANAGER.get_group (id_.link_group_);
      group->remove_region (*this, true, true);
    }
  else
    {
      z_warn_if_reached ();
    }

  z_warn_if_fail (id_.link_group_ == -1);

  update_identifier ();
}

std::optional<RegionPtrVariant>
Region::find (const RegionIdentifier &id)
{
  std::optional<RegionPtrVariant> result;

  auto try_find = [&]<typename RegionT> () {
    auto found = RegionImpl<RegionT>::find (id);
    if (!result && found)
      {
        result = found;
      }
  };

  try_find.operator()<MidiRegion> ();
  try_find.operator()<AudioRegion> ();
  try_find.operator()<AutomationRegion> ();
  try_find.operator()<ChordRegion> ();

  return result;
}

template <typename RegionT>
RegionImpl<RegionT>::RegionTPtr
RegionImpl<RegionT>::find (const RegionIdentifier &id)
{
  z_return_val_if_fail (id.track_name_hash_ != 0, nullptr);

  if constexpr (is_laned ())
    {
      z_return_val_if_fail (id.is_audio () || id.is_midi (), nullptr);
      auto track_var = TRACKLIST->find_track_by_name_hash (id.track_name_hash_);
      z_return_val_if_fail (track_var, nullptr);

      return std::visit (
        [&] (auto &&track) -> RegionTPtr {
          using TrackT = base_type<decltype (track)>;
          if constexpr (std::derived_from<TrackT, LanedTrack>)
            {
              if (id.lane_pos_ >= (int) track->lanes_.size ())
                {
                  z_error (
                    "given lane pos {} is greater than track '{}' ({}) number of lanes {}",
                    id.lane_pos_, track->name_, track->pos_,
                    track->lanes_.size ());
                  return nullptr;
                }
              auto lane_var = track->lanes_.at (id.lane_pos_);
              return std::visit (
                [&] (auto &&lane) -> RegionTPtr {
                  z_return_val_if_fail (lane, nullptr);

                  return std::get<RegionT *> (
                    lane->region_list_->regions_.at (id.idx_));
                },
                lane_var);
            }
          else
            {
              z_error ("Track {} is not a LanedTrack", track->name_);
              return nullptr;
            }
        },
        *track_var);
    }
  else if constexpr (is_automation ())
    {
      z_return_val_if_fail (id.is_automation (), nullptr);
      auto track_var = TRACKLIST->find_track_by_name_hash (id.track_name_hash_);
      z_return_val_if_fail (track_var, nullptr);

      return std::visit (
        [&] (auto &&track) -> RegionTPtr {
          using TrackT = base_type<decltype (track)>;
          if constexpr (std::derived_from<TrackT, AutomationTrack>)
            {
              auto &atl = track->automation_tracklist_;
              z_return_val_if_fail (
                id.at_idx_ < (int) atl->ats_.size (), nullptr);
              auto &at = atl->ats_[id.at_idx_];
              z_return_val_if_fail (at, nullptr);

              if (id.idx_ >= (int) at->regions_.size ())
                {
                  atl->print_regions ();
                  z_error (
                    "Automation track for {} has less regions ({}) than the given index {}",
                    at->port_id_->get_label (), at->regions_.size (), id.idx_);
                  return nullptr;
                }

              return std::dynamic_pointer_cast<RegionT> (at->regions_[id.idx_]);
            }
          else
            {
              z_error ("Track {} is not an AutomationTrack", track->name_);
              return nullptr;
            }
        },
        *track_var);
    }
  else if constexpr (is_chord ())
    {
      z_return_val_if_fail (id.is_chord (), nullptr);
      auto * track = P_CHORD_TRACK;
      z_return_val_if_fail (track, nullptr);

      return std::get<RegionT *> (track->region_list_->regions_.at (id.idx_));
    }
  else
    {
      [[maybe_unused]] typedef typename RegionT::something_made_up X;
    }
}

template <typename RegionT>
void
RegionImpl<RegionT>::update_identifier ()
{
  /* reset link group */
  set_link_group (id_.link_group_, false);

  track_name_hash_ = id_.track_name_hash_;
  if constexpr (RegionWithChildren<RegionT>)
    {
      auto objs = get_objects ();
      for (size_t i = 0; i < objs.size (); ++i)
        {
          auto &obj = objs[i];
          obj->set_region_and_index (*this, static_cast<int> (i));
        }
    }
}

void
Region::update_link_group ()
{
  z_debug ("updating link group {}", id_.link_group_);
  if (id_.link_group_ >= 0)
    {
      auto group = REGION_LINK_GROUP_MANAGER.get_group (id_.link_group_);
      group->update (*this);
    }
}

template <typename RegionT>
std::optional<typename RegionImpl<RegionT>::ChildTPtr>
RegionImpl<RegionT>::remove_object (ChildT &obj, bool free_obj, bool fire_events)
  requires RegionWithChildren<RegionT>
{
  /* deselect the object */
  obj.select (false, true, false);

  if constexpr (std::is_same_v<ChildT, AutomationPoint>)
    {
      if (derived_.last_recorded_ap_ == &obj)
        {
          derived_.last_recorded_ap_ = nullptr;
        }
    }

  using SharedPtrType = ChildTPtr;
  using ObjVectorType = std::vector<SharedPtrType>;

  auto remove_type = [&] (ObjVectorType &objects, ChildT &_obj) -> SharedPtrType {
    /* find the object in the list */
    auto it = std::ranges::find_if (
      objects.begin (), objects.end (),
      [&_obj] (const auto &cur_ptr) { return cur_ptr == &_obj; });
    z_return_val_if_fail (it != objects.end (), nullptr);

    /* get a shared pointer before erasing to possibly keep it alive */
    auto ret = *it;

    /* erase it */
    size_t erase_idx = std::distance (objects.begin (), it);
    dynamic_cast<RegionT *> (this)->beginRemoveRows ({}, erase_idx, erase_idx);
    it = objects.erase (it);
    ret->setParent (nullptr);
    if (free_obj)
      {
        ret->deleteLater ();
      }

    /* set region and index for all objects after the erased one */
    for (size_t i = erase_idx; it != objects.end (); ++i, ++it)
      {
        (*it)->set_region_and_index (*this, i);
      }

    dynamic_cast<RegionT *> (this)->endRemoveRows ();

    if (fire_events)
      {
        // EVENTS_PUSH (EventType::ET_ARRANGER_OBJECT_REMOVED, ret->type_);
      }

    return ret;
  };

  if constexpr (std::is_same_v<ChildT, AutomationPoint>)
    {
      return remove_type (derived_.aps_, obj);
    }
  else if constexpr (std::is_same_v<ChildT, ChordObject>)
    {
      return remove_type (derived_.chord_objects_, obj);
    }
  else if constexpr (std::is_same_v<ChildT, MidiNote>)
    {
      return remove_type (derived_.midi_notes_, obj);
    }
  else
    {
      [[maybe_unused]] typedef typename RegionT::something_made_up X;
    }
}

template <typename RegionT>
void
RegionImpl<RegionT>::remove_all_children ()
  requires RegionWithChildren<RegionT>
{
  z_debug ("removing all children from {} {}", id_.idx_, name_);

  auto objs = get_objects ();
  for (auto &obj : objs)
    {
      remove_object (*obj, true);
    }
}

template <typename RegionT>
void
RegionImpl<RegionT>::copy_children (const RegionImpl &other)
{
  z_return_if_fail (id_.type_ == other.id_.type_);

  z_debug (
    "copying children from %d %s to %d %s", other.id_.idx_, other.name_,
    id_.idx_, name_);

  if constexpr (std::is_same_v<RegionT, MidiRegion>)
    {
      for (auto &obj : other.derived_.midi_notes_)
        {
          append_object (obj->clone_raw_ptr (), false);
        }
    }
  else if constexpr (std::is_same_v<RegionT, AutomationRegion>)
    {
      for (auto &obj : other.derived_.aps_)
        {
          append_object (obj->clone_raw_ptr (), false);
        }
    }
  else if constexpr (std::is_same_v<RegionT, ChordRegion>)
    {
      for (auto &obj : other.derived_.chord_objects_)
        {
          append_object (obj->clone_raw_ptr (), false);
        }
    }
}

std::string
Region::print_to_str () const
{
  return fmt::format (
    "{} [{}] - track name hash {} - lane pos {} - "
    "idx {} - address {} - <{}> to <{}> ({} frames, {} ticks) - "
    "loop end <{}> - link group {}",
    name_, id_.type_, id_.track_name_hash_, id_.lane_pos_, id_.idx_,
    (void *) this, *pos_, *end_pos_, end_pos_->frames_ - pos_->frames_,
    end_pos_->ticks_ - pos_->ticks_, loop_end_pos_, id_.link_group_);
}

template <typename RegionT>
RegionT *
RegionImpl<RegionT>::at_position (
  const Track *           track,
  const AutomationTrack * at,
  const dsp::Position     pos)
{
  auto region_is_at_pos = [&pos] (const auto &region) {
    return *std::get<RegionT *> (region)->pos_ <= pos
           && *std::get<RegionT *> (region)->end_pos_ >= pos;
  };

  if constexpr (is_automation ())
    {
      z_return_val_if_fail (at, nullptr);
      auto it =
        std::ranges::find_if (at->region_list_->regions_, region_is_at_pos);
      return it != at->region_list_->regions_.end ()
               ? std::get<RegionT *> (*it)
               : nullptr;
    }
  else
    {
      z_return_val_if_fail (track, nullptr);

      if constexpr (is_laned ())
        {
          using TrackLaneT = LaneOwnedObjectImpl<RegionT>::TrackLaneT;
          auto laned_track =
            dynamic_cast<const LanedTrackImpl<TrackLaneT> *> (track);
          for (auto &lane_var : laned_track->lanes_)
            {
              auto lane = std::get<TrackLaneT *> (lane_var);
              auto it = std::ranges::find_if (
                lane->region_list_->regions_, region_is_at_pos);
              return it != at->region_list_->regions_.end ()
                       ? std::get<RegionT *> (*it)
                       : nullptr;
            }
        }
      else if constexpr (is_chord ())
        {
          auto it = std::ranges::find_if (
            P_CHORD_TRACK->region_list_->regions_, region_is_at_pos);
          return it != at->region_list_->regions_.end ()
                   ? std::get<RegionT *> (*it)
                   : nullptr;
        }
    }

  return nullptr;
}

signed_frame_t
Region::timeline_frames_to_local (
  const signed_frame_t timeline_frames,
  const bool           normalize) const
{
  if (normalize)
    {
      signed_frame_t diff_frames = timeline_frames - pos_->frames_;

      /* special case: timeline frames is exactly at the end of the region */
      if (timeline_frames == end_pos_->frames_)
        return diff_frames;

      const signed_frame_t loop_end_frames = loop_end_pos_.frames_;
      const signed_frame_t clip_start_frames = clip_start_pos_.frames_;
      const signed_frame_t loop_size = get_loop_length_in_frames ();
      z_return_val_if_fail_cmp (loop_size, >, 0, 0);

      diff_frames += clip_start_frames;

      while (diff_frames >= loop_end_frames)
        {
          diff_frames -= loop_size;
        }

      return diff_frames;
    }

  return timeline_frames - pos_->frames_;
}

void
Region::get_frames_till_next_loop_or_end (
  const signed_frame_t timeline_frames,
  signed_frame_t *     ret_frames,
  bool *               is_loop) const
{
  signed_frame_t loop_size = get_loop_length_in_frames ();
  z_return_if_fail_cmp (loop_size, >, 0);

  signed_frame_t local_frames = timeline_frames - pos_->frames_;

  local_frames += clip_start_pos_.frames_;

  while (local_frames >= loop_end_pos_.frames_)
    {
      local_frames -= loop_size;
    }

  signed_frame_t frames_till_next_loop = loop_end_pos_.frames_ - local_frames;

  signed_frame_t frames_till_end = end_pos_->frames_ - timeline_frames;

  *is_loop = frames_till_next_loop < frames_till_end;
  *ret_frames = std::min (frames_till_end, frames_till_next_loop);
}

std::string
Region::generate_filename ()
{
  return std::visit (
    [&] (auto &&track) {
      return fmt::sprintf (REGION_PRINTF_FILENAME, track->name_, name_);
    },
    get_track ());
}

bool
Region::is_recording ()
{
  if (RECORDING_MANAGER->num_active_recordings_ == 0)
    return false;

  return std::any_of (
    RECORDING_MANAGER->recorded_ids_.begin (),
    RECORDING_MANAGER->recorded_ids_.end (),
    [&] (const auto &cur_id) { return id_ == cur_id; });
}

template <typename RegionT>
void
RegionImpl<RegionT>::disconnect_region ()
{
  auto clip_editor_region = CLIP_EDITOR->get_region ();
  if (
    clip_editor_region.has_value ()
    && std::holds_alternative<RegionT *> (clip_editor_region.value ()))
    {
      auto * this_region = dynamic_cast<RegionT *> (this);
      if (this_region == std::get<RegionT *> (clip_editor_region.value ()))
        CLIP_EDITOR->set_region (std::nullopt, true);
    }

  if (TL_SELECTIONS)
    {
      TL_SELECTIONS->remove_object (*this);
    }

  if constexpr (RegionWithChildren<RegionT>)
    {
      auto objs = get_objects ();
      auto sel_opt = get_arranger_selections ();
      z_return_if_fail (sel_opt.has_value ());
      std::visit (
        [&] (auto &&sel) {
          for (auto &obj : objs)
            {
              sel->remove_object (*obj);
            }
        },
        *sel_opt);
    }

#if 0
  if (ZRYTHM_HAVE_UI && MAIN_WINDOW)
    {
      ArrangerWidget * arranger = get_arranger ();
      if (arranger->hovered_object.lock ().get () == this)
        {
          arranger->hovered_object.reset ();
        }
    }
#endif
}

std::optional<RegionPtrVariant>
Region::get_at_pos (
  const dsp::Position pos,
  Track *             track,
  AutomationTrack *   at,
  bool                include_region_end)
{
  auto is_at_pos = [&] (const auto &region_var) {
    return std::visit (
      [&] (auto &&region) {
        return *region->pos_ <= pos
               && (include_region_end ? *region->end_pos_ >= pos : *region->end_pos_ > pos);
      },
      region_var);
  };

  if (track)
    {
      if (track->has_lanes ())
        {
          auto laned_track_variant =
            convert_to_variant<LanedTrackPtrVariant> (track);
          return std::visit (
            [&] (auto &&t) -> std::optional<RegionPtrVariant> {
              using T = base_type<decltype (t)>;
              for (auto &lane_var : t->lanes_)
                {
                  using TrackLaneT = T::LanedTrackImpl::TrackLaneType;
                  auto lane = std::get<TrackLaneT *> (lane_var);
                  auto ret = lane->get_region_at_pos (pos, include_region_end);
                  if (ret)
                    return ret;

                  auto it = std::ranges::find_if (
                    lane->region_list_->regions_, is_at_pos);
                  if (it != lane->region_list_->regions_.end ())
                    {
                      return *it;
                    }
                }
              return std::nullopt;
            },
            laned_track_variant);
        }
      else if (track->is_chord ())
        {
          auto * chord_track = dynamic_cast<ChordTrack *> (track);
          auto   it = std::ranges::find_if (
            chord_track->region_list_->regions_, is_at_pos);
          if (it != chord_track->region_list_->regions_.end ())
            {
              return *it;
            }
          return std::nullopt;
        }
    }
  else if (at)
    {
      auto it = std::ranges::find_if (at->region_list_->regions_, is_at_pos);
      if (it != at->region_list_->regions_.end ())
        {
          return *it;
        }
      return std::nullopt;
    }
  z_return_val_if_reached (std::nullopt);
}

template <typename RegionT>
RegionOwnerImpl<RegionT> *
RegionImpl<RegionT>::get_region_owner () const
{
  auto * self = dynamic_cast<const RegionT *> (this);
  if constexpr (is_laned ())
    {
      auto lane = self->get_lane ();
      return lane;
    }
  else if constexpr (is_automation ())
    {
      return std::visit (
        [&] (auto &&automatable_track) -> RegionOwnerImpl<RegionT> * {
          using TrackT = base_type<decltype (automatable_track)>;
          if constexpr (std::derived_from<TrackT, AutomatableTrack>)
            {
              auto &at = automatable_track->get_automation_tracklist ().ats_.at (
                id_.at_idx_);
              return at;
            }
          else
            {
              throw std::runtime_error ("Invalid track");
            }
        },
        get_track ());
    }
  else if constexpr (is_chord ())
    {
      return std::get<ChordTrack *> (
        TRACKLIST->find_track_by_name_hash (id_.track_name_hash_).value ());
    }
}

template <typename RegionT>
void
RegionImpl<RegionT>::append_object (ChildTPtr obj, bool fire_events)
  requires RegionWithChildren<RegionT>
{
  auto &objects = get_objects_vector ();
  insert_object (obj, objects.size (), fire_events);
}

template class RegionImpl<MidiRegion>;
template class RegionImpl<AutomationRegion>;
template class RegionImpl<ChordRegion>;
template class RegionImpl<AudioRegion>;
