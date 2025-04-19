// SPDX-FileCopyrightText: © 2018-2019, 2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"
#include "gui/dsp/arranger_object.h"
#include "gui/dsp/chord_track.h"
#include "gui/dsp/scale_object.h"
#include "gui/dsp/tracklist.h"

#include "dsp/position.h"
#include <fmt/format.h>

ScaleObject::ScaleObject (ArrangerObjectRegistry &obj_registry, QObject * parent)
    : ArrangerObject (ArrangerObject::Type::ScaleObject), QObject (parent)
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

void
ScaleObject::init_loaded ()
{
  ArrangerObject::init_loaded_base ();
}

std::optional<ArrangerObjectPtrVariant>
ScaleObject::find_in_project () const
{
  return {};
#if 0
  z_return_val_if_fail (
    index_in_chord_track_ < (int) P_CHORD_TRACK->scales_.size (), std::nullopt);
  auto &ret = P_CHORD_TRACK->scales_.at (index_in_chord_track_);
  z_return_val_if_fail (*ret == *this, std::nullopt);
  return ret;
#endif
}

std::string
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
ScaleObject::validate (bool is_project, double frames_per_tick) const
{
  // TODO
  return true;
}
