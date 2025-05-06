// SPDX-FileCopyrightText: © 2018-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/backend/clip_editor.h"
#include "gui/backend/backend/project.h"
#include "gui/dsp/chord_object.h"
#include "gui/dsp/chord_region.h"
#include "gui/dsp/chord_track.h"
#include "utils/debug.h"

#include <fmt/format.h>

ChordObject::ChordObject (
  ArrangerObjectRegistry &obj_registry,
  TrackResolver           track_resolver,
  QObject *               parent)
    : ArrangerObject (Type::ChordObject, track_resolver), QObject (parent),
      RegionOwnedObject (obj_registry)
{
  ArrangerObject::set_parent_on_base_qproperties (*this);
}
void
ChordObject::init_after_cloning (
  const ChordObject &other,
  ObjectCloneType    clone_type)
{
  MuteableObject::copy_members_from (other, clone_type);
  RegionOwnedObject::copy_members_from (other, clone_type);
  ArrangerObject::copy_members_from (other, clone_type);
  chord_index_ = other.chord_index_;
}

/**
 * Returns the ChordDescriptor associated with this ChordObject.
 */
ChordObject::ChordDescriptor *
ChordObject::getChordDescriptor () const
{
  z_return_val_if_fail (CLIP_EDITOR, nullptr);
  return &CHORD_EDITOR->get_chord_at_index (chord_index_);
}

void
ChordObject::setChordDescriptor (dsp::ChordDescriptor * descr)
{
  assert (CLIP_EDITOR);
  auto idx = CHORD_EDITOR->get_chord_index (*descr);
  if (idx == chord_index_)
    return;
  chord_index_ = idx;
  Q_EMIT chordDescriptorChanged (descr);
}

void
ChordObject::set_chord_descriptor (int index)
{
  chord_index_ = index;
}

ArrangerObjectPtrVariant
ChordObject::add_clone_to_project (bool fire_events) const
{
  return {};
#if 0
  auto * clone = clone_raw_ptr ();
  get_region ()->append_object (clone, true);
  return clone;
#endif
}

ArrangerObjectPtrVariant
ChordObject::insert_clone_to_project () const
{
  return {};
#if 0
  auto * clone = clone_raw_ptr ();
  get_region ()->insert_object (clone, index_, true);
  return clone;
#endif
}

utils::Utf8String
ChordObject::gen_human_friendly_name () const
{
  return getChordDescriptor ()->to_string ();
}

bool
ChordObject::validate (bool is_project, dsp::FramesPerTick frames_per_tick) const
{
  // TODO
  return true;
}
