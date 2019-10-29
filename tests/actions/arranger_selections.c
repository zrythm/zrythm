/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "actions/arranger_selections.h"
#include "actions/undoable_action.h"
#include "audio/midi_note.h"
#include "audio/region.h"
#include "project.h"
#include "utils/flags.h"
#include "zrythm.h"

#include "tests/helpers/zrythm.h"

#include <glib.h>

/** MidiNote value to use. */
#define MN_VAL 78

/** MidiNote velocity to use. */
#define MN_VEL 23

Position p1, p2;

/**
 * Bootstraps the test with test data.
 */
static void
bootstrap ()
{
  /* Create and add a Region, Marker and
   * ScaleObject. */
  position_set_to_bar (&p1, 2);
  position_set_to_bar (&p2, 4);
  Region * r =
    midi_region_new (&p1, &p2, 1);
  Track * track =
    track_new (TRACK_TYPE_MIDI, "Midi track", 1);
  tracklist_append_track (
    TRACKLIST, track, F_NO_PUBLISH_EVENTS,
    F_NO_RECALC_GRAPH);
  track_add_region (
    track, r, NULL, 0, 1, 0);
  arranger_selections_add_object (
    (ArrangerSelections *) TL_SELECTIONS,
    (ArrangerObject *) r);

  /* add a midi note to the region */
  MidiNote * mn =
    midi_note_new (r, &p1, &p2, MN_VAL, MN_VEL, 1);
  midi_region_add_midi_note (
    r, mn);
  arranger_selections_add_object (
    (ArrangerSelections *) MA_SELECTIONS,
    (ArrangerObject *) mn);

  Marker * m =
    marker_new ("marker_name", 1);
  marker_track_add_marker (
    P_MARKER_TRACK, m);
  arranger_selections_add_object (
    (ArrangerSelections *) TL_SELECTIONS,
    (ArrangerObject *) m);
  arranger_object_pos_setter (
    (ArrangerObject *) m, &p1);

  MusicalScale * ms =
    musical_scale_new (
      SCALE_IONIAN, NOTE_C);
  ScaleObject * s =
    scale_object_new (
      ms, 1);
  chord_track_add_scale (
    P_CHORD_TRACK, s);
  arranger_selections_add_object (
    (ArrangerSelections *) TL_SELECTIONS,
    (ArrangerObject *) s);
  arranger_object_pos_setter (
    (ArrangerObject *) s, &p1);
}

static void
test_create ()
{
  /* do create */
  UndoableAction * ua =
    arranger_selections_action_new_create (
      TL_SELECTIONS);
  undo_manager_perform (UNDO_MANAGER, ua);

  /* check */
  g_assert_cmpint (
    arranger_selections_get_num_objects (
      (ArrangerSelections *) TL_SELECTIONS), ==, 3);
  g_assert_cmpint (
    arranger_selections_get_num_objects (
      (ArrangerSelections *) MA_SELECTIONS), ==, 1);

  /* undo and check that the objects are deleted */
  undo_manager_undo (UNDO_MANAGER);
  g_assert_cmpint (
    arranger_selections_get_num_objects (
      (ArrangerSelections *) TL_SELECTIONS), ==, 0);
  g_assert_cmpint (
    arranger_selections_get_num_objects (
      (ArrangerSelections *) MA_SELECTIONS), ==, 0);

  /* redo and check that the objects are there */
  undo_manager_redo (UNDO_MANAGER);
  g_assert_cmpint (
    arranger_selections_get_num_objects (
      (ArrangerSelections *) TL_SELECTIONS), ==, 3);
  ArrangerObject * obj =
    (ArrangerObject *) TL_SELECTIONS->regions[0];
  g_assert_cmppos (&obj->pos, &p1);
  g_assert_cmppos (&obj->end_pos, &p2);
  obj =
    (ArrangerObject *) MA_SELECTIONS->midi_notes[0];
  MidiNote * mn = (MidiNote *) obj;
  g_assert_cmpuint (mn->val, ==, MN_VAL);
  g_assert_cmpuint (mn->vel->vel, ==, MN_VEL);
  g_assert_cmppos (&obj->pos, &p1);
  g_assert_cmppos (&obj->end_pos, &p2);
  obj =
    (ArrangerObject *) TL_SELECTIONS->markers[0];
  g_assert_cmppos (&obj->pos, &p1);
  obj =
    (ArrangerObject *) TL_SELECTIONS->scale_objects[0];
  g_assert_cmppos (&obj->pos, &p1);
}

static void
test_delete ()
{
  /* do delete */
  UndoableAction * ua =
    arranger_selections_action_new_delete (
      TL_SELECTIONS);
  undo_manager_perform (UNDO_MANAGER, ua);

  /* check */
  g_assert_cmpint (
    arranger_selections_get_num_objects (
      (ArrangerSelections *) TL_SELECTIONS), ==, 0);
  g_assert_cmpint (
    arranger_selections_get_num_objects (
      (ArrangerSelections *) MA_SELECTIONS), ==, 0);

  /* undo and check that the objects are created */
  undo_manager_undo (UNDO_MANAGER);
  g_assert_cmpint (
    arranger_selections_get_num_objects (
      (ArrangerSelections *) TL_SELECTIONS), ==, 3);

  /* redo and check that the objects are gone */
  undo_manager_redo (UNDO_MANAGER);
  g_assert_cmpint (
    arranger_selections_get_num_objects (
      (ArrangerSelections *) TL_SELECTIONS), ==, 0);
  g_assert_cmpint (
    arranger_selections_get_num_objects (
      (ArrangerSelections *) MA_SELECTIONS), ==, 0);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  test_helper_zrythm_init ();

  bootstrap ();

#define TEST_PREFIX "/actions/arranger_selections/"

  g_test_add_func (
    TEST_PREFIX "test create",
    (GTestFunc) test_create);
  g_test_add_func (
    TEST_PREFIX "test delete",
    (GTestFunc) test_delete);

  return g_test_run ();
}

