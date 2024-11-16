// SPDX-FileCopyrightText: © 2018-2019, 2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "common/dsp/arranger_object.h"
#include "common/dsp/chord_track.h"
#include "common/dsp/position.h"
#include "common/dsp/scale_object.h"
#include "common/dsp/tracklist.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"

#include <fmt/format.h>

ScaleObject::ScaleObject (QObject * parent)
    : ArrangerObject (ArrangerObject::Type::ScaleObject), QObject (parent)
{
  ArrangerObject::parent_base_qproperties (*this);
}

ScaleObject::ScaleObject (const MusicalScale &descr, QObject * parent)
    : ScaleObject (parent)
{
  scale_ = descr;
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
    "ScaleObject: {} | {}", "TODO print scale", pos_->to_string ());
}

std::optional<ArrangerObjectPtrVariant>
ScaleObject::find_in_project () const
{
  z_return_val_if_fail (
    index_in_chord_track_ < (int) P_CHORD_TRACK->scales_.size (), std::nullopt);
  auto &ret = P_CHORD_TRACK->scales_.at (index_in_chord_track_);
  z_return_val_if_fail (*ret == *this, std::nullopt);
  return ret;
}

std::string
ScaleObject::gen_human_friendly_name () const
{
  return scale_.to_string ();
}

ArrangerObjectPtrVariant
ScaleObject::add_clone_to_project (bool fire_events) const
{
  auto * clone = clone_raw_ptr ();
  P_CHORD_TRACK->add_scale (*clone);
  return clone;
}

ArrangerObjectPtrVariant
ScaleObject::insert_clone_to_project () const
{
  auto * clone = clone_raw_ptr ();
  P_CHORD_TRACK->insert_scale (*clone, index_in_chord_track_);
  return clone;
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