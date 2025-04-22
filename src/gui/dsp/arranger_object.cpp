// SPDX-FileCopyrightText: © 2019-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/backend/project.h"
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
#include "gui/dsp/tracklist.h"
#include "utils/debug.h"
#include "utils/rt_thread_id.h"

using namespace zrythm;

ArrangerObject::ArrangerObject (Type type, TrackResolver track_resolver) noexcept
    : track_resolver_ (std::move (track_resolver)),
      pos_ (new (std::nothrow) PositionProxy ()), type_ (type)
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
ArrangerObject::is_position_valid (
  const Position    &pos,
  PositionType       pos_type,
  dsp::TicksPerFrame ticks_per_frame) const
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
                  expected_fade_out.add_frames (-frames_diff, ticks_per_frame);
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
                    expected_fade_out.add_frames (frames_diff, ticks_per_frame);
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
                  ticks_per_frame);
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
                  ticks_per_frame);
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
ArrangerObject::set_position (
  const Position    &pos,
  PositionType       pos_type,
  const bool         validate,
  dsp::TicksPerFrame ticks_per_frame)
{
  /* return if validate is on and position is invalid */
  if (validate && !is_position_valid (pos, pos_type, ticks_per_frame))
    return false;

  auto * pos_ptr = get_position_ptr (pos_type);
  z_return_val_if_fail (pos_ptr, false);
  *pos_ptr = pos;

  z_trace (
    "set {} position {} to {}", fmt::ptr (this), ENUM_NAME (pos_type), pos);

  return true;
}

void
ArrangerObject::move (
  const double             ticks,
  const dsp::FramesPerTick frames_per_tick)
{
  const auto ticks_per_frame = dsp::to_ticks_per_frame (frames_per_tick);
  std::visit (
    [&] (auto &&self) {
      using ObjT = base_type<decltype (self)>;
      if constexpr (std::derived_from<ObjT, BoundedObject>)
        {
          signed_frame_t length_frames = self->get_length_in_frames ();

          /* start pos */
          auto tmp = get_position ();
          tmp.add_ticks (ticks, frames_per_tick);
          set_position (tmp, PositionType::Start, false, ticks_per_frame);

          /* end pos */
          if constexpr (std::derived_from<ObjT, Region>)
            {
              /* audio regions need the exact number of frames to match the clip.
               *
               * this should be used for all objects but currently moving
               * objects before 1.1.1.1 causes bugs hence this (temporary) if
               * statement */
              tmp.add_frames (length_frames, ticks_per_frame);
            }
          else
            {
              tmp = self->get_end_position ();
              tmp.add_ticks (ticks, frames_per_tick);
            }
          set_position (tmp, PositionType::End, false, ticks_per_frame);
        }
      else
        {
          auto tmp = get_position ();
          tmp.add_ticks (ticks, frames_per_tick);
          set_position (tmp, PositionType::Start, false, ticks_per_frame);
        }
    },
    convert_to_variant<ArrangerObjectPtrVariant> (this));
}

void
ArrangerObject::update_positions (
  bool               from_ticks,
  bool               bpm_change,
  dsp::FramesPerTick frames_per_tick)
{
  z_return_if_fail (type_safe::get (frames_per_tick) > 1e-10);

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

      const auto update_position =
        [frames_per_tick, from_ticks] (dsp::Position &pos) {
          const auto ticks_per_frame = dsp::to_ticks_per_frame (frames_per_tick);

          if (from_ticks)
            {
              pos.update_frames_from_ticks (frames_per_tick);
            }
          else
            {
              pos.update_ticks_from_frames (ticks_per_frame);
            }
        };

      update_position (*obj->pos_);
      if constexpr (std::derived_from<ObjT, BoundedObject>)
        {
          update_position (*obj->end_pos_);
        }
      if constexpr (std::derived_from<ObjT, LoopableObject>)
        {
          update_position (obj->clip_start_pos_);
          update_position (obj->loop_start_pos_);
          update_position (obj->loop_end_pos_);
        }
      if constexpr (std::derived_from<ObjT, FadeableObject>)
        {
          update_position (obj->fade_in_pos_);
          update_position (obj->fade_out_pos_);
        }

      if constexpr (std::is_same_v<ObjT, AudioRegion>)
        {
          if (!obj->get_musical_mode ())
            {
              signed_frame_t frames_len_after = obj->get_length_in_frames ();
              if (bpm_change && frames_len_after != frames_len_before)
                {
                  double ticks =
                    static_cast<double> (frames_len_before - frames_len_after)
                    * type_safe::get (dsp::to_ticks_per_frame (frames_per_tick));
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
                  double ticks =
                    -type_safe::get (dsp::to_ticks_per_frame (frames_per_tick));
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
              cur_obj->update_positions (
                from_ticks, bpm_change, frames_per_tick);
            }
        }
    },
    convert_to_variant<ArrangerObjectPtrVariant> (this));
}

#if 0
/**
 * Callback when beginning to edit the object.
 *
 * This saves a clone of its current state to its arranger.
 */
void
ArrangerObject::edit_begin () const
{
  auto arranger = get_arranger ();
  z_return_if_fail (arranger);
  auto variant = convert_to_variant<ArrangerObjectPtrVariant> (this);
  std::visit (
    [&arranger] (auto &&obj) { arranger->start_object = obj->clone_unique (); },
    variant);
}

/**
 * Callback when finishing editing the object.
 *
 * This performs an undoable action.
 *
 * @param action_edit_type A @ref ArrangerSelectionsAction::EditType.
 */
void
ArrangerObject::edit_finish (int action_edit_type) const
{
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
}

void
ArrangerObject::edit_position_finish () const
{
  edit_finish (ENUM_VALUE_TO_INT (
    gui::actions::ArrangerSelectionsAction::EditType::Position));
}
#endif

void
ArrangerObject::position_setter_validated (
  const Position    &pos,
  dsp::TicksPerFrame ticks_per_frame)
{
  bool success = set_position (pos, PositionType::Start, true, ticks_per_frame);
  if (!success)
    {
      z_debug ("failed to set position [{}]: (invalid)", pos);
    }
}

bool
ArrangerObject::are_members_valid (
  bool               is_project,
  dsp::FramesPerTick frames_per_tick) const
{
  if (!is_position_valid (
        *pos_, PositionType::Start, dsp::to_ticks_per_frame (frames_per_tick)))
    {
      z_trace ("invalid start pos {}", *pos_);
      return false;
    }
  return true;
}

void
ArrangerObject::copy_members_from (
  const ArrangerObject &other,
  ObjectCloneType       clone_type)
{
  pos_ = other.pos_->clone_qobject (dynamic_cast<QObject *> (this));
  type_ = other.type_;
  track_id_ = other.track_id_;
}
