// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <utility>

#include "structure/tracks/track_lane.h"

#include <QUndoCommand>

namespace zrythm::commands
{
class MoveRegionToLaneCommand : public QUndoCommand
{
public:
  MoveRegionToLaneCommand (
    structure::arrangement::ArrangerObjectUuidReference region_ref,
    structure::tracks::TrackLane                       &source_lane,
    structure::tracks::TrackLane                       &target_lane)
      : QUndoCommand (QObject::tr ("Move Plugin")),
        region_ref_ (std::move (region_ref)),
        target_automation_track_ (target_lane),
        source_automation_track_ (source_lane)
  {
    if (
      !std::ranges::contains (
        source_automation_track_.structure::arrangement::ArrangerObjectOwner<
          structure::arrangement::AudioRegion>::get_children_vector (),
        region_ref_.id (),
        &structure::arrangement::ArrangerObjectUuidReference::id)
      && !std::ranges::contains (
        source_automation_track_.structure::arrangement::ArrangerObjectOwner<
          structure::arrangement::MidiRegion>::get_children_vector (),
        region_ref_.id (),
        &structure::arrangement::ArrangerObjectUuidReference::id))
      {
        throw std::invalid_argument ("Source lane does not include the region");
      }
  }

  void undo () override
  {
    // move region back
    auto region_ref =
      is_midi ()
        ? target_automation_track_.structure::arrangement::ArrangerObjectOwner<
            structure::arrangement::MidiRegion>::remove_object (region_ref_.id ())
        : target_automation_track_.structure::arrangement::ArrangerObjectOwner<
            structure::arrangement::AudioRegion>::remove_object (region_ref_.id ());
    is_midi ()
      ? source_automation_track_.structure::arrangement::ArrangerObjectOwner<
          structure::arrangement::MidiRegion>::add_object (region_ref)
      : source_automation_track_.structure::arrangement::ArrangerObjectOwner<
          structure::arrangement::AudioRegion>::add_object (region_ref);
  }
  void redo () override
  {
    // move region
    auto region_ref =
      is_midi ()
        ? source_automation_track_.structure::arrangement::ArrangerObjectOwner<
            structure::arrangement::MidiRegion>::remove_object (region_ref_.id ())
        : source_automation_track_.structure::arrangement::ArrangerObjectOwner<
            structure::arrangement::AudioRegion>::remove_object (region_ref_.id ());
    is_midi ()
      ? target_automation_track_.structure::arrangement::ArrangerObjectOwner<
          structure::arrangement::MidiRegion>::add_object (region_ref)
      : target_automation_track_.structure::arrangement::ArrangerObjectOwner<
          structure::arrangement::AudioRegion>::add_object (region_ref);
  }

private:
  bool is_midi ()
  {
    return std::holds_alternative<structure::arrangement::MidiRegion *> (
      region_ref_.get_object ());
  }

private:
  structure::arrangement::ArrangerObjectUuidReference region_ref_;
  structure::tracks::TrackLane                       &target_automation_track_;
  structure::tracks::TrackLane                       &source_automation_track_;
};

} // namespace zrythm::commands
