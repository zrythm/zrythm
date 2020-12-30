/*
 * Copyright (C) 2019-2020 Alexandros Theodotou <alex at zrythm dot org>
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
  int check_selections,
  int _check_has_single_undo,
  int _check_has_single_redo)
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
  /* clear undo/redo stacks */
  undo_manager_clear_stacks (UNDO_MANAGER, true);

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
}

static void
test_delete_timeline ()
{
  /* clear undo/redo stacks */
  undo_manager_clear_stacks (UNDO_MANAGER, true);

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
  /* clear undo/redo stacks */
  undo_manager_clear_stacks (UNDO_MANAGER, true);

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

          /* rebootstrap */
          rebootstrap_timeline ();
        }
      g_assert_cmpint (
        arranger_selections_get_num_objects (
          (ArrangerSelections *) TL_SELECTIONS), ==,
        TOTAL_TL_SELECTIONS);
      check_timeline_objects_vs_original_state (
        1, 0, 1);
    }
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
  /* clear undo/redo stacks */
  undo_manager_clear_stacks (UNDO_MANAGER, true);

  /* when i == 1 we are moving to new tracks */
  for (int i = 0; i < 2; i++)
    {
      rebootstrap_timeline ();
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
            new_midi_track);
          region_move_to_track (
            audio_track->lanes[AUDIO_REGION_LANE]->
              regions[0],
            new_audio_track);
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
}

static void
test_link_timeline ()
{
  /* clear undo/redo stacks */
  undo_manager_clear_stacks (UNDO_MANAGER, true);

  /* when i == 1 we are moving to new tracks */
  for (int i = 0; i < 2; i++)
    {
      rebootstrap_timeline ();

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
            new_midi_track);
          region_move_to_track (
            audio_track->lanes[AUDIO_REGION_LANE]->
              regions[0],
            new_audio_track);
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
}

static void
test_edit_marker ()
{
  /* clear undo/redo stacks */
  undo_manager_clear_stacks (UNDO_MANAGER, true);

  rebootstrap_timeline ();

  /* create marker with name "aa" */
  Marker * marker = marker_new ("aa");
  ArrangerObject * marker_obj =
    (ArrangerObject *) marker;
  arranger_object_select (
    marker_obj, F_SELECT, F_NO_APPEND);
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
}

static void
test_mute ()
{
  /* clear undo/redo stacks */
  undo_manager_clear_stacks (UNDO_MANAGER, true);

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
}

static void
test_split ()
{
  /* clear undo/redo stacks */
  undo_manager_clear_stacks (UNDO_MANAGER, true);

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
    r_obj, F_SELECT, F_NO_APPEND);

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
  r_obj =
    (ArrangerObject *) region;
  arranger_object_select (
    r_obj, F_SELECT, F_NO_APPEND);
  AudioClip * clip =
    audio_region_get_clip (region);
  float first_frame = clip->frames[0];
  g_assert_true (clip->frames[0] > 0.000001f);

  position_set_to_bar (&pos, 2);
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
}

static void
test_audio_functions ()
{
  /* clear undo/redo stacks */
  undo_manager_clear_stacks (UNDO_MANAGER, true);

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
    r_obj, F_SELECT, F_NO_APPEND);
  AUDIO_SELECTIONS->region_id = region->id;
  AUDIO_SELECTIONS->has_selection = true;
  AUDIO_SELECTIONS->sel_start = r_obj->pos;
  AUDIO_SELECTIONS->sel_end = r_obj->end_pos;

  AudioClip * orig_clip =
    audio_region_get_clip (region);
  size_t channels = orig_clip->channels;
  float frames[
    (size_t) orig_clip->num_frames * channels];
  dsp_copy (
    frames, orig_clip->frames,
    (size_t) orig_clip->num_frames * channels);

  /* invert */
  UndoableAction * ua =
    arranger_selections_action_new_edit_audio_function (
      (ArrangerSelections *) AUDIO_SELECTIONS,
      AUDIO_FUNCTION_INVERT);
  undo_manager_perform (UNDO_MANAGER, ua);

  AudioClip * clip = audio_region_get_clip (region);
  for (size_t i = 0;
       i < (size_t) clip->num_frames; i++)
    {
      for (size_t j = 0; j < channels; j++)
        {
          g_assert_cmpfloat_with_epsilon (
            - frames[channels * i + j],
            clip->frames[channels * i + j],
            0.0001f);
        }
    }

  test_project_save_and_reload ();

  undo_manager_undo (UNDO_MANAGER);

  /* verify that frames are returned to normal */
  track =
    tracklist_find_track_by_name (
      TRACKLIST, AUDIO_TRACK_NAME);
  lane = track->lanes[3];
  region = lane->regions[0];
  clip = audio_region_get_clip (region);
  for (size_t i = 0;
       i < (size_t) clip->num_frames; i++)
    {
      for (size_t j = 0; j < channels; j++)
        {
          g_assert_cmpfloat_with_epsilon (
            frames[channels * i + j],
            clip->frames[channels * i + j],
            0.0001f);
        }
    }

  undo_manager_redo (UNDO_MANAGER);

  /* verify that frames are edited again */
  for (size_t i = 0;
       i < (size_t) clip->num_frames; i++)
    {
      for (size_t j = 0; j < channels; j++)
        {
          g_assert_cmpfloat_with_epsilon (
            - frames[channels * i + j],
            clip->frames[channels * i + j],
            0.0001f);
        }
    }

  undo_manager_undo (UNDO_MANAGER);
}

static void
test_quantize ()
{
  /* clear undo/redo stacks */
  undo_manager_clear_stacks (UNDO_MANAGER, true);

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
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  test_helper_zrythm_init ();

#define TEST_PREFIX "/actions/arranger_selections/"

  rebootstrap_timeline ();
  g_test_add_func (
    TEST_PREFIX "test create timeline",
    (GTestFunc) test_create_timeline);
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
    TEST_PREFIX "test audio functions",
    (GTestFunc) test_audio_functions);
  g_test_add_func (
    TEST_PREFIX "test split",
    (GTestFunc) test_split);
  g_test_add_func (
    TEST_PREFIX "test quantize",
    (GTestFunc) test_quantize);

  return g_test_run ();
}
