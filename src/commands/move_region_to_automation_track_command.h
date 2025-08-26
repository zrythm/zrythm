// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <utility>

#include "structure/tracks/automation_track.h"

#include <QUndoCommand>

namespace zrythm::commands
{
class MoveRegionToAutomationTrackCommand : public QUndoCommand
{
public:
  MoveRegionToAutomationTrackCommand (
    structure::arrangement::ArrangerObjectUuidReference region_ref,
    structure::tracks::AutomationTrack                 &source_at,
    structure::tracks::AutomationTrack                 &target_at)
      : QUndoCommand (QObject::tr ("Move Plugin")),
        region_ref_ (std::move (region_ref)),
        target_automation_track_ (target_at), source_automation_track_ (source_at)
  {
    if (
      !std::ranges::contains (
        source_automation_track_.get_children_vector (), region_ref_.id (),
        &structure::arrangement::ArrangerObjectUuidReference::id))
      {
        throw std::invalid_argument (
          "Source automation track does not include the region");
      }
  }

  void undo () override
  {
    // move region back
    auto region_ref = target_automation_track_.remove_object (region_ref_.id ());
    source_automation_track_.add_object (region_ref);
  }
  void redo () override
  {
    // move region
    auto region_ref = source_automation_track_.remove_object (region_ref_.id ());
    target_automation_track_.add_object (region_ref);
  }

private:
  bool is_midi ()
  {
    return std::holds_alternative<structure::arrangement::MidiRegion *> (
      region_ref_.get_object ());
  }

private:
  structure::arrangement::ArrangerObjectUuidReference region_ref_;
  structure::tracks::AutomationTrack                 &target_automation_track_;
  structure::tracks::AutomationTrack                 &source_automation_track_;
};

} // namespace zrythm::commands
