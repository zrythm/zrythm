// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <algorithm>
#include <ranges>
#include <vector>

#include "structure/arrangement/arranger_object_all.h"

#include <QUndoCommand>

namespace zrythm::commands
{
Q_NAMESPACE

/// Enumeration of supported resize types
enum class ResizeType : int
{
  Bounds = 0,
  LoopPoints = 1,
  Fades = 2
};
Q_ENUM_NS (ResizeType)

/// Enumeration of resize directions
enum class ResizeDirection : int
{
  FromStart = 0,
  FromEnd = 1
};
Q_ENUM_NS (ResizeDirection)

class ResizeArrangerObjectsCommand : public QUndoCommand
{
  static constexpr int ID = 1762503480;

public:
  /// Structure to hold original state of an arranger object for undo
  struct OriginalState
  {
    units::precise_tick_t position = units::ticks (0);
    units::precise_tick_t length = units::ticks (0);
    units::precise_tick_t clipStart = units::ticks (0);
    units::precise_tick_t loopStart = units::ticks (0);
    units::precise_tick_t loopEnd = units::ticks (0);
    units::precise_tick_t fadeInOffset = units::ticks (0);
    units::precise_tick_t fadeOutOffset = units::ticks (0);
  };

  ResizeArrangerObjectsCommand (
    std::vector<structure::arrangement::ArrangerObjectUuidReference> objects,
    ResizeType                                                       type,
    ResizeDirection                                                  direction,
    double                                                           delta)
      : QUndoCommand (QObject::tr ("Resize Objects")),
        objects_ (std::move (objects)), type_ (type), direction_ (direction),
        delta_ (delta)
  {
    // Validate
    if (
      type == ResizeType::LoopPoints && direction == ResizeDirection::FromStart)
      {
        throw std::invalid_argument (
          "Modifying loop points from start not supported");
      }

    // Store original state for undo
    original_states_.reserve (objects_.size ());

    for (const auto &obj_ref : objects_)
      {
        OriginalState state;
        std::visit (
          [&] (auto &&obj) {
            // Always store position
            state.position = units::ticks (obj->position ()->ticks ());

            // Store bounds if available
            if (obj->bounds () != nullptr)
              {
                state.length =
                  units::ticks (obj->bounds ()->length ()->ticks ());
              }

            // Store loop points if available
            if (obj->loopRange () != nullptr)
              {
                state.clipStart = units::ticks (
                  obj->loopRange ()->clipStartPosition ()->ticks ());
                state.loopStart = units::ticks (
                  obj->loopRange ()->loopStartPosition ()->ticks ());
                state.loopEnd = units::ticks (
                  obj->loopRange ()->loopEndPosition ()->ticks ());
              }

            // Store fade offsets if available
            if (obj->fadeRange () != nullptr)
              {
                state.fadeInOffset =
                  units::ticks (obj->fadeRange ()->startOffset ()->ticks ());
                state.fadeOutOffset =
                  units::ticks (obj->fadeRange ()->endOffset ()->ticks ());
              }
          },
          obj_ref.get_object ());

        original_states_.push_back (state);
      }
  }

  int id () const override { return ID; }

  bool mergeWith (const QUndoCommand * other) override
  {
    if (other->id () != id ())
      return false;

    // Only merge if other command was made in quick succession of this
    // command's redo() and operates on the same objects
    const auto * other_cmd =
      dynamic_cast<const ResizeArrangerObjectsCommand *> (other);
    const auto cur_time = std::chrono::steady_clock::now ();
    const auto duration = cur_time - last_redo_timestamp_;
    if (
      std::chrono::duration_cast<std::chrono::milliseconds> (duration).count ()
      > 1'000)
      {
        return false;
      }

    // Check if we're operating on the same set of objects, type, and direction
    if (type_ != other_cmd->type_ || direction_ != other_cmd->direction_)
      return false;

    if (objects_.size () != other_cmd->objects_.size ())
      return false;

    if (
      !std::ranges::equal (
        objects_, other_cmd->objects_, {},
        &structure::arrangement::ArrangerObjectUuidReference::id,
        &structure::arrangement::ArrangerObjectUuidReference::id))
      {
        return false;
      }

    last_redo_timestamp_ = cur_time;
    delta_ += other_cmd->delta_;
    return true;
  }

  void undo () override
  {
    for (size_t i = 0; i < objects_.size () && i < original_states_.size (); ++i)
      {
        const auto &obj_ref = objects_[i];
        const auto &original_state = original_states_[i];

        std::visit (
          [&] (auto &&obj) {
            // Restore position
            obj->position ()->setTicks (
              original_state.position.in (units::ticks));

            // Restore bounds
            if (
              obj->bounds () != nullptr
              && original_state.length.in (units::ticks) > 0)
              {
                obj->bounds ()->length ()->setTicks (
                  original_state.length.in (units::ticks));
              }

            // Restore loop points
            if (obj->loopRange () != nullptr)
              {
                obj->loopRange ()->clipStartPosition ()->setTicks (
                  original_state.clipStart.in (units::ticks));
                obj->loopRange ()->loopStartPosition ()->setTicks (
                  original_state.loopStart.in (units::ticks));
                obj->loopRange ()->loopEndPosition ()->setTicks (
                  original_state.loopEnd.in (units::ticks));
              }

            // Restore fade offsets
            if (obj->fadeRange () != nullptr)
              {
                obj->fadeRange ()->startOffset ()->setTicks (
                  original_state.fadeInOffset.in (units::ticks));
                obj->fadeRange ()->endOffset ()->setTicks (
                  original_state.fadeOutOffset.in (units::ticks));
              }
          },
          obj_ref.get_object ());
      }
  }

  void redo () override
  {
    for (size_t i = 0; i < objects_.size () && i < original_states_.size (); ++i)
      {
        const auto &obj_ref = objects_[i];
        const auto &original_state = original_states_[i];

        std::visit (
          [&] (auto &&obj) {
            switch (type_)
              {
              case ResizeType::Bounds:
                {
                  if (obj->bounds () != nullptr)
                    {
                      if (direction_ == ResizeDirection::FromStart)
                        {
                          // Adjust position and length to maintain end position
                          const double new_position =
                            original_state.position.in (units::ticks) + delta_;
                          const double new_length = std::max (
                            original_state.length.in (units::ticks) -delta_,
                            1.0);
                          obj->position ()->setTicks (new_position);
                          obj->bounds ()->length ()->setTicks (new_length);
                        }
                      else // FromEnd
                        {
                          // Adjust length only
                          const double new_length = std::max (
                            original_state.length.in (units::ticks) + delta_,
                            1.0);
                          obj->bounds ()->length ()->setTicks (new_length);
                        }
                    }
                  break;
                }
              case ResizeType::LoopPoints:
                {
                  if (obj->loopRange () != nullptr)
                    {
                      assert (direction_ == ResizeDirection::FromEnd);
                      // Adjust loop end
                      const double new_loop_end = std::max (
                        original_state.loopEnd.in (units::ticks) + delta_,
                        original_state.loopStart.in (units::ticks));
                      obj->loopRange ()->loopEndPosition ()->setTicks (
                        new_loop_end);
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
                            original_state.fadeInOffset.in (units::ticks)
                              + delta_,
                            0.0);
                          obj->fadeRange ()->startOffset ()->setTicks (
                            new_fade_in);
                        }
                      else // FromEnd
                        {
                          // Adjust fade-out end offset
                          const double new_fade_out = std::max (
                            original_state.fadeOutOffset.in (units::ticks)
                              + delta_,
                            0.0);
                          obj->fadeRange ()->endOffset ()->setTicks (
                            new_fade_out);
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

private:
  std::vector<structure::arrangement::ArrangerObjectUuidReference> objects_;
  ResizeType                                                       type_;
  ResizeDirection                                                  direction_;
  double                                                           delta_;

  // Original state storage for undo
  std::vector<OriginalState> original_states_;

  std::chrono::time_point<std::chrono::steady_clock> last_redo_timestamp_;
};

} // namespace zrythm::commands
