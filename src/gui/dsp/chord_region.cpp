// SPDX-FileCopyrightText: © 2019-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"
#include "gui/dsp/chord_region.h"
#include "gui/dsp/chord_track.h"
#include "gui/dsp/tracklist.h"

ChordRegion::ChordRegion (QObject * parent)
    : ArrangerObject (Type::Region), QAbstractListModel (parent)
{
  ArrangerObject::parent_base_qproperties (*this);
  BoundedObject::parent_base_qproperties (*this);
}

ChordRegion::ChordRegion (
  const Position &start_pos,
  const Position &end_pos,
  int             idx,
  double          ticks_per_frame,
  QObject *       parent)
    : ChordRegion (parent)
{
  id_.type_ = RegionType::Chord;

  init (start_pos, end_pos, P_CHORD_TRACK->get_uuid (), 0, idx, ticks_per_frame);
}

void
ChordRegion::init_loaded ()
{
  ArrangerObject::init_loaded_base ();
  NamedObject::init_loaded_base ();
  for (auto &chord : chord_objects_)
    {
      chord->init_loaded ();
    }
}

void
ChordRegion::init_after_cloning (
  const ChordRegion &other,
  ObjectCloneType    clone_type)
{
  // init (
  // *other.pos_, *other.end_pos_, other.id_.track_name_hash_, 0, other.id_.idx_);
  chord_objects_.reserve (other.chord_objects_.size ());
  for (const auto &chord : other.chord_objects_)
    {
      auto * new_chord = chord->clone_raw_ptr ();
      new_chord->setParent (this);
      chord_objects_.push_back (new_chord);
    }
  RegionT::copy_members_from (other, clone_type);
  TimelineObject::copy_members_from (other, clone_type);
  NamedObject::copy_members_from (other, clone_type);
  LoopableObject::copy_members_from (other, clone_type);
  MuteableObject::copy_members_from (other, clone_type);
  BoundedObject::copy_members_from (other, clone_type);
  ColoredObject::copy_members_from (other, clone_type);
  ArrangerObject::copy_members_from (other, clone_type);
}

// ========================================================================
// QML Interface
// ========================================================================

QHash<int, QByteArray>
ChordRegion::roleNames () const
{
  QHash<int, QByteArray> roles;
  roles[ChordObjectPtrRole] = "chordObject";
  return roles;
}
int
ChordRegion::rowCount (const QModelIndex &parent) const
{
  return chord_objects_.size ();
}

QVariant
ChordRegion::data (const QModelIndex &index, int role) const
{
  if (
    index.row () < 0
    || index.row () >= static_cast<int> (chord_objects_.size ()))
    return {};

  if (role == ChordObjectPtrRole)
    {
      return QVariant::fromValue (chord_objects_.at (index.row ()));
    }
  return {};
}

// ========================================================================

bool
ChordRegion::validate (bool is_project, double frames_per_tick) const
{
  int idx = 0;
  for (auto &chord : chord_objects_)
    {
      z_return_val_if_fail (chord->index_ == idx++, false);
    }

  if (
    !Region::are_members_valid (is_project)
    || !TimelineObject::are_members_valid (is_project)
    || !NamedObject::are_members_valid (is_project)
    || !LoopableObject::are_members_valid (is_project)
    || !MuteableObject::are_members_valid (is_project)
    || !BoundedObject::are_members_valid (is_project)
    || !ColoredObject::are_members_valid (is_project)
    || !ArrangerObject::are_members_valid (is_project))
    {
      return false;
    }

  return true;
}
