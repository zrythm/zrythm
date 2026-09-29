// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "commands/delete_lane_command.h"
#include "utils/logger.h"

namespace zrythm::commands
{

DeleteLaneCommand::DeleteLaneCommand (
  structure::tracks::TrackLaneList         &list,
  structure::tracks::TrackLaneUuidReference lane_ref)
    : QUndoCommand (QObject::tr ("Delete Lane")), list_ (list),
      lane_ref_ (std::move (lane_ref))
{
  if (list_.size () <= 1)
    {
      throw std::invalid_argument ("a track keeps at least one lane");
    }
  const auto index = list_.indexOfLane (lane_ref_.get ());
  if (index == std::nullopt)
    {
      throw std::invalid_argument ("lane is not in this list");
    }
  if (*index == list_.size () - 1 && !list_.at (list_.size () - 2)->is_empty ())
    {
      throw std::invalid_argument (
        "removing the last lane would leave no trailing empty lane");
    }
}

void
DeleteLaneCommand::redo ()
{
  try
    {
      // The lane's current index is looked up at execution time:
      // earlier commands of the same batch may have removed lanes
      // before this one
      const auto index = list_.indexOfLane (lane_ref_.get ());
      if (index == std::nullopt)
        {
          throw std::runtime_error ("lane is no longer in this list");
        }
      list_.removeLane (*index);
      // removeLane refuses to remove the last lane: verify the removal
      // happened so a refused command is never recorded as executed
      if (list_.indexOfLane (lane_ref_.get ()) != std::nullopt)
        {
          throw std::runtime_error ("lane removal was refused");
        }
      original_index_ = index;
    }
  catch (const std::exception &e)
    {
      // A failure during execution leaves the undo stack and the list
      // in inconsistent state: logged as an error and swallowed so the
      // exception cannot escape into the undo stack's caller
      z_error ("Failed to execute DeleteLaneCommand: {}", e.what ());
    }
}

void
DeleteLaneCommand::undo ()
{
  try
    {
      if (!original_index_.has_value ())
        {
          throw std::runtime_error ("command was never executed");
        }
      list_.reinsert_lane (*original_index_, lane_ref_);
    }
  catch (const std::exception &e)
    {
      z_error ("Failed to undo DeleteLaneCommand: {}", e.what ());
    }
}

} // namespace zrythm::commands
