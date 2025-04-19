// SPDX-FileCopyrightText: © 2019-2020, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"
#include "gui/dsp/marker.h"
#include "gui/dsp/marker_track.h"
#include "gui/dsp/tracklist.h"

Marker::Marker (
  ArrangerObjectRegistry &obj_registry,
  NameValidator           validator,
  QObject *               parent)
    : ArrangerObject (ArrangerObject::Type::Marker), QObject (parent),
      NamedObject (validator)
{
  ArrangerObject::set_parent_on_base_qproperties (*this);
}

void
Marker::init_loaded ()
{
  ArrangerObject::init_loaded_base ();
  NamedObject::init_loaded_base ();
}

void
Marker::init_after_cloning (const Marker &other, ObjectCloneType clone_type)

{
  marker_type_ = other.marker_type_;
  NamedObject::copy_members_from (other, clone_type);
  TimelineObject::copy_members_from (other, clone_type);
  ArrangerObject::copy_members_from (other, clone_type);
}

std::string
Marker::print_to_str () const
{
  return fmt::format (
    "Marker: name: {}, type: {},  position: {}", name_,
    ENUM_NAME (marker_type_), *pos_);
}

std::optional<ArrangerObjectPtrVariant>
Marker::find_in_project () const
{
  // TODO
  return std::nullopt;
#if 0
  z_return_val_if_fail (
    (int) P_MARKER_TRACK->markers_.size () > marker_track_index_, std::nullopt);

  auto * marker = P_MARKER_TRACK->get_markers ()[marker_track_index_];
  z_return_val_if_fail (*marker == *this, std::nullopt);
  return marker;
#endif
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
Marker::validate (bool is_project, double frames_per_tick) const
{
  if (
    !ArrangerObject::are_members_valid (is_project)
    || !NamedObject::are_members_valid (is_project))
    return false;

  return true;
}
