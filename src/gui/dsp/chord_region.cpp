// SPDX-FileCopyrightText: © 2019-2022, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"
#include "gui/dsp/chord_region.h"
#include "gui/dsp/chord_track.h"
#include "gui/dsp/tracklist.h"

ChordRegion::ChordRegion (ArrangerObjectRegistry &obj_registry, QObject * parent)
    : ArrangerObject (Type::ChordRegion), Region (obj_registry),
      QAbstractListModel (parent)
{
  ArrangerObject::parent_base_qproperties (*this);
  BoundedObject::parent_base_qproperties (*this);
  init_colored_object ();
}

void
ChordRegion::init_loaded ()
{
  ArrangerObject::init_loaded_base ();
  NamedObject::init_loaded_base ();
  for (const auto &chord : get_object_ptrs_view ())
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
// TODO
#if 0
  for (const auto * chord : get_object_ptrs_view ())
    {
      auto * new_chord = chord->clone_and_register (object_registry_);
      chord_objects_.push_back (new_chord->get_uuid ());
    }
#endif
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
      return QVariant::fromValue<ChordObject *> (
        get_object_ptrs_view ()[index.row ()]);
    }
  return {};
}

// ========================================================================

bool
ChordRegion::validate (bool is_project, double frames_per_tick) const
{
#if 0
  int idx = 0;
  for (const auto &chord : get_object_ptrs_view ())
    {
      z_return_val_if_fail (chord->index_ == idx++, false);
    }
#endif

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
