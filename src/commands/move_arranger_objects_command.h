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
class MoveArrangerObjectsCommand : public QUndoCommand
{
public:
  MoveArrangerObjectsCommand (
    std::vector<structure::arrangement::ArrangerObjectUuidReference> objects,
    units::precise_tick_t                                            tick_delta,
    double vertical_delta = 0.0)
      : QUndoCommand (QObject::tr ("Move Objects")),
        objects_ (std::move (objects)), tick_delta_ (tick_delta),
        vertical_delta_ (vertical_delta)
  {
    // Store original positions for undo
    original_positions_.reserve (objects_.size ());
    for (const auto &obj_ref : objects_)
      {
        std::visit (
          [&] (auto &&obj) {
            using ObjectT = base_type<decltype (obj)>;
            original_positions_.push_back (
              units::ticks (obj->position ()->ticks ()));
            if constexpr (
              std::is_same_v<ObjectT, structure::arrangement::MidiNote>)
              {
                original_vertical_positions_.push_back (obj->pitch ());
              }
            else if constexpr (
              std::is_same_v<ObjectT, structure::arrangement::AutomationPoint>)
              {
                original_vertical_positions_.push_back (obj->value ());
              }
            else
              {
                original_vertical_positions_.push_back (0);
              }
          },
          obj_ref.get_object ());
      }
  }

  int  id () const override { return 894553188; }
  bool mergeWith (const QUndoCommand * other) override
  {
    if (other->id () != id ())
      return false;

    // only merge if other command was made in quick succession of this
    // command's redo() and operates on the same objects
    const auto * other_cmd =
      dynamic_cast<const MoveArrangerObjectsCommand *> (other);
    const auto cur_time = std::chrono::steady_clock::now ();
    const auto duration = cur_time - last_redo_timestamp_;
    if (
      std::chrono::duration_cast<std::chrono::milliseconds> (duration).count ()
      > 1'000)
      {
        return false;
      }

    // Check if we're operating on the same set of objects
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
    tick_delta_ += other_cmd->tick_delta_;
    return true;
  }

  void undo () override
  {
    for (
      const auto &[obj_ref, original_pos, original_vertical_pos] :
      std::views::zip (
        objects_, original_positions_, original_vertical_positions_))
      {
        std::visit (
          [&] (auto &&obj) {
            using ObjectT = base_type<decltype (obj)>;
            obj->position ()->setTicks (original_pos.in (units::ticks));
            if (vertical_delta_ != 0)
              {
                if constexpr (
                  std::is_same_v<ObjectT, structure::arrangement::MidiNote>)
                  {
                    obj->setPitch (static_cast<int> (original_vertical_pos));
                  }
                else if constexpr (
                  std::is_same_v<ObjectT, structure::arrangement::AutomationPoint>)
                  {
                    obj->setValue (static_cast<float> (original_vertical_pos));
                  }
              }
          },
          obj_ref.get_object ());
      }
  }

  void redo () override
  {
    for (
      const auto &[obj_ref, original_pos, original_vertical_pos] :
      std::views::zip (
        objects_, original_positions_, original_vertical_positions_))
      {
        std::visit (
          [&] (auto &&obj) {
            using ObjectT = base_type<decltype (obj)>;
            obj->position ()->setTicks (
              (original_pos + tick_delta_).in (units::ticks));
            if (vertical_delta_ != 0)
              {
                if constexpr (
                  std::is_same_v<ObjectT, structure::arrangement::MidiNote>)
                  {
                    obj->setPitch (
                      static_cast<int> (original_vertical_pos + vertical_delta_));
                  }
                else if constexpr (
                  std::is_same_v<ObjectT, structure::arrangement::AutomationPoint>)
                  {
                    obj->setValue (
                      static_cast<float> (
                        original_vertical_pos + vertical_delta_));
                  }
              }
          },
          obj_ref.get_object ());
      }
    last_redo_timestamp_ = std::chrono::steady_clock::now ();
  }

private:
  std::vector<structure::arrangement::ArrangerObjectUuidReference> objects_;
  std::vector<units::precise_tick_t> original_positions_;
  std::vector<double>                original_vertical_positions_;
  units::precise_tick_t              tick_delta_;
  double                             vertical_delta_{};
  std::chrono::time_point<std::chrono::steady_clock> last_redo_timestamp_;
};

class MoveTempoMapAffectingArrangerObjectsCommand
    : public MoveArrangerObjectsCommand
{
public:
  static constexpr int CommandId = 1762956469;
  using MoveArrangerObjectsCommand::MoveArrangerObjectsCommand;

  int id () const override { return CommandId; }
};

} // namespace zrythm::commands
