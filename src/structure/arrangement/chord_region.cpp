// SPDX-FileCopyrightText: © 2019-2022, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"
#include "structure/arrangement/chord_region.h"
#include "structure/tracks/chord_track.h"
#include "structure/tracks/tracklist.h"

namespace zrythm::structure::arrangement
{
ChordRegion::ChordRegion (
  ArrangerObjectRegistry &obj_registry,
  TrackResolver           track_resolver,
  QObject *               parent)
    : ArrangerObject (Type::ChordRegion, track_resolver), Region (obj_registry),
      QObject (parent)
{
  ArrangerObject::set_parent_on_base_qproperties (*this);
  BoundedObject::parent_base_qproperties (*this);
  init_colored_object ();
}

void
ChordRegion::init_after_cloning (
  const ChordRegion &other,
  ObjectCloneType    clone_type)
{
  RegionT::copy_members_from (other, clone_type);
  TimelineObject::copy_members_from (other, clone_type);
  NamedObject::copy_members_from (other, clone_type);
  LoopableObject::copy_members_from (other, clone_type);
  MuteableObject::copy_members_from (other, clone_type);
  BoundedObject::copy_members_from (other, clone_type);
  ColoredObject::copy_members_from (other, clone_type);
  ArrangerObject::copy_members_from (other, clone_type);
  ArrangerObjectOwner::copy_members_from (other, clone_type);
}

bool
ChordRegion::validate (bool is_project, dsp::FramesPerTick frames_per_tick) const
{
#if 0
  int idx = 0;
  for (const auto &chord : get_children_view ())
    {
      z_return_val_if_fail (chord->index_ == idx++, false);
    }
#endif

  if (
    !Region::are_members_valid (is_project, frames_per_tick)
    || !TimelineObject::are_members_valid (is_project, frames_per_tick)
    || !NamedObject::are_members_valid (is_project, frames_per_tick)
    || !LoopableObject::are_members_valid (is_project, frames_per_tick)
    || !MuteableObject::are_members_valid (is_project, frames_per_tick)
    || !BoundedObject::are_members_valid (is_project, frames_per_tick)
    || !ColoredObject::are_members_valid (is_project, frames_per_tick)
    || !ArrangerObject::are_members_valid (is_project, frames_per_tick))
    {
      return false;
    }

  return true;
}
}
