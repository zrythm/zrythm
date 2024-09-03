// SPDX-FileCopyrightText: © 2018-2019, 2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/arranger_object.h"
#include "dsp/chord_track.h"
#include "dsp/position.h"
#include "dsp/scale_object.h"
#include "dsp/tracklist.h"
#include "project.h"
#include "zrythm.h"

#include <fmt/format.h>

ScaleObject::ScaleObject (const MusicalScale &descr)
    : ArrangerObject (ArrangerObject::Type::ScaleObject), scale_ (descr)
{
}

void
ScaleObject::init_after_cloning (const ScaleObject &other)
{
  index_in_chord_track_ = other.index_in_chord_track_;
  scale_ = other.scale_;
  TimelineObject::copy_members_from (other);
  MuteableObject::copy_members_from (other);
  ArrangerObject::copy_members_from (other);
}

void
ScaleObject::init_loaded ()
{
  ArrangerObject::init_loaded_base ();
}

std::string
ScaleObject::print_to_str () const
{
  return fmt::format (
    "ScaleObject: {} | {}", "TODO print scale", pos_.to_string ());
}

ScaleObject::ArrangerObjectPtr
ScaleObject::find_in_project () const
{
  z_return_val_if_fail (
    index_in_chord_track_ < (int) P_CHORD_TRACK->scales_.size (), nullptr);
  auto &ret = P_CHORD_TRACK->scales_[index_in_chord_track_];
  z_return_val_if_fail (*ret == *this, nullptr);
  return ret;
}

std::string
ScaleObject::gen_human_friendly_name () const
{
  return scale_.to_string ();
}

ScaleObject::ArrangerObjectPtr
ScaleObject::add_clone_to_project (bool fire_events) const
{
  return P_CHORD_TRACK->add_scale (clone_shared ());
}

ScaleObject::ArrangerObjectPtr
ScaleObject::insert_clone_to_project () const
{
  return P_CHORD_TRACK->insert_scale (clone_shared (), index_in_chord_track_);
}

void
ScaleObject::set_index_in_chord_track (int index)
{
  index_in_chord_track_ = index;
}

bool
ScaleObject::validate (bool is_project, double frames_per_tick) const
{
  // TODO
  return true;
}