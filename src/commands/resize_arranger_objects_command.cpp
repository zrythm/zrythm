// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "commands/resize_arranger_objects_command.h"
#include "dsp/content_time_warp.h"
#include "dsp/position.h"
#include "dsp/tick_types.h"
#include "utils/math_utils.h"

namespace zrythm::commands
{
ResizeArrangerObjectsCommand::ResizeArrangerObjectsCommand (
  std::vector<structure::arrangement::ArrangerObjectUuidReference> objects,
  ResizeType                                                       type,
  ResizeDirection                                                  direction,
  double                                                           delta)
    : QUndoCommand (QObject::tr ("Resize Objects")),
      objects_ (std::move (objects)), type_ (type), direction_ (direction),
      delta_ (delta)
{
  // Store original state for undo
  original_states_.reserve (objects_.size ());

  for (const auto &obj_ref : objects_)
    {
      OriginalState state;
      std::visit (
        [&] (auto &&obj) {
          using ObjectT = utils::base_type<decltype (obj)>;

          // Always store position
          state.position = units::ticks (obj->position ()->ticks ());

          // Store length if this object type has Bounds
          if constexpr (structure::arrangement::BoundedObject<ObjectT>)
            {
              state.length = units::ticks (obj->length ()->ticks ());
            }

          // Store loop points if this is a Clip
          if constexpr (structure::arrangement::ClipObject<ObjectT>)
            {
              state.clipStart =
                units::ticks (obj->clipStartPosition ()->ticks ());
              state.loopStart =
                units::ticks (obj->loopStartPosition ()->ticks ());
              state.loopEnd = units::ticks (obj->loopEndPosition ()->ticks ());
              state.boundsTracked = obj->trackBounds ();
            }

          // Store fade offsets if this is an AudioClip
          if constexpr (structure::arrangement::FadeableObject<ObjectT>)
            {
              state.fadeInOffset =
                units::ticks (obj->fadeRange ()->startOffset ()->ticks ());
              state.fadeOutOffset =
                units::ticks (obj->fadeRange ()->endOffset ()->ticks ());
            }

          if constexpr (
            utils::is_derived_from_template_v<
              structure::arrangement::ArrangerObjectOwner, ObjectT>)
            {
              auto children = obj->get_children_view ();
              if (!children.empty ())
                {
                  state.firstChildTicks =
                    units::ticks ((*children.begin ())->position ()->ticks ());
                }
            }
        },
        convert_to_variant_qobj<
          structure::arrangement::ArrangerObjectPtrVariant> (obj_ref.get ()));

      original_states_.push_back (state);
    }
}

void
ResizeArrangerObjectsCommand::undo ()
{
  for (size_t i = 0; i < objects_.size () && i < original_states_.size (); ++i)
    {
      const auto &obj_ref = objects_[i];
      const auto &original_state = original_states_[i];

      std::visit (
        [&] (auto &&obj) {
          using ObjectT = utils::base_type<decltype (obj)>;

          // Restore position
          obj->position ()->setTicks (original_state.position.in (units::ticks));

          // Restore loop points if this is a Clip
          if constexpr (structure::arrangement::ClipObject<ObjectT>)
            {
              obj->setTrackBounds (original_state.boundsTracked);
              obj->clipStartPosition ()->setTicks (
                original_state.clipStart.in (units::ticks));
              obj->loopStartPosition ()->setTicks (
                original_state.loopStart.in (units::ticks));
              obj->loopEndPosition ()->setTicks (
                original_state.loopEnd.in (units::ticks));
            }

          // Restore length if this object type has Bounds
          if constexpr (structure::arrangement::BoundedObject<ObjectT>)
            {
              if (original_state.length.in (units::ticks) > 0)
                {
                  obj->length ()->setTicks (
                    original_state.length.in (units::ticks));
                }
            }

          // Restore fade offsets if this is an AudioClip
          if constexpr (structure::arrangement::FadeableObject<ObjectT>)
            {
              obj->fadeRange ()->startOffset ()->setTicks (
                original_state.fadeInOffset.in (units::ticks));
              obj->fadeRange ()->endOffset ()->setTicks (
                original_state.fadeOutOffset.in (units::ticks));
            }

          // Restore children positions
          if constexpr (
            utils::is_derived_from_template_v<
              structure::arrangement::ArrangerObjectOwner, ObjectT>)
            {
              if (original_state.firstChildTicks.has_value ())
                {
                  auto       children = obj->get_children_view ();
                  const auto child_delta =
                    original_state.firstChildTicks->in (units::ticks)
                    - (*children.begin ())->position ()->ticks ();
                  for (const auto &child : children)
                    {
                      child->position ()->addTicks (child_delta);
                    }
                }
            }
        },
        convert_to_variant_qobj<
          structure::arrangement::ArrangerObjectPtrVariant> (obj_ref.get ()));
    }
}

void
ResizeArrangerObjectsCommand::redo ()
{
  for (size_t i = 0; i < objects_.size () && i < original_states_.size (); ++i)
    {
      const auto &obj_ref = objects_[i];
      const auto &original_state = original_states_[i];

      std::visit (
        [&] (auto &&obj) {
          using ObjectT = utils::base_type<decltype (obj)>;

          // Delta_ arrives in timeline ticks (from the QML ruler). Position
          // is also in timeline ticks, but length and loop positions are in
          // content ticks. Convert using the warp map's reverse lookup so
          // the conversion is accurate even when tempo varies across the clip.
          const dsp::TimelineTick tl_delta{ units::ticks (delta_) };
          dsp::ContentTick        content_delta{ units::ticks (delta_) };
          if constexpr (structure::arrangement::ClipObject<ObjectT>)
            {
              auto * warp = obj->contentWarp ();
              if (!warp->is_identity ())
                {
                  const auto clip_start = obj->position ()->asTick ();
                  const dsp::ContentTick old_content_len =
                    obj->length ()->asTick ();

                  if (direction_ == ResizeDirection::FromEnd)
                    {
                      const dsp::TimelineTick old_tl_end =
                        warp->contentToTimeline (old_content_len);
                      const dsp::ContentTick new_content_len =
                        warp->timelineToContent (old_tl_end + tl_delta);
                      content_delta = new_content_len - old_content_len;
                    }
                  else
                    {
                      content_delta =
                        warp->timelineToContent (clip_start + tl_delta)
                        - warp->timelineToContent (clip_start);
                    }
                }
            }
          const double content_delta_val = content_delta.asDouble ();

          switch (type_)
            {
            case ResizeType::Bounds:
              {
                if constexpr (structure::arrangement::BoundedObject<ObjectT>)
                  {
                    // If normal-resizing and not already looped, force track
                    // bounds
                    if constexpr (structure::arrangement::ClipObject<ObjectT>)
                      {
                        if (!obj->looped ())
                          {
                            obj->setTrackBounds (true);
                          }
                      }

                    if (direction_ == ResizeDirection::FromStart)
                      {
                        // Adjust position and length to maintain end position
                        const double new_position =
                          original_state.position.in (units::ticks) + delta_;
                        const double new_length = std::max (
                          original_state.length.in (
                            units::ticks) -content_delta_val,
                          1.0);
                        obj->position ()->setTicks (new_position);
                        obj->length ()->setTicks (new_length);

                        // Adjust children positions accordingly
                        if constexpr (
                          utils::is_derived_from_template_v<
                            structure::arrangement::ArrangerObjectOwner, ObjectT>)
                          {
                            for (const auto &child : obj->get_children_view ())
                              {
                                child->position ()->addTicks (
                                  -content_delta_val);
                              }
                          }
                      }
                    else // FromEnd
                      {
                        // Adjust length
                        const double new_length = std::max (
                          original_state.length.in (units::ticks)
                            + content_delta_val,
                          1.0);
                        obj->length ()->setTicks (new_length);
                      }

                    // Automatically re-track bounds
                    if constexpr (structure::arrangement::ClipObject<ObjectT>)
                      {
                        if (!obj->looped ())
                          {
                            obj->setTrackBounds (true);
                          }
                      }
                  }
                break;
              }
            case ResizeType::LoopPoints:
              {
                if constexpr (structure::arrangement::ClipObject<ObjectT>)
                  {
                    // If loop-resizing, don't track bounds
                    obj->setTrackBounds (false);

                    if (direction_ == ResizeDirection::FromStart)
                      {
                        // Adjust position and length to maintain end position
                        const double new_position =
                          original_state.position.in (units::ticks) + delta_;
                        const double new_length = std::max (
                          original_state.length.in (
                            units::ticks) -content_delta_val,
                          1.0);
                        obj->position ()->setTicks (new_position);
                        obj->length ()->setTicks (new_length);

                        const auto loop_len =
                          obj->get_loop_length_in_ticks ().asDouble ();

                        // We only need to adjust Clip Start
                        auto new_clip_start_pos =
                          obj->clipStartPosition ()->ticks ();
                        new_clip_start_pos += content_delta_val;

                        // make sure clip start goes back to loop start
                        // if it exceeds loop end
                        while (
                          new_clip_start_pos
                          >= obj->loopEndPosition ()->ticks ())
                          {
                            new_clip_start_pos -= loop_len;
                          }

                        // If original clip start position was within the loop
                        // and now we're before the loop start, just keep
                        // looping backwards
                        if (
                          obj->clipStartPosition ()->ticks ()
                            >= obj->loopStartPosition ()->ticks ()
                          && new_clip_start_pos
                               < obj->loopStartPosition ()->ticks ())
                          {
                            while (
                              new_clip_start_pos
                              < obj->loopStartPosition ()->ticks ())
                              {
                                new_clip_start_pos += loop_len;
                              }
                          }
                        // Otherwise special case (clip start was already before
                        // the looped part): expand the clip backwards while
                        // keeping the rest of the contents in place
                        else if (new_clip_start_pos < 0.0)
                          {
                            if constexpr (
                              utils::is_derived_from_template_v<
                                structure::arrangement::ArrangerObjectOwner,
                                ObjectT>)
                              {
                                for (
                                  const auto &child : obj->get_children_view ())
                                  {
                                    child->position ()->addTicks (
                                      -new_clip_start_pos);
                                  }
                              }

                            obj->loopStartPosition ()->addTicks (
                              -new_clip_start_pos);
                            obj->loopEndPosition ()->addTicks (
                              -new_clip_start_pos);
                            new_clip_start_pos = 0.f;
                          }

                        obj->clipStartPosition ()->setTicks (new_clip_start_pos);
                      }
                    else // FromEnd
                      {
                        // Adjust length
                        const double new_length = std::max (
                          original_state.length.in (units::ticks)
                            + content_delta_val,
                          1.0);
                        obj->length ()->setTicks (new_length);
                      }
                  }
                break;
              }
            case ResizeType::Fades:
              {
                if constexpr (structure::arrangement::FadeableObject<ObjectT>)
                  {
                    if (direction_ == ResizeDirection::FromStart)
                      {
                        // Adjust fade-in start offset
                        const double new_fade_in = std::max (
                          original_state.fadeInOffset.in (units::ticks) + delta_,
                          0.0);
                        obj->fadeRange ()->startOffset ()->setTicks (
                          new_fade_in);
                      }
                    else // FromEnd
                      {
                        // Adjust fade-out end offset
                        const double new_fade_out = std::max (
                          original_state.fadeOutOffset.in (units::ticks) + delta_,
                          0.0);
                        obj->fadeRange ()->endOffset ()->setTicks (new_fade_out);
                      }
                  }
                break;
              }
            }
        },
        convert_to_variant_qobj<
          structure::arrangement::ArrangerObjectPtrVariant> (obj_ref.get ()));
    }
  last_redo_timestamp_ = std::chrono::steady_clock::now ();
}
}
