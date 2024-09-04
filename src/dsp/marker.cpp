// SPDX-FileCopyrightText: © 2019-2020, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/marker.h"
#include "dsp/marker_track.h"
#include "dsp/tracklist.h"
#include "project.h"
#include "zrythm.h"

void
Marker::init_loaded ()
{
  ArrangerObject::init_loaded_base ();
  NameableObject::init_loaded_base ();
}

void
Marker::init_after_cloning (const Marker &other)
{
  marker_type_ = other.marker_type_;
  marker_track_index_ = other.marker_track_index_;
  NameableObject::copy_members_from (other);
  TimelineObject::copy_members_from (other);
  ArrangerObject::copy_members_from (other);
}

Marker *
Marker::find_by_name (const std::string &name)
{
  for (auto &marker : P_MARKER_TRACK->markers_)
    {
      if (name == marker->name_)
        return marker.get ();
    }

  return nullptr;
}

std::string
Marker::print_to_str () const
{
  return fmt::format (
    "Marker: name: {}, type: {}, track index: {}, position: {}", name_,
    ENUM_NAME (marker_type_), marker_track_index_, pos_.to_string ());
}

Marker::ArrangerObjectPtr
Marker::find_in_project () const
{
  z_return_val_if_fail (
    (int) P_MARKER_TRACK->markers_.size () > marker_track_index_, nullptr);

  auto &marker = P_MARKER_TRACK->markers_.at (marker_track_index_);
  z_return_val_if_fail (*marker == *this, nullptr);
  return marker;
}

Marker::ArrangerObjectPtr
Marker::add_clone_to_project (bool fire_events) const
{
  return P_MARKER_TRACK->add_marker (clone_shared ());
}

Marker::ArrangerObjectPtr
Marker::insert_clone_to_project () const
{
  return P_MARKER_TRACK->insert_marker (clone_shared (), marker_track_index_);
}

bool
Marker::validate (bool is_project, double frames_per_tick) const
{
  if (
    !ArrangerObject::are_members_valid (is_project)
    || !NameableObject::are_members_valid (is_project))
    return false;

  return true;
}