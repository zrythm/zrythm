// SPDX-FileCopyrightText: © 2019-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "common/dsp/chord_region.h"
#include "common/dsp/chord_track.h"
#include "common/dsp/tracklist.h"
#include "gui/cpp/backend/project.h"
#include "gui/cpp/backend/zrythm.h"
#include "gui/cpp/gtk_widgets/bot_dock_edge.h"
#include "gui/cpp/gtk_widgets/center_dock.h"
#include "gui/cpp/gtk_widgets/chord_arranger.h"
#include "gui/cpp/gtk_widgets/chord_editor_space.h"
#include "gui/cpp/gtk_widgets/clip_editor.h"
#include "gui/cpp/gtk_widgets/clip_editor_inner.h"
#include "gui/cpp/gtk_widgets/main_window.h"
#include "gui/cpp/gtk_widgets/zrythm_app.h"

#include <glib/gi18n.h>

ChordRegion::
  ChordRegion (const Position &start_pos, const Position &end_pos, int idx)
{
  id_.type_ = RegionType::Chord;

  init (start_pos, end_pos, P_CHORD_TRACK->get_name_hash (), 0, idx);
}

void
ChordRegion::init_loaded ()
{
  ArrangerObject::init_loaded_base ();
  NameableObject::init_loaded_base ();
  for (auto &chord : chord_objects_)
    {
      chord->init_loaded ();
    }
}

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
    || !NameableObject::are_members_valid (is_project)
    || !LoopableObject::are_members_valid (is_project)
    || !MuteableObject::are_members_valid (is_project)
    || !LengthableObject::are_members_valid (is_project)
    || !ColoredObject::are_members_valid (is_project)
    || !ArrangerObject::are_members_valid (is_project))
    {
      return false;
    }

  return true;
}

ArrangerSelections *
ChordRegion::get_arranger_selections () const
{
  return CHORD_SELECTIONS.get ();
}

ArrangerWidget *
ChordRegion::get_arranger_for_children () const
{
  return MW_CHORD_ARRANGER;
}