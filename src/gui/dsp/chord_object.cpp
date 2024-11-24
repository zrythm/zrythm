// SPDX-FileCopyrightText: © 2018-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/backend/clip_editor.h"
#include "gui/backend/backend/project.h"
#include "gui/dsp/chord_object.h"
#include "gui/dsp/chord_region.h"
#include "gui/dsp/chord_track.h"

#include "utils/debug.h"
#include <fmt/format.h>

ChordObject::ChordObject (QObject * parent)
    : ArrangerObject (Type::ChordObject), QObject (parent)
{
  ArrangerObject::parent_base_qproperties (*this);
}

ChordObject::ChordObject (
  const RegionIdentifier &region_id,
  int                     chord_index,
  int                     index,
  QObject *               parent)
    : ArrangerObject (Type::ChordObject), QObject (parent),
      RegionOwnedObjectImpl (region_id, index), chord_index_ (chord_index)
{
  ArrangerObject::parent_base_qproperties (*this);
}

void
ChordObject::init_after_cloning (const ChordObject &other)
{
  MuteableObject::copy_members_from (other);
  RegionOwnedObjectImpl::copy_members_from (other);
  ArrangerObject::copy_members_from (other);
  chord_index_ = other.chord_index_;
}

void
ChordObject::init_loaded ()
{
  ArrangerObject::init_loaded_base ();
  RegionOwnedObjectImpl::init_loaded_base ();
  MuteableObject::init_loaded_base ();
}

/**
 * Returns the ChordDescriptor associated with this ChordObject.
 */
ChordDescriptor *
ChordObject::get_chord_descriptor () const
{
  z_return_val_if_fail (CLIP_EDITOR, nullptr);
  z_return_val_if_fail_cmp (chord_index_, >=, 0, nullptr);
  return &CHORD_EDITOR->chords_[chord_index_];
}

std::optional<ArrangerObjectPtrVariant>
ChordObject::find_in_project () const
{
  /* get actual region - clone's region might be an unused clone */
  auto r = RegionImpl<ChordRegion>::find (region_id_);
  z_return_val_if_fail (r, std::nullopt);

  z_return_val_if_fail (
    r && r->chord_objects_.size () > (size_t) index_, std::nullopt);
  z_return_val_if_fail_cmp (index_, >=, 0, std::nullopt);

  auto &co = r->chord_objects_[index_];
  z_return_val_if_fail (co, std::nullopt);
  z_return_val_if_fail (*co == *this, std::nullopt);
  return co;
}

ArrangerObjectPtrVariant
ChordObject::add_clone_to_project (bool fire_events) const
{
  auto * clone = clone_raw_ptr ();
  get_region ()->append_object (clone, true);
  return clone;
}

ArrangerObjectPtrVariant
ChordObject::insert_clone_to_project () const
{
  auto * clone = clone_raw_ptr ();
  get_region ()->insert_object (clone, index_, true);
  return clone;
}

std::string
ChordObject::print_to_str () const
{
  return fmt::format ("ChordObject: {} {}", index_, chord_index_);
}

std::string
ChordObject::gen_human_friendly_name () const
{
  return get_chord_descriptor ()->to_string ();
}

bool
ChordObject::validate (bool is_project, double frames_per_tick) const
{
  // TODO
  return true;
}
