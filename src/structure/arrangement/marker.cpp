// SPDX-FileCopyrightText: © 2019-2020, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"
#include "structure/arrangement/marker.h"
#include "structure/tracks/marker_track.h"
#include "structure/tracks/tracklist.h"

namespace zrythm::structure::arrangement
{
Marker::Marker (
  ArrangerObjectRegistry &obj_registry,
  TrackResolver           track_resolver,
  NameValidator           validator,
  QObject *               parent)
    : ArrangerObject (ArrangerObject::Type::Marker, track_resolver),
      QObject (parent), NamedObject (validator)
{
  ArrangerObject::set_parent_on_base_qproperties (*this);
}

void
init_from (Marker &obj, const Marker &other, utils::ObjectCloneType clone_type)

{
  obj.marker_type_ = other.marker_type_;
  init_from (
    static_cast<NamedObject &> (obj), static_cast<const NamedObject &> (other),
    clone_type);
  init_from (
    static_cast<TimelineObject &> (obj),
    static_cast<const TimelineObject &> (other), clone_type);
  init_from (
    static_cast<ArrangerObject &> (obj),
    static_cast<const ArrangerObject &> (other), clone_type);
}

ArrangerObjectPtrVariant
Marker::add_clone_to_project (bool fire_events) const
{
  return {};
#if 0
  auto * clone = clone_raw_ptr ();
  P_MARKER_TRACK->add_marker (clone);
  return clone;
#endif
}

ArrangerObjectPtrVariant
Marker::insert_clone_to_project () const
{
  return {};
#if 0
  auto * clone = clone_raw_ptr ();
  P_MARKER_TRACK->insert_marker (clone, marker_track_index_);
  return clone;
#endif
}

bool
Marker::validate (bool is_project, dsp::FramesPerTick frames_per_tick) const
{
  if (
    !ArrangerObject::are_members_valid (is_project, frames_per_tick)
    || !NamedObject::are_members_valid (is_project, frames_per_tick))
    return false;

  return true;
}
}
