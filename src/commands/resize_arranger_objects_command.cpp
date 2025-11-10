// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "commands/resize_arranger_objects_command.h"

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
          using ObjectT = base_type<decltype (obj)>;

          // Always store position
          state.position = units::ticks (obj->position ()->ticks ());

          // Store bounds if available
          if (obj->bounds () != nullptr)
            {
              state.length = units::ticks (obj->bounds ()->length ()->ticks ());
            }

          // Store loop points if available
          if (obj->loopRange () != nullptr)
            {
              state.clipStart = units::ticks (
                obj->loopRange ()->clipStartPosition ()->ticks ());
              state.loopStart = units::ticks (
                obj->loopRange ()->loopStartPosition ()->ticks ());
              state.loopEnd =
                units::ticks (obj->loopRange ()->loopEndPosition ()->ticks ());
              state.boundsTracked = obj->loopRange ()->trackBounds ();
            }

          // Store fade offsets if available
          if (obj->fadeRange () != nullptr)
            {
              state.fadeInOffset =
                units::ticks (obj->fadeRange ()->startOffset ()->ticks ());
              state.fadeOutOffset =
                units::ticks (obj->fadeRange ()->endOffset ()->ticks ());
            }

          if constexpr (
            is_derived_from_template_v<
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
        obj_ref.get_object ());

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
          using ObjectT = base_type<decltype (obj)>;

          // Restore position
          obj->position ()->setTicks (original_state.position.in (units::ticks));

          // Restore loop points
          if (obj->loopRange () != nullptr)
            {
              obj->loopRange ()->setTrackBounds (original_state.boundsTracked);
              obj->loopRange ()->clipStartPosition ()->setTicks (
                original_state.clipStart.in (units::ticks));
              obj->loopRange ()->loopStartPosition ()->setTicks (
                original_state.loopStart.in (units::ticks));
              obj->loopRange ()->loopEndPosition ()->setTicks (
                original_state.loopEnd.in (units::ticks));
            }

          // Restore bounds
          if (
            obj->bounds () != nullptr
            && original_state.length.in (units::ticks) > 0)
            {
              obj->bounds ()->length ()->setTicks (
                original_state.length.in (units::ticks));
            }

          // Restore fade offsets
          if (obj->fadeRange () != nullptr)
            {
              obj->fadeRange ()->startOffset ()->setTicks (
                original_state.fadeInOffset.in (units::ticks));
              obj->fadeRange ()->endOffset ()->setTicks (
                original_state.fadeOutOffset.in (units::ticks));
            }

          // Restore children positions
          if constexpr (
            is_derived_from_template_v<
              structure::arrangement::ArrangerObjectOwner, ObjectT>)
            {
              if (
                !utils::math::floats_near (
                  original_state.firstChildTicks.in (units::ticks), 0.0, 0.001))
                {
                  auto       children = obj->get_children_view ();
                  const auto child_delta =
                    original_state.firstChildTicks.in (units::ticks)
                    - (*children.begin ())->position ()->ticks ();
                  for (const auto &child : children)
                    {
                      child->position ()->addTicks (child_delta);
                    }
                }
            }
        },
        obj_ref.get_object ());
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
          using ObjectT = base_type<decltype (obj)>;
          switch (type_)
            {
            case ResizeType::Bounds:
              {
                if (obj->bounds () != nullptr)
                  {
                    // If normal-resizing and not already looped, force track
                    // bounds
                    if (obj->loopRange () && !obj->loopRange ()->looped ())
                      {
                        obj->loopRange ()->setTrackBounds (true);
                      }

                    if (direction_ == ResizeDirection::FromStart)
                      {
                        // Adjust position and length to maintain end position
                        const double new_position =
                          original_state.position.in (units::ticks) + delta_;
                        const double new_length = std::max (
                          original_state.length.in (units::ticks) -delta_, 1.0);
                        obj->position ()->setTicks (new_position);
                        obj->bounds ()->length ()->setTicks (new_length);

                        // Adjust children positions accordingly
                        if constexpr (
                          is_derived_from_template_v<
                            structure::arrangement::ArrangerObjectOwner, ObjectT>)
                          {
                            for (const auto &child : obj->get_children_view ())
                              {
                                child->position ()->addTicks (-delta_);
                              }
                          }
                      }
                    else // FromEnd
                      {
                        // Adjust length
                        const double new_length = std::max (
                          original_state.length.in (units::ticks) + delta_, 1.0);
                        obj->bounds ()->length ()->setTicks (new_length);
                      }

                    // Automatically re-track bounds
                    if (
                      obj->loopRange () != nullptr
                      && !obj->loopRange ()->looped ())
                      {
                        obj->loopRange ()->setTrackBounds (true);
                      }
                  }
                break;
              }
            case ResizeType::LoopPoints:
              {
                if (obj->bounds () != nullptr && obj->loopRange () != nullptr)
                  {
                    // If loop-resizing, don't track bounds
                    obj->loopRange ()->setTrackBounds (false);

                    if (direction_ == ResizeDirection::FromStart)
                      {
                        // Adjust position and length to maintain end position
                        const double new_position =
                          original_state.position.in (units::ticks) + delta_;
                        const double new_length = std::max (
                          original_state.length.in (units::ticks) -delta_, 1.0);
                        obj->position ()->setTicks (new_position);
                        obj->bounds ()->length ()->setTicks (new_length);

                        auto *     loop_range = obj->loopRange ();
                        const auto loop_len =
                          loop_range->get_loop_length_in_ticks ().in (
                            units::ticks);

                        // We only need to adjust Clip Start
                        auto new_clip_start_pos =
                          loop_range->clipStartPosition ()->ticks ();
                        new_clip_start_pos += delta_;

                        // make sure clip start goes back to loop start
                        // if it exceeds loop end
                        while (
                          new_clip_start_pos
                          >= loop_range->loopEndPosition ()->ticks ())
                          {
                            new_clip_start_pos -= loop_len;
                          }

                        // If original clip start position was within the loop
                        // and now we're before the loop start, just keep
                        // looping backwards
                        if (
                          loop_range->clipStartPosition ()->ticks ()
                            >= loop_range->loopStartPosition ()->ticks ()
                          && new_clip_start_pos
                               < loop_range->loopStartPosition ()->ticks ())
                          {
                            while (
                              new_clip_start_pos
                              < loop_range->loopStartPosition ()->ticks ())
                              {
                                new_clip_start_pos += loop_len;
                              }
                          }
                        // Otherwise special case (clip start was already before
                        // the looped part): expand the region backwards while
                        // keeping the rest of the contents in place
                        else if (new_clip_start_pos < 0.0)
                          {
                            if constexpr (
                              is_derived_from_template_v<
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

                            loop_range->loopStartPosition ()->addTicks (
                              -new_clip_start_pos);
                            loop_range->loopEndPosition ()->addTicks (
                              -new_clip_start_pos);
                            new_clip_start_pos = 0.f;
                          }

                        loop_range->clipStartPosition ()->setTicks (
                          new_clip_start_pos);
                      }
                    else // FromEnd
                      {
                        // Adjust length
                        const double new_length = std::max (
                          original_state.length.in (units::ticks) + delta_, 1.0);
                        obj->bounds ()->length ()->setTicks (new_length);
                      }
                  }
                break;
              }
            case ResizeType::Fades:
              {
                if (obj->fadeRange () != nullptr)
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
        obj_ref.get_object ());
    }
  last_redo_timestamp_ = std::chrono::steady_clock::now ();
}
}
