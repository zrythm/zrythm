// SPDX-FileCopyrightText: © 2018-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "common/dsp/chord_object.h"
#include "common/dsp/chord_region.h"
#include "common/dsp/chord_track.h"
#include "common/utils/debug.h"
#include "gui/cpp/backend/clip_editor.h"
#include "gui/cpp/backend/project.h"
#include "gui/cpp/gtk_widgets/bot_dock_edge.h"
#include "gui/cpp/gtk_widgets/center_dock.h"
#include "gui/cpp/gtk_widgets/chord_arranger.h"
#include "gui/cpp/gtk_widgets/chord_editor_space.h"
#include "gui/cpp/gtk_widgets/clip_editor.h"
#include "gui/cpp/gtk_widgets/clip_editor_inner.h"
#include "gui/cpp/gtk_widgets/main_window.h"
#include "gui/cpp/gtk_widgets/zrythm_app.h"

#include <fmt/format.h>

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

ArrangerObject::ArrangerObjectPtr
ChordObject::find_in_project () const
{
  /* get actual region - clone's region might be an unused clone */
  auto r = RegionImpl<ChordRegion>::find (region_id_);
  z_return_val_if_fail (r, nullptr);

  z_return_val_if_fail (
    r && r->chord_objects_.size () > (size_t) index_, nullptr);
  z_return_val_if_fail_cmp (index_, >=, 0, nullptr);

  auto &co = r->chord_objects_[index_];
  z_return_val_if_fail (co, nullptr);
  z_return_val_if_fail (*co == *this, nullptr);
  return co;
}

ArrangerWidget *
ChordObject::get_arranger () const
{
  return (ArrangerWidget *) (MW_CHORD_ARRANGER);
}

ArrangerObject::ArrangerObjectPtr
ChordObject::add_clone_to_project (bool fire_events) const
{
  return get_region ()->append_object (clone_shared (), true);
}

ArrangerObject::ArrangerObjectPtr
ChordObject::insert_clone_to_project () const
{
  return get_region ()->insert_object (clone_shared (), index_, true);
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