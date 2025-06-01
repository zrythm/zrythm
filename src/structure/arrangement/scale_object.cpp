// SPDX-FileCopyrightText: © 2018-2019, 2022, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/position.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"
#include "structure/arrangement/arranger_object.h"
#include "structure/arrangement/scale_object.h"
#include "structure/tracks/chord_track.h"
#include "structure/tracks/tracklist.h"

#include <fmt/format.h>

namespace zrythm::structure::arrangement
{
ScaleObject::ScaleObject (
  ArrangerObjectRegistry &obj_registry,
  TrackResolver           track_resolver,
  QObject *               parent)
    : ArrangerObject (ArrangerObject::Type::ScaleObject, track_resolver),
      QObject (parent)
{
  ArrangerObject::set_parent_on_base_qproperties (*this);
}

void
ScaleObject::init_after_cloning (
  const ScaleObject &other,
  ObjectCloneType    clone_type)
{
  scale_ = other.scale_;
  TimelineObject::copy_members_from (other, clone_type);
  MuteableObject::copy_members_from (other, clone_type);
  ArrangerObject::copy_members_from (other, clone_type);
}

utils::Utf8String
ScaleObject::gen_human_friendly_name () const
{
  return scale_.to_string ();
}

ArrangerObjectPtrVariant
ScaleObject::add_clone_to_project (bool fire_events) const
{
  return {};
#if 0
  auto * clone = clone_raw_ptr ();
  P_CHORD_TRACK->add_scale (*clone);
  return clone;
#endif
}

ArrangerObjectPtrVariant
ScaleObject::insert_clone_to_project () const
{
  return {};
#if 0
  auto * clone = clone_raw_ptr ();
  P_CHORD_TRACK->insert_scale (*clone, index_in_chord_track_);
  return clone;
#endif
}

bool
ScaleObject::validate (bool is_project, dsp::FramesPerTick frames_per_tick) const
{
  // TODO
  return true;
}
}
