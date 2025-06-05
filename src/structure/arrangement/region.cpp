// SPDX-FileCopyrightText: © 2018-2022, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/stretcher.h"
#include "engine/session/clip.h"
#include "engine/session/pool.h"
#include "engine/session/recording_manager.h"
#include "engine/session/router.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"
#include "gui/dsp/control_port.h"
#include "structure/arrangement/audio_region.h"
#include "structure/arrangement/automation_region.h"
#include "structure/arrangement/chord_region.h"
#include "structure/arrangement/lane_owned_object.h"
#include "structure/arrangement/midi_note.h"
#include "structure/arrangement/midi_region.h"
#include "structure/arrangement/region.h"
#include "structure/arrangement/region_link_group_manager.h"
#include "structure/tracks/audio_lane.h"
#include "structure/tracks/channel.h"
#include "structure/tracks/chord_track.h"
#include "structure/tracks/instrument_track.h"
#include "structure/tracks/midi_lane.h"
#include "structure/tracks/track.h"
#include "structure/tracks/tracklist.h"
#include "utils/debug.h"
#include "utils/gtest_wrapper.h"
#include "utils/logger.h"
#include "utils/rt_thread_id.h"

namespace zrythm::structure::arrangement
{

void
Region::copy_members_from (const Region &other, ObjectCloneType clone_type)
{
  bounce_ = other.bounce_;
}

template <typename RegionT>
void
RegionImpl<RegionT>::send_note_offs (
  dsp::MidiEventVector       &midi_events,
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
          for (const auto &mn : region->get_children_view ())
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
  TrackPtrVariant              track_var,
  const EngineProcessTimeInfo &time_nfo,
  bool                         note_off_at_end,
  bool                         is_note_off_for_loop_or_region_end,
  dsp::MidiEventVector        &midi_events) const
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
        const auto obj_start_pos = obj.get_position ();
        if (
          obj_start_pos.frames_ >= 0 && obj_start_pos.frames_ >= r_local_pos
          && obj_start_pos.frames_
               < r_local_pos + (signed_frame_t) time_nfo.nframes_)
          {
            auto _time =
              (midi_time_t) (time_nfo.local_offset_
                             + (obj_start_pos.frames_ - r_local_pos));

            if constexpr (std::is_same_v<RegionT, MidiRegion>)
              {
                midi_events.add_note_on (
                  r->get_midi_ch (), obj.pitch_, obj.vel_->vel_, _time);
              }
            else if constexpr (std::is_same_v<ObjectType, ChordObject>)
              {
                midi_events.add_note_ons_from_chord_descr (
                  *descr, 1, Velocity::DEFAULT_VALUE, _time);
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
              obj_start_pos.frames_
              + TRANSPORT->ticks_per_beat_
                  * type_safe::get (AUDIO_ENGINE->frames_per_tick_));
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
      if constexpr (
        std::is_same_v<RegionT, MidiRegion>
        || std::is_same_v<RegionT, ChordRegion>)
        {
          for (const auto &obj : get_derived ().get_children_view ())
            {
              process_object (*obj);
            }
        }
    },
    track_var);
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
RegionImpl<RegionT>::stretch (double ratio)
{
  z_debug ("stretching region {} (ratio {:f})", fmt::ptr (this), ratio);

  stretching_ = true;

  if constexpr (is_midi () || is_automation () || is_chord ())
    {
      auto &self = get_derived ();
      auto  objs = self.get_children_view ();
      for (auto * obj : objs)
        {
          using ObjT = base_type<decltype (obj)>;
          /* set start pos */
          double        before_ticks = obj->get_position ().ticks_;
          double        new_ticks = before_ticks * ratio;
          dsp::Position tmp (new_ticks, AUDIO_ENGINE->frames_per_tick_);
          obj->position_setter_validated (tmp, AUDIO_ENGINE->ticks_per_frame_);

          if constexpr (std::derived_from<ObjT, BoundedObject>)
            {
              /* set end pos */
              before_ticks = obj->end_pos_->ticks_;
              new_ticks = before_ticks * ratio;
              tmp = dsp::Position (new_ticks, AUDIO_ENGINE->frames_per_tick_);
              obj->end_position_setter_validated (
                tmp, AUDIO_ENGINE->ticks_per_frame_);
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
        AUDIO_ENGINE->get_sample_rate (), new_clip->get_num_channels (), ratio,
        1.0, false);

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

      set_loop_end_position_unvalidated (new_end_pos);
      new_end_pos.add_frames (pos_->frames_, AUDIO_ENGINE->ticks_per_frame_);
      set_end_position_unvalidated (new_end_pos);
    }
  else
    {
      DEBUG_TEMPLATE_PARAM (RegionT)
    }

  stretching_ = false;
}

template <typename RegionT>
bool
RegionImpl<RegionT>::are_members_valid (
  bool               is_project,
  dsp::FramesPerTick frames_per_tick) const
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
      z_debug ("creating link group for region: {}", get_derived ());
      int new_group = REGION_LINK_GROUP_MANAGER.add_group ();
      set_link_group (new_group, true);

      z_debug ("after link group ({}): {}", new_group, get_derived ());
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
RegionT *
RegionImpl<RegionT>::at_position (
  const Track *                   track,
  const tracks::AutomationTrack * at,
  const dsp::Position             pos)
{
  auto region_is_at_pos = [&pos] (const auto &region) {
    return *region->pos_ <= pos && *region->end_pos_ >= pos;
  };

  if constexpr (is_automation ())
    {
      z_return_val_if_fail (at, nullptr);
      auto regions = at->get_children_view ();
      auto it = std::ranges::find_if (regions, region_is_at_pos);
      return it != regions.end () ? (*it) : nullptr;
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
              if constexpr (std::derived_from<TrackT, tracks::LanedTrack>)
                {
                  using TrackLaneT = TrackT::TrackLaneType;
                  if constexpr (
                    std::is_same_v<RegionT, typename TrackLaneT::RegionT>)
                    {
                      for (auto &lane_var : track_ptr->lanes_)
                        {
                          auto lane = std::get<TrackLaneT *> (lane_var);
                          auto regions = lane->get_children_view ();
                          auto it =
                            std::ranges::find_if (regions, region_is_at_pos);
                          return it != regions.end () ? (*it) : nullptr;
                        }
                    }
                }
              throw std::runtime_error ("track is not laned");
            },
            track_var);
        }
      else if constexpr (is_chord ())
        {
          auto regions = P_CHORD_TRACK->ArrangerObjectOwner<
            ChordRegion>::get_children_view ();
          auto it = std::ranges::find_if (regions, region_is_at_pos);
          return it != regions.end () ? (*it) : nullptr;
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

bool
Region::is_recording ()
{
  if (RECORDING_MANAGER->num_active_recordings_ == 0)
    return false;

  return std::ranges::contains (RECORDING_MANAGER->recorded_ids_, get_uuid ());
}

#if 0
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

  auto &self = get_derived ();

  {
    auto selection_mgr =
      ArrangerObjectFactory::get_instance ()->get_selection_manager_for_object (
        self);
    selection_mgr.remove_from_selection (get_uuid ());
  }

  if constexpr (RegionWithChildren<RegionT>)
    {
      for (auto * obj : get_derived ().get_children_view ())
        {
          auto selection_mgr =
            ArrangerObjectFactory::get_instance ()
              ->get_selection_manager_for_object (*obj);
          selection_mgr.remove_from_selection (obj->get_uuid ());
        }
    }

#  if 0
  if (ZRYTHM_HAVE_UI && MAIN_WINDOW)
    {
      ArrangerWidget * arranger = get_arranger ();
      if (arranger->hovered_object.lock ().get () == this)
        {
          arranger->hovered_object.reset ();
        }
    }
#  endif
}
#endif

std::optional<ArrangerObjectPtrVariant>
Region::get_at_pos (
  const dsp::Position       pos,
  Track *                   track,
  tracks::AutomationTrack * at,
  bool                      include_region_end)
{
  auto is_at_pos = [&] (const auto &region_var) -> bool {
    return std::visit (
      [&] (auto &&region) -> bool {
        using RegionT = base_type<decltype (region)>;
        if constexpr (std::derived_from<RegionT, Region>)
          {
            return *region->pos_ <= pos
                   && (include_region_end ? *region->end_pos_ >= pos : *region->end_pos_ > pos);
          }
        else
          {
            throw std::runtime_error ("expected region");
          }
      },
      region_var.get_object ());
  };

  if (track)
    {
      return std::visit (
        [&] (auto &&track_derived) -> std::optional<ArrangerObjectPtrVariant> {
          using TrackT = base_type<decltype (track_derived)>;
          if constexpr (std::derived_from<TrackT, tracks::LanedTrack>)
            {
              for (auto &lane_var : track_derived->lanes_)
                {
                  using TrackLaneT = TrackT::LanedTrackImpl::TrackLaneType;
                  auto lane = std::get<TrackLaneT *> (lane_var);
                  auto ret_var =
                    ArrangerObjectSpan{ lane->get_children_vector () }
                      .get_bounded_object_at_pos (pos, include_region_end);
                  if (ret_var)
                    return std::get<typename TrackLaneT::RegionT *> (*ret_var);

                  auto region_vars = lane->get_children_vector ();
                  auto it = std::ranges::find_if (region_vars, is_at_pos);
                  if (it != region_vars.end ())
                    {
                      return (*it).get_object ();
                    }
                }
            }
          if constexpr (std::is_same_v<TrackT, tracks::ChordTrack>)
            {
              auto region_vars = track_derived->ArrangerObjectOwner<
                ChordRegion>::get_children_vector ();
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
  if (at)
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

template class RegionImpl<MidiRegion>;
template class RegionImpl<AutomationRegion>;
template class RegionImpl<ChordRegion>;
template class RegionImpl<AudioRegion>;
}
