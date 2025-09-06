// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <vector>

#include "structure/arrangement/arranger_object.h"
#include "utils/monotonic_time_provider.h"

#include <QUndoCommand>

namespace zrythm::commands
{
class MoveArrangerObjectsCommand
    : public QUndoCommand,
      public utils::QElapsedTimeProvider
{
public:
  MoveArrangerObjectsCommand (
    std::vector<structure::arrangement::ArrangerObjectUuidReference> objects,
    double                                                           tick_delta)
      : QUndoCommand (QObject::tr ("Move Objects")),
        objects_ (std::move (objects)), tick_delta_ (tick_delta)
  {
    // Store original positions for undo
    original_positions_.reserve (objects_.size ());
    for (const auto &obj_ref : objects_)
      {
        if (auto * obj = obj_ref.get_object_base ())
          {
            original_positions_.push_back (obj->position ()->ticks ());
          }
        else
          {
            // Object not found, store invalid position
            original_positions_.push_back (-1.0);
          }
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

    for (size_t i = 0; i < objects_.size (); ++i)
      {
        if (objects_[i].id () != other_cmd->objects_[i].id ())
          return false;
      }

    last_redo_timestamp_ = cur_time;
    tick_delta_ += other_cmd->tick_delta_;
    return true;
  }

  void undo () override
  {
    for (size_t i = 0; i < objects_.size (); ++i)
      {
        if (auto * obj = objects_[i].get_object_base ())
          {
            if (original_positions_[i] >= 0.0)
              {
                obj->position ()->setTicks (original_positions_[i]);
              }
          }
      }
  }

  void redo () override
  {
    for (size_t i = 0; i < objects_.size (); ++i)
      {
        if (auto * obj = objects_[i].get_object_base ())
          {
            if (original_positions_[i] >= 0.0)
              {
                obj->position ()->setTicks (
                  original_positions_[i] + tick_delta_);
              }
          }
      }
    last_redo_timestamp_ = std::chrono::steady_clock::now ();
  }

private:
  std::vector<structure::arrangement::ArrangerObjectUuidReference> objects_;
  std::vector<double>                                original_positions_;
  double                                             tick_delta_;
  std::chrono::time_point<std::chrono::steady_clock> last_redo_timestamp_;
};

} // namespace zrythm::commands
