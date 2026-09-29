// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "commands/arranger_object_owner_ref.h"

namespace zrythm::commands
{

ArrangerObjectOwnerRef
make_owner_ref (structure::tracks::Track &track, utils::IObjectRegistry &registry)
{
  return ArrangerObjectOwnerRef (
    structure::tracks::TrackUuidReference (track.get_uuid (), registry));
}

ArrangerObjectOwnerRef
make_owner_ref (
  structure::tracks::TrackLane &lane,
  utils::IObjectRegistry       &registry)
{
  return ArrangerObjectOwnerRef (
    structure::tracks::TrackLaneUuidReference (lane.get_uuid (), registry));
}

ArrangerObjectOwnerRef
make_owner_ref (
  structure::arrangement::ArrangerObject &obj,
  utils::IObjectRegistry                 &registry)
{
  return ArrangerObjectOwnerRef (
    structure::arrangement::ArrangerObjectUuidReference (
      obj.get_uuid (), registry));
}

ArrangerObjectOwnerRef
make_owner_ref (
  structure::arrangement::TempoObjectManager &manager,
  utils::IObjectRegistry                     &registry)
{
  return ArrangerObjectOwnerRef (
    structure::arrangement::TempoObjectManagerUuidReference (
      manager.get_uuid (), registry));
}

ArrangerObjectOwnerRef
make_owner_ref (
  structure::tracks::AutomationTrack &automation_track,
  utils::IObjectRegistry             &registry)
{
  return ArrangerObjectOwnerRef (
    structure::tracks::AutomationTrackUuidReference (
      automation_track.get_uuid (), registry));
}

} // namespace zrythm::commands
