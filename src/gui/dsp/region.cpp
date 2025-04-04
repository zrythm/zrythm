// SPDX-FileCopyrightText: © 2018-2022, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
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
Region::copy_members_from (const Region &other, ObjectCloneType clone_type)
{
  bounce_ = other.bounce_;
}

void
Region::gen_name (
  std::optional<std::string> base_name,
  AutomationTrack *          at,
  Track *                    track)
{
  /* Name to try to assign */
  std::string orig_name;
  if (base_name)
    orig_name = *base_name;
  else if (at)
    orig_name = fmt::format ("{} - {}", track->get_name (), at->getLabel ());
  else
    orig_name = track->get_name ();

  z_return_if_fail (orig_name.length () > 0);

  std::visit (
    [&] (auto &&region) { region->set_name (orig_name); },
    convert_to_variant<RegionPtrVariant> (this));
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
          for (const auto &mn : region->get_object_ptrs_view ())
            {
              if (mn->is_hit (frame_for_note_off))
                {
                  midi_events.add_note_off (
                    channel, mn->pitch_, midi_time_for_note_off);
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

        dsp::ChordDescriptor * descr = nullptr;
        if constexpr (std::is_same_v<ObjectType, ChordObject>)
          {
            descr = obj.getChordDescriptor ();
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
                  r->get_midi_ch (), obj.pitch_, obj.vel_->vel_, _time);
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
            obj_end_frames = utils::math::round_to_signed_frame_t (
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
                midi_events.add_note_off (r->get_midi_ch (), obj.pitch_, _time);
              }
            else if constexpr (std::is_same_v<ObjectType, ChordObject>)
              {
                for (
                  size_t l = 0; l < ChordObject::ChordDescriptor::MAX_NOTES; l++)
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
          for (
            const auto &mn :
            ArrangerObjectRegistrySpan{
              get_arranger_object_registry (), get_derived ().midi_notes_ }
              .as_type<MidiNote> ())
            {
              process_object (*mn);
            }
        }
      else if constexpr (std::is_same_v<RegionT, ChordRegion>)
        {
          for (
            const auto &co :
            ArrangerObjectRegistrySpan{
              get_arranger_object_registry (), get_derived ().chord_objects_ }
              .as_type<ChordObject> ())
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
          auto &lane = get_derived ().get_lane ();
          if (lane.is_effectively_muted ())
            return true;
        }
    }
  return muted_;
}

#if 0
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
#endif

template <typename RegionT>
RegionT *
RegionImpl<RegionT>::insert_clone_to_project_at_index (
  int  index,
  bool fire_events) const
{
  // TODO
  return nullptr;
#if 0
  auto      track_var = get_track ();
  RegionT * ret = static_cast<const RegionT *> (this)->clone_raw_ptr ();
  std::visit (
    [&] (auto &&t) {
      using TrackT = base_type<decltype (t)>;
      if constexpr (
        is_automation () && std::derived_from<TrackT, AutomatableTrack>)
        {
          auto * at =
            dynamic_cast<const AutomationRegion &> (get_derived ())
              .get_automation_track ();
          t->Track::insert_region (ret, at, -1, index, true, fire_events);
        }
      else if constexpr (is_chord () && std::is_same_v<TrackT, ChordTrack>)
        {
          t->Track::insert_region (ret, nullptr, -1, index, true, fire_events);
        }
      else if constexpr (is_laned () && std::derived_from<TrackT, LanedTrack>)
        {
          if constexpr (is_laned ())
            {
              const auto &lane = get_derived ().get_lane ();

              t->Track::insert_region (
                ret, nullptr, t->get_lane_index (lane), index, true,
                fire_events);
            }
        }
    },
    track_var);

  /* also set is as the clip editor region */
  CLIP_EDITOR->set_region (ret->get_uuid ());
  return ret;
#endif
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
                CLIP_EDITOR->set_region (self->get_uuid ());
              }
            }
        }

      /* reselect if necessary */
      self->setSelected (selected);

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
      auto &self = get_derived ();
      auto  objs = self.get_object_ptrs_view ();
      for (auto * obj : objs)
        {
          using ObjT = base_type<decltype (obj)>;
          /* set start pos */
          double        before_ticks = obj->pos_->ticks_;
          double        new_ticks = before_ticks * ratio;
          dsp::Position tmp (new_ticks, AUDIO_ENGINE->frames_per_tick_);
          obj->pos_setter (tmp);

          if constexpr (std::derived_from<ObjT, BoundedObject>)
            {
              /* set end pos */
              before_ticks = obj->end_pos_->ticks_;
              new_ticks = before_ticks * ratio;
              tmp = dsp::Position (new_ticks, AUDIO_ENGINE->frames_per_tick_);
              obj->end_pos_setter (tmp);
            }
        }
    }
  else if constexpr (is_audio ())
    {
      auto * clip = get_derived ().get_clip ();
      auto new_clip_id = AUDIO_POOL->duplicate_clip (clip->get_uuid (), false);
      auto * new_clip = AUDIO_POOL->get_clip (new_clip_id);
      get_derived ().set_clip_id (new_clip->get_uuid ());

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

      AUDIO_POOL->write_clip (*new_clip, false, false);

      /* readjust end position to match the number of frames exactly */
      dsp::Position new_end_pos (
        static_cast<signed_frame_t> (num_frames_per_channel),
        AUDIO_ENGINE->ticks_per_frame_);
      set_position (new_end_pos, ArrangerObject::PositionType::LoopEnd, false);
      new_end_pos.add_frames (pos_->frames_, AUDIO_ENGINE->ticks_per_frame_);
      set_position (new_end_pos, ArrangerObject::PositionType::End, false);
    }
  else
    {
      DEBUG_TEMPLATE_PARAM (RegionT)
    }

  stretching_ = false;
}

template <typename RegionT>
bool
RegionImpl<RegionT>::are_members_valid (bool is_project) const
{

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
    has_link_group ()
      && REGION_LINK_GROUP_MANAGER.groups_.size ()
           > static_cast<size_t> (link_group_.value ()),
    nullptr);
  auto group = REGION_LINK_GROUP_MANAGER.get_group (link_group_.value ());
  return group;
}

template <typename RegionT>
void
RegionImpl<RegionT>::set_link_group (int group_idx, bool update_identifier)
{
  auto &self = get_derived ();
  if (ENUM_BITSET_TEST (flags_, Flags::NonProject))
    {
      link_group_ = group_idx;
      return;
    }

  if (has_link_group () && *link_group_ != group_idx)
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

  z_return_if_fail (group_idx == *link_group_);

  if (update_identifier)
    self.update_identifier ();
}

template <typename RegionT>
void
RegionImpl<RegionT>::create_link_group_if_none ()
{
  if (ENUM_BITSET_TEST (flags_, Flags::NonProject))
    return;

  if (!has_link_group ())
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
  if (ENUM_BITSET_TEST (flags_, ArrangerObject::Flags::NonProject))
    {
      link_group_.reset ();
    }
  else if (has_link_group ())
    {
      auto group = get_link_group ();
      group->remove_region (*this, true, true);
    }
  else
    {
      z_return_if_reached ();
    }

  z_return_if_fail (!has_link_group ());

  get_derived ().update_identifier ();
}

#if 0
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
  if constexpr (is_laned ())
    {
      z_return_val_if_fail (id.is_audio () || id.is_midi (), nullptr);
      auto track_var = *TRACKLIST->get_track (id.track_uuid_);

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
        track_var);
    }
  else if constexpr (is_automation ())
    {
      z_return_val_if_fail (id.is_automation (), nullptr);
      auto track_var = *TRACKLIST->get_track (id.track_uuid_);
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
        track_var);
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
#endif

void
Region::update_link_group ()
{
  z_debug ("updating link group {}", link_group_);
  if (has_link_group ())
    {
      auto group = get_link_group ();
      group->update (*this);
    }
}

template <typename RegionT>
void
RegionImpl<RegionT>::remove_object (const ArrangerObject::Uuid &child_id)
  requires RegionWithChildren<RegionT>
{
  auto &self = get_derived ();
  auto  obj = get_object_ptr (child_id);
  /* deselect the object */
  obj->setSelected (false);

  if constexpr (std::is_same_v<ChildT, AutomationPoint>)
    {
      if (self.last_recorded_ap_ == obj)
        {
          self.last_recorded_ap_ = nullptr;
        }
    }

  auto &objects = self.get_objects_vector ();

  /* find the object in the list */
  auto it = std::ranges::find (objects, child_id);
  assert (it != objects.end ());

  /* get a shared pointer before erasing to possibly keep it alive */
  auto ret = *it;

  /* erase it */
  size_t erase_index = std::distance (objects.begin (), it);
  self.beginRemoveRows ({}, erase_index, erase_index);
  it = objects.erase (it);

  /* set region and index for all objects after the erased one */
  for (size_t index = erase_index; it != objects.end (); ++index, ++it)
    {
      auto cur_obj = get_object_ptr (*it);
      cur_obj->set_region_and_index (*this);
    }

  self.endRemoveRows ();
}

template <typename RegionT>
void
RegionImpl<RegionT>::copy_children (const RegionImpl &other)
  requires RegionWithChildren<RegionT>
{
  z_return_if_fail (type_ == other.type_);

  z_debug (
    "copying children from [{}] '{}' to [{}] '{}'", other.get_uuid (),
    other.name_, get_uuid (), name_);

// TODO
#if 0
  auto &self = get_derived ();
  for (auto * obj : self.get_object_ptrs_view ())
    {
      append_object (
        obj->clone_and_register (get_arranger_object_registry ())->get_uuid ());
    }
#endif
}

std::string
Region::print_to_str () const
{
  return fmt::format (
    "{} [{}] - track name hash {} - "
    // "lane pos {} - idx {} - "
    "address {} - <{}> to <{}> ({} frames, {} ticks) - "
    "loop end <{}> - link group {}",
    name_, type_, get_track_id (),
    // id_.lane_pos_, id_.idx_,
    fmt::ptr (this), *pos_, *end_pos_, end_pos_->frames_ - pos_->frames_,
    end_pos_->ticks_ - pos_->ticks_, loop_end_pos_, link_group_);
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
          auto track_var = convert_to_variant<TrackPtrVariant> (track);
          return std::visit (
            [&] (auto &&track_ptr) -> RegionT * {
              using TrackT = base_type<decltype (track_ptr)>;
              if constexpr (std::derived_from<TrackT, LanedTrack>)
                {
                  for (auto &lane_var : track_ptr->lanes_)
                    {
                      using TrackLaneT = TrackT::TrackLaneType;
                      auto lane = std::get<TrackLaneT *> (lane_var);
                      auto it = std::ranges::find_if (
                        lane->region_list_->regions_, region_is_at_pos);
                      return it != at->region_list_->regions_.end ()
                               ? std::get<RegionT *> (*it)
                               : nullptr;
                    }
                }
              throw std::runtime_error ("track is not laned");
            },
            track_var);
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

  return std::ranges::contains (RECORDING_MANAGER->recorded_ids_, get_uuid ());
}

template <typename RegionT>
void
RegionImpl<RegionT>::disconnect_region ()
{
  if (
    CLIP_EDITOR->has_region ()
    && CLIP_EDITOR->get_region_id ().value () == get_uuid ())
    {
      CLIP_EDITOR->unsetRegion ();
    }

  auto self = dynamic_cast<RegionT *> (this);
  self->setSelected (false);

  if constexpr (RegionWithChildren<RegionT>)
    {
      for (auto * obj : get_derived ().get_object_ptrs_view ())
        {
          obj->setSelected (false);
        }
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
      return std::visit (
        [&] (auto &&track_derived) -> std::optional<RegionPtrVariant> {
          using TrackT = base_type<decltype (track_derived)>;
          if constexpr (std::derived_from<TrackT, LanedTrack>)
            {
              for (auto &lane_var : track_derived->lanes_)
                {
                  using TrackLaneT = TrackT::LanedTrackImpl::TrackLaneType;
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
            }
          if constexpr (std::is_same_v<TrackT, ChordTrack>)
            {
              auto it = std::ranges::find_if (
                track_derived->region_list_->regions_, is_at_pos);
              if (it != track_derived->region_list_->regions_.end ())
                {
                  return *it;
                }
            }
          return std::nullopt;
        },
        convert_to_variant<TrackPtrVariant> (track));
    }
  if (at)
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
RegionOwner<RegionT> *
RegionImpl<RegionT>::get_region_owner () const
{
  auto * self = dynamic_cast<const RegionT *> (this);
  if constexpr (is_laned ())
    {
      auto &lane = self->get_lane ();
      return std::addressof (lane);
    }
  else if constexpr (is_automation ())
    {
      return std::visit (
        [&] (auto &&automatable_track) -> RegionOwner<RegionT> * {
          using TrackT = base_type<decltype (automatable_track)>;
          if constexpr (std::derived_from<TrackT, AutomatableTrack>)
            {
              auto * at =
                automatable_track->get_automation_tracklist ()
                  .get_automation_track_by_port_id (self->automatable_port_id_);
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
      return std::get<ChordTrack *> (get_track ());
    }
}

template class RegionImpl<MidiRegion>;
template class RegionImpl<AutomationRegion>;
template class RegionImpl<ChordRegion>;
template class RegionImpl<AudioRegion>;
