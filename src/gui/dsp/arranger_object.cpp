// SPDX-FileCopyrightText: © 2019-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/backend/actions/arranger_selections_action.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/settings_manager.h"
#include "gui/backend/backend/zrythm.h"
#include "gui/dsp/arranger_object.h"
#include "gui/dsp/audio_region.h"
#include "gui/dsp/automation_point.h"
#include "gui/dsp/automation_region.h"
#include "gui/dsp/chord_object.h"
#include "gui/dsp/chord_region.h"
#include "gui/dsp/chord_track.h"
#include "gui/dsp/marker.h"
#include "gui/dsp/marker_track.h"
#include "gui/dsp/midi_region.h"
#include "gui/dsp/router.h"
#include "gui/dsp/tracklist.h"
#include "utils/debug.h"
#include "utils/gtest_wrapper.h"
#include "utils/logger.h"
#include "utils/rt_thread_id.h"

using namespace zrythm;

ArrangerObject::ArrangerObject (Type type)
    : pos_ (new PositionProxy ()), type_ (type)
{
}

void
ArrangerObject::set_parent_on_base_qproperties (QObject &derived)
{
  pos_->setParent (&derived);
}

std::optional<ArrangerObjectPtrVariant>
ArrangerObject::remove_from_project (bool free_obj, bool fire_events)
{
  /* TODO make sure no event contains this object */
  /*event_manager_remove_events_for_obj (*/
  /*EVENT_MANAGER, obj);*/

  // FIXME: this method should be deleted/moved elsewhere
  // arranger objects shouldn't know so much about their parents
  return std::nullopt;

#if 0
  return std::visit (
    [&] (auto &&obj) -> std::optional<ArrangerObjectPtrVariant> {
      using ObjT = base_type<decltype (obj)>;

      if constexpr (std::is_same_v<ObjT, Velocity>)
        {
          z_return_val_if_reached (std::nullopt);
        }
      else if constexpr (std::derived_from<ObjT, RegionOwnedObject>)
        {
          auto * region = obj->get_region ();
          z_return_val_if_fail (region, std::nullopt);
          region->remove_object (obj->get_uuid ());
          region->update_link_group ();
        }
      else if constexpr (std::derived_from<ObjT, Region>)
        {
          auto * owner = obj->get_region_owner ();
          z_return_val_if_fail (owner, std::nullopt);
          owner->remove_region (*obj);
        }
      else if constexpr (std::is_same_v<ObjT, ScaleObject>)
        {
          P_CHORD_TRACK->remove_scale (obj->get_uuid ());
        }
      else if constexpr (std::is_same_v<ObjT, Marker>)
        {
          P_MARKER_TRACK->remove_marker (obj->get_uuid ());
        }

      if (free_obj)
        {
          return std::nullopt;
        }
      return obj;
    },
    convert_to_variant<ArrangerObjectPtrVariant> (this));
#endif
}

ArrangerObject::Position *
ArrangerObject::get_position_ptr (PositionType pos_type)
{
  switch (pos_type)
    {
    case PositionType::Start:
      return pos_;
    case PositionType::End:
      {
        auto lo = dynamic_cast<BoundedObject *> (this);
        z_return_val_if_fail (lo != nullptr, nullptr);
        return lo->end_pos_;
      }
    case PositionType::ClipStart:
      {
        auto lo = dynamic_cast<LoopableObject *> (this);
        z_return_val_if_fail (lo != nullptr, nullptr);
        return &lo->clip_start_pos_;
      }
    case PositionType::LoopStart:
      {
        auto lo = dynamic_cast<LoopableObject *> (this);
        z_return_val_if_fail (lo != nullptr, nullptr);
        return &lo->loop_start_pos_;
      }
    case PositionType::LoopEnd:
      {
        auto lo = dynamic_cast<LoopableObject *> (this);
        z_return_val_if_fail (lo != nullptr, nullptr);
        return &lo->loop_end_pos_;
      }
    case PositionType::FadeIn:
      {
        auto fo = dynamic_cast<FadeableObject *> (this);
        z_return_val_if_fail (fo != nullptr, nullptr);
        return &fo->fade_in_pos_;
      }
    case PositionType::FadeOut:
      {
        auto fo = dynamic_cast<FadeableObject *> (this);
        z_return_val_if_fail (fo != nullptr, nullptr);
        return &fo->fade_out_pos_;
      }
    }
  z_return_val_if_reached (nullptr);
}

bool
ArrangerObject::is_position_valid (const Position &pos, PositionType pos_type)
  const
{
  auto obj_var = convert_to_variant<ArrangerObjectPtrVariant> (this);
  return std::visit (
    [&] (auto &&obj) {
      using ObjT = base_type<decltype (obj)>;
      switch (pos_type)
        {
        case PositionType::Start:
          if constexpr (std::derived_from<ObjT, BoundedObject>)
            {
              if (pos >= *obj->end_pos_)
                return false;

              if constexpr (!std::derived_from<ObjT, RegionOwnedObject>)
                {
                  if (pos < Position ())
                    return false;
                }

              if constexpr (std::derived_from<ObjT, FadeableObject>)
                {
                  signed_frame_t frames_diff =
                    pos.get_total_frames () - this->pos_->get_total_frames ();
                  Position expected_fade_out = obj->fade_out_pos_;
                  expected_fade_out.add_frames (
                    -frames_diff, AUDIO_ENGINE->ticks_per_frame_);
                  if (expected_fade_out <= obj->fade_in_pos_)
                    return false;
                }

              return true;
            }
          else if constexpr (std::derived_from<ObjT, TimelineObject>)
            {
              return pos >= Position ();
            }
          else
            {
              return true;
            }
          break;
        case PositionType::LoopStart:
          {
            if constexpr (std::derived_from<ObjT, LoopableObject>)
              {
                return pos < obj->loop_end_pos_ && pos >= Position ();
              }
          }
          break;
        case PositionType::LoopEnd:
          {
            if constexpr (std::derived_from<ObjT, LoopableObject>)
              {
                if (pos < obj->clip_start_pos_ || pos <= obj->loop_start_pos_)
                  return false;

                bool is_valid = true;
                if constexpr (std::is_same_v<ObjT, AudioRegion>)
                  {
                    AudioClip * clip = obj->get_clip ();
                    is_valid =
                      pos.frames_
                      <= static_cast<signed_frame_t> (clip->get_num_frames ());
                  }
                return is_valid;
              }
          }
          break;
        case PositionType::ClipStart:
          {
            if constexpr (std::derived_from<ObjT, LoopableObject>)
              {
                return pos < obj->loop_end_pos_ && pos >= Position ();
              }
          }
          break;
        case PositionType::End:
          {
            if constexpr (std::derived_from<ObjT, BoundedObject>)
              {
                bool is_valid = pos > *pos_;

                if constexpr (std::derived_from<ObjT, FadeableObject>)
                  {
                    signed_frame_t frames_diff =
                      pos.get_total_frames ()
                      - obj->end_pos_->get_total_frames ();
                    Position expected_fade_out = obj->fade_out_pos_;
                    expected_fade_out.add_frames (
                      frames_diff, AUDIO_ENGINE->ticks_per_frame_);
                    if (expected_fade_out <= obj->fade_in_pos_)
                      return false;
                  }
                return is_valid;
              }
          }
          break;
        case PositionType::FadeIn:
          {
            if constexpr (std::derived_from<ObjT, FadeableObject>)
              {
                Position local_end_pos (
                  obj->end_pos_->get_total_frames ()
                    - this->pos_->get_total_frames (),
                  AUDIO_ENGINE->ticks_per_frame_);
                return pos >= Position () && pos.get_total_frames () >= 0
                       && pos < local_end_pos && pos < obj->fade_out_pos_;
              }
          }
          break;
        case PositionType::FadeOut:
          {
            if constexpr (std::derived_from<ObjT, FadeableObject>)
              {
                Position local_end_pos (
                  obj->end_pos_->get_total_frames ()
                    - this->pos_->get_total_frames (),
                  AUDIO_ENGINE->ticks_per_frame_);
                return pos >= Position () && pos <= local_end_pos
                       && pos > obj->fade_in_pos_;
              }
          }
          break;
        }

      z_return_val_if_reached (false);
    },
    obj_var);
}

bool
ArrangerObject::
  set_position (const Position &pos, PositionType pos_type, const bool validate)
{
  /* return if validate is on and position is invalid */
  if (validate && !is_position_valid (pos, pos_type))
    return false;

  auto * pos_ptr = get_position_ptr (pos_type);
  z_return_val_if_fail (pos_ptr, false);
  *pos_ptr = pos;

  z_trace (
    "set {} position {} to {}", fmt::ptr (this), ENUM_NAME (pos_type), pos);

  return true;
}

void
ArrangerObject::move (const double ticks)
{
  std::visit (
    [&] (auto &&self) {
      using ObjT = base_type<decltype (self)>;
      if constexpr (std::derived_from<ObjT, BoundedObject>)
        {
          signed_frame_t length_frames = self->get_length_in_frames ();

          /* start pos */
          Position tmp;
          get_pos (&tmp);
          tmp.add_ticks (ticks, AUDIO_ENGINE->frames_per_tick_);
          set_position (tmp, PositionType::Start, false);

          /* end pos */
          if constexpr (std::derived_from<ObjT, Region>)
            {
              /* audio regions need the exact number of frames to match the
               * clip.
               *
               * this should be used for all objects but currently moving
               * objects before 1.1.1.1 causes bugs hence this (temporary) if
               * statement */
              tmp.add_frames (length_frames, AUDIO_ENGINE->ticks_per_frame_);
            }
          else
            {
              self->get_end_pos (&tmp);
              tmp.add_ticks (ticks, AUDIO_ENGINE->frames_per_tick_);
            }
          set_position (tmp, PositionType::End, false);
        }
      else
        {
          Position tmp;
          get_pos (&tmp);
          tmp.add_ticks (ticks, AUDIO_ENGINE->frames_per_tick_);
          set_position (tmp, PositionType::Start, false);
        }
    },
    convert_to_variant<ArrangerObjectPtrVariant> (this));
}

void
ArrangerObject::get_position_from_type (Position * pos, PositionType type) const
{
  switch (type)
    {
    case PositionType::Start:
      get_pos (pos);
      break;
    case PositionType::ClipStart:
      {
        auto const * lo = dynamic_cast<const LoopableObject *> (this);
        *pos = lo->get_clip_start_pos ();
      }
      break;
    case PositionType::End:
      {
        auto const * lo = dynamic_cast<const BoundedObject *> (this);
        lo->get_end_pos (pos);
      }
      break;
    case PositionType::LoopStart:
      {
        auto const * lo = dynamic_cast<const LoopableObject *> (this);
        *pos = lo->get_loop_start_pos ();
      }
      break;
    case PositionType::LoopEnd:
      {
        auto const * lo = dynamic_cast<const LoopableObject *> (this);
        *pos = lo->get_loop_end_pos ();
      }
      break;
    case PositionType::FadeIn:
      {
        auto const * fo = dynamic_cast<const FadeableObject *> (this);
        fo->get_fade_in_pos (pos);
      }
      break;
    case PositionType::FadeOut:
      {
        auto const * fo = dynamic_cast<const FadeableObject *> (this);
        fo->get_fade_out_pos (pos);
      }
      break;
    }
}

void
ArrangerObject::
  update_positions (bool from_ticks, bool bpm_change, double frames_per_tick)
{
  z_return_if_fail (frames_per_tick > 1e-10);

  std::visit (
    [from_ticks, bpm_change, frames_per_tick] (auto &&obj) {
      using ObjT = base_type<decltype (obj)>;
      signed_frame_t frames_len_before = 0;
      if constexpr (std::derived_from<ObjT, BoundedObject>)
        {
          if (bpm_change)
            {
              frames_len_before = obj->get_length_in_frames ();
            }
        }

      const double ratio = from_ticks ? frames_per_tick : 1.0 / frames_per_tick;

      obj->pos_->update (from_ticks, ratio);
      if constexpr (std::derived_from<ObjT, BoundedObject>)
        {
          obj->end_pos_->update (from_ticks, ratio);

          if (ROUTER->is_processing_kickoff_thread ())
            {
              /* do some validation */
              z_return_if_fail (
                obj->is_position_valid (*obj->end_pos_, PositionType::End));
            }
        }
      if constexpr (std::derived_from<ObjT, LoopableObject>)
        {
          obj->clip_start_pos_.update (from_ticks, ratio);
          obj->loop_start_pos_.update (from_ticks, ratio);
          obj->loop_end_pos_.update (from_ticks, ratio);
        }
      if constexpr (std::derived_from<ObjT, FadeableObject>)
        {
          obj->fade_in_pos_.update (from_ticks, ratio);
          obj->fade_out_pos_.update (from_ticks, ratio);
        }

      if constexpr (std::is_same_v<ObjT, AudioRegion>)
        {
          if (!obj->get_musical_mode ())
            {
              signed_frame_t frames_len_after = obj->get_length_in_frames ();
              if (bpm_change && frames_len_after != frames_len_before)
                {
                  double ticks =
                    (frames_len_before - frames_len_after)
                    * AUDIO_ENGINE->ticks_per_frame_;
                  try
                    {
                      obj->resize (
                        false, ArrangerObject::ResizeType::StretchTempoChange,
                        ticks, false);
                    }
                  catch (const ZrythmException &e)
                    {
                      e.handle ("Failed to resize object");
                      return;
                    }
                }
              z_return_if_fail_cmp (obj->loop_end_pos_.frames_, >=, 0);
              signed_frame_t tl_frames = obj->end_pos_->frames_ - 1;
              signed_frame_t local_frames;
              AudioClip *    clip;
              local_frames = obj->timeline_frames_to_local (tl_frames, true);
              clip = obj->get_clip ();
              z_return_if_fail (clip);

              /* sometimes due to rounding errors, the region frames are 1
               * frame more than the clip frames. this works around it by
               * resizing the region by -1 frame*/
              while (local_frames == (signed_frame_t) clip->get_num_frames ())
                {
                  z_debug ("adjusting for rounding error");
                  double ticks = -AUDIO_ENGINE->ticks_per_frame_;
                  z_debug ("ticks {:f}", ticks);
                  try
                    {
                      obj->resize (
                        false, ArrangerObject::ResizeType::StretchTempoChange,
                        ticks, false);
                    }
                  catch (const ZrythmException &e)
                    {
                      e.handle ("Failed to resize object");
                      return;
                    }
                  local_frames = obj->timeline_frames_to_local (tl_frames, true);
                }

              z_return_if_fail_cmp (
                local_frames, <, (signed_frame_t) clip->get_num_frames ());
            }
        } // end if audio region

      else if constexpr (RegionWithChildren<ObjT>)
        {
          for (auto * cur_obj : obj->get_children_view ())
            {
              cur_obj->update_positions (from_ticks, bpm_change, ratio);
            }
        }
    },
    convert_to_variant<ArrangerObjectPtrVariant> (this));
}

void
ArrangerObject::post_deserialize ()
{
  std::visit (
    [&] (auto &&self) {
      using ObjT = base_type<decltype (self)>;
      if constexpr (std::is_same_v<ObjT, AudioRegion>)
        {
          // self->read_from_pool_ = true;
        }

      /* TODO: this acts as if a BPM change happened (and is only effective if
       * so), so if no BPM change happened this is unnecessary, so this should
       * be refactored in the future. this was added to fix copy-pasting audio
       * regions after changing the BPM (see #4993) */
      update_positions (true, true, AUDIO_ENGINE->frames_per_tick_);

      if constexpr (RegionWithChildren<ObjT>)
        {
          self->post_deserialize_children ();
        }
    },
    convert_to_variant<ArrangerObjectPtrVariant> (this));
}

void
ArrangerObject::edit_begin () const
{
#if 0
  auto arranger = get_arranger ();
  z_return_if_fail (arranger);
  auto variant = convert_to_variant<ArrangerObjectPtrVariant> (this);
  std::visit (
    [&arranger] (auto &&obj) { arranger->start_object = obj->clone_unique (); },
    variant);
#endif
}

void
ArrangerObject::edit_finish (int action_edit_type) const
{
#if 0
  auto arranger = get_arranger ();
  z_return_if_fail (arranger);
  z_return_if_fail (arranger->start_object);
  try
    {
      UNDO_MANAGER->perform (EditArrangerSelectionsAction::create (
        *arranger->start_object, *this,
        ENUM_INT_TO_VALUE (ArrangerSelectionsAction::EditType, action_edit_type),
        true));
    }
  catch (const ZrythmException &e)
    {
      e.handle (QObject::tr ("Failed to edit object"));
    }
  arranger->start_object.reset ();
#endif
}

void
ArrangerObject::edit_position_finish () const
{
  edit_finish (ENUM_VALUE_TO_INT (
    gui::actions::ArrangerSelectionsAction::EditType::Position));
}

void
ArrangerObject::pos_setter (const Position &pos)
{
  bool success = set_position (pos, PositionType::Start, true);
  if (!success)
    {
      z_debug ("failed to set position [{}]: (invalid)", pos);
    }
}

bool
ArrangerObject::are_members_valid (bool is_project) const
{
  if (!is_position_valid (*pos_, PositionType::Start))
    {
      if (ZRYTHM_TESTING)
        {
          z_info ("invalid start pos {}", *pos_);
        }
      return false;
    }
  return true;
}

TrackPtrVariant
ArrangerObject::get_track () const
{
  if (track_)
    return *track_;

  if (track_id_.is_null ()) [[unlikely]]
    {
      throw ZrythmException ("track_name_hash_ is 0");
    }

  const auto &tracklist =
    is_auditioner_ ? *SAMPLE_PROCESSOR->tracklist_ : *TRACKLIST;

  auto track_opt = tracklist.get_track (track_id_);
  if (!track_opt) [[unlikely]]
    {
      throw ZrythmException ("track not found");
    }

  return track_opt.value ();
}

bool
ArrangerObject::is_frozen () const
{
  return std::visit (
    [&] (auto &&track) { return track->is_frozen (); }, get_track ());
}

void
ArrangerObject::copy_members_from (
  const ArrangerObject &other,
  ObjectCloneType       clone_type)
{
  pos_ = other.pos_->clone_raw_ptr ();
  if (auto * qobject = dynamic_cast<QObject *> (this))
    {
      pos_->setParent (qobject);
    }
  type_ = other.type_;
  track_id_ = other.track_id_;
  deleted_temporarily_ = other.deleted_temporarily_;
}

void
ArrangerObject::init_loaded_base () {};
