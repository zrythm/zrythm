// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/backend/zrythm.h"
#include "gui/dsp/fadeable_object.h"
#include "gui/dsp/lengthable_object.h"
#include "gui/dsp/loopable_object.h"
#include "gui/dsp/midi_note.h"
#include "gui/dsp/region.h"

#include "utils/debug.h"
#include "utils/dsp.h"
#include "utils/flags.h"
#include "utils/gtest_wrapper.h"
#include "utils/rt_thread_id.h"

LengthableObject::LengthableObject () : end_pos_ (new PositionProxy ()) { }

void
LengthableObject::parent_base_qproperties (QObject &derived)
{
  end_pos_->setParent (&derived);
}

void
LengthableObject::copy_members_from (const LengthableObject &other)
{
  end_pos_ = other.end_pos_->clone_raw_ptr ();
  if (auto * qobject = dynamic_cast<QObject *> (this))
    {
      end_pos_->setParent (qobject);
    }
}

void
LengthableObject::init_loaded_base ()
{
}

bool
LengthableObject::are_members_valid (bool is_project) const
{
  if (!is_position_valid (*end_pos_, PositionType::End))
    {
      if (ZRYTHM_TESTING)
        {
          z_info ("invalid end pos {}", *end_pos_);
        }
      return false;
    }

  return true;
}

void
LengthableObject::resize (
  const bool   left,
  ResizeType   type,
  const double ticks,
  bool         during_ui_action)
{
  z_trace (
    "resizing object{{ left: {}, type: {}, ticks: {}, during UI action: {}}}",
    left, ENUM_NAME (type), ticks, during_ui_action);

  double   before_length = get_length_in_ticks ();
  Position tmp;
  if (left)
    {
      if (type == ResizeType::Fade)
        {
          tmp = dynamic_cast<FadeableObject *> (this)->fade_in_pos_;
          tmp.add_ticks (ticks, AUDIO_ENGINE->frames_per_tick_);
          set_position (&tmp, PositionType::FadeIn, false);
        }
      else
        {
          tmp = *static_cast<Position *> (pos_);
          tmp.add_ticks (ticks, AUDIO_ENGINE->frames_per_tick_);
          set_position (&tmp, PositionType::Start, false);

          if (can_fade ())
            {
              tmp = dynamic_cast<FadeableObject *> (this)->fade_out_pos_;
              tmp.add_ticks (-ticks, AUDIO_ENGINE->frames_per_tick_);
              set_position (&tmp, PositionType::FadeOut, false);
            }

          if (type == ResizeType::Loop)
            {
              auto   lo = dynamic_cast<LoopableObject *> (this);
              double loop_len = lo->get_loop_length_in_ticks ();

              /* if clip start is not before loop start, adjust clip start pos */
              Position clip_start_pos = lo->clip_start_pos_;
              Position loop_start_pos = lo->loop_start_pos_;
              if (clip_start_pos >= loop_start_pos)
                {
                  clip_start_pos.add_ticks (
                    ticks, AUDIO_ENGINE->frames_per_tick_);

                  while (clip_start_pos < loop_start_pos)
                    {
                      clip_start_pos.add_ticks (
                        loop_len, AUDIO_ENGINE->frames_per_tick_);
                    }
                  lo->clip_start_pos_setter (&clip_start_pos);
                }

              /* make sure clip start goes back to loop start if it exceeds loop
               * end */
              Position loop_end_pos = lo->loop_end_pos_;
              while (clip_start_pos > loop_end_pos)
                {
                  clip_start_pos.add_ticks (
                    -loop_len, AUDIO_ENGINE->frames_per_tick_);
                }
              lo->clip_start_pos_setter (&clip_start_pos);
            }
          else if (can_loop ())
            {
              auto lo = dynamic_cast<LoopableObject *> (this);
              tmp = lo->loop_end_pos_;
              tmp.add_ticks (-ticks, AUDIO_ENGINE->frames_per_tick_);
              set_position (&tmp, PositionType::LoopEnd, false);

              /* move containing items */
              auto r = dynamic_cast<Region *> (this);
              r->add_ticks_to_children (-ticks);
            }
        }
    }
  /* else if resizing right side */
  else
    {
      if (type == ResizeType::Fade)
        {
          auto * fo = dynamic_cast<FadeableObject *> (this);
          tmp = fo->fade_out_pos_;
          tmp.add_ticks (ticks, AUDIO_ENGINE->frames_per_tick_);
          set_position (&tmp, PositionType::FadeOut, false);
        }
      else
        {
          tmp = *end_pos_;
          Position prev_end_pos = tmp;
          tmp.add_ticks (ticks, AUDIO_ENGINE->frames_per_tick_);
          set_position (&tmp, PositionType::End, false);

          double change_ratio =
            (get_length_in_ticks ()) / (prev_end_pos.ticks_ - pos_->ticks_);

          if (type != ResizeType::Loop && can_loop ())
            {
              auto * lo = dynamic_cast<LoopableObject *> (this);
              tmp = lo->loop_end_pos_;
              if (
                type == ResizeType::RESIZE_STRETCH_BPM_CHANGE
                || type == ResizeType::Stretch)
                {
                  tmp.from_ticks (
                    tmp.ticks_ * change_ratio, AUDIO_ENGINE->frames_per_tick_);
                }
              else
                {
                  tmp.add_ticks (ticks, AUDIO_ENGINE->frames_per_tick_);
                }
              z_return_if_fail (tmp.is_positive ());
              set_position (&tmp, PositionType::LoopEnd, false);
              z_return_if_fail (lo->loop_end_pos_.is_positive ());

              /* if stretching, also stretch loop start */
              if (
                type == ResizeType::RESIZE_STRETCH_BPM_CHANGE
                || type == ResizeType::Stretch)
                {
                  tmp = lo->loop_start_pos_;
                  tmp.from_ticks (
                    tmp.ticks_ * change_ratio, AUDIO_ENGINE->frames_per_tick_);
                  z_return_if_fail (tmp.is_positive ());
                  set_position (&tmp, PositionType::LoopStart, false);
                  z_return_if_fail (lo->loop_start_pos_.is_positive ());
                }
            }
          if (can_fade ())
            {
              auto fo = dynamic_cast<FadeableObject *> (this);
              tmp = fo->fade_out_pos_;
              if (
                type == ResizeType::RESIZE_STRETCH_BPM_CHANGE
                || type == ResizeType::Stretch)
                {
                  tmp.from_ticks (
                    tmp.ticks_ * change_ratio, AUDIO_ENGINE->frames_per_tick_);
                }
              else
                {
                  tmp.add_ticks (ticks, AUDIO_ENGINE->frames_per_tick_);
                }
              z_return_if_fail (tmp.is_positive ());
              set_position (&tmp, PositionType::FadeOut, false);
              z_return_if_fail (fo->fade_out_pos_.is_positive ());

              /* if stretching, also stretch fade in */
              if (
                type == ResizeType::RESIZE_STRETCH_BPM_CHANGE
                || type == ResizeType::Stretch)
                {
                  tmp = fo->fade_in_pos_;
                  tmp.from_ticks (
                    tmp.ticks_ * change_ratio, AUDIO_ENGINE->frames_per_tick_);
                  z_return_if_fail (tmp.is_positive ());
                  set_position (&tmp, PositionType::FadeIn, false);
                  z_return_if_fail (fo->fade_in_pos_.is_positive ());
                }
            }

          if (type == ResizeType::Stretch && is_region ())
            {
              std::visit (
                [&] (auto &&region) {
                  double new_length = get_length_in_ticks ();

                  if (type != ResizeType::RESIZE_STRETCH_BPM_CHANGE)
                    {
                      /* FIXME this flag is not good,
                       * remove from this function and
                       * do it in the arranger */
                      if (during_ui_action)
                        {
                          region->stretch_ratio_ =
                            new_length / region->before_length_;
                        }
                      /* else if as part of an action */
                      else
                        {
                          /* stretch contents */
                          double stretch_ratio = new_length / before_length;
                          try
                            {
                              region->stretch (stretch_ratio);
                            }
                          catch (const ZrythmException &ex)
                            {
                              throw ZrythmException ("Failed to stretch region");
                            }
                        }
                    }
                },
                convert_to_variant<RegionPtrVariant> (this));
            }
        }
    }
}

void
LengthableObject::set_loop_and_fade_to_full_size ()
{
  if (can_loop ())
    {
      auto ticks = get_length_in_ticks ();
      auto loopable_object = dynamic_cast<LoopableObject *> (this);
      loopable_object->loop_end_pos_.from_ticks (
        ticks, AUDIO_ENGINE->frames_per_tick_);
    }
  if (can_fade ())
    {
      auto ticks = get_length_in_ticks ();
      auto fadeable_object = dynamic_cast<FadeableObject *> (this);
      fadeable_object->fade_out_pos_.from_ticks (
        ticks, AUDIO_ENGINE->frames_per_tick_);
    }
}

void
LengthableObject::set_start_pos_full_size (const Position * pos)
{
  pos_setter (pos);
  set_loop_and_fade_to_full_size ();
  z_warn_if_fail (pos->frames_ == pos_->frames_);
}

void
LengthableObject::set_end_pos_full_size (const Position * pos)
{
  end_pos_setter (pos);
  set_loop_and_fade_to_full_size ();
  z_warn_if_fail (pos->frames_ == end_pos_->frames_);
}

template <typename T>
std::pair<T *, T *>
LengthableObject::
  split (T &self, const Position pos, const bool pos_is_local, bool is_project)
{
  static_assert (FinalLengthedObjectSubclass<T>);

  using SelfT = T;

  /* create the new objects */
  auto * r1 = self.clone_raw_ptr ();
  auto * r2 = self.clone_raw_ptr ();

  z_debug ("splitting objects...");

  bool set_clip_editor_region = false;
  if constexpr (RegionSubclass<SelfT>)
    {
      if (is_project)
        {
          /* change to r1 if the original region was the clip editor region */
          auto clip_editor_region = CLIP_EDITOR->get_region ();
          if (clip_editor_region)
            {
              if (
                std::holds_alternative<T *> (clip_editor_region.value ())
                && std::get<T *> (clip_editor_region.value ()) == &self)
                {
                  set_clip_editor_region = true;
                  CLIP_EDITOR->set_region (std::nullopt, true);
                }
            }
        }
    }

  /* get global/local positions (the local pos is after traversing the
   * loops) */
  auto [globalp, localp] = [&] () {
    if (pos_is_local)
      {
        Position global = pos;
        global.add_ticks (self.pos_->ticks_, AUDIO_ENGINE->frames_per_tick_);
        return std::make_pair (global, pos);
      }
    else
      {
        Position local = pos;
        if constexpr (RegionSubclass<SelfT>)
          {
            long local_frames =
              self.timeline_frames_to_local (pos.frames_, true);
            local.from_frames (local_frames, AUDIO_ENGINE->ticks_per_frame_);
          }
        return std::make_pair (pos, local);
      }
  }();

  /*
   * for first object set:
   * - end pos
   * - fade out pos
   */
  r1->end_pos_setter (&globalp);
  r1->set_position (&localp, ArrangerObject::PositionType::FadeOut, false);

  if constexpr (std::derived_from<SelfT, LoopableObject>)
    {
      /* if original object was not looped, make the new object unlooped
       * also */
      if (!self.is_looped ())
        {
          r1->loop_end_pos_setter (&localp);

          if constexpr (std::is_same_v<SelfT, AudioRegion>)
            {
              /* create new audio region */
              auto prev_r1 = r1;
              auto prev_r1_clip = prev_r1->get_clip ();
              z_return_val_if_fail (
                prev_r1_clip, std::make_pair (nullptr, nullptr));
              std::vector<float> frames;
              frames.resize ((size_t) localp.frames_ * prev_r1_clip->channels_);
              utils::float_ranges::copy (
                &frames[0], &prev_r1_clip->frames_.getReadPointer (0)[0],
                (size_t) localp.frames_ * prev_r1_clip->channels_);
              z_return_val_if_fail (
                !prev_r1->name_.empty (), std::make_pair (nullptr, nullptr));
              z_return_val_if_fail_cmp (
                localp.frames_, >=, 0, std::make_pair (nullptr, nullptr));
              auto new_r1 = new AudioRegion (
                -1, std::nullopt, true, frames.data (),
                (unsigned_frame_t) localp.frames_, prev_r1->name_,
                prev_r1_clip->channels_, prev_r1_clip->bit_depth_,
                *prev_r1->pos_, prev_r1->id_.track_name_hash_,
                prev_r1->id_.lane_pos_, prev_r1->id_.idx_);
              z_return_val_if_fail (
                new_r1->pool_id_ != prev_r1->pool_id_,
                std::make_pair (nullptr, nullptr));
              r1 = new_r1; // FIXME? memleak?
            }
          else if constexpr (RegionSubclass<SelfT>)
            {
              /* remove objects starting after the end */
              std::vector<RegionOwnedObjectImpl<SelfT> *> children;
              r1->append_children (children);
              for (auto child : children)
                {
                  if (*child->pos_ > localp)
                    {
                      auto casted_child =
                        dynamic_cast<RegionChildType_t<SelfT> *> (child);
                      r1->remove_object (*casted_child, false);
                    }
                }
            }
        }
    }

  /*
   * for second object set:
   * - start pos
   * - clip start pos
   */
  if constexpr (std::derived_from<SelfT, LoopableObject>)
    {
      r2->clip_start_pos_setter (&localp);
    }
  r2->pos_setter (&globalp);
  Position r2_local_end = *r2->end_pos_;
  r2_local_end.add_ticks (-r2->pos_->ticks_, AUDIO_ENGINE->frames_per_tick_);
  if constexpr (std::derived_from<SelfT, FadeableObject>)
    {
      r2->set_position (
        &r2_local_end, ArrangerObject::PositionType::FadeOut, false);
    }

  /* if original object was not looped, make the new object unlooped also */
  if constexpr (std::derived_from<SelfT, LoopableObject>)
    {
      if (!self.is_looped ())
        {
          Position init_pos;
          r2->clip_start_pos_setter (&init_pos);
          r2->loop_start_pos_setter (&init_pos);
          r2->loop_end_pos_setter (&r2_local_end);

          if constexpr (RegionSubclass<SelfT>)
            {
              /* move all objects backwards */
              r2->add_ticks_to_children (-localp.ticks_);

              /* if audio region, create a new region */
              if constexpr (std::is_same_v<SelfT, AudioRegion>)
                {
                  auto prev_r2 = r2;
                  auto prev_r2_clip = prev_r2->get_clip ();
                  z_return_val_if_fail (
                    prev_r2_clip, std::make_pair (nullptr, nullptr));
                  size_t num_frames =
                    (size_t) r2_local_end.frames_ * prev_r2_clip->channels_;
                  z_return_val_if_fail_cmp (
                    num_frames, >, 0, std::make_pair (nullptr, nullptr));
                  std::vector<float> frames;
                  frames.resize (num_frames);
                  utils::float_ranges::copy (
                    &frames[0],
                    &prev_r2_clip->frames_.getReadPointer (
                      0)[(size_t) localp.frames_ * prev_r2_clip->channels_],
                    num_frames);
                  z_return_val_if_fail (
                    !prev_r2->name_.empty (), std::make_pair (nullptr, nullptr));
                  z_return_val_if_fail_cmp (
                    r2_local_end.frames_, >=, 0,
                    std::make_pair (nullptr, nullptr));
                  auto new_r2 = new AudioRegion (
                    -1, std::nullopt, true, frames.data (),
                    (unsigned_frame_t) r2_local_end.frames_, prev_r2->name_,
                    prev_r2_clip->channels_, prev_r2_clip->bit_depth_, globalp,
                    prev_r2->id_.track_name_hash_, prev_r2->id_.lane_pos_,
                    prev_r2->id_.idx_);
                  z_return_val_if_fail (
                    new_r2->pool_id_ != prev_r2->pool_id_,
                    std::make_pair (nullptr, nullptr));
                  r2 = new_r2;
                }
              else if constexpr (RegionSubclass<SelfT>)
                {
                  using ChildType = RegionChildType_t<SelfT>;
                  /* remove objects starting before the start */
                  std::vector<RegionOwnedObjectImpl<SelfT> *> children;
                  r2->append_children (children);
                  for (auto child : children)
                    {
                      if (child->pos_->frames_ < 0)
                        r2->remove_object (
                          *static_cast<ChildType *> (child), false);
                    }
                }
            }
        }
    }

  /* make sure regions have names */
  if constexpr (RegionSubclass<SelfT>)
    {
      auto track_var = self.get_track ();
      std::visit (
        [&] (auto &&track) {
          AutomationTrack * at = nullptr;
          if constexpr (std::is_same_v<SelfT, AutomationRegion>)
            {
              at = self.get_automation_track ();
            }
          r1->gen_name (self.name_.c_str (), at, track);
          r2->gen_name (self.name_.c_str (), at, track);
        },
        track_var);
    }

  /* skip rest if non-project object */
  if (!is_project)
    {
      return std::make_pair (nullptr, nullptr);
    }

  /* add them to the parent */
  if constexpr (RegionSubclass<SelfT>)
    {
      auto track_var = self.get_track ();
      std::visit (
        [&] (auto &&track) {
          AutomationTrack * at = nullptr;
          if constexpr (std::is_same_v<SelfT, AutomationRegion>)
            {
              at = self.get_automation_track ();
            }
          track->Track::add_region (r1, at, self.id_.lane_pos_, true, true);

          if (!r2->is_looped ())
            {
              if constexpr (std::is_same_v<SelfT, AutomationRegion>)
                {
                  /* adjust indices before adding r2 */
                  for (auto &ap : r2->aps_)
                    {
                      /* note: not sure what this is supposed to do,
                       * was `region2->num_aps` but changed to fix
                       * a bug - TODO replace this with a comment
                       * about what this does and why it's needed */
                      const int r1_aps_size = r1->aps_.size ();
                      z_return_if_fail (ap->index_ >= r1_aps_size);
                      ap->index_ -= r1_aps_size;
                    }
                }
            }

          track->Track::add_region (r2, at, self.id_.lane_pos_, true, true);
        },
        track_var);
    }
  else if constexpr (std::is_same_v<SelfT, MidiNote>)
    {
      auto parent_region = self.get_region ();
      parent_region->append_object (r1, true);
      parent_region->append_object (r2, true);
    }

  /* select the first one */
  auto sel = self.get_selections_for_type (self.type_);
  sel->remove_object (self);
  sel->add_object_ref (*r1);
  /*arranger_selections_add_object (sel, *r2);*/

  /* remove and free the original object */
  if constexpr (RegionSubclass<SelfT>)
    {
      self.remove_from_project (true);
      if constexpr (std::is_same_v<SelfT, ChordRegion>)
        {
          z_return_val_if_fail (
            r1->id_.idx_ < (int) P_CHORD_TRACK->region_list_->regions_.size (),
            std::make_pair (nullptr, nullptr));
          z_return_val_if_fail (
            r2->id_.idx_ < (int) P_CHORD_TRACK->region_list_->regions_.size (),
            std::make_pair (nullptr, nullptr));
        }
    }
  else if constexpr (std::is_same_v<SelfT, MidiNote>)
    {
      auto parent_region = self.get_region ();
      parent_region->remove_object (self, true);
    }

  if constexpr (RegionSubclass<SelfT>)
    {
      if (set_clip_editor_region)
        {
          CLIP_EDITOR->set_region (r1, true);
        }
    }

  // EVENTS_PUSH (EventType::ET_ARRANGER_OBJECT_CREATED, r1.get ());
  // EVENTS_PUSH (EventType::ET_ARRANGER_OBJECT_CREATED, r2.get ());

  return std::make_pair (r1, r2);
}

template <typename T>
T *
LengthableObject::unsplit (T &r1, T &r2, bool fire_events)
{
  static_assert (FinalLengthedObjectSubclass<T>);

  z_debug ("unsplitting objects...");

  /* change to the original region if the clip editor region is r1 or r2 */
  auto clip_editor_region_opt = CLIP_EDITOR->get_region ();
  bool set_clip_editor_region = false;
  if (clip_editor_region_opt)
    {
      if constexpr (RegionSubclass<T>)
        {
          auto * clip_editor_region = std::get<T *> (*clip_editor_region_opt);
          if (clip_editor_region == &r1 || clip_editor_region == &r2)
            {
              set_clip_editor_region = true;
              CLIP_EDITOR->set_region (std::nullopt, true);
            }
        }
    }

  /* create the new object */
  auto * obj = r1.clone_raw_ptr ();

  /* set the end pos to the end pos of r2 and
   * fade out */
  obj->end_pos_setter (r2.end_pos_);
  Position fade_out_pos = *r2.end_pos_;
  fade_out_pos.add_ticks (-r2.pos_->ticks_, AUDIO_ENGINE->frames_per_tick_);
  obj->set_position (
    &fade_out_pos, ArrangerObject::PositionType::FadeOut, F_NO_VALIDATE);

  /* add it to the parent */
  if constexpr (RegionSubclass<T>)
    {
      AutomationTrack * at = nullptr;
      if constexpr (std::is_same_v<T, AutomationRegion>)
        {
          at = r1.get_automation_track ();
        }
      std::visit (
        [&] (auto &&track) {
          track->Track::add_region (
            obj, at, r1.id_.lane_pos_, true, fire_events);
        },
        r1.get_track ());
    }
  else if constexpr (std::is_same_v<T, MidiNote>)
    {
      auto parent_region = r1.get_region ();
      parent_region->append_object (obj, true);
    }

  /* generate widgets so update visibility in the
   * arranger can work */
  /*arranger_object_gen_widget (*obj);*/

  /* select it */
  auto sel = obj->get_selections_for_type (obj->type_);
  z_return_val_if_fail (sel, nullptr);
  sel->remove_object (r1);
  sel->remove_object (r2);
  sel->add_object_ref (*obj);

  /* remove and free the original regions */
  if constexpr (RegionSubclass<T>)
    {
      r1.remove_from_project (fire_events);
      r2.remove_from_project (fire_events);
    }
  else if constexpr (std::is_same_v<T, MidiNote>)
    {
      auto region1 = r1.get_region ();
      auto region2 = r2.get_region ();
      region1->remove_object (r1, fire_events);
      region2->remove_object (r2, fire_events);
    }

  if constexpr (RegionSubclass<T>)
    {
      if (set_clip_editor_region)
        {
          CLIP_EDITOR->set_region (obj, true);
        }
    }

  if (fire_events)
    {
      // EVENTS_PUSH (EventType::ET_ARRANGER_OBJECT_CREATED, obj.get ());
    }

  return obj;
}

DEFINE_TEMPLATES_FOR_LENGTHABLE_OBJECT (MidiRegion);
DEFINE_TEMPLATES_FOR_LENGTHABLE_OBJECT (AutomationRegion);
DEFINE_TEMPLATES_FOR_LENGTHABLE_OBJECT (AudioRegion);
DEFINE_TEMPLATES_FOR_LENGTHABLE_OBJECT (ChordRegion);
DEFINE_TEMPLATES_FOR_LENGTHABLE_OBJECT (MidiNote);
