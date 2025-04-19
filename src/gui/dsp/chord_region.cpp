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
  ArrangerObject::set_parent_on_base_qproperties (*this);
  BoundedObject::parent_base_qproperties (*this);
  init_colored_object ();
}

void
ChordRegion::init_loaded ()
{
  ArrangerObject::init_loaded_base ();
  NamedObject::init_loaded_base ();
  for (const auto &chord : get_children_view ())
    {
      chord->init_loaded ();
    }
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
  return get_children_vector ().size ();
}

QVariant
ChordRegion::data (const QModelIndex &index, int role) const
{
  if (
    index.row () < 0
    || index.row () >= static_cast<int> (get_children_vector ().size ()))
    return {};

  if (role == ChordObjectPtrRole)
    {
      return QVariant::fromValue<ChordObject *> (
        get_children_view ()[index.row ()]);
    }
  return {};
}

// ========================================================================

bool
ChordRegion::validate (bool is_project, double frames_per_tick) const
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
