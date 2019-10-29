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
#include "audio/automation_region.h"
#include "audio/chord_region.h"
#include "audio/master_track.h"
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

/** First AP value. */
#define AP_VAL1 0.6
/** Second AP value. */
#define AP_VAL2 0.9

/** Marker name. */
#define MARKER_NAME "Marker name"

#define MUSICAL_SCALE_TYPE SCALE_IONIAN
#define MUSICAL_SCALE_ROOT NOTE_A

Position p1, p2;

/**
 * Bootstraps the test with test data.
 */
static void
bootstrap ()
{
  /* Create and add a MidiRegion with a MidiNote */
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

  /* Create and add an automation region with
   * 2 AutomationPoint's */
  r =
    automation_region_new (&p1, &p2, 1);
  AutomationTracklist * atl =
    track_get_automation_tracklist (P_MASTER_TRACK);
  Automatable * a =
    channel_get_automatable (
      P_MASTER_TRACK->channel,
      AUTOMATABLE_TYPE_CHANNEL_PAN);
  AutomationTrack * at =
    automation_tracklist_get_at_from_automatable (
      atl, a);
  track_add_region (
    P_MASTER_TRACK, r, at, 0, 1, 0);
  arranger_selections_add_object (
    (ArrangerSelections *) TL_SELECTIONS,
    (ArrangerObject *) r);

  /* add 2 automation points to the region */
  AutomationPoint * ap =
    automation_point_new_float (
      AP_VAL1, &p1, 1);
  automation_region_add_ap (
    r, ap, F_GEN_CURVE_POINTS);
  arranger_selections_add_object (
    (ArrangerSelections *) AUTOMATION_SELECTIONS,
    (ArrangerObject *) ap);
  ap =
    automation_point_new_float (
      AP_VAL2, &p2, 1);
  automation_region_add_ap (
    r, ap, F_GEN_CURVE_POINTS);
  arranger_selections_add_object (
    (ArrangerSelections *) AUTOMATION_SELECTIONS,
    (ArrangerObject *) ap);

  /* Create and add a chord region with
   * 2 Chord's */
  r =
    chord_region_new (&p1, &p2, 1);
  track_add_region (
    P_CHORD_TRACK, r, NULL, 0, 1, 0);
  arranger_selections_add_object (
    (ArrangerSelections *) TL_SELECTIONS,
    (ArrangerObject *) r);

  /* add 2 chorsd to the region */
  ChordObject * c =
    chord_object_new (
      r, 0, 1);
  chord_region_add_chord_object (
    r, c);
  arranger_object_pos_setter (
    (ArrangerObject *) c, &p1);
  arranger_selections_add_object (
    (ArrangerSelections *) CHORD_SELECTIONS,
    (ArrangerObject *) c);
  c =
    chord_object_new (
      r, 0, 1);
  chord_region_add_chord_object (
    r, c);
  arranger_object_pos_setter (
    (ArrangerObject *) c, &p2);
  arranger_selections_add_object (
    (ArrangerSelections *) CHORD_SELECTIONS,
    (ArrangerObject *) c);

  /* create and add a Marker */
  Marker * m =
    marker_new (MARKER_NAME, 1);
  marker_track_add_marker (
    P_MARKER_TRACK, m);
  arranger_selections_add_object (
    (ArrangerSelections *) TL_SELECTIONS,
    (ArrangerObject *) m);
  arranger_object_pos_setter (
    (ArrangerObject *) m, &p1);

  /* create and add a ScaleObject */
  MusicalScale * ms =
    musical_scale_new (
      MUSICAL_SCALE_TYPE, MUSICAL_SCALE_ROOT);
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

/**
 * Checks that the objects are back to their original
 * state.
 */
static void
check_objects_vs_original_state ()
{
  g_assert_cmpint (
    TL_SELECTIONS->num_regions, ==, 3);

  /* check midi region */
  ArrangerObject * obj =
    (ArrangerObject *) TL_SELECTIONS->regions[0];
  g_assert_cmppos (&obj->pos, &p1);
  g_assert_cmppos (&obj->end_pos, &p2);
  Region * r =
    (Region *) obj;
  g_assert_cmpint (r->num_midi_notes, ==, 1);
  MidiNote * mn =
    r->midi_notes[0];
  obj =
    (ArrangerObject *) mn;
  g_assert_cmpuint (mn->val, ==, MN_VAL);
  g_assert_cmpuint (mn->vel->vel, ==, MN_VEL);
  g_assert_cmppos (&obj->pos, &p1);
  g_assert_cmppos (&obj->end_pos, &p2);

  /* check automation region */
  obj =
    (ArrangerObject *) TL_SELECTIONS->regions[1];
  g_assert_cmppos (&obj->pos, &p1);
  g_assert_cmppos (&obj->end_pos, &p2);
  r =
    (Region *) obj;
  g_assert_cmpint (r->num_aps, ==, 2);
  AutomationPoint * ap =
    r->aps[0];
  obj = (ArrangerObject *) ap;
  g_assert_cmppos (&obj->pos, &p1);
  g_assert_cmpfloat_with_epsilon (
    ap->fvalue, AP_VAL1, 0.000001f);
  ap =
    r->aps[1];
  obj = (ArrangerObject *) ap;
  g_assert_cmppos (&obj->pos, &p2);
  g_assert_cmpfloat_with_epsilon (
    ap->fvalue, AP_VAL2, 0.000001f);

  /* check marker */
  g_assert_cmpint (
    TL_SELECTIONS->num_markers, ==, 1);
  obj =
    (ArrangerObject *) TL_SELECTIONS->markers[0];
  Marker * m = (Marker *) obj;
  g_assert_cmppos (&obj->pos, &p1);
  g_assert_cmpstr (m->name, ==, MARKER_NAME);

  /* check scale object */
  g_assert_cmpint (
    TL_SELECTIONS->num_scale_objects, ==, 1);
  obj =
    (ArrangerObject *)
    TL_SELECTIONS->scale_objects[0];
  ScaleObject * s = (ScaleObject *) obj;
  g_assert_cmppos (&obj->pos, &p1);
  g_assert_cmpint (
    s->scale->type, ==, MUSICAL_SCALE_TYPE);
  g_assert_cmpint (
    s->scale->root_key, ==, MUSICAL_SCALE_ROOT);
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
      (ArrangerSelections *) TL_SELECTIONS), ==, 5);
  g_assert_cmpint (
    arranger_selections_get_num_objects (
      (ArrangerSelections *) MA_SELECTIONS), ==, 1);
  check_objects_vs_original_state ();

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
      (ArrangerSelections *) TL_SELECTIONS), ==, 5);
  check_objects_vs_original_state ();
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
  g_assert_cmpint (
    arranger_selections_get_num_objects (
      (ArrangerSelections *) CHORD_SELECTIONS),
    ==, 0);
  g_assert_cmpint (
    arranger_selections_get_num_objects (
      (ArrangerSelections *) AUTOMATION_SELECTIONS),
    ==, 0);

  /* undo and check that the objects are created */
  undo_manager_undo (UNDO_MANAGER);
  g_assert_cmpint (
    arranger_selections_get_num_objects (
      (ArrangerSelections *) TL_SELECTIONS), ==, 5);
  check_objects_vs_original_state ();

  /* redo and check that the objects are gone */
  undo_manager_redo (UNDO_MANAGER);
  g_assert_cmpint (
    arranger_selections_get_num_objects (
      (ArrangerSelections *) TL_SELECTIONS), ==, 0);
  g_assert_cmpint (
    arranger_selections_get_num_objects (
      (ArrangerSelections *) MA_SELECTIONS), ==, 0);
  g_assert_cmpint (
    arranger_selections_get_num_objects (
      (ArrangerSelections *) CHORD_SELECTIONS),
    ==, 0);
  g_assert_cmpint (
    arranger_selections_get_num_objects (
      (ArrangerSelections *) AUTOMATION_SELECTIONS),
    ==, 0);
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

