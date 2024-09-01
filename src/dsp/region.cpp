// SPDX-FileCopyrightText: © 2018-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/audio_region.h"
#include "dsp/automation_region.h"
#include "dsp/channel.h"
#include "dsp/chord_region.h"
#include "dsp/chord_track.h"
#include "dsp/clip.h"
#include "dsp/control_port.h"
#include "dsp/instrument_track.h"
#include "dsp/lane_owned_object.h"
#include "dsp/midi_note.h"
#include "dsp/midi_region.h"
#include "dsp/pool.h"
#include "dsp/recording_manager.h"
#include "dsp/region.h"
#include "dsp/region_link_group_manager.h"
#include "dsp/router.h"
#include "dsp/stretcher.h"
#include "dsp/track.h"
#include "dsp/tracklist.h"
#include "gui/backend/event.h"
#include "gui/widgets/arranger_wrapper.h"
#include "gui/widgets/audio_arranger.h"
#include "gui/widgets/audio_editor_space.h"
#include "gui/widgets/automation_arranger.h"
#include "gui/widgets/automation_editor_space.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/chord_arranger.h"
#include "gui/widgets/chord_editor_space.h"
#include "gui/widgets/clip_editor.h"
#include "gui/widgets/clip_editor_inner.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/midi_arranger.h"
#include "gui/widgets/midi_editor_space.h"
#include "gui/widgets/timeline_arranger.h"
#include "gui/widgets/timeline_panel.h"
#include "project.h"
#include "utils/debug.h"
#include "utils/logger.h"
#include "utils/rt_thread_id.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

void
Region::init (
  const Position &start_pos,
  const Position &end_pos,
  unsigned int    track_name_hash,
  int             lane_pos_or_at_idx,
  int             idx_inside_lane_or_at)
{
  type_ = Type::Region;

  id_.track_name_hash_ = track_name_hash;
  id_.lane_pos_ = lane_pos_or_at_idx;
  id_.at_idx_ = lane_pos_or_at_idx;
  id_.idx_ = idx_inside_lane_or_at;

  pos_ = start_pos;
  end_pos_ = end_pos;
  long length = get_length_in_frames ();
  z_return_if_fail (length > 0);
  loop_end_pos_.from_frames (length);

  if (region_type_can_fade (id_.type_))
    {
      auto fo = dynamic_cast<FadeableObject *> (this);
      /* set fade positions to start/end */
      fo->fade_out_pos_.from_frames (length);
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
    orig_name = fmt::format ("{} - {}", track->name_, at->port_id_.get_label ());
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
  bool                        is_note_off_for_loop_or_region_end) const requires
  RegionTypeWithMidiEvents<RegionT>
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
  MidiEventVector &midi_events) const requires RegionTypeWithMidiEvents<RegionT>
{
  auto track = get_track ();
  z_return_if_fail (track);

  auto r = static_cast<const RegionT *> (this);

  /* send note offs if needed */
  if (note_off_at_end)
    {
      send_note_offs (midi_events, time_nfo, is_note_off_for_loop_or_region_end);
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
      obj.pos_.frames_ >= 0 && obj.pos_.frames_ >= r_local_pos
      && obj.pos_.frames_ < r_local_pos + (signed_frame_t) time_nfo.nframes_)
      {
        auto _time =
          (midi_time_t) (time_nfo.local_offset_ + (obj.pos_.frames_ - r_local_pos));

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

    signed_frame_t obj_end_frames;
    if constexpr (std::is_same_v<ObjectType, MidiNote>)
      {
        obj_end_frames = obj.end_pos_.frames_;
      }
    else if constexpr (std::is_same_v<ObjectType, ChordObject>)
      {
        obj_end_frames = math_round_double_to_signed_frame_t (
          obj.pos_.frames_
          + TRANSPORT->ticks_per_beat_ * AUDIO_ENGINE->frames_per_tick_);
      }

    /* if note ends within the cycle */
    if (
      obj_end_frames >= r_local_pos
      && (obj_end_frames <= (r_local_pos + time_nfo.nframes_)))
      {
        auto _time =
          (midi_time_t) (time_nfo.local_offset_ + (obj_end_frames - r_local_pos));

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
std::span<std::shared_ptr<typename RegionImpl<RegionT>::ChildT>>
RegionImpl<RegionT>::get_objects () requires RegionWithChildren<RegionT>
{
  using SharedPtrType = std::shared_ptr<ChildT>;
  using SpanType = std::span<SharedPtrType>;
  if constexpr (std::is_same_v<RegionT, AutomationRegion>)
    {
      auto ar = static_cast<AutomationRegion *> (this);
      return SpanType (
        reinterpret_cast<SharedPtrType *> (ar->aps_.data ()), ar->aps_.size ());
    }
  else if constexpr (std::is_same_v<RegionT, ChordRegion>)
    {
      auto cr = static_cast<ChordRegion *> (this);
      return SpanType (
        reinterpret_cast<SharedPtrType *> (cr->chord_objects_.data ()),
        cr->chord_objects_.size ());
    }
  else if constexpr (std::is_same_v<RegionT, MidiRegion>)
    {
      auto mr = dynamic_cast<MidiRegion *> (this);
      return SpanType (
        reinterpret_cast<SharedPtrType *> (mr->midi_notes_.data ()),
        mr->midi_notes_.size ());
    }
  return SpanType ();
}

template <typename RegionT>
std::vector<std::shared_ptr<typename RegionImpl<RegionT>::ChildT>> &
RegionImpl<RegionT>::get_objects_vector () requires RegionWithChildren<RegionT>
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
      static_assert (false, "Invalid region type");
    }
}

template <typename RegionT>
std::shared_ptr<typename RegionImpl<RegionT>::ChildT>
RegionImpl<RegionT>::insert_object (
  std::shared_ptr<ChildT> obj,
  int                     index,
  bool                    fire_events) requires RegionWithChildren<RegionT>
{
  z_debug ("inserting {} at index {}", obj, index);
  auto &objects = get_objects_vector ();

  // don't allow duplicates
  z_return_val_if_fail (
    std::find (objects.begin (), objects.end (), obj) == objects.end (),
    nullptr);

  objects.insert (objects.begin () + index, std::move (obj));
  for (size_t i = index; i < objects.size (); ++i)
    {
      objects[i]->set_region_and_index (*this, i);
    }
  auto ret = objects[index];

  if (fire_events)
    {
      EVENTS_PUSH (EventType::ET_ARRANGER_OBJECT_CREATED, ret.get ());
    }

  return ret;
}

template <typename RegionT>
std::shared_ptr<RegionT>
RegionImpl<RegionT>::insert_clone_to_project_at_index (
  int  index,
  bool fire_events) const
{
  auto track = TRACKLIST->find_track_by_name_hash (id_.track_name_hash_);
  z_return_val_if_fail (track, nullptr);
  std::shared_ptr<RegionT> ret;
  if constexpr (is_automation ())
    {
      auto automatable_track = dynamic_cast<AutomatableTrack *> (track);
      z_return_val_if_fail (automatable_track, nullptr);
      auto &at =
        automatable_track->get_automation_tracklist ().ats_[id_.at_idx_];
      ret = track->insert_region (
        static_cast<const RegionT *> (this)->clone_shared (), at.get (), -1,
        index, true, fire_events);
    }
  else if constexpr (is_chord ())
    {
      auto chord_track = dynamic_cast<ChordTrack *> (track);
      z_return_val_if_fail (chord_track, nullptr);
      ret = track->insert_region (
        static_cast<const RegionT *> (this)->clone_shared (), nullptr, -1,
        index, true, fire_events);
    }
  else if constexpr (is_laned ())
    {
      auto laned_track = dynamic_cast<LanedTrackImpl<RegionT> *> (track);
      z_return_val_if_fail (laned_track, nullptr);
      ret = track->insert_region (
        static_cast<const RegionT *> (this)->clone_shared (), nullptr,
        id_.lane_pos_, index, true, fire_events);
    }

  /* also set is as the clip editor region */
  CLIP_EDITOR->set_region (ret.get (), true);
  return ret;
}

template <typename RegionT>
void
RegionImpl<
  RegionT>::move_to_track (Track * track, int lane_or_at_index, int index)
{
  z_return_if_fail (track);

  auto region_track = get_track ();

  z_debug ("moving region {} to track {}", name_, track->name_);
  z_debug ("before: {}", print_to_str ());

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
  auto shared_this = shared_from_this_as<RegionT> ();
  z_return_if_fail (shared_this);

  if (region_track)
    {
      /* remove the region from its old track */
      remove_from_project ();
    }

  int lane_pos = lane_or_at_index >= 0 ? lane_or_at_index : id_.lane_pos_;
  int at_pos = lane_or_at_index >= 0 ? lane_or_at_index : id_.at_idx_;

  if constexpr (is_automation ())
    {
      auto  automatable_track = dynamic_cast<AutomatableTrack *> (track);
      auto &at = automatable_track->automation_tracklist_->ats_[at_pos];

      /* convert the automation points to match the new automatable */
      auto port = Port::find_from_identifier<ControlPort> (at->port_id_);
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
                shared_this, at.get (), -1, index, false, false);
            }
          else
            {
              track->add_region (shared_this, at.get (), -1, false, false);
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
          auto laned_track = dynamic_cast<LanedTrackImpl<RegionT> *> (track);
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
              track->add_region (shared_this, nullptr, lane_pos, false, false);
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
          auto laned_track = dynamic_cast<LanedTrackImpl<RegionT> *> (track);
          z_return_if_fail (
            !laned_track->lanes_[lane_pos]->regions_.empty ()
            && laned_track->lanes_[lane_pos]->regions_[id_.idx_].get () == this);
          derived_.set_lane (*laned_track->lanes_[lane_pos]);

          laned_track->create_missing_lanes (lane_pos);

          if (region_track)
            {
              /* remove empty lanes if the region was the last on its track lane
               */
              auto region_laned_track =
                dynamic_cast<LanedTrackImpl<RegionT> *> (region_track);
              region_laned_track->remove_empty_last_lanes ();
            }
        }

      if (link_group)
        {
          link_group->add_region (*this);
        }
    }

  /* reset the clip editor region because track_remove_region clears it */
  if (this == clip_editor_region)
    {
      CLIP_EDITOR->set_region (this, true);
    }

  /* reselect if necessary */
  select (selected, true, false);

  z_debug ("after: {}", print_to_str ());

  if (ZRYTHM_TESTING)
    {
      REGION_LINK_GROUP_MANAGER.validate ();
    }
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
          double   before_ticks = obj->pos_.ticks_;
          double   new_ticks = before_ticks * ratio;
          Position tmp (new_ticks);
          obj->pos_setter (&tmp);

          if (obj->has_length ())
            {
              /* set end pos */
              auto lengthable_obj =
                dynamic_cast<LengthableObject *> (obj.get ());
              before_ticks = lengthable_obj->end_pos_.ticks_;
              new_ticks = before_ticks * ratio;
              tmp = Position (new_ticks);
              lengthable_obj->end_pos_setter (&tmp);
            }
        }
    }
  else if constexpr (is_audio ())
    {
      auto * clip = derived_.get_clip ();
      int    new_clip_id = AUDIO_POOL->duplicate_clip (clip->pool_id_, false);
      z_return_if_fail (new_clip_id >= 0);
      auto * new_clip = AUDIO_POOL->get_clip (new_clip_id);
      derived_.set_clip_id (new_clip->pool_id_);

      auto stretcher = stretcher_new_rubberband (
        AUDIO_ENGINE->sample_rate_, new_clip->channels_, ratio, 1.0, false);

      float * new_frames = nullptr;
      ssize_t returned_frames = stretcher_stretch_interleaved (
        stretcher, new_clip->frames_.getReadPointer (0),
        static_cast<size_t> (new_clip->num_frames_), &new_frames);
      z_return_if_fail (returned_frames > 0);

      new_clip->frames_ = juce::AudioBuffer<sample_t> (
        1, static_cast<int> (returned_frames * new_clip->channels_));
      new_clip->frames_.copyFrom (
        0, 0, new_frames,
        static_cast<int> (returned_frames * new_clip->channels_));
      new_clip->num_frames_ = static_cast<unsigned_frame_t> (returned_frames);

      new_clip->write_to_pool (false, false);

      /* readjust end position to match the number of frames exactly */
      Position new_end_pos (static_cast<signed_frame_t> (returned_frames));
      set_position (&new_end_pos, ArrangerObject::PositionType::LoopEnd, false);
      new_end_pos.add_frames (pos_.frames_);
      set_position (&new_end_pos, ArrangerObject::PositionType::End, false);
    }
  else
    {
      static_assert (false, "unknown region type");
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
      if (found.get () != this)
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

std::shared_ptr<Region>
Region::find (const RegionIdentifier &id)
{
  std::shared_ptr<Region> result;

  auto try_find = [&]<typename RegionT> () {
    if (!result)
      {
        result = RegionImpl<RegionT>::find (id);
      }
  };

  try_find.operator()<MidiRegion> ();
  try_find.operator()<AudioRegion> ();
  try_find.operator()<AutomationRegion> ();
  try_find.operator()<ChordRegion> ();

  return result;
}

template <typename RegionT>
std::shared_ptr<RegionT>
RegionImpl<RegionT>::find (const RegionIdentifier &id)
{
  z_return_val_if_fail (id.track_name_hash_ != 0, nullptr);

  if constexpr (is_laned ())
    {
      z_return_val_if_fail (id.is_audio () || id.is_midi (), nullptr);
      auto track = TRACKLIST->find_track_by_name_hash<LanedTrackImpl<RegionT>> (
        id.track_name_hash_);
      z_return_val_if_fail (track, nullptr);

      if (id.lane_pos_ >= (int) track->lanes_.size ())
        {
          z_error (
            "given lane pos {} is greater than track '{}' ({}) number of lanes {}",
            id.lane_pos_, track->name_, track->pos_, track->lanes_.size ());
          return NULL;
        }
      auto &lane = track->lanes_[id.lane_pos_];
      z_return_val_if_fail (lane, nullptr);

      z_return_val_if_fail_cmp (id.idx_, >=, 0, nullptr);
      z_return_val_if_fail_cmp (
        id.idx_, <, (int) lane->regions_.size (), nullptr);

      return std::dynamic_pointer_cast<RegionT> (lane->regions_[id.idx_]);
    }
  else if constexpr (is_automation ())
    {
      z_return_val_if_fail (id.is_automation (), nullptr);
      auto track = TRACKLIST->find_track_by_name_hash<AutomatableTrack> (
        id.track_name_hash_);
      z_return_val_if_fail (track, nullptr);

      auto &atl = track->automation_tracklist_;
      z_return_val_if_fail (id.at_idx_ < (int) atl->ats_.size (), nullptr);
      auto &at = atl->ats_[id.at_idx_];
      z_return_val_if_fail (at, nullptr);

      if (id.idx_ >= (int) at->regions_.size ())
        {
          atl->print_regions ();
          z_error (
            "Automation track for {} has less regions ({}) than the given index {}",
            at->port_id_.get_label (), at->regions_.size (), id.idx_);
          return NULL;
        }

      return std::dynamic_pointer_cast<RegionT> (at->regions_[id.idx_]);
    }
  else if constexpr (is_chord ())
    {
      z_return_val_if_fail (id.is_chord (), nullptr);
      auto track = P_CHORD_TRACK;
      z_return_val_if_fail (track, nullptr);

      if (id.idx_ >= (int) track->regions_.size ())
        z_return_val_if_reached (nullptr);
      return dynamic_pointer_cast<RegionT> (track->regions_[id.idx_]);
    }
  else
    {
      static_assert (false, "unknown region type");
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
std::shared_ptr<typename RegionImpl<RegionT>::ChildT>
RegionImpl<RegionT>::remove_object (ChildT &obj, bool fire_events) requires
  RegionWithChildren<RegionT>
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

  using SharedPtrType = std::shared_ptr<ChildT>;
  using ObjVectorType = std::vector<SharedPtrType>;

  auto remove_type = [&] (ObjVectorType &objects, ChildT &_obj) -> SharedPtrType {
    /* find the object in the list */
    auto it = std::ranges::find_if (
      objects.begin (), objects.end (),
      [&_obj] (const auto &cur_ptr) { return cur_ptr.get () == &_obj; });
    z_return_val_if_fail (it != objects.end (), nullptr);

    /* get a shared pointer before erasing to possibly keep it alive */
    auto ret = *it;

    /* erase it */
    size_t erase_idx = std::distance (objects.begin (), it);
    it = objects.erase (it);

    /* set region and index for all objects after the erased one */
    for (size_t i = erase_idx; it != objects.end (); ++i, ++it)
      {
        (*it)->set_region_and_index (*this, i);
      }

    if (fire_events)
      {
        EVENTS_PUSH (EventType::ET_ARRANGER_OBJECT_REMOVED, ret->type_);
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
      static_assert (false, "Unsupported type");
    }
}

template <typename RegionT>
void
RegionImpl<RegionT>::remove_all_children () requires RegionWithChildren<RegionT>
{
  z_debug ("removing all children from {} {}", id_.idx_, name_);

  auto objs = get_objects ();
  for (auto &obj : objs)
    {
      remove_object (*obj);
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
          append_object (obj->clone_shared (), false);
        }
    }
  else if constexpr (std::is_same_v<RegionT, AutomationRegion>)
    {
      for (auto &obj : other.derived_.aps_)
        {
          append_object (obj->clone_shared (), false);
        }
    }
  else if constexpr (std::is_same_v<RegionT, ChordRegion>)
    {
      for (auto &obj : other.derived_.chord_objects_)
        {
          append_object (obj->clone_shared (), false);
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
    (void *) this, pos_.to_string (), end_pos_.to_string (),
    end_pos_.frames_ - pos_.frames_, end_pos_.ticks_ - pos_.ticks_,
    loop_end_pos_.to_string (), id_.link_group_);
}

template <typename RegionT>
RegionT *
RegionImpl<RegionT>::at_position (
  const Track *           track,
  const AutomationTrack * at,
  const Position          pos)
{
  auto region_is_at_pos = [] (const auto &region, const auto &_pos) {
    return region->pos_ <= _pos && region->end_pos_ >= _pos;
  };

  if constexpr (is_automation ())
    {
      z_return_val_if_fail (at, nullptr);

      for (auto &region : at->regions_)
        {
          if (region_is_at_pos (region, pos))
            return region.get ();
        }
    }
  else
    {
      z_return_val_if_fail (track, nullptr);

      if constexpr (is_laned ())
        {
          auto laned_track =
            dynamic_cast<const LanedTrackImpl<RegionT> *> (track);
          for (auto &lane : laned_track->lanes_)
            {
              for (auto &region : lane->regions_)
                {
                  if (region_is_at_pos (region, pos))
                    return region.get ();
                }
            }
        }
      else if constexpr (is_chord ())
        {
          for (auto &region : P_CHORD_TRACK->regions_)
            {
              if (region_is_at_pos (region, pos))
                return region.get ();
            }
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
      signed_frame_t diff_frames = timeline_frames - pos_.frames_;

      /* special case: timeline frames is exactly at the end of the region */
      if (timeline_frames == end_pos_.frames_)
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
  else
    {
      return timeline_frames - pos_.frames_;
    }
}

void
Region::get_frames_till_next_loop_or_end (
  const signed_frame_t timeline_frames,
  signed_frame_t *     ret_frames,
  bool *               is_loop) const
{
  signed_frame_t loop_size = get_loop_length_in_frames ();
  z_return_if_fail_cmp (loop_size, >, 0);

  signed_frame_t local_frames = timeline_frames - pos_.frames_;

  local_frames += clip_start_pos_.frames_;

  while (local_frames >= loop_end_pos_.frames_)
    {
      local_frames -= loop_size;
    }

  signed_frame_t frames_till_next_loop = loop_end_pos_.frames_ - local_frames;

  signed_frame_t frames_till_end = end_pos_.frames_ - timeline_frames;

  *is_loop = frames_till_next_loop < frames_till_end;
  *ret_frames = MIN (frames_till_end, frames_till_next_loop);
}

std::string
Region::generate_filename ()
{
  Track * track = get_track ();
  return fmt::sprintf (REGION_PRINTF_FILENAME, track->name_, name_);
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
RegionImpl<RegionT>::disconnect ()
{
  Region * clip_editor_region = CLIP_EDITOR->get_region ();
  if (clip_editor_region == this)
    {
      CLIP_EDITOR->set_region (nullptr, true);
    }

  if (TL_SELECTIONS)
    {
      TL_SELECTIONS->remove_object (*this);
    }

  if constexpr (RegionWithChildren<RegionT>)
    {
      auto objs = get_objects ();
      auto sel = get_arranger_selections ();
      for (auto &obj : objs)
        {
          sel->remove_object (*obj);
        }
    }

  if (ZRYTHM_HAVE_UI && MAIN_WINDOW)
    {
      ArrangerWidget * arranger = get_arranger ();
      if (arranger->hovered_object.lock ().get () == this)
        {
          arranger->hovered_object.reset ();
        }
    }
}

Region *
Region::get_at_pos (
  const Position    pos,
  Track *           track,
  AutomationTrack * at,
  bool              include_region_end)
{
  if (track)
    {
      if (track->has_lanes ())
        {
          auto laned_track_variant =
            convert_to_variant<LanedTrackPtrVariant> (track);
          auto found = std::visit (
            [&] (auto &&t) -> Region * {
              for (auto &lane : t->lanes_)
                {
                  auto ret = lane->get_region_at_pos (pos, include_region_end);
                  if (ret)
                    return ret;

                  for (auto &region : lane->regions_)
                    {
                      if (region->pos_ <= pos &&
                  (include_region_end? region->end_pos_ >= pos : region->end_pos_ > pos))
                        {
                          return region.get ();
                        }
                    }
                }
              return nullptr;
            },
            laned_track_variant);
          if (found)
            {
              return found;
            }
        }
      else if (track->is_chord ())
        {
          auto chord_track = dynamic_cast<ChordTrack *> (track);
          for (auto &region : chord_track->regions_)
            {
              if (
                region->pos_ <= pos
                && (include_region_end ? region->end_pos_ >= pos : region->end_pos_ > pos))
                {
                  return region.get ();
                }
            }
        }
    }
  else if (at)
    {
      for (auto &region : at->regions_)
        {
          if (
            region->pos_ <= pos
            && (include_region_end ? region->end_pos_ >= pos : region->end_pos_ > pos))
            {
              return region.get ();
            }
        }
    }
  return nullptr;
}

template <typename RegionT>
RegionOwnerImpl<RegionT> *
RegionImpl<RegionT>::get_region_owner () const
{
  if constexpr (is_laned ())
    {
      auto lane_owned_obj =
        dynamic_cast<const LaneOwnedObjectImpl<RegionT> *> (this);
      auto lane = lane_owned_obj->get_lane ();
      return lane;
    }
  else if constexpr (is_automation ())
    {
      auto automatable_track = dynamic_cast<AutomatableTrack *> (get_track ());
      z_return_val_if_fail (automatable_track, nullptr);
      auto &at =
        automatable_track->get_automation_tracklist ().ats_[id_.at_idx_];
      return at.get ();
    }
  else if constexpr (is_chord ())
    {
      return dynamic_cast<ChordTrack *> (
        TRACKLIST->find_track_by_name_hash<ChordTrack> (id_.track_name_hash_));
    }
}

template <typename RegionT>
std::shared_ptr<typename RegionImpl<RegionT>::ChildT>
RegionImpl<RegionT>::append_object (
  std::shared_ptr<ChildT> obj,
  bool                    fire_events) requires RegionWithChildren<RegionT>
{
  auto &objects = get_objects_vector ();
  return insert_object (obj, objects.size (), fire_events);
}

template class RegionImpl<MidiRegion>;
template class RegionImpl<AutomationRegion>;
template class RegionImpl<ChordRegion>;
template class RegionImpl<AudioRegion>;