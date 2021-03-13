/*
 * Copyright (C) 2019-2021 Alexandros Theodotou <alex at zrythm dot org>
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

#include "zrythm-test-config.h"

#include "actions/arranger_selections.h"
#include "actions/undoable_action.h"
#include "audio/audio_region.h"
#include "audio/automation_region.h"
#include "audio/chord_region.h"
#include "audio/master_track.h"
#include "audio/midi_note.h"
#include "audio/region.h"
#include "project.h"
#include "utils/dsp.h"
#include "utils/flags.h"
#include "utils/string.h"
#include "zrythm.h"

#include "tests/helpers/project.h"

#include <glib.h>

Position p1, p2;

/**
 * Bootstraps the test with test data.
 */
static void
rebootstrap_timeline ()
{
  test_project_rebootstrap_timeline (
    &p1, &p2);
}

static void
select_audio_and_midi_regions_only ()
{
  arranger_selections_clear (
    (ArrangerSelections *) TL_SELECTIONS,
    F_NO_FREE, F_NO_PUBLISH_EVENTS);
  g_assert_cmpint (
    TL_SELECTIONS->num_regions, ==, 0);

  Track * midi_track =
    tracklist_find_track_by_name (
      TRACKLIST, MIDI_TRACK_NAME);
  g_assert_nonnull (midi_track);
  arranger_selections_add_object (
    (ArrangerSelections *) TL_SELECTIONS,
    (ArrangerObject *)
    midi_track->lanes[MIDI_REGION_LANE]->
      regions[0]);
  g_assert_cmpint (
    TL_SELECTIONS->num_regions, ==, 1);
  Track * audio_track =
    tracklist_find_track_by_name (
      TRACKLIST, AUDIO_TRACK_NAME);
  g_assert_nonnull (audio_track);
  arranger_selections_add_object (
    (ArrangerSelections *) TL_SELECTIONS,
    (ArrangerObject *)
    audio_track->lanes[AUDIO_REGION_LANE]->
      regions[0]);
  g_assert_cmpint (
    TL_SELECTIONS->num_regions, ==, 2);
}

/**
 * Check if the undo stack has a single element.
 */
static void
check_has_single_undo ()
{
  g_assert_cmpint (
    UNDO_MANAGER->undo_stack->stack->top, ==, 0);
  g_assert_cmpint (
    UNDO_MANAGER->redo_stack->stack->top, ==, -1);
  g_assert_cmpint (
    (int)
    UNDO_MANAGER->undo_stack->num_as_actions, ==,
    1);
  g_assert_cmpint (
    (int)
    UNDO_MANAGER->redo_stack->num_as_actions, ==,
    0);
}

/**
 * Check if the redo stack has a single element.
 */
static void
check_has_single_redo ()
{
  g_assert_cmpint (
    (int)
    UNDO_MANAGER->undo_stack->stack->top, ==, -1);
  g_assert_cmpint (
    (int)
    UNDO_MANAGER->redo_stack->stack->top, ==, 0);
  g_assert_cmpint (
    (int)
    UNDO_MANAGER->undo_stack->num_as_actions, ==, 0);
  g_assert_cmpint (
    (int)
    UNDO_MANAGER->redo_stack->num_as_actions, ==, 1);
}

/**
 * Checks that the objects are back to their original
 * state.
 * @param check_selections Also checks that the
 *   selections are back to where they were.
 */
static void
check_timeline_objects_vs_original_state (
  bool check_selections,
  bool _check_has_single_undo,
  bool _check_has_single_redo)
{
  test_project_check_vs_original_state (
    &p1, &p2, check_selections);

  /* check that undo/redo stacks have the correct
   * counts */
  if (_check_has_single_undo)
    {
      check_has_single_undo ();
    }
  else if (_check_has_single_redo)
    {
      check_has_single_redo ();
    }
}
/**
 * Checks that the objects are deleted.
 *
 * @param creating Whether this is part of a create
 *   test.
 */
static void
check_timeline_objects_deleted (
  int creating)
{
  g_assert_cmpint (
    arranger_selections_get_num_objects (
      (ArrangerSelections *) TL_SELECTIONS), ==, 0);

  Track * midi_track =
    tracklist_find_track_by_name (
      TRACKLIST, MIDI_TRACK_NAME);
  g_assert_nonnull (midi_track);
  Track * audio_track =
    tracklist_find_track_by_name (
      TRACKLIST, AUDIO_TRACK_NAME);
  g_assert_nonnull (audio_track);

  /* check midi region */
  g_assert_cmpint (
    midi_track->num_lanes, ==, 1);
  g_assert_cmpint (
    midi_track->lanes[0]->num_regions, ==, 0);

  /* check audio region */
  g_assert_cmpint (
    midi_track->num_lanes, ==, 1);
  g_assert_cmpint (
    audio_track->lanes[0]->num_regions, ==, 0);

  /* check automation region */
  AutomationTracklist * atl =
    track_get_automation_tracklist (P_MASTER_TRACK);
  g_assert_nonnull (atl);
  AutomationTrack * at =
    channel_get_automation_track (
      P_MASTER_TRACK->channel,
      PORT_FLAG_STEREO_BALANCE);
  g_assert_nonnull (at);
  g_assert_cmpint (at->num_regions, ==, 0);

  /* check marker */
  g_assert_cmpint (
    P_MARKER_TRACK->num_markers, ==, 2);

  /* check scale object */
  g_assert_cmpint (
    P_CHORD_TRACK->num_scales, ==, 0);

  if (creating)
    {
      check_has_single_redo ();
    }
  else
    {
      check_has_single_undo ();
    }
}

static void
test_create_timeline ()
{
  rebootstrap_timeline ();

  /* do create */
  UndoableAction * ua =
    arranger_selections_action_new_create (
      TL_SELECTIONS);
  undo_manager_perform (UNDO_MANAGER, ua);

  /* check */
  g_assert_cmpint (
    arranger_selections_get_num_objects (
      (ArrangerSelections *) TL_SELECTIONS), ==,
    TOTAL_TL_SELECTIONS);
  g_assert_cmpint (
    arranger_selections_get_num_objects (
      (ArrangerSelections *) MA_SELECTIONS), ==, 1);
  check_timeline_objects_vs_original_state (1, 1, 0);

  /* undo and check that the objects are deleted */
  undo_manager_undo (UNDO_MANAGER);
  g_assert_cmpint (
    arranger_selections_get_num_objects (
      (ArrangerSelections *) MA_SELECTIONS), ==, 0);
  check_timeline_objects_deleted (1);

  /* redo and check that the objects are there */
  undo_manager_redo (UNDO_MANAGER);
  g_assert_cmpint (
    arranger_selections_get_num_objects (
      (ArrangerSelections *) TL_SELECTIONS), ==,
    TOTAL_TL_SELECTIONS);
  check_timeline_objects_vs_original_state (1, 1, 0);

  test_helper_zrythm_cleanup ();
}

static void
test_delete_timeline ()
{
  rebootstrap_timeline ();

  /* do delete */
  UndoableAction * ua =
    arranger_selections_action_new_delete (
      TL_SELECTIONS);
  undo_manager_perform (UNDO_MANAGER, ua);

  /* check */
  check_timeline_objects_deleted (0);
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
      (ArrangerSelections *) TL_SELECTIONS), ==,
    TOTAL_TL_SELECTIONS);
  check_timeline_objects_vs_original_state (1, 0, 1);

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
  check_timeline_objects_deleted (0);

  /* undo again to prepare for next test */
  undo_manager_undo (UNDO_MANAGER);
  g_assert_cmpint (
    arranger_selections_get_num_objects (
      (ArrangerSelections *) TL_SELECTIONS), ==,
    TOTAL_TL_SELECTIONS);
  check_timeline_objects_vs_original_state (1, 0, 1);

  test_helper_zrythm_cleanup ();
}

static void
test_delete_chords ()
{
  rebootstrap_timeline ();

  ZRegion * r = P_CHORD_TRACK->chord_regions[0];
  g_assert_true (region_validate (r, F_PROJECT));

  /* add another chord */
  ChordObject * c =
    chord_object_new (&r->id, 2, 2);
  chord_region_add_chord_object (
    r, c, F_NO_PUBLISH_EVENTS);
  arranger_selections_add_object (
    (ArrangerSelections *) CHORD_SELECTIONS,
    (ArrangerObject *) c);
  UndoableAction * ua =
    arranger_selections_action_new_create (
      CHORD_SELECTIONS);
  undo_manager_perform (UNDO_MANAGER, ua);

  /* delete the first chord */
  arranger_selections_clear (
    (ArrangerSelections *) CHORD_SELECTIONS,
    F_NO_FREE, F_NO_PUBLISH_EVENTS);
  arranger_selections_add_object (
    (ArrangerSelections *) CHORD_SELECTIONS,
    (ArrangerObject *) r->chord_objects[0]);
  ua =
    arranger_selections_action_new_delete (
      CHORD_SELECTIONS);
  undo_manager_perform (UNDO_MANAGER, ua);
  g_assert_true (region_validate (r, F_PROJECT));

  undo_manager_undo (UNDO_MANAGER);
  undo_manager_redo (UNDO_MANAGER);
  undo_manager_undo (UNDO_MANAGER);
  undo_manager_undo (UNDO_MANAGER);
  undo_manager_redo (UNDO_MANAGER);
  undo_manager_undo (UNDO_MANAGER);

  test_helper_zrythm_cleanup ();
}

static void
test_delete_automation_points ()
{
  rebootstrap_timeline ();

  AutomationTrack * at =
    channel_get_automation_track (
      P_MASTER_TRACK->channel,
      PORT_FLAG_STEREO_BALANCE);
  ZRegion * r = at->regions[0];
  g_assert_true (region_validate (r, F_PROJECT));

  /* add another ap */
  Position pos;
  position_init (&pos);
  AutomationPoint * ap =
    automation_point_new_float (
      AP_VAL1, AP_VAL1, &pos);
  automation_region_add_ap (
    r, ap, F_NO_PUBLISH_EVENTS);
  arranger_selections_add_object (
    (ArrangerSelections *) AUTOMATION_SELECTIONS,
    (ArrangerObject *) ap);
  UndoableAction * ua =
    arranger_selections_action_new_create (
      AUTOMATION_SELECTIONS);
  undo_manager_perform (UNDO_MANAGER, ua);

  /* delete the first chord */
  arranger_selections_clear (
    (ArrangerSelections *) AUTOMATION_SELECTIONS,
    F_NO_FREE, F_NO_PUBLISH_EVENTS);
  arranger_selections_add_object (
    (ArrangerSelections *) AUTOMATION_SELECTIONS,
    (ArrangerObject *) r->aps[0]);
  ua =
    arranger_selections_action_new_delete (
      AUTOMATION_SELECTIONS);
  undo_manager_perform (UNDO_MANAGER, ua);
  g_assert_true (region_validate (r, F_PROJECT));

  undo_manager_undo (UNDO_MANAGER);
  undo_manager_redo (UNDO_MANAGER);
  undo_manager_undo (UNDO_MANAGER);
  undo_manager_undo (UNDO_MANAGER);
  undo_manager_redo (UNDO_MANAGER);
  undo_manager_undo (UNDO_MANAGER);

  test_helper_zrythm_cleanup ();
}

/**
 * Checks the objects after moving.
 *
 * @param new_tracks Whether objects were moved to
 *   new tracks or in the same track.
 */
static void
check_after_move_timeline (
  int new_tracks)
{
  /* check */
  g_assert_cmpint (
    arranger_selections_get_num_objects (
      (ArrangerSelections *) TL_SELECTIONS), ==,
    new_tracks ? 2 : TOTAL_TL_SELECTIONS);

  /* check that undo/redo stacks have the correct
   * counts (1 and 0) */
  check_has_single_undo ();
  g_assert_cmpint (
    UNDO_MANAGER->undo_stack->as_actions[0]->
      delta_tracks, ==, new_tracks ? 2 : 0);

  /* get tracks */
  Track * midi_track_before =
    tracklist_find_track_by_name (
      TRACKLIST, MIDI_TRACK_NAME);
  g_assert_nonnull (midi_track_before);
  g_assert_cmpint (midi_track_before->pos, >, 0);
  Track * audio_track_before =
    tracklist_find_track_by_name (
      TRACKLIST, AUDIO_TRACK_NAME);
  g_assert_nonnull (audio_track_before);
  g_assert_cmpint (audio_track_before->pos, >, 0);
  Track * midi_track_after =
    tracklist_find_track_by_name (
      TRACKLIST, TARGET_MIDI_TRACK_NAME);
  g_assert_nonnull (midi_track_after);
  g_assert_cmpint (
    midi_track_after->pos, >,
    midi_track_before->pos);
  Track * audio_track_after =
    tracklist_find_track_by_name (
      TRACKLIST, TARGET_AUDIO_TRACK_NAME);
  g_assert_nonnull (audio_track_after);
  g_assert_cmpint (
    audio_track_after->pos, >,
    audio_track_before->pos);

  /* check midi region */
  ArrangerObject * obj;
  if (new_tracks)
    {
      obj =
        (ArrangerObject *)
        midi_track_after->
          lanes[MIDI_REGION_LANE]->regions[0];
    }
  else
    {
      obj =
        (ArrangerObject *)
        midi_track_before->
          lanes[MIDI_REGION_LANE]->regions[0];
    }
  ZRegion * r = (ZRegion *) obj;
  g_assert_true (IS_REGION (obj));
  g_assert_cmpint (
    r->id.type, ==, REGION_TYPE_MIDI);
  Position p1_after_move, p2_after_move;
  p1_after_move = p1;
  p2_after_move = p2;
  position_add_ticks (&p1_after_move, MOVE_TICKS);
  position_add_ticks (&p2_after_move, MOVE_TICKS);
  g_assert_cmppos (&obj->pos, &p1_after_move);
  g_assert_cmppos (&obj->end_pos, &p2_after_move);
  if (new_tracks)
    {
      Track * tmp =
        arranger_object_get_track (obj);
      g_assert_true (tmp == midi_track_after);
      g_assert_cmpint (
        r->id.track_pos, ==, midi_track_after->pos);
    }
  else
    {
      Track * tmp =
        arranger_object_get_track (obj);
      g_assert_true (tmp == midi_track_before);
      g_assert_cmpint (
        r->id.track_pos, ==, midi_track_before->pos);
    }
  g_assert_cmpint (r->num_midi_notes, ==, 1);
  MidiNote * mn =
    r->midi_notes[0];
  obj =
    (ArrangerObject *) mn;
  g_assert_cmpuint (mn->val, ==, MN_VAL);
  g_assert_cmpuint (mn->vel->vel, ==, MN_VEL);
  g_assert_cmppos (&obj->pos, &p1);
  g_assert_cmppos (&obj->end_pos, &p2);
  g_assert_true (
    region_identifier_is_equal (
      &obj->region_id, &r->id));

  /* check audio region */
  obj =
    (ArrangerObject *)
    TL_SELECTIONS->regions[new_tracks ? 1 : 3];
  p1_after_move = p1;
  position_add_ticks (&p1_after_move, MOVE_TICKS);
  g_assert_cmppos (&obj->pos, &p1_after_move);
  r = (ZRegion *) obj;
  g_assert_true (IS_REGION (obj));
  g_assert_cmpint (
    r->id.type, ==, REGION_TYPE_AUDIO);
  if (new_tracks)
    {
      Track * tmp =
        arranger_object_get_track (obj);
      g_assert_true (tmp == audio_track_after);
      g_assert_cmpint (
        r->id.track_pos, ==,
        audio_track_after->pos);
    }
  else
    {
      Track * tmp =
        arranger_object_get_track (obj);
      g_assert_true (tmp == audio_track_before);
      g_assert_cmpint (
        r->id.track_pos, ==,
        audio_track_before->pos);
    }

  if (!new_tracks)
    {
      /* check automation region */
      obj =
        (ArrangerObject *)
        TL_SELECTIONS->regions[1];
      g_assert_cmppos (&obj->pos, &p1_after_move);
      g_assert_cmppos (
        &obj->end_pos, &p2_after_move);
      r =
        (ZRegion *) obj;
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
        (ArrangerObject *)
        TL_SELECTIONS->markers[0];
      Marker * m = (Marker *) obj;
      g_assert_cmppos (&obj->pos, &p1_after_move);
      g_assert_cmpstr (m->name, ==, MARKER_NAME);

      /* check scale object */
      g_assert_cmpint (
        TL_SELECTIONS->num_scale_objects, ==, 1);
      obj =
        (ArrangerObject *)
        TL_SELECTIONS->scale_objects[0];
      ScaleObject * s = (ScaleObject *) obj;
      g_assert_cmppos (&obj->pos, &p1_after_move);
      g_assert_cmpint (
        s->scale->type, ==, MUSICAL_SCALE_TYPE);
      g_assert_cmpint (
        s->scale->root_key, ==,
        MUSICAL_SCALE_ROOT);
    }
}

/**
 * Tests the move action.
 *
 * @param new_tracks Whether to move objects to new
 *   tracks or in the same track.
 */
static void
test_move_timeline ()
{
  rebootstrap_timeline ();

  /* when i == 1 we are moving to new tracks */
  for (int i = 0; i < 2; i++)
    {
      int track_diff =  i ? 2 : 0;
      if (track_diff)
        {
          select_audio_and_midi_regions_only ();
        }

      /* check undo/redo stacks */
      g_assert_cmpint (
        UNDO_MANAGER->undo_stack->stack->top, ==, -1);
      g_assert_cmpint (
        UNDO_MANAGER->redo_stack->stack->top, ==,
        i ? 0 : -1);
      g_assert_cmpuint (
        UNDO_MANAGER->undo_stack->num_as_actions,
          ==, 0);
      g_assert_cmpuint (
        UNDO_MANAGER->redo_stack->num_as_actions,
          ==, i ? 1 : 0);

      /* do move ticks */
      UndoableAction * ua =
        arranger_selections_action_new_move_timeline (
          TL_SELECTIONS, MOVE_TICKS, track_diff, 0,
          F_NOT_ALREADY_MOVED);
      undo_manager_perform (UNDO_MANAGER, ua);

      /* check */
      check_after_move_timeline (i);

      /* undo and check that the objects are at
       * their original state*/
      undo_manager_undo (UNDO_MANAGER);
      g_assert_cmpint (
        arranger_selections_get_num_objects (
          (ArrangerSelections *) TL_SELECTIONS), ==,
        i ? 2 : TOTAL_TL_SELECTIONS);

      check_timeline_objects_vs_original_state (
        i ? 0 : 1, 0, 1);

      /* redo and check that the objects are moved
       * again */
      undo_manager_redo (UNDO_MANAGER);
      check_after_move_timeline (i);

      /* undo again to prepare for next test */
      undo_manager_undo (UNDO_MANAGER);
      if (track_diff)
        {
          g_assert_cmpint (
            arranger_selections_get_num_objects (
              (ArrangerSelections *)
              /* 2 regions */
              TL_SELECTIONS), ==, 2);
        }
    }

  test_helper_zrythm_cleanup ();
}

/**
 * @param new_tracks Whether midi/audio regions
 *   were moved to new tracks.
 * @param link Whether this is a link action.
 */
static void
check_after_duplicate_timeline (
  int  new_tracks,
  bool link)
{
  /* check */
  g_assert_cmpint (
    arranger_selections_get_num_objects (
      (ArrangerSelections *) TL_SELECTIONS), ==,
    new_tracks ? 2 : TOTAL_TL_SELECTIONS);

  Track * midi_track =
    tracklist_find_track_by_name (
      TRACKLIST, MIDI_TRACK_NAME);
  g_assert_nonnull (midi_track);
  Track * audio_track =
    tracklist_find_track_by_name (
      TRACKLIST, AUDIO_TRACK_NAME);
  g_assert_nonnull (audio_track);
  Track * new_midi_track =
    tracklist_find_track_by_name (
      TRACKLIST, TARGET_MIDI_TRACK_NAME);
  g_assert_nonnull (midi_track);
  Track * new_audio_track =
    tracklist_find_track_by_name (
      TRACKLIST, TARGET_AUDIO_TRACK_NAME);
  g_assert_nonnull (audio_track);

  /* check prev midi region */
  ArrangerObject * obj;
  if (new_tracks)
    {
      g_assert_cmpint (
        midi_track->lanes[MIDI_REGION_LANE]->
          num_regions, ==, 1);
      g_assert_cmpint (
        new_midi_track->lanes[MIDI_REGION_LANE]->
          num_regions, ==, 1);
    }
  else
    {
      g_assert_cmpint (
        midi_track->lanes[MIDI_REGION_LANE]->
          num_regions, ==, 2);
    }
  obj =
    (ArrangerObject *)
    midi_track->lanes[MIDI_REGION_LANE]->
      regions[0];
  Position p1_before_move, p2_before_move;
  p1_before_move = p1;
  p2_before_move = p2;
  g_assert_cmppos (&obj->pos, &p1_before_move);
  g_assert_cmppos (&obj->end_pos, &p2_before_move);
  ZRegion * r = (ZRegion *) obj;
  g_assert_cmpint (
    r->id.track_pos, ==, midi_track->pos);
  g_assert_cmpint (
    r->id.lane_pos, ==, MIDI_REGION_LANE);
  g_assert_cmpint (r->id.idx, ==, 0);
  g_assert_cmpint (r->num_midi_notes, ==, 1);
  MidiNote * mn = r->midi_notes[0];
  obj = (ArrangerObject *) mn;
  g_assert_true (
    region_identifier_is_equal (
      &obj->region_id, &r->id));
  g_assert_cmpuint (mn->val, ==, MN_VAL);
  g_assert_cmpuint (mn->vel->vel, ==, MN_VEL);
  g_assert_cmppos (&obj->pos, &p1);
  g_assert_cmppos (&obj->end_pos, &p2);
  int link_group = r->id.link_group;
  if (link)
    {
      g_assert_cmpint (
        r->id.link_group, >, -1);
    }

  /* check new midi region */
  if (new_tracks)
    {
      obj =
        (ArrangerObject *)
        new_midi_track->lanes[MIDI_REGION_LANE]->
          regions[0];
    }
  else
    {
      obj =
        (ArrangerObject *)
        midi_track->lanes[MIDI_REGION_LANE]->
          regions[1];
    }
  Position p1_after_move, p2_after_move;
  p1_after_move = p1;
  p2_after_move = p2;
  position_add_ticks (
    &p1_after_move, MOVE_TICKS);
  position_add_ticks (
    &p2_after_move, MOVE_TICKS);
  g_assert_cmppos (&obj->pos, &p1_after_move);
  g_assert_cmppos (&obj->end_pos, &p2_after_move);
  r = (ZRegion *) obj;
  if (new_tracks)
    {
      g_assert_cmpint (
        r->id.track_pos, ==, new_midi_track->pos);
      g_assert_cmpint (r->id.idx, ==, 0);
    }
  else
    {
      g_assert_cmpint (
        r->id.track_pos, ==, midi_track->pos);
      g_assert_cmpint (r->id.idx, ==, 1);
    }
  g_assert_cmpint (
    r->id.lane_pos, ==, MIDI_REGION_LANE);
  g_assert_cmpint (r->num_midi_notes, ==, 1);
  if (new_tracks)
    {
      g_assert_cmpint (
        r->id.track_pos, ==, new_midi_track->pos);
    }
  else
    {
      g_assert_cmpint (
        r->id.track_pos, ==, midi_track->pos);
    }
  g_assert_cmpint (
    r->id.lane_pos, ==, MIDI_REGION_LANE);
  mn = r->midi_notes[0];
  obj = (ArrangerObject *) mn;
  g_assert_true (
    region_identifier_is_equal (
      &obj->region_id, &r->id));
  g_assert_cmpuint (mn->val, ==, MN_VAL);
  g_assert_cmpuint (mn->vel->vel, ==, MN_VEL);
  g_assert_cmppos (&obj->pos, &p1);
  g_assert_cmppos (&obj->end_pos, &p2);
  if (link)
    {
      g_assert_cmpint (
        r->id.link_group, ==, link_group);
    }

  /* check prev audio region */
  if (new_tracks)
    {
      g_assert_cmpint (
        audio_track->lanes[AUDIO_REGION_LANE]->
          num_regions, ==, 1);
      g_assert_cmpint (
        new_audio_track->lanes[AUDIO_REGION_LANE]->
          num_regions, ==, 1);
    }
  else
    {
      g_assert_cmpint (
        audio_track->lanes[AUDIO_REGION_LANE]->
          num_regions, ==, 2);
    }
  obj =
    (ArrangerObject *)
    audio_track->lanes[AUDIO_REGION_LANE]->
    regions[0];
  g_assert_cmppos (&obj->pos, &p1_before_move);
  r = (ZRegion *) obj;
  g_assert_cmpint (
    r->id.track_pos, ==, audio_track->pos);
  g_assert_cmpint (r->id.idx, ==, 0);
  g_assert_cmpint (
    r->id.lane_pos, ==, AUDIO_REGION_LANE);
  link_group = r->id.link_group;
  if (link)
    {
      g_assert_cmpint (
        r->id.link_group, >, -1);
    }

  /* check new audio region */
  if (new_tracks)
    {
      obj =
        (ArrangerObject *)
        new_audio_track->lanes[AUDIO_REGION_LANE]->
        regions[0];
    }
  else
    {
      obj =
        (ArrangerObject *)
        audio_track->lanes[AUDIO_REGION_LANE]->
        regions[1];
    }
  g_assert_cmppos (&obj->pos, &p1_after_move);
  r = (ZRegion *) obj;
  g_assert_cmpint (
    r->id.lane_pos, ==, AUDIO_REGION_LANE);
  if (new_tracks)
    {
      g_assert_cmpint (
        r->id.track_pos, ==, new_audio_track->pos);
    }
  else
    {
      g_assert_cmpint (
        r->id.track_pos, ==, audio_track->pos);
    }
  if (link)
    {
      g_assert_cmpint (
        r->id.link_group, ==, link_group);
    }

  if (!new_tracks)
    {
      /* check automation region */
      AutomationTracklist * atl =
        track_get_automation_tracklist (
          P_MASTER_TRACK);
      g_assert_nonnull (atl);
      AutomationTrack * at =
        channel_get_automation_track (
          P_MASTER_TRACK->channel,
          PORT_FLAG_STEREO_BALANCE);
      g_assert_nonnull (at);
      g_assert_cmpint (at->num_regions, ==, 2);
      obj =
        (ArrangerObject *) at->regions[0];
      g_assert_cmppos (
        &obj->pos, &p1_before_move);
      g_assert_cmppos (
        &obj->end_pos, &p2_before_move);
      r =
        (ZRegion *) obj;
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
      obj =
        (ArrangerObject *) at->regions[1];
      g_assert_cmppos (
        &obj->pos, &p1_after_move);
      g_assert_cmppos (
        &obj->end_pos, &p2_after_move);
      r =
        (ZRegion *) obj;
      g_assert_cmpint (r->num_aps, ==, 2);
      ap = r->aps[0];
      obj = (ArrangerObject *) ap;
      g_assert_cmppos (&obj->pos, &p1);
      g_assert_cmpfloat_with_epsilon (
        ap->fvalue, AP_VAL1, 0.000001f);
      ap = r->aps[1];
      obj = (ArrangerObject *) ap;
      g_assert_cmppos (&obj->pos, &p2);
      g_assert_cmpfloat_with_epsilon (
        ap->fvalue, AP_VAL2, 0.000001f);

      /* check marker */
      g_assert_cmpint (
        P_MARKER_TRACK->num_markers, ==, 4);
      obj =
        (ArrangerObject *) P_MARKER_TRACK->markers[2];
      Marker * m = (Marker *) obj;
      g_assert_cmppos (&obj->pos, &p1_before_move);
      g_assert_cmpstr (m->name, ==, MARKER_NAME);
      obj =
        (ArrangerObject *) P_MARKER_TRACK->markers[3];
      m = (Marker *) obj;
      g_assert_cmppos (&obj->pos, &p1_after_move);
      g_assert_cmpstr (m->name, ==, MARKER_NAME);

      /* check scale object */
      g_assert_cmpint (
        P_CHORD_TRACK->num_scales, ==, 2);
      obj =
        (ArrangerObject *)
        P_CHORD_TRACK->scales[0];
      ScaleObject * s = (ScaleObject *) obj;
      g_assert_cmppos (&obj->pos, &p1_before_move);
      g_assert_cmpint (
        s->scale->type, ==, MUSICAL_SCALE_TYPE);
      g_assert_cmpint (
        s->scale->root_key, ==, MUSICAL_SCALE_ROOT);
      obj =
        (ArrangerObject *)
        P_CHORD_TRACK->scales[1];
      s = (ScaleObject *) obj;
      g_assert_cmppos (&obj->pos, &p1_after_move);
      g_assert_cmpint (
        s->scale->type, ==, MUSICAL_SCALE_TYPE);
      g_assert_cmpint (
        s->scale->root_key, ==, MUSICAL_SCALE_ROOT);
    }
}

static void
test_duplicate_timeline ()
{
  rebootstrap_timeline ();

  /* when i == 1 we are moving to new tracks */
  for (int i = 0; i < 2; i++)
    {
      int track_diff =  i ? 2 : 0;
      if (track_diff)
        {
          select_audio_and_midi_regions_only ();
          Track * midi_track =
            tracklist_find_track_by_name (
              TRACKLIST, MIDI_TRACK_NAME);
          Track * audio_track =
            tracklist_find_track_by_name (
              TRACKLIST, AUDIO_TRACK_NAME);
          Track * new_midi_track =
            tracklist_find_track_by_name (
              TRACKLIST, TARGET_MIDI_TRACK_NAME);
          Track * new_audio_track =
            tracklist_find_track_by_name (
              TRACKLIST, TARGET_AUDIO_TRACK_NAME);
          region_move_to_track (
            midi_track->lanes[MIDI_REGION_LANE]->
              regions[0],
            new_midi_track, -1);
          region_move_to_track (
            audio_track->lanes[AUDIO_REGION_LANE]->
              regions[0],
            new_audio_track, -1);
        }
      /* do move ticks */
      arranger_selections_add_ticks (
        (ArrangerSelections *) TL_SELECTIONS,
        MOVE_TICKS);

      /* do duplicate */
      UndoableAction * ua =
        arranger_selections_action_new_duplicate_timeline (
          TL_SELECTIONS, MOVE_TICKS,
          i > 0 ? 2 : 0, 0, F_ALREADY_MOVED);
      undo_manager_perform (UNDO_MANAGER, ua);

      /* check */
      check_after_duplicate_timeline (i, false);

      /* undo and check that the objects are at
       * their original state*/
      undo_manager_undo (UNDO_MANAGER);
      check_timeline_objects_vs_original_state (
        0, 0, 1);

      /* redo and check that the objects are moved
       * again */
      undo_manager_redo (UNDO_MANAGER);
      check_after_duplicate_timeline (i, false);

      /* undo again to prepare for next test */
      undo_manager_undo (UNDO_MANAGER);
      check_timeline_objects_vs_original_state (
        0, 0, 1);
    }

  test_helper_zrythm_cleanup ();
}

static void
test_duplicate_automation_region ()
{
  rebootstrap_timeline ();

  AutomationTracklist * atl =
    track_get_automation_tracklist (P_MASTER_TRACK);
  g_assert_nonnull (atl);
  AutomationTrack * at =
    channel_get_automation_track (
      P_MASTER_TRACK->channel,
      PORT_FLAG_STEREO_BALANCE);
  g_assert_nonnull (at);
  g_assert_cmpint (at->num_regions, ==, 1);

  Position start_pos, end_pos;
  position_init (&start_pos);
  position_set_to_bar (&end_pos, 4);
  ZRegion * r1 = at->regions[0];

  AutomationPoint * ap =
    automation_point_new_float (
      0.5f, 0.5f, &start_pos);
  automation_region_add_ap (
    r1, ap, F_NO_PUBLISH_EVENTS);
  arranger_object_select (
    (ArrangerObject *) ap, F_SELECT,
    F_NO_APPEND, F_NO_PUBLISH_EVENTS);
  UndoableAction * ua =
    arranger_selections_action_new_create (
      AUTOMATION_SELECTIONS);
  undo_manager_perform (UNDO_MANAGER, ua);
  position_add_frames (&start_pos, 14);
  ap =
    automation_point_new_float (
      0.6f, 0.6f, &start_pos);
  automation_region_add_ap (
    r1, ap, F_NO_PUBLISH_EVENTS);
  arranger_object_select (
    (ArrangerObject *) ap, F_SELECT,
    F_NO_APPEND, F_NO_PUBLISH_EVENTS);
  ua =
    arranger_selections_action_new_create (
      AUTOMATION_SELECTIONS);
  undo_manager_perform (UNDO_MANAGER, ua);

  float curviness_after = 0.8f;
  ap = r1->aps[0];
  arranger_object_select (
    (ArrangerObject *) ap, F_SELECT,
    F_NO_APPEND, F_NO_PUBLISH_EVENTS);
  ArrangerSelections * before =
    arranger_selections_clone (
      (ArrangerSelections *) AUTOMATION_SELECTIONS);
  ap->curve_opts.curviness = curviness_after;
  ua =
    arranger_selections_action_new_edit (
      before,
      (ArrangerSelections *) AUTOMATION_SELECTIONS,
      ARRANGER_SELECTIONS_ACTION_EDIT_PRIMITIVE,
      F_ALREADY_EDITED);
  undo_manager_perform (UNDO_MANAGER, ua);

  ap = r1->aps[0];
  g_assert_cmpfloat_with_epsilon (
    ap->curve_opts.curviness, curviness_after,
    0.00001f);

  undo_manager_undo (UNDO_MANAGER);
  ap = r1->aps[0];
  g_assert_cmpfloat_with_epsilon (
    ap->curve_opts.curviness, 0.f, 0.00001f);

  undo_manager_redo (UNDO_MANAGER);
  ap = r1->aps[0];
  g_assert_cmpfloat_with_epsilon (
    ap->curve_opts.curviness, curviness_after,
    0.00001f);

  arranger_object_select (
    (ArrangerObject *) r1, F_SELECT,
    F_NO_APPEND, F_NO_PUBLISH_EVENTS);

  ua =
    arranger_selections_action_new_duplicate_timeline (
      TL_SELECTIONS, MOVE_TICKS, 0, 0,
      F_NOT_ALREADY_MOVED);
  undo_manager_perform (UNDO_MANAGER, ua);

  r1 = at->regions[1];
  ap = r1->aps[0];
  g_assert_cmpfloat_with_epsilon (
    ap->curve_opts.curviness, curviness_after,
    0.00001f);

  undo_manager_undo (UNDO_MANAGER);
  undo_manager_redo (UNDO_MANAGER);

  r1 = at->regions[1];
  ap = r1->aps[0];
  g_assert_cmpfloat_with_epsilon (
    ap->curve_opts.curviness, curviness_after,
    0.00001f);

  test_helper_zrythm_cleanup ();
}

static void
test_link_timeline ()
{
  rebootstrap_timeline ();

  /* when i == 1 we are moving to new tracks */
  for (int i = 0; i < 2; i++)
    {
      ArrangerSelections * sel_before = NULL;

      int track_diff =  i ? 2 : 0;
      if (track_diff)
        {
          select_audio_and_midi_regions_only ();
          sel_before =
            arranger_selections_clone (
              (ArrangerSelections *) TL_SELECTIONS);

          Track * midi_track =
            tracklist_find_track_by_name (
              TRACKLIST, MIDI_TRACK_NAME);
          Track * audio_track =
            tracklist_find_track_by_name (
              TRACKLIST, AUDIO_TRACK_NAME);
          Track * new_midi_track =
            tracklist_find_track_by_name (
              TRACKLIST, TARGET_MIDI_TRACK_NAME);
          Track * new_audio_track =
            tracklist_find_track_by_name (
              TRACKLIST, TARGET_AUDIO_TRACK_NAME);
          region_move_to_track (
            midi_track->lanes[MIDI_REGION_LANE]->
              regions[0],
            new_midi_track, -1);
          region_move_to_track (
            audio_track->lanes[AUDIO_REGION_LANE]->
              regions[0],
            new_audio_track, -1);
        }
      else
        {
          sel_before =
            arranger_selections_clone (
              (ArrangerSelections *) TL_SELECTIONS);
        }

      /* do move ticks */
      arranger_selections_add_ticks (
        (ArrangerSelections *) TL_SELECTIONS,
        MOVE_TICKS);

      /* do duplicate */
      UndoableAction * ua =
        arranger_selections_action_new_link (
          sel_before,
          (ArrangerSelections *)TL_SELECTIONS,
          MOVE_TICKS,
          i > 0 ? 2 : 0, 0, F_ALREADY_MOVED);
      undo_manager_perform (UNDO_MANAGER, ua);

      /* check */
      check_after_duplicate_timeline (i, true);

      /* undo and check that the objects are at
       * their original state*/
      undo_manager_undo (UNDO_MANAGER);
      check_timeline_objects_vs_original_state (
        0, 0, 1);

      /* redo and check that the objects are moved
       * again */
      undo_manager_redo (UNDO_MANAGER);
      check_after_duplicate_timeline (i, true);

      /* undo again to prepare for next test */
      undo_manager_undo (UNDO_MANAGER);
      check_timeline_objects_vs_original_state (
        0, 0, 1);
    }

  test_helper_zrythm_cleanup ();
}

static void
test_edit_marker ()
{
  rebootstrap_timeline ();

  /* create marker with name "aa" */
  Marker * marker = marker_new ("aa");
  ArrangerObject * marker_obj =
    (ArrangerObject *) marker;
  arranger_object_select (
    marker_obj, F_SELECT, F_NO_APPEND, F_NO_PUBLISH_EVENTS);
  arranger_object_add_to_project (
    marker_obj, F_NO_PUBLISH_EVENTS);
  UndoableAction * ua =
    arranger_selections_action_new_create (
      TL_SELECTIONS);
  undo_manager_perform (UNDO_MANAGER, ua);

  /* change name */
  ArrangerSelections * clone_sel =
    arranger_selections_clone (
      (ArrangerSelections *) TL_SELECTIONS);
  marker_set_name (
    ((TimelineSelections *) clone_sel)->markers[0],
    "bb");
  ua =
    arranger_selections_action_new_edit (
      (ArrangerSelections *) TL_SELECTIONS,
      clone_sel,
      ARRANGER_SELECTIONS_ACTION_EDIT_NAME,
      F_NOT_ALREADY_EDITED);
  undo_manager_perform (UNDO_MANAGER, ua);

  /* assert name changed */
  g_assert_true (
    string_is_equal (marker->name, "bb"));

  undo_manager_undo (UNDO_MANAGER);
  g_assert_true (
    string_is_equal (marker->name, "aa"));

  /* undo again and check that all objects are at
   * original state */
  undo_manager_undo (UNDO_MANAGER);

  /* redo and check that the name is changed */
  undo_manager_redo (UNDO_MANAGER);
  undo_manager_redo (UNDO_MANAGER);

  /* return to original state */
  undo_manager_undo (UNDO_MANAGER);
  undo_manager_undo (UNDO_MANAGER);

  test_helper_zrythm_cleanup ();
}

static void
test_mute ()
{
  rebootstrap_timeline ();

  Track * midi_track = TRACKLIST->tracks[5];
  g_assert_true (
    midi_track->type == TRACK_TYPE_MIDI);

  ZRegion * r =
    midi_track->lanes[MIDI_REGION_LANE]->regions[0];
  ArrangerObject * obj = (ArrangerObject *) r;
  g_assert_true (IS_ARRANGER_OBJECT (obj));

  UndoableAction * ua =
    arranger_selections_action_new_edit (
      (ArrangerSelections *) TL_SELECTIONS, NULL,
      ARRANGER_SELECTIONS_ACTION_EDIT_MUTE,
      F_NOT_ALREADY_EDITED);
  undo_manager_perform (UNDO_MANAGER, ua);

  /* assert muted */
  g_assert_true (IS_ARRANGER_OBJECT (obj));
  g_assert_true (obj->muted);

  undo_manager_undo (UNDO_MANAGER);
  g_assert_true (IS_ARRANGER_OBJECT (obj));
  g_assert_false (obj->muted);

  /* redo and recheck */
  undo_manager_redo (UNDO_MANAGER);
  g_assert_true (IS_ARRANGER_OBJECT (obj));
  g_assert_true (obj->muted);

  /* return to original state */
  undo_manager_undo (UNDO_MANAGER);
  g_assert_true (IS_ARRANGER_OBJECT (obj));
  g_assert_false (obj->muted);

  test_helper_zrythm_cleanup ();
}

static void
test_split ()
{
  rebootstrap_timeline ();

  g_assert_cmpint (
    P_CHORD_TRACK->num_chord_regions, ==, 1);

  Position pos, end_pos;
  position_set_to_bar (&pos, 2);
  position_set_to_bar (&end_pos, 4);
  ZRegion * r =
    chord_region_new (&pos, &end_pos, 0);
  ArrangerObject * r_obj = (ArrangerObject *) r;
  track_add_region (
    P_CHORD_TRACK, r, NULL, -1, F_GEN_NAME,
    F_NO_PUBLISH_EVENTS);
  arranger_object_select (
    r_obj, F_SELECT, F_NO_APPEND,
    F_NO_PUBLISH_EVENTS);

  UndoableAction * ua =
    arranger_selections_action_new_create (
      TL_SELECTIONS);
  undo_manager_perform (UNDO_MANAGER, ua);

  g_assert_cmpint (
    P_CHORD_TRACK->num_chord_regions, ==, 2);

  position_set_to_bar (&pos, 3);
  ua =
    arranger_selections_action_new_split (
      (ArrangerSelections *) TL_SELECTIONS, &pos);
  undo_manager_perform (UNDO_MANAGER, ua);

  g_assert_cmpint (
    P_CHORD_TRACK->num_chord_regions, ==, 3);

  Track * track2 =
    tracklist_find_track_by_name (
      TRACKLIST, AUDIO_TRACK_NAME);
  TrackLane * lane2 = track2->lanes[3];
  g_assert_cmpint (
    lane2->num_regions, ==, 1);

  ZRegion * region2 = lane2->regions[0];
  region_validate (region2, false);

  undo_manager_undo (UNDO_MANAGER);
  undo_manager_redo (UNDO_MANAGER);
  undo_manager_undo (UNDO_MANAGER);
  undo_manager_undo (UNDO_MANAGER);

  /* --- test audio region split --- */

  Track * track =
    tracklist_find_track_by_name (
      TRACKLIST, AUDIO_TRACK_NAME);
  TrackLane * lane = track->lanes[3];
  g_assert_cmpint (
    lane->num_regions, ==, 1);

  ZRegion * region = lane->regions[0];
  region_validate (region, false);
  r_obj = (ArrangerObject *) region;
  arranger_object_select (
    r_obj, F_SELECT, F_NO_APPEND,
    F_NO_PUBLISH_EVENTS);
  AudioClip * clip =
    audio_region_get_clip (region);
  float first_frame = clip->frames[0];
  g_assert_true (clip->frames[0] > 0.000001f);

  arranger_object_print (r_obj);

  position_set_to_bar (&pos, 2);
  position_print (&pos);
  position_add_beats (&pos, 1);
  position_print (&pos);
  ua =
    arranger_selections_action_new_split (
      (ArrangerSelections *) TL_SELECTIONS, &pos);
  undo_manager_perform (UNDO_MANAGER, ua);

  test_project_save_and_reload ();

  /* check that clip frames are the same as
   * before */
  track =
    tracklist_find_track_by_name (
      TRACKLIST, AUDIO_TRACK_NAME);
  lane = track->lanes[3];
  region = lane->regions[0];
  r_obj = (ArrangerObject *) region;
  clip = audio_region_get_clip (region);
  g_assert_cmpfloat_with_epsilon (
    first_frame, clip->frames[0], 0.000001f);

  undo_manager_undo (UNDO_MANAGER);

  test_helper_zrythm_cleanup ();
}

static void
test_quantize ()
{
  rebootstrap_timeline ();

  Track * audio_track =
    tracklist_find_track_by_name (
      TRACKLIST, AUDIO_TRACK_NAME);
  g_assert_true (
    audio_track->type == TRACK_TYPE_AUDIO);

  UndoableAction * ua =
    arranger_selections_action_new_quantize (
      (ArrangerSelections *) AUDIO_SELECTIONS,
      QUANTIZE_OPTIONS_EDITOR);
  undo_manager_perform (
    UNDO_MANAGER, ua);

  /* TODO test audio/MIDI quantization */
  TrackLane * lane = audio_track->lanes[3];
  ZRegion * region = lane->regions[0];
  g_assert_true (IS_ARRANGER_OBJECT (region));

  /* return to original state */
  undo_manager_undo (UNDO_MANAGER);

  test_helper_zrythm_cleanup ();
}

static void
verify_audio_function (
  float * frames,
  size_t  max_frames)
{
  Track * track =
    tracklist_find_track_by_name (
      TRACKLIST, AUDIO_TRACK_NAME);
  TrackLane * lane = track->lanes[3];
  ZRegion * region = lane->regions[0];
  AudioClip * clip = audio_region_get_clip (region);
  size_t num_frames =
    MIN (max_frames, (size_t) clip->num_frames);
  for (size_t i = 0; i < num_frames; i++)
    {
      for (size_t j = 0;
           j < clip->channels; j++)
        {
          g_assert_cmpfloat_with_epsilon (
            frames[clip->channels * i + j],
            clip->frames[clip->channels * i + j],
            0.0001f);
        }
    }
}

static void
test_audio_functions ()
{
  rebootstrap_timeline ();

  Track * track =
    tracklist_find_track_by_name (
      TRACKLIST, AUDIO_TRACK_NAME);
  TrackLane * lane = track->lanes[3];
  g_assert_cmpint (
    lane->num_regions, ==, 1);

  ZRegion * region = lane->regions[0];
  ArrangerObject * r_obj =
    (ArrangerObject *) region;
  arranger_object_select (
    r_obj, F_SELECT, F_NO_APPEND, F_NO_PUBLISH_EVENTS);
  AUDIO_SELECTIONS->region_id = region->id;
  AUDIO_SELECTIONS->has_selection = true;
  AUDIO_SELECTIONS->sel_start = r_obj->pos;
  AUDIO_SELECTIONS->sel_end = r_obj->end_pos;

  AudioClip * orig_clip =
    audio_region_get_clip (region);
  size_t channels = orig_clip->channels;
  size_t frames_per_channel =
    (size_t) orig_clip->num_frames;
  size_t total_frames =
    (size_t) orig_clip->num_frames * channels;
  float * orig_frames =
    object_new_n (total_frames, float);
  float * inverted_frames =
    object_new_n (total_frames, float);
  dsp_copy (
    orig_frames, orig_clip->frames,
    total_frames);
  dsp_copy (
    inverted_frames, orig_clip->frames,
    total_frames);
  dsp_mul_k2 (
    inverted_frames, -1.f, total_frames);

  verify_audio_function (
    orig_frames, frames_per_channel);

  /* invert */
  UndoableAction * ua =
    arranger_selections_action_new_edit_audio_function (
      (ArrangerSelections *) AUDIO_SELECTIONS,
      AUDIO_FUNCTION_INVERT);
  undo_manager_perform (UNDO_MANAGER, ua);

  verify_audio_function (
    inverted_frames, frames_per_channel);

  test_project_save_and_reload ();

  undo_manager_undo (UNDO_MANAGER);

  verify_audio_function (
    orig_frames, frames_per_channel);

  undo_manager_redo (UNDO_MANAGER);

  /* verify that frames are edited again */
  verify_audio_function (
    inverted_frames, frames_per_channel);

  undo_manager_undo (UNDO_MANAGER);

  free (orig_frames);
  free (inverted_frames);

  test_helper_zrythm_cleanup ();
}

static void
test_automation_fill ()
{
  rebootstrap_timeline ();

  /* check automation region */
  AutomationTracklist * atl =
    track_get_automation_tracklist (P_MASTER_TRACK);
  g_assert_nonnull (atl);
  AutomationTrack * at =
    channel_get_automation_track (
      P_MASTER_TRACK->channel,
      PORT_FLAG_STEREO_BALANCE);
  g_assert_nonnull (at);
  g_assert_cmpint (at->num_regions, ==, 1);

  Position start_pos, end_pos;
  position_init (&start_pos);
  position_set_to_bar (&end_pos, 4);
  ZRegion * r1 = at->regions[0];

  ZRegion * r1_clone =
    (ZRegion *)
    arranger_object_clone (
      (ArrangerObject *) r1,
      ARRANGER_OBJECT_CLONE_COPY_MAIN);

  AutomationPoint * ap =
    automation_point_new_float (
      0.5f, 0.5f, &start_pos);
  automation_region_add_ap (
    r1, ap, F_NO_PUBLISH_EVENTS);
  arranger_object_select (
    (ArrangerObject *) ap, F_SELECT,
    F_NO_APPEND, F_NO_PUBLISH_EVENTS);
  position_add_frames (&start_pos, 14);
  ap =
    automation_point_new_float (
      0.6f, 0.6f, &start_pos);
  automation_region_add_ap (
    r1, ap, F_NO_PUBLISH_EVENTS);
  arranger_object_select (
    (ArrangerObject *) ap, F_SELECT,
    F_APPEND, F_NO_PUBLISH_EVENTS);
  UndoableAction * ua =
    arranger_selections_action_new_automation_fill (
      r1_clone, r1, F_ALREADY_EDITED);
  undo_manager_perform (UNDO_MANAGER, ua);

  undo_manager_undo (UNDO_MANAGER);
  undo_manager_redo (UNDO_MANAGER);

  test_helper_zrythm_cleanup ();
}

static void
test_duplicate_midi_regions_to_track_below ()
{
  rebootstrap_timeline ();

  Track * midi_track =
    tracklist_find_track_by_name (
      TRACKLIST, MIDI_TRACK_NAME);
  TrackLane * lane =
    midi_track->lanes[0];
  g_assert_cmpint (lane->num_regions, ==, 0);

  Position pos, end_pos;
  position_set_to_bar (&pos, 2);
  position_set_to_bar (&end_pos, 4);
  ZRegion * r1 =
    midi_region_new (
      &pos, &end_pos, midi_track->pos, 0,
      lane->num_regions);
  track_add_region (
    midi_track, r1, NULL, lane->pos,
    F_GEN_NAME, F_NO_PUBLISH_EVENTS);
  arranger_object_select (
    (ArrangerObject *) r1, F_SELECT, F_NO_APPEND,
    F_PUBLISH_EVENTS);
  UndoableAction * ua =
    arranger_selections_action_new_create (
      TL_SELECTIONS);
  undo_manager_perform (UNDO_MANAGER, ua);
  g_assert_cmpint (lane->num_regions, ==, 1);

  position_set_to_bar (&pos, 5);
  position_set_to_bar (&end_pos, 7);
  ZRegion * r2 =
    midi_region_new (
      &pos, &end_pos, midi_track->pos, 0,
      lane->num_regions);
  track_add_region (
    midi_track, r2, NULL, lane->pos,
    F_GEN_NAME, F_NO_PUBLISH_EVENTS);
  arranger_object_select (
    (ArrangerObject *) r2, F_SELECT, F_NO_APPEND,
    F_PUBLISH_EVENTS);
  ua =
    arranger_selections_action_new_create (
      TL_SELECTIONS);
  undo_manager_perform (UNDO_MANAGER, ua);
  g_assert_cmpint (lane->num_regions, ==, 2);

  /* select the regions */
  arranger_object_select (
    (ArrangerObject *) r2, F_SELECT, F_NO_APPEND,
    F_PUBLISH_EVENTS);
  arranger_object_select (
    (ArrangerObject *) r1, F_SELECT, F_APPEND,
    F_PUBLISH_EVENTS);

  Track * new_midi_track =
    tracklist_find_track_by_name (
      TRACKLIST, TARGET_MIDI_TRACK_NAME);
  TrackLane * target_lane =
    new_midi_track->lanes[0];
  g_assert_cmpint (target_lane->num_regions, ==, 0);

  double ticks = 100.0;

  /* replicate the logic from the arranger */
  timeline_selections_set_index_in_prev_lane (
    TL_SELECTIONS);
  timeline_selections_move_regions_to_new_tracks (
    TL_SELECTIONS,
    new_midi_track->pos - midi_track->pos);
  arranger_selections_add_ticks (
    (ArrangerSelections *) TL_SELECTIONS,
    ticks);
  g_assert_cmpint (target_lane->num_regions, ==, 2);
  g_assert_cmpint (
    TL_SELECTIONS->num_regions, ==, 2);

  g_debug ("selections:");
  region_print (TL_SELECTIONS->regions[0]);
  region_print (TL_SELECTIONS->regions[1]);

  ua =
    arranger_selections_action_new_duplicate (
      TL_SELECTIONS, ticks, 0, 0,
      new_midi_track->pos - midi_track->pos, 0, 0,
      F_ALREADY_MOVED);
  undo_manager_perform (UNDO_MANAGER, ua);

  /* check that new regions are created */
  g_assert_cmpint (target_lane->num_regions, ==, 2);

  undo_manager_undo (UNDO_MANAGER);

  /* check that new regions are deleted */
  g_assert_cmpint (target_lane->num_regions, ==, 0);

  undo_manager_redo (UNDO_MANAGER);

  /* check that new regions are created */
  g_assert_cmpint (target_lane->num_regions, ==, 2);

  test_helper_zrythm_cleanup ();
}

static void
test_midi_region_split ()
{
  rebootstrap_timeline ();

  Track * midi_track =
    tracklist_find_track_by_name (
      TRACKLIST, MIDI_TRACK_NAME);
  TrackLane * lane =
    midi_track->lanes[0];
  g_assert_cmpint (lane->num_regions, ==, 0);

  Position pos, end_pos;
  position_set_to_bar (&pos, 1);
  position_set_to_bar (&end_pos, 5);
  ZRegion * r =
    midi_region_new (
      &pos, &end_pos, midi_track->pos, 0,
      lane->num_regions);
  track_add_region (
    midi_track, r, NULL, lane->pos,
    F_GEN_NAME, F_NO_PUBLISH_EVENTS);
  arranger_object_select (
    (ArrangerObject *) r, F_SELECT, F_NO_APPEND,
    F_PUBLISH_EVENTS);
  UndoableAction * ua =
    arranger_selections_action_new_create (
      TL_SELECTIONS);
  undo_manager_perform (UNDO_MANAGER, ua);
  g_assert_cmpint (lane->num_regions, ==, 1);

  /* create some MIDI notes */
  for (int i = 0; i < 4; i++)
    {
      position_set_to_bar (&pos, i + 1);
      position_set_to_bar (&end_pos, i + 2);
      MidiNote * mn =
        midi_note_new (
          &r->id, &pos, &end_pos, 34 + i, 70);
      midi_region_add_midi_note (
        r, mn, F_NO_PUBLISH_EVENTS);
      arranger_object_select (
        (ArrangerObject *) mn, F_SELECT,
        F_NO_APPEND, F_NO_PUBLISH_EVENTS);
      ua =
        arranger_selections_action_new_create (
          MA_SELECTIONS);
      undo_manager_perform (UNDO_MANAGER, ua);
      g_assert_cmpint (
        r->num_midi_notes, ==, i + 1);
    }

  /* select the region */
  arranger_object_select (
    (ArrangerObject *) r, F_SELECT, F_NO_APPEND,
    F_NO_PUBLISH_EVENTS);

  /* split at bar 2 */
  position_set_to_bar (&pos, 2);
  ua =
    arranger_selections_action_new_split (
      (ArrangerSelections *) TL_SELECTIONS, &pos);
  undo_manager_perform (UNDO_MANAGER, ua);
  g_assert_cmpint (
    lane->num_regions, ==, 2);
  r = lane->regions[1];
  g_assert_cmpint (
    pos.frames, ==, r->base.pos.frames);
  position_set_to_bar (&pos, 5);
  g_assert_cmpint (
    pos.frames, ==, r->base.end_pos.frames);

  /* split at bar 4 */
  position_set_to_bar (&pos, 4);
  arranger_object_select (
    (ArrangerObject *) r, F_SELECT, F_NO_APPEND,
    F_NO_PUBLISH_EVENTS);
  ua =
    arranger_selections_action_new_split (
      (ArrangerSelections *) TL_SELECTIONS, &pos);
  undo_manager_perform (UNDO_MANAGER, ua);
  g_assert_cmpint (
    lane->num_regions, ==, 3);

  r = lane->regions[0];
  position_set_to_bar (&pos, 1);
  g_assert_cmpint (
    pos.frames, ==, r->base.pos.frames);
  position_set_to_bar (&pos, 2);
  g_assert_cmpint (
    pos.frames, ==, r->base.end_pos.frames);

  r = lane->regions[1];
  position_set_to_bar (&pos, 2);
  g_assert_cmpint (
    pos.frames, ==, r->base.pos.frames);
  position_set_to_bar (&pos, 4);
  g_assert_cmpint (
    pos.frames, ==, r->base.end_pos.frames);

  r = lane->regions[2];
  position_set_to_bar (&pos, 4);
  g_assert_cmpint (
    pos.frames, ==, r->base.pos.frames);
  position_set_to_bar (&pos, 5);
  g_assert_cmpint (
    pos.frames, ==, r->base.end_pos.frames);

  /* split at bar 3 */
  r = lane->regions[1];
  arranger_object_select (
    (ArrangerObject *) r, F_SELECT, F_NO_APPEND,
    F_NO_PUBLISH_EVENTS);
  position_set_to_bar (&pos, 3);
  arranger_object_select (
    (ArrangerObject *) r, F_SELECT, F_NO_APPEND,
    F_NO_PUBLISH_EVENTS);
  ua =
    arranger_selections_action_new_split (
      (ArrangerSelections *) TL_SELECTIONS, &pos);
  undo_manager_perform (UNDO_MANAGER, ua);
  g_assert_cmpint (
    lane->num_regions, ==, 4);

  r = lane->regions[0];
  position_set_to_bar (&pos, 1);
  g_assert_cmpint (
    pos.frames, ==, r->base.pos.frames);
  position_set_to_bar (&pos, 2);
  g_assert_cmpint (
    pos.frames, ==, r->base.end_pos.frames);

  r = lane->regions[1];
  position_set_to_bar (&pos, 4);
  g_assert_cmpint (
    pos.frames, ==, r->base.pos.frames);
  position_set_to_bar (&pos, 5);
  g_assert_cmpint (
    pos.frames, ==, r->base.end_pos.frames);

  r = lane->regions[2];
  position_set_to_bar (&pos, 2);
  g_assert_cmpint (
    pos.frames, ==, r->base.pos.frames);
  position_set_to_bar (&pos, 3);
  g_assert_cmpint (
    pos.frames, ==, r->base.end_pos.frames);

  r = lane->regions[3];
  position_set_to_bar (&pos, 3);
  g_assert_cmpint (
    pos.frames, ==, r->base.pos.frames);
  position_set_to_bar (&pos, 4);
  g_assert_cmpint (
    pos.frames, ==, r->base.end_pos.frames);

  /* undo and verify */
  undo_manager_undo (UNDO_MANAGER);
  g_assert_cmpint (
    lane->num_regions, ==, 3);

  r = lane->regions[0];
  position_set_to_bar (&pos, 1);
  g_assert_cmpint (
    pos.frames, ==, r->base.pos.frames);
  position_set_to_bar (&pos, 2);
  g_assert_cmpint (
    pos.frames, ==, r->base.end_pos.frames);

  r = lane->regions[1];
  position_set_to_bar (&pos, 2);
  g_assert_cmpint (
    pos.frames, ==, r->base.pos.frames);
  position_set_to_bar (&pos, 4);
  g_assert_cmpint (
    pos.frames, ==, r->base.end_pos.frames);

  r = lane->regions[2];
  position_set_to_bar (&pos, 4);
  g_assert_cmpint (
    pos.frames, ==, r->base.pos.frames);
  position_set_to_bar (&pos, 5);
  g_assert_cmpint (
    pos.frames, ==, r->base.end_pos.frames);

  /* undo and verify */
  undo_manager_undo (UNDO_MANAGER);
  g_assert_cmpint (
    lane->num_regions, ==, 2);

  r = lane->regions[0];
  position_set_to_bar (&pos, 1);
  g_assert_cmpint (
    pos.frames, ==, r->base.pos.frames);
  position_set_to_bar (&pos, 2);
  g_assert_cmpint (
    pos.frames, ==, r->base.end_pos.frames);

  r = lane->regions[1];
  position_set_to_bar (&pos, 2);
  g_assert_cmpint (
    pos.frames, ==, r->base.pos.frames);
  position_set_to_bar (&pos, 5);
  g_assert_cmpint (
    pos.frames, ==, r->base.end_pos.frames);

  /* undo and verify */
  undo_manager_undo (UNDO_MANAGER);
  g_assert_cmpint (
    lane->num_regions, ==, 1);

  r = lane->regions[0];
  position_set_to_bar (&pos, 1);
  g_assert_cmpint (
    pos.frames, ==, r->base.pos.frames);
  position_set_to_bar (&pos, 5);
  g_assert_cmpint (
    pos.frames, ==, r->base.end_pos.frames);

  /* redo to bring 3 regions back */
  undo_manager_redo (UNDO_MANAGER);
  g_assert_cmpint (
    lane->num_regions, ==, 2);

  r = lane->regions[0];
  position_set_to_bar (&pos, 1);
  g_assert_cmpint (
    pos.frames, ==, r->base.pos.frames);
  position_set_to_bar (&pos, 2);
  g_assert_cmpint (
    pos.frames, ==, r->base.end_pos.frames);

  r = lane->regions[1];
  position_set_to_bar (&pos, 2);
  g_assert_cmpint (
    pos.frames, ==, r->base.pos.frames);
  position_set_to_bar (&pos, 5);
  g_assert_cmpint (
    pos.frames, ==, r->base.end_pos.frames);

  undo_manager_redo (UNDO_MANAGER);
  g_assert_cmpint (
    lane->num_regions, ==, 3);

  r = lane->regions[0];
  position_set_to_bar (&pos, 1);
  g_assert_cmpint (
    pos.frames, ==, r->base.pos.frames);
  position_set_to_bar (&pos, 2);
  g_assert_cmpint (
    pos.frames, ==, r->base.end_pos.frames);

  r = lane->regions[1];
  position_set_to_bar (&pos, 2);
  g_assert_cmpint (
    pos.frames, ==, r->base.pos.frames);
  position_set_to_bar (&pos, 4);
  g_assert_cmpint (
    pos.frames, ==, r->base.end_pos.frames);

  r = lane->regions[2];
  position_set_to_bar (&pos, 4);
  g_assert_cmpint (
    pos.frames, ==, r->base.pos.frames);
  position_set_to_bar (&pos, 5);
  g_assert_cmpint (
    pos.frames, ==, r->base.end_pos.frames);

  /* delete middle cut */
  r = lane->regions[1];
  arranger_object_select (
    (ArrangerObject *) r, F_SELECT, F_NO_APPEND,
    F_NO_PUBLISH_EVENTS);
  ua =
    arranger_selections_action_new_delete (
      (ArrangerSelections *) TL_SELECTIONS);
  undo_manager_perform (UNDO_MANAGER, ua);

  g_assert_cmpint (
    lane->num_regions, ==, 2);

  r = lane->regions[0];
  position_set_to_bar (&pos, 1);
  g_assert_cmpint (
    pos.frames, ==, r->base.pos.frames);
  position_set_to_bar (&pos, 2);
  g_assert_cmpint (
    pos.frames, ==, r->base.end_pos.frames);

  r = lane->regions[1];
  position_set_to_bar (&pos, 4);
  g_assert_cmpint (
    pos.frames, ==, r->base.pos.frames);
  position_set_to_bar (&pos, 5);
  g_assert_cmpint (
    pos.frames, ==, r->base.end_pos.frames);

  /* undo to bring it back */
  undo_manager_undo (UNDO_MANAGER);
  g_assert_cmpint (
    lane->num_regions, ==, 3);

  r = lane->regions[0];
  position_set_to_bar (&pos, 1);
  g_assert_cmpint (
    pos.frames, ==, r->base.pos.frames);
  position_set_to_bar (&pos, 2);
  g_assert_cmpint (
    pos.frames, ==, r->base.end_pos.frames);

  r = lane->regions[1];
  position_set_to_bar (&pos, 2);
  g_assert_cmpint (
    pos.frames, ==, r->base.pos.frames);
  position_set_to_bar (&pos, 4);
  g_assert_cmpint (
    pos.frames, ==, r->base.end_pos.frames);

  r = lane->regions[2];
  position_set_to_bar (&pos, 4);
  g_assert_cmpint (
    pos.frames, ==, r->base.pos.frames);
  position_set_to_bar (&pos, 5);
  g_assert_cmpint (
    pos.frames, ==, r->base.end_pos.frames);

  test_helper_zrythm_cleanup ();
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

#define TEST_PREFIX "/actions/arranger_selections/"

  g_test_add_func (
    TEST_PREFIX "test delete chords",
    (GTestFunc) test_delete_chords);
  g_test_add_func (
    TEST_PREFIX "test delete automation points",
    (GTestFunc) test_delete_automation_points);
  g_test_add_func (
    TEST_PREFIX "test audio functions",
    (GTestFunc) test_audio_functions);
  g_test_add_func (
    TEST_PREFIX "test create timeline",
    (GTestFunc) test_create_timeline);
  g_test_add_func (
    TEST_PREFIX "test midi region split",
    (GTestFunc) test_midi_region_split);
  g_test_add_func (
    TEST_PREFIX "test duplicate midi regions to track below",
    (GTestFunc) test_duplicate_midi_regions_to_track_below);
  g_test_add_func (
    TEST_PREFIX "test delete timeline",
    (GTestFunc) test_delete_timeline);
  g_test_add_func (
    TEST_PREFIX "test move timeline",
    (GTestFunc) test_move_timeline);
  g_test_add_func (
    TEST_PREFIX "test duplicate timeline",
    (GTestFunc) test_duplicate_timeline);
  g_test_add_func (
    TEST_PREFIX "test link timeline",
    (GTestFunc) test_link_timeline);
  g_test_add_func (
    TEST_PREFIX "test edit marker",
    (GTestFunc) test_edit_marker);
  g_test_add_func (
    TEST_PREFIX "test mute",
    (GTestFunc) test_mute);
  g_test_add_func (
    TEST_PREFIX "test split",
    (GTestFunc) test_split);
  g_test_add_func (
    TEST_PREFIX "test quantize",
    (GTestFunc) test_quantize);
  g_test_add_func (
    TEST_PREFIX "test automation fill",
    (GTestFunc) test_automation_fill);
  g_test_add_func (
    TEST_PREFIX "test duplicate automation region",
    (GTestFunc) test_duplicate_automation_region);

  return g_test_run ();
}
