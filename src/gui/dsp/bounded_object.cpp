// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/backend/zrythm.h"
#include "gui/dsp/bounded_object.h"
#include "gui/dsp/fadeable_object.h"
#include "gui/dsp/loopable_object.h"
#include "gui/dsp/midi_note.h"
#include "gui/dsp/region.h"
#include "utils/debug.h"
#include "utils/dsp.h"

#include "utils/gtest_wrapper.h"
#include "utils/rt_thread_id.h"

BoundedObject::BoundedObject () : end_pos_ (new PositionProxy ()) { }

void
BoundedObject::parent_base_qproperties (QObject &derived)
{
  end_pos_->setParent (&derived);
}

void
BoundedObject::copy_members_from (
  const BoundedObject &other,
  ObjectCloneType      clone_type)
{
  end_pos_ = other.end_pos_->clone_raw_ptr ();
  if (auto * qobject = dynamic_cast<QObject *> (this))
    {
      end_pos_->setParent (qobject);
    }
}

void
BoundedObject::init_loaded_base ()
{
}

bool
BoundedObject::are_members_valid (bool is_project, double frames_per_tick) const
{
  if (!is_position_valid (*end_pos_, PositionType::End, 1.0 / frames_per_tick))
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
BoundedObject::resize (
  const bool   left,
  ResizeType   type,
  const double ticks,
  bool         during_ui_action)
{
  z_trace (
    "resizing object{{ left: {}, type: {}, ticks: {}, during UI action: {}}}",
    left, ENUM_NAME (type), ticks, during_ui_action);

  std::visit (
    [&] (auto &&self) {
      using ObjT = base_type<decltype (self)>;
      double   before_length = get_length_in_ticks ();
      Position tmp;
      if (left)
        {
          if (type == ResizeType::Fade)
            {
              if constexpr (std::derived_from<ObjT, FadeableObject>)
                {
                  tmp = self->fade_in_pos_;
                  tmp.add_ticks (ticks, AUDIO_ENGINE->frames_per_tick_);
                  self->set_fade_in_position_unvalidated (tmp);
                }
            }
          else
            {
              tmp = *static_cast<Position *> (pos_);
              tmp.add_ticks (ticks, AUDIO_ENGINE->frames_per_tick_);
              self->set_position_unvalidated (tmp);

              if constexpr (std::derived_from<ObjT, FadeableObject>)
                {
                  tmp = self->fade_out_pos_;
                  tmp.add_ticks (-ticks, AUDIO_ENGINE->frames_per_tick_);
                  self->set_fade_out_position_unvalidated (tmp);
                }

              if (type == ResizeType::Loop)
                {
                  if constexpr (std::derived_from<ObjT, LoopableObject>)
                    {
                      double loop_len = self->get_loop_length_in_ticks ();

                      /* if clip start is not before loop start, adjust clip
                       * start pos
                       */
                      Position clip_start_pos = self->clip_start_pos_;
                      Position loop_start_pos = self->loop_start_pos_;
                      if (clip_start_pos >= loop_start_pos)
                        {
                          clip_start_pos.add_ticks (
                            ticks, AUDIO_ENGINE->frames_per_tick_);

                          while (clip_start_pos < loop_start_pos)
                            {
                              clip_start_pos.add_ticks (
                                loop_len, AUDIO_ENGINE->frames_per_tick_);
                            }
                          self->clip_start_position_setter_validated (
                            clip_start_pos, AUDIO_ENGINE->ticks_per_frame_);
                        }

                      /* make sure clip start goes back to loop start if it
                       * exceeds loop end */
                      Position loop_end_pos = self->loop_end_pos_;
                      while (clip_start_pos > loop_end_pos)
                        {
                          clip_start_pos.add_ticks (
                            -loop_len, AUDIO_ENGINE->frames_per_tick_);
                        }
                      self->clip_start_position_setter_validated (
                        clip_start_pos, AUDIO_ENGINE->ticks_per_frame_);
                    }
                }
              else if constexpr (std::derived_from<ObjT, LoopableObject>)
                {
                  tmp = self->loop_end_pos_;
                  tmp.add_ticks (-ticks, AUDIO_ENGINE->frames_per_tick_);
                  self->set_loop_end_position_unvalidated (tmp);

                  /* move containing items */
                  if constexpr (RegionWithChildren<ObjT>)
                    {
                      self->add_ticks_to_children (
                        -ticks, AUDIO_ENGINE->frames_per_tick_);
                    }
                }
            }
        }
      /* else if resizing right side */
      else
        {
          if (type == ResizeType::Fade)
            {
              if constexpr (std::derived_from<ObjT, FadeableObject>)
                {
                  tmp = self->fade_out_pos_;
                  tmp.add_ticks (ticks, AUDIO_ENGINE->frames_per_tick_);
                  self->set_fade_out_position_unvalidated (tmp);
                }
            }
          else
            {
              tmp = *end_pos_;
              Position prev_end_pos = tmp;
              tmp.add_ticks (ticks, AUDIO_ENGINE->frames_per_tick_);
              if constexpr (std::derived_from<ObjT, BoundedObject>)
                {
                  self->set_end_position_unvalidated (tmp);
                }

              double change_ratio =
                (get_length_in_ticks ()) / (prev_end_pos.ticks_ - pos_->ticks_);

              if (type != ResizeType::Loop)
                {
                  if constexpr (std::derived_from<ObjT, LoopableObject>)
                    {
                      tmp = self->loop_end_pos_;
                      if (
                        type == ResizeType::StretchTempoChange
                        || type == ResizeType::Stretch)
                        {
                          tmp.from_ticks (
                            tmp.ticks_ * change_ratio,
                            AUDIO_ENGINE->frames_per_tick_);
                        }
                      else
                        {
                          tmp.add_ticks (ticks, AUDIO_ENGINE->frames_per_tick_);
                        }
                      z_return_if_fail (tmp.is_positive ());
                      self->set_loop_end_position_unvalidated (tmp);
                      z_return_if_fail (self->loop_end_pos_.is_positive ());

                      /* if stretching, also stretch loop start */
                      if (
                        type == ResizeType::StretchTempoChange
                        || type == ResizeType::Stretch)
                        {
                          tmp = self->loop_start_pos_;
                          tmp.from_ticks (
                            tmp.ticks_ * change_ratio,
                            AUDIO_ENGINE->frames_per_tick_);
                          z_return_if_fail (tmp.is_positive ());
                          self->set_loop_start_position_unvalidated (tmp);
                          z_return_if_fail (
                            self->loop_start_pos_.is_positive ());
                        }
                    }
                }
              if constexpr (std::derived_from<ObjT, FadeableObject>)
                {
                  tmp = self->fade_out_pos_;
                  if (
                    type == ResizeType::StretchTempoChange
                    || type == ResizeType::Stretch)
                    {
                      tmp.from_ticks (
                        tmp.ticks_ * change_ratio,
                        AUDIO_ENGINE->frames_per_tick_);
                    }
                  else
                    {
                      tmp.add_ticks (ticks, AUDIO_ENGINE->frames_per_tick_);
                    }
                  z_return_if_fail (tmp.is_positive ());
                  self->set_fade_out_position_unvalidated (tmp);
                  z_return_if_fail (self->fade_out_pos_.is_positive ());

                  /* if stretching, also stretch fade in */
                  if (
                    type == ResizeType::StretchTempoChange
                    || type == ResizeType::Stretch)
                    {
                      tmp = self->fade_in_pos_;
                      tmp.from_ticks (
                        tmp.ticks_ * change_ratio,
                        AUDIO_ENGINE->frames_per_tick_);
                      z_return_if_fail (tmp.is_positive ());
                      self->set_fade_in_position_unvalidated (tmp);
                      z_return_if_fail (self->fade_in_pos_.is_positive ());
                    }
                }

              if (type == ResizeType::Stretch)
                {
                  if constexpr (std::derived_from<ObjT, Region>)
                    {
                      double new_length = get_length_in_ticks ();

                      if (type != ResizeType::StretchTempoChange)
                        {
                          /* FIXME this flag is not good,
                           * remove from this function and
                           * do it in the arranger */
                          if (during_ui_action)
                            {
                              self->stretch_ratio_ =
                                new_length / self->before_length_;
                            }
                          /* else if as part of an action */
                          else
                            {
                              /* stretch contents */
                              double stretch_ratio = new_length / before_length;
                              try
                                {
                                  self->stretch (stretch_ratio);
                                }
                              catch (const ZrythmException &ex)
                                {
                                  throw ZrythmException (
                                    "Failed to stretch region");
                                }
                            }
                        }
                    }
                }
            }
        }
    },
    convert_to_variant<ArrangerObjectPtrVariant> (this));
}
