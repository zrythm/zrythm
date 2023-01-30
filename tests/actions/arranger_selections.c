// SPDX-FileCopyrightText: © 2019-2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

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

#include <glib.h>

#include "tests/helpers/project.h"

Position p1, p2;

/**
 * Bootstraps the test with test data.
 */
static void
rebootstrap_timeline (void)
{
  test_project_rebootstrap_timeline (&p1, &p2);
}

static void
select_audio_and_midi_regions_only (void)
{
  arranger_selections_clear (
    (ArrangerSelections *) TL_SELECTIONS, F_NO_FREE,
    F_NO_PUBLISH_EVENTS);
  g_assert_cmpint (TL_SELECTIONS->num_regions, ==, 0);

  Track * midi_track =
    tracklist_find_track_by_name (TRACKLIST, MIDI_TRACK_NAME);
  g_assert_nonnull (midi_track);
  arranger_selections_add_object (
    (ArrangerSelections *) TL_SELECTIONS,
    (ArrangerObject *) midi_track->lanes[MIDI_REGION_LANE]
      ->regions[0]);
  g_assert_cmpint (TL_SELECTIONS->num_regions, ==, 1);
  Track * audio_track = tracklist_find_track_by_name (
    TRACKLIST, AUDIO_TRACK_NAME);
  g_assert_nonnull (audio_track);
  arranger_selections_add_object (
    (ArrangerSelections *) TL_SELECTIONS,
    (ArrangerObject *) audio_track->lanes[AUDIO_REGION_LANE]
      ->regions[0]);
  g_assert_cmpint (TL_SELECTIONS->num_regions, ==, 2);
}

/**
 * Check if the undo stack has a single element.
 */
static void
check_has_single_undo (void)
{
  g_assert_cmpint (
    UNDO_MANAGER->undo_stack->stack->top, ==, 0);
  g_assert_cmpint (
    UNDO_MANAGER->redo_stack->stack->top, ==, -1);
  g_assert_cmpint (
    (int) UNDO_MANAGER->undo_stack->num_as_actions, ==, 1);
  g_assert_cmpint (
    (int) UNDO_MANAGER->redo_stack->num_as_actions, ==, 0);
}

/**
 * Check if the redo stack has a single element.
 */
static void
check_has_single_redo (void)
{
  g_assert_cmpint (
    (int) UNDO_MANAGER->undo_stack->stack->top, ==, -1);
  g_assert_cmpint (
    (int) UNDO_MANAGER->redo_stack->stack->top, ==, 0);
  g_assert_cmpint (
    (int) UNDO_MANAGER->undo_stack->num_as_actions, ==, 0);
  g_assert_cmpint (
    (int) UNDO_MANAGER->redo_stack->num_as_actions, ==, 1);
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
check_timeline_objects_deleted (int creating)
{
  g_assert_cmpint (
    arranger_selections_get_num_objects (
      (ArrangerSelections *) TL_SELECTIONS),
    ==, 0);

  Track * midi_track =
    tracklist_find_track_by_name (TRACKLIST, MIDI_TRACK_NAME);
  g_assert_nonnull (midi_track);
  Track * audio_track = tracklist_find_track_by_name (
    TRACKLIST, AUDIO_TRACK_NAME);
  g_assert_nonnull (audio_track);

  /* check midi region */
  g_assert_cmpint (midi_track->num_lanes, ==, 1);
  g_assert_cmpint (midi_track->lanes[0]->num_regions, ==, 0);

  /* check audio region */
  g_assert_cmpint (midi_track->num_lanes, ==, 1);
  g_assert_cmpint (audio_track->lanes[0]->num_regions, ==, 0);

  /* check automation region */
  AutomationTracklist * atl =
    track_get_automation_tracklist (P_MASTER_TRACK);
  g_assert_nonnull (atl);
  AutomationTrack * at = channel_get_automation_track (
    P_MASTER_TRACK->channel, PORT_FLAG_STEREO_BALANCE);
  g_assert_nonnull (at);
  g_assert_cmpint (at->num_regions, ==, 0);

  /* check marker */
  g_assert_cmpint (P_MARKER_TRACK->num_markers, ==, 2);

  /* check scale object */
  g_assert_cmpint (P_CHORD_TRACK->num_scales, ==, 0);

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
test_create_timeline (void)
{
  rebootstrap_timeline ();

  /* do create */
  arranger_selections_action_perform_create (
    TL_SELECTIONS, NULL);

  /* check */
  g_assert_cmpint (
    arranger_selections_get_num_objects (
      (ArrangerSelections *) TL_SELECTIONS),
    ==, TOTAL_TL_SELECTIONS);
  g_assert_cmpint (
    arranger_selections_get_num_objects (
      (ArrangerSelections *) MA_SELECTIONS),
    ==, 1);
  check_timeline_objects_vs_original_state (1, 1, 0);

  /* undo and check that the objects are deleted */
  undo_manager_undo (UNDO_MANAGER, NULL);
  g_assert_cmpint (
    arranger_selections_get_num_objects (
      (ArrangerSelections *) MA_SELECTIONS),
    ==, 0);
  check_timeline_objects_deleted (1);

  /* redo and check that the objects are there */
  undo_manager_redo (UNDO_MANAGER, NULL);
  g_assert_cmpint (
    arranger_selections_get_num_objects (
      (ArrangerSelections *) TL_SELECTIONS),
    ==, TOTAL_TL_SELECTIONS);
  check_timeline_objects_vs_original_state (1, 1, 0);

  test_helper_zrythm_cleanup ();
}

static void
test_delete_timeline (void)
{
  rebootstrap_timeline ();

  /* do delete */
  bool ret = arranger_selections_action_perform_delete (
    TL_SELECTIONS, NULL);
  g_assert_true (ret);

  g_assert_null (clip_editor_get_region (CLIP_EDITOR));

  /* check */
  check_timeline_objects_deleted (0);
  g_assert_cmpint (
    arranger_selections_get_num_objects (
      (ArrangerSelections *) MA_SELECTIONS),
    ==, 0);
  g_assert_cmpint (
    arranger_selections_get_num_objects (
      (ArrangerSelections *) CHORD_SELECTIONS),
    ==, 0);
  g_assert_cmpint (
    arranger_selections_get_num_objects (
      (ArrangerSelections *) AUTOMATION_SELECTIONS),
    ==, 0);

  /* undo and check that the objects are created */
  int iret = undo_manager_undo (UNDO_MANAGER, NULL);
  g_assert_true (iret == 0);
  g_assert_cmpint (
    arranger_selections_get_num_objects (
      (ArrangerSelections *) TL_SELECTIONS),
    ==, TOTAL_TL_SELECTIONS);
  check_timeline_objects_vs_original_state (1, 0, 1);

  /* redo and check that the objects are gone */
  undo_manager_redo (UNDO_MANAGER, NULL);
  g_assert_cmpint (
    arranger_selections_get_num_objects (
      (ArrangerSelections *) TL_SELECTIONS),
    ==, 0);
  g_assert_cmpint (
    arranger_selections_get_num_objects (
      (ArrangerSelections *) MA_SELECTIONS),
    ==, 0);
  g_assert_cmpint (
    arranger_selections_get_num_objects (
      (ArrangerSelections *) CHORD_SELECTIONS),
    ==, 0);
  g_assert_cmpint (
    arranger_selections_get_num_objects (
      (ArrangerSelections *) AUTOMATION_SELECTIONS),
    ==, 0);
  check_timeline_objects_deleted (0);

  g_assert_false (clip_editor_get_region (CLIP_EDITOR));

  /* undo again to prepare for next test */
  undo_manager_undo (UNDO_MANAGER, NULL);
  g_assert_cmpint (
    arranger_selections_get_num_objects (
      (ArrangerSelections *) TL_SELECTIONS),
    ==, TOTAL_TL_SELECTIONS);
  check_timeline_objects_vs_original_state (1, 0, 1);

  test_helper_zrythm_cleanup ();
}

static void
test_delete_chords (void)
{
  rebootstrap_timeline ();

  ZRegion * r = P_CHORD_TRACK->chord_regions[0];
  g_assert_true (region_validate (r, F_PROJECT, 0));

  /* add another chord */
  ChordObject * c = chord_object_new (&r->id, 2, 2);
  chord_region_add_chord_object (r, c, F_NO_PUBLISH_EVENTS);
  arranger_selections_add_object (
    (ArrangerSelections *) CHORD_SELECTIONS,
    (ArrangerObject *) c);
  arranger_selections_action_perform_create (
    CHORD_SELECTIONS, NULL);

  /* delete the first chord */
  arranger_selections_clear (
    (ArrangerSelections *) CHORD_SELECTIONS, F_NO_FREE,
    F_NO_PUBLISH_EVENTS);
  arranger_selections_add_object (
    (ArrangerSelections *) CHORD_SELECTIONS,
    (ArrangerObject *) r->chord_objects[0]);
  arranger_selections_action_perform_delete (
    CHORD_SELECTIONS, NULL);
  g_assert_true (region_validate (r, F_PROJECT, 0));

  undo_manager_undo (UNDO_MANAGER, NULL);
  undo_manager_redo (UNDO_MANAGER, NULL);
  undo_manager_undo (UNDO_MANAGER, NULL);
  undo_manager_undo (UNDO_MANAGER, NULL);
  undo_manager_redo (UNDO_MANAGER, NULL);
  undo_manager_undo (UNDO_MANAGER, NULL);

  test_helper_zrythm_cleanup ();
}

/**
 * Checks the objects after moving.
 *
 * @param new_tracks Whether objects were moved to
 *   new tracks or in the same track.
 */
static void
check_after_move_timeline (int new_tracks)
{
  /* check */
  g_assert_cmpint (
    arranger_selections_get_num_objects (
      (ArrangerSelections *) TL_SELECTIONS),
    ==, new_tracks ? 2 : TOTAL_TL_SELECTIONS);

  /* check that undo/redo stacks have the correct
   * counts (1 and 0) */
  check_has_single_undo ();
  g_assert_cmpint (
    UNDO_MANAGER->undo_stack->as_actions[0]->delta_tracks, ==,
    new_tracks ? 2 : 0);

  /* get tracks */
  Track * midi_track_before =
    tracklist_find_track_by_name (TRACKLIST, MIDI_TRACK_NAME);
  g_assert_nonnull (midi_track_before);
  g_assert_cmpint (midi_track_before->pos, >, 0);
  Track * audio_track_before = tracklist_find_track_by_name (
    TRACKLIST, AUDIO_TRACK_NAME);
  g_assert_nonnull (audio_track_before);
  g_assert_cmpint (audio_track_before->pos, >, 0);
  Track * midi_track_after = tracklist_find_track_by_name (
    TRACKLIST, TARGET_MIDI_TRACK_NAME);
  g_assert_nonnull (midi_track_after);
  g_assert_cmpint (
    midi_track_after->pos, >, midi_track_before->pos);
  Track * audio_track_after = tracklist_find_track_by_name (
    TRACKLIST, TARGET_AUDIO_TRACK_NAME);
  g_assert_nonnull (audio_track_after);
  g_assert_cmpint (
    audio_track_after->pos, >, audio_track_before->pos);

  /* check midi region */
  ArrangerObject * obj;
  if (new_tracks)
    {
      obj =
        (ArrangerObject *) midi_track_after
          ->lanes[MIDI_REGION_LANE]
          ->regions[0];
    }
  else
    {
      obj =
        (ArrangerObject *) midi_track_before
          ->lanes[MIDI_REGION_LANE]
          ->regions[0];
    }
  ZRegion * r = (ZRegion *) obj;
  g_assert_true (IS_REGION (obj));
  g_assert_cmpint (r->id.type, ==, REGION_TYPE_MIDI);
  Position p1_after_move, p2_after_move;
  p1_after_move = p1;
  p2_after_move = p2;
  position_add_ticks (&p1_after_move, MOVE_TICKS);
  position_add_ticks (&p2_after_move, MOVE_TICKS);
  g_assert_cmppos (&obj->pos, &p1_after_move);
  g_assert_cmppos (&obj->end_pos, &p2_after_move);
  if (new_tracks)
    {
      Track * tmp = arranger_object_get_track (obj);
      g_assert_true (tmp == midi_track_after);
      g_assert_cmpuint (
        r->id.track_name_hash, ==,
        track_get_name_hash (midi_track_after));
    }
  else
    {
      Track * tmp = arranger_object_get_track (obj);
      g_assert_true (tmp == midi_track_before);
      g_assert_cmpuint (
        r->id.track_name_hash, ==,
        track_get_name_hash (midi_track_before));
    }
  g_assert_cmpint (r->num_midi_notes, ==, 1);
  MidiNote * mn = r->midi_notes[0];
  obj = (ArrangerObject *) mn;
  g_assert_cmpuint (mn->val, ==, MN_VAL);
  g_assert_cmpuint (mn->vel->vel, ==, MN_VEL);
  g_assert_cmppos (&obj->pos, &p1);
  g_assert_cmppos (&obj->end_pos, &p2);
  g_assert_true (
    region_identifier_is_equal (&obj->region_id, &r->id));

  /* check audio region */
  obj =
    (ArrangerObject *)
      TL_SELECTIONS->regions[new_tracks ? 1 : 3];
  p1_after_move = p1;
  position_add_ticks (&p1_after_move, MOVE_TICKS);
  g_assert_cmppos (&obj->pos, &p1_after_move);
  r = (ZRegion *) obj;
  g_assert_true (IS_REGION (obj));
  g_assert_cmpint (r->id.type, ==, REGION_TYPE_AUDIO);
  if (new_tracks)
    {
      Track * tmp = arranger_object_get_track (obj);
      g_assert_true (tmp == audio_track_after);
      g_assert_cmpuint (
        r->id.track_name_hash, ==,
        track_get_name_hash (audio_track_after));
    }
  else
    {
      Track * tmp = arranger_object_get_track (obj);
      g_assert_true (tmp == audio_track_before);
      g_assert_cmpuint (
        r->id.track_name_hash, ==,
        track_get_name_hash (audio_track_before));
    }

  if (!new_tracks)
    {
      /* check automation region */
      obj = (ArrangerObject *) TL_SELECTIONS->regions[1];
      g_assert_cmppos (&obj->pos, &p1_after_move);
      g_assert_cmppos (&obj->end_pos, &p2_after_move);
      r = (ZRegion *) obj;
      g_assert_cmpint (r->num_aps, ==, 2);
      AutomationPoint * ap = r->aps[0];
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
      g_assert_cmpint (TL_SELECTIONS->num_markers, ==, 1);
      obj = (ArrangerObject *) TL_SELECTIONS->markers[0];
      Marker * m = (Marker *) obj;
      g_assert_cmppos (&obj->pos, &p1_after_move);
      g_assert_cmpstr (m->name, ==, MARKER_NAME);

      /* check scale object */
      g_assert_cmpint (
        TL_SELECTIONS->num_scale_objects, ==, 1);
      obj = (ArrangerObject *) TL_SELECTIONS->scale_objects[0];
      ScaleObject * s = (ScaleObject *) obj;
      g_assert_cmppos (&obj->pos, &p1_after_move);
      g_assert_cmpint (s->scale->type, ==, MUSICAL_SCALE_TYPE);
      g_assert_cmpint (
        s->scale->root_key, ==, MUSICAL_SCALE_ROOT);
    }
}

static void
test_move_audio_region_and_lower_bpm (void)
{
  test_helper_zrythm_init ();

  char audio_file_path[2000];
  sprintf (
    audio_file_path, "%s%s%s", TESTS_SRCDIR,
    G_DIR_SEPARATOR_S, "test.wav");

  /* create audio track with region */
  Position pos;
  position_init (&pos);
  int             track_pos = TRACKLIST->num_tracks;
  SupportedFile * file =
    supported_file_new_from_path (audio_file_path);
  Track * track = track_create_with_action (
    TRACK_TYPE_AUDIO, NULL, file, &pos, track_pos, 1, NULL);

  /* move the region */
  arranger_object_select (
    (ArrangerObject *) track->lanes[0]->regions[0], F_SELECT,
    F_NO_APPEND, F_NO_PUBLISH_EVENTS);
  arranger_selections_action_perform_move_timeline (
    TL_SELECTIONS, MOVE_TICKS, 0, 0, NULL,
    F_NOT_ALREADY_MOVED, NULL);

  for (int i = 0; i < 2; i++)
    {
      float bpm_diff = (i == 1) ? 20.f : 40.f;

      /* lower BPM and attempt to save */
      bpm_t bpm_before =
        tempo_track_get_current_bpm (P_TEMPO_TRACK);
      ZRegion * r = TRACKLIST->tracks[5]->lanes[0]->regions[0];
      long frames_len = arranger_object_get_length_in_frames (
        (ArrangerObject *) r);
      double ticks_len = arranger_object_get_length_in_ticks (
        (ArrangerObject *) r);
      AudioClip * clip = AUDIO_ENGINE->pool->clips[r->pool_id];
      g_message (
        "before | r size: %ld (ticks %f), clip size %ld",
        frames_len, ticks_len, clip->num_frames);
      region_print (r);
      region_validate (r, true, 0);
      tempo_track_set_bpm (
        P_TEMPO_TRACK, bpm_before - bpm_diff, bpm_before,
        Z_F_NOT_TEMPORARY, F_NO_PUBLISH_EVENTS);
      frames_len = arranger_object_get_length_in_frames (
        (ArrangerObject *) r);
      ticks_len = arranger_object_get_length_in_ticks (
        (ArrangerObject *) r);
      g_message (
        "after | r size: %ld (ticks %f), clip size %ld",
        frames_len, ticks_len, clip->num_frames);
      region_print (r);
      region_validate (r, true, 0);
      test_project_save_and_reload ();

      /* undo lowering BPM */
      undo_manager_undo (UNDO_MANAGER, NULL);
    }

  test_helper_zrythm_cleanup ();
}

static void
test_move_audio_region_and_lower_samplerate (void)
{
  test_helper_zrythm_init ();

  char audio_file_path[2000];
  sprintf (
    audio_file_path, "%s%s%s", TESTS_SRCDIR,
    G_DIR_SEPARATOR_S, "test.wav");

  /* create audio track with region */
  Position pos;
  position_init (&pos);
  int             track_pos = TRACKLIST->num_tracks;
  SupportedFile * file =
    supported_file_new_from_path (audio_file_path);
  Track * track = track_create_with_action (
    TRACK_TYPE_AUDIO, NULL, file, &pos, track_pos, 1, NULL);

  /* move the region */
  arranger_object_select (
    (ArrangerObject *) track->lanes[0]->regions[0], F_SELECT,
    F_NO_APPEND, F_NO_PUBLISH_EVENTS);
  arranger_selections_action_perform_move_timeline (
    TL_SELECTIONS, MOVE_TICKS, 0, 0, NULL,
    F_NOT_ALREADY_MOVED, NULL);

  for (int i = 0; i < 4; i++)
    {
      /* save the project */
      GError * err = NULL;
      bool     success = project_save (
        PROJECT, PROJECT->dir, 0, 0, F_NO_ASYNC, &err);
      g_assert_true (success);
      char * prj_file =
        g_build_filename (PROJECT->dir, PROJECT_FILE, NULL);

      /* adjust the samplerate to be given at startup */
      zrythm_app->samplerate =
        (int) AUDIO_ENGINE->sample_rate / 2;

      object_free_w_func_and_null (project_free, PROJECT);

      /* reload */
      success = project_load (prj_file, 0, &err);
      g_assert_true (success);
    }

  test_helper_zrythm_cleanup ();
}

/**
 * Tests the move action.
 *
 * @param new_tracks Whether to move objects to new
 *   tracks or in the same track.
 */
static void
test_move_timeline (void)
{
  rebootstrap_timeline ();

  /* when i == 1 we are moving to new tracks */
  for (int i = 0; i < 2; i++)
    {
      int track_diff = i ? 2 : 0;
      if (track_diff)
        {
          select_audio_and_midi_regions_only ();
        }

      /* check undo/redo stacks */
      g_assert_cmpint (
        UNDO_MANAGER->undo_stack->stack->top, ==, -1);
      g_assert_cmpint (
        UNDO_MANAGER->redo_stack->stack->top, ==, i ? 0 : -1);
      g_assert_cmpuint (
        UNDO_MANAGER->undo_stack->num_as_actions, ==, 0);
      g_assert_cmpuint (
        UNDO_MANAGER->redo_stack->num_as_actions, ==,
        i ? 1 : 0);

      /* do move ticks */
      arranger_selections_action_perform_move_timeline (
        TL_SELECTIONS, MOVE_TICKS, track_diff, 0, NULL,
        F_NOT_ALREADY_MOVED, NULL);

      /* check */
      check_after_move_timeline (i);

      /* undo and check that the objects are at
       * their original state*/
      undo_manager_undo (UNDO_MANAGER, NULL);
      g_assert_cmpint (
        arranger_selections_get_num_objects (
          (ArrangerSelections *) TL_SELECTIONS),
        ==, i ? 2 : TOTAL_TL_SELECTIONS);

      check_timeline_objects_vs_original_state (
        i ? 0 : 1, 0, 1);

      /* redo and check that the objects are moved
       * again */
      undo_manager_redo (UNDO_MANAGER, NULL);
      check_after_move_timeline (i);

      /* undo again to prepare for next test */
      undo_manager_undo (UNDO_MANAGER, NULL);
      if (track_diff)
        {
          g_assert_cmpint (
            arranger_selections_get_num_objects (
              (ArrangerSelections *)
              /* 2 regions */
              TL_SELECTIONS),
            ==, 2);
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
check_after_duplicate_timeline (int new_tracks, bool link)
{
  /* check */
  g_assert_cmpint (
    arranger_selections_get_num_objects (
      (ArrangerSelections *) TL_SELECTIONS),
    ==, new_tracks ? 2 : TOTAL_TL_SELECTIONS);

  Track * midi_track =
    tracklist_find_track_by_name (TRACKLIST, MIDI_TRACK_NAME);
  g_assert_nonnull (midi_track);
  Track * audio_track = tracklist_find_track_by_name (
    TRACKLIST, AUDIO_TRACK_NAME);
  g_assert_nonnull (audio_track);
  Track * new_midi_track = tracklist_find_track_by_name (
    TRACKLIST, TARGET_MIDI_TRACK_NAME);
  g_assert_nonnull (midi_track);
  Track * new_audio_track = tracklist_find_track_by_name (
    TRACKLIST, TARGET_AUDIO_TRACK_NAME);
  g_assert_nonnull (audio_track);

  /* check prev midi region */
  ArrangerObject * obj;
  if (new_tracks)
    {
      g_assert_cmpint (
        midi_track->lanes[MIDI_REGION_LANE]->num_regions, ==,
        1);
      g_assert_cmpint (
        new_midi_track->lanes[MIDI_REGION_LANE]->num_regions,
        ==, 1);
    }
  else
    {
      g_assert_cmpint (
        midi_track->lanes[MIDI_REGION_LANE]->num_regions, ==,
        2);
    }
  obj =
    (ArrangerObject *) midi_track->lanes[MIDI_REGION_LANE]
      ->regions[0];
  Position p1_before_move, p2_before_move;
  p1_before_move = p1;
  p2_before_move = p2;
  g_assert_cmppos (&obj->pos, &p1_before_move);
  g_assert_cmppos (&obj->end_pos, &p2_before_move);
  ZRegion * r = (ZRegion *) obj;
  g_assert_cmpuint (
    r->id.track_name_hash, ==,
    track_get_name_hash (midi_track));
  g_assert_cmpint (r->id.lane_pos, ==, MIDI_REGION_LANE);
  g_assert_cmpint (r->id.idx, ==, 0);
  g_assert_cmpint (r->num_midi_notes, ==, 1);
  MidiNote * mn = r->midi_notes[0];
  obj = (ArrangerObject *) mn;
  g_assert_true (
    region_identifier_is_equal (&obj->region_id, &r->id));
  g_assert_cmpuint (mn->val, ==, MN_VAL);
  g_assert_cmpuint (mn->vel->vel, ==, MN_VEL);
  g_assert_cmppos (&obj->pos, &p1);
  g_assert_cmppos (&obj->end_pos, &p2);
  int link_group = r->id.link_group;
  if (link)
    {
      g_assert_cmpint (r->id.link_group, >, -1);
    }
  g_message ("prev:");
  region_print (r);

  /* check new midi region */
  if (new_tracks)
    {
      obj =
        (ArrangerObject *) new_midi_track
          ->lanes[MIDI_REGION_LANE]
          ->regions[0];
    }
  else
    {
      obj =
        (ArrangerObject *) midi_track->lanes[MIDI_REGION_LANE]
          ->regions[1];
    }
  Position p1_after_move, p2_after_move;
  p1_after_move = p1;
  p2_after_move = p2;
  position_add_ticks (&p1_after_move, MOVE_TICKS);
  position_add_ticks (&p2_after_move, MOVE_TICKS);
  g_assert_cmppos (&obj->pos, &p1_after_move);
  g_assert_cmppos (&obj->end_pos, &p2_after_move);
  r = (ZRegion *) obj;
  if (new_tracks)
    {
      g_assert_cmpuint (
        r->id.track_name_hash, ==,
        track_get_name_hash (new_midi_track));
      g_assert_cmpint (r->id.idx, ==, 0);
    }
  else
    {
      g_assert_cmpuint (
        r->id.track_name_hash, ==,
        track_get_name_hash (midi_track));
      g_assert_cmpint (r->id.idx, ==, 1);
    }
  g_assert_cmpint (r->id.lane_pos, ==, MIDI_REGION_LANE);
  g_assert_cmpint (r->num_midi_notes, ==, 1);
  if (new_tracks)
    {
      g_assert_cmpuint (
        r->id.track_name_hash, ==,
        track_get_name_hash (new_midi_track));
    }
  else
    {
      g_assert_cmpuint (
        r->id.track_name_hash, ==,
        track_get_name_hash (midi_track));
    }
  g_assert_cmpint (r->id.lane_pos, ==, MIDI_REGION_LANE);
  mn = r->midi_notes[0];
  obj = (ArrangerObject *) mn;
  g_assert_true (
    region_identifier_is_equal (&obj->region_id, &r->id));
  g_assert_cmpuint (mn->val, ==, MN_VAL);
  g_assert_cmpuint (mn->vel->vel, ==, MN_VEL);
  g_assert_cmppos (&obj->pos, &p1);
  g_assert_cmppos (&obj->end_pos, &p2);
  g_message ("new:");
  region_print (r);
  if (link)
    {
      g_assert_cmpint (r->id.link_group, ==, link_group);
    }

  /* check prev audio region */
  if (new_tracks)
    {
      g_assert_cmpint (
        audio_track->lanes[AUDIO_REGION_LANE]->num_regions,
        ==, 1);
      g_assert_cmpint (
        new_audio_track->lanes[AUDIO_REGION_LANE]->num_regions,
        ==, 1);
    }
  else
    {
      g_assert_cmpint (
        audio_track->lanes[AUDIO_REGION_LANE]->num_regions,
        ==, 2);
    }
  obj =
    (ArrangerObject *) audio_track->lanes[AUDIO_REGION_LANE]
      ->regions[0];
  g_assert_cmppos (&obj->pos, &p1_before_move);
  r = (ZRegion *) obj;
  g_assert_cmpuint (
    r->id.track_name_hash, ==,
    track_get_name_hash (audio_track));
  g_assert_cmpint (r->id.idx, ==, 0);
  g_assert_cmpint (r->id.lane_pos, ==, AUDIO_REGION_LANE);
  link_group = r->id.link_group;
  if (link)
    {
      g_assert_cmpint (r->id.link_group, >, -1);
    }

  /* check new audio region */
  if (new_tracks)
    {
      obj =
        (ArrangerObject *) new_audio_track
          ->lanes[AUDIO_REGION_LANE]
          ->regions[0];
    }
  else
    {
      obj =
        (ArrangerObject *) audio_track
          ->lanes[AUDIO_REGION_LANE]
          ->regions[1];
    }
  g_assert_cmppos (&obj->pos, &p1_after_move);
  r = (ZRegion *) obj;
  g_assert_cmpint (r->id.lane_pos, ==, AUDIO_REGION_LANE);
  if (new_tracks)
    {
      g_assert_cmpuint (
        r->id.track_name_hash, ==,
        track_get_name_hash (new_audio_track));
    }
  else
    {
      g_assert_cmpuint (
        r->id.track_name_hash, ==,
        track_get_name_hash (audio_track));
    }
  if (link)
    {
      g_assert_cmpint (r->id.link_group, ==, link_group);
    }

  if (!new_tracks)
    {
      /* check automation region */
      AutomationTracklist * atl =
        track_get_automation_tracklist (P_MASTER_TRACK);
      g_assert_nonnull (atl);
      AutomationTrack * at = channel_get_automation_track (
        P_MASTER_TRACK->channel, PORT_FLAG_STEREO_BALANCE);
      g_assert_nonnull (at);
      g_assert_cmpint (at->num_regions, ==, 2);
      obj = (ArrangerObject *) at->regions[0];
      g_assert_cmppos (&obj->pos, &p1_before_move);
      g_assert_cmppos (&obj->end_pos, &p2_before_move);
      r = (ZRegion *) obj;
      g_assert_cmpint (r->num_aps, ==, 2);
      AutomationPoint * ap = r->aps[0];
      obj = (ArrangerObject *) ap;
      g_assert_cmppos (&obj->pos, &p1);
      g_assert_cmpfloat_with_epsilon (
        ap->fvalue, AP_VAL1, 0.000001f);
      ap = r->aps[1];
      obj = (ArrangerObject *) ap;
      g_assert_cmppos (&obj->pos, &p2);
      g_assert_cmpfloat_with_epsilon (
        ap->fvalue, AP_VAL2, 0.000001f);
      obj = (ArrangerObject *) at->regions[1];
      g_assert_cmppos (&obj->pos, &p1_after_move);
      g_assert_cmppos (&obj->end_pos, &p2_after_move);
      r = (ZRegion *) obj;
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
      g_assert_cmpint (P_MARKER_TRACK->num_markers, ==, 4);
      obj = (ArrangerObject *) P_MARKER_TRACK->markers[2];
      Marker * m = (Marker *) obj;
      g_assert_cmppos (&obj->pos, &p1_before_move);
      g_assert_cmpstr (m->name, ==, MARKER_NAME);
      obj = (ArrangerObject *) P_MARKER_TRACK->markers[3];
      m = (Marker *) obj;
      g_assert_cmppos (&obj->pos, &p1_after_move);
      g_assert_cmpstr (m->name, ==, MARKER_NAME);

      /* check scale object */
      g_assert_cmpint (P_CHORD_TRACK->num_scales, ==, 2);
      obj = (ArrangerObject *) P_CHORD_TRACK->scales[0];
      ScaleObject * s = (ScaleObject *) obj;
      g_assert_cmppos (&obj->pos, &p1_before_move);
      g_assert_cmpint (s->scale->type, ==, MUSICAL_SCALE_TYPE);
      g_assert_cmpint (
        s->scale->root_key, ==, MUSICAL_SCALE_ROOT);
      obj = (ArrangerObject *) P_CHORD_TRACK->scales[1];
      s = (ScaleObject *) obj;
      g_assert_cmppos (&obj->pos, &p1_after_move);
      g_assert_cmpint (s->scale->type, ==, MUSICAL_SCALE_TYPE);
      g_assert_cmpint (
        s->scale->root_key, ==, MUSICAL_SCALE_ROOT);
    }
}

static void
test_duplicate_timeline (void)
{
  rebootstrap_timeline ();

  /* when i == 1 we are moving to new tracks */
  for (int i = 0; i < 2; i++)
    {
      int track_diff = i ? 2 : 0;
      if (track_diff)
        {
          select_audio_and_midi_regions_only ();
          Track * midi_track = tracklist_find_track_by_name (
            TRACKLIST, MIDI_TRACK_NAME);
          Track * audio_track = tracklist_find_track_by_name (
            TRACKLIST, AUDIO_TRACK_NAME);
          Track * new_midi_track =
            tracklist_find_track_by_name (
              TRACKLIST, TARGET_MIDI_TRACK_NAME);
          Track * new_audio_track =
            tracklist_find_track_by_name (
              TRACKLIST, TARGET_AUDIO_TRACK_NAME);
          region_move_to_track (
            midi_track->lanes[MIDI_REGION_LANE]->regions[0],
            new_midi_track, -1, -1);
          region_move_to_track (
            audio_track->lanes[AUDIO_REGION_LANE]->regions[0],
            new_audio_track, -1, -1);
        }
      /* do move ticks */
      arranger_selections_add_ticks (
        (ArrangerSelections *) TL_SELECTIONS, MOVE_TICKS);

      /* do duplicate */
      arranger_selections_action_perform_duplicate_timeline (
        TL_SELECTIONS, MOVE_TICKS, i > 0 ? 2 : 0, 0, NULL,
        F_ALREADY_MOVED, NULL);

      /* check */
      check_after_duplicate_timeline (i, false);

      /* undo and check that the objects are at
       * their original state*/
      undo_manager_undo (UNDO_MANAGER, NULL);
      check_timeline_objects_vs_original_state (0, 0, 1);

      /* redo and check that the objects are moved
       * again */
      undo_manager_redo (UNDO_MANAGER, NULL);
      check_after_duplicate_timeline (i, false);

      /* undo again to prepare for next test */
      undo_manager_undo (UNDO_MANAGER, NULL);
      check_timeline_objects_vs_original_state (0, 0, 1);
    }

  test_helper_zrythm_cleanup ();
}

static void
test_duplicate_automation_region (void)
{
  rebootstrap_timeline ();

  AutomationTracklist * atl =
    track_get_automation_tracklist (P_MASTER_TRACK);
  g_assert_nonnull (atl);
  AutomationTrack * at = channel_get_automation_track (
    P_MASTER_TRACK->channel, PORT_FLAG_STEREO_BALANCE);
  g_assert_nonnull (at);
  g_assert_cmpint (at->num_regions, ==, 1);

  Position start_pos, end_pos;
  position_init (&start_pos);
  position_set_to_bar (&end_pos, 4);
  ZRegion * r1 = at->regions[0];

  AutomationPoint * ap =
    automation_point_new_float (0.5f, 0.5f, &start_pos);
  automation_region_add_ap (r1, ap, F_NO_PUBLISH_EVENTS);
  arranger_object_select (
    (ArrangerObject *) ap, F_SELECT, F_NO_APPEND,
    F_NO_PUBLISH_EVENTS);
  arranger_selections_action_perform_create (
    AUTOMATION_SELECTIONS, NULL);
  position_add_frames (&start_pos, 14);
  ap = automation_point_new_float (0.6f, 0.6f, &start_pos);
  automation_region_add_ap (r1, ap, F_NO_PUBLISH_EVENTS);
  arranger_object_select (
    (ArrangerObject *) ap, F_SELECT, F_NO_APPEND,
    F_NO_PUBLISH_EVENTS);
  arranger_selections_action_perform_create (
    AUTOMATION_SELECTIONS, NULL);

  float curviness_after = 0.8f;
  ap = r1->aps[0];
  arranger_object_select (
    (ArrangerObject *) ap, F_SELECT, F_NO_APPEND,
    F_NO_PUBLISH_EVENTS);
  ArrangerSelections * before = arranger_selections_clone (
    (ArrangerSelections *) AUTOMATION_SELECTIONS);
  ap->curve_opts.curviness = curviness_after;
  arranger_selections_action_perform_edit (
    before, (ArrangerSelections *) AUTOMATION_SELECTIONS,
    ARRANGER_SELECTIONS_ACTION_EDIT_PRIMITIVE,
    F_ALREADY_EDITED, NULL);

  ap = r1->aps[0];
  g_assert_cmpfloat_with_epsilon (
    ap->curve_opts.curviness, curviness_after, 0.00001f);

  undo_manager_undo (UNDO_MANAGER, NULL);
  ap = r1->aps[0];
  g_assert_cmpfloat_with_epsilon (
    ap->curve_opts.curviness, 0.f, 0.00001f);

  undo_manager_redo (UNDO_MANAGER, NULL);
  ap = r1->aps[0];
  g_assert_cmpfloat_with_epsilon (
    ap->curve_opts.curviness, curviness_after, 0.00001f);

  arranger_object_select (
    (ArrangerObject *) r1, F_SELECT, F_NO_APPEND,
    F_NO_PUBLISH_EVENTS);

  arranger_selections_action_perform_duplicate_timeline (
    TL_SELECTIONS, MOVE_TICKS, 0, 0, NULL,
    F_NOT_ALREADY_MOVED, NULL);

  r1 = at->regions[1];
  ap = r1->aps[0];
  g_assert_cmpfloat_with_epsilon (
    ap->curve_opts.curviness, curviness_after, 0.00001f);

  undo_manager_undo (UNDO_MANAGER, NULL);
  undo_manager_redo (UNDO_MANAGER, NULL);

  r1 = at->regions[1];
  ap = r1->aps[0];
  g_assert_cmpfloat_with_epsilon (
    ap->curve_opts.curviness, curviness_after, 0.00001f);

  test_helper_zrythm_cleanup ();
}

static void
test_link_timeline (void)
{
  rebootstrap_timeline ();

  /* when i == 1 we are moving to new tracks */
  for (int i = 0; i < 2; i++)
    {
      ArrangerSelections * sel_before = NULL;

      int track_diff = i ? 2 : 0;
      if (track_diff)
        {
          select_audio_and_midi_regions_only ();
          sel_before = arranger_selections_clone (
            (ArrangerSelections *) TL_SELECTIONS);

          Track * midi_track = tracklist_find_track_by_name (
            TRACKLIST, MIDI_TRACK_NAME);
          Track * audio_track = tracklist_find_track_by_name (
            TRACKLIST, AUDIO_TRACK_NAME);
          Track * new_midi_track =
            tracklist_find_track_by_name (
              TRACKLIST, TARGET_MIDI_TRACK_NAME);
          Track * new_audio_track =
            tracklist_find_track_by_name (
              TRACKLIST, TARGET_AUDIO_TRACK_NAME);
          region_move_to_track (
            midi_track->lanes[MIDI_REGION_LANE]->regions[0],
            new_midi_track, -1, -1);
          region_move_to_track (
            audio_track->lanes[AUDIO_REGION_LANE]->regions[0],
            new_audio_track, -1, -1);
        }
      else
        {
          sel_before = arranger_selections_clone (
            (ArrangerSelections *) TL_SELECTIONS);
        }

      /* do move ticks */
      arranger_selections_add_ticks (
        (ArrangerSelections *) TL_SELECTIONS, MOVE_TICKS);

      /* do link */
      arranger_selections_action_perform_link (
        sel_before, (ArrangerSelections *) TL_SELECTIONS,
        MOVE_TICKS, i > 0 ? 2 : 0, 0, F_ALREADY_MOVED, NULL);

      /* check */
      check_after_duplicate_timeline (i, true);

      /* undo and check that the objects are at
       * their original state*/
      undo_manager_undo (UNDO_MANAGER, NULL);
      check_timeline_objects_vs_original_state (0, 0, 1);

      /* redo and check that the objects are moved
       * again */
      undo_manager_redo (UNDO_MANAGER, NULL);
      check_after_duplicate_timeline (i, true);

      /* undo again to prepare for next test */
      undo_manager_undo (UNDO_MANAGER, NULL);
      check_timeline_objects_vs_original_state (0, 0, 1);

      g_assert_cmpint (
        REGION_LINK_GROUP_MANAGER->num_groups, ==, 0);
    }

  test_helper_zrythm_cleanup ();
}

static void
test_link_then_duplicate (void)
{
  rebootstrap_timeline ();

  /* when i == 1 we are moving to new tracks */
  for (int i = 0; i < 2; i++)
    {
      ArrangerSelections * sel_before = NULL;

      int track_diff = i ? 2 : 0;
      if (track_diff)
        {
          select_audio_and_midi_regions_only ();
          sel_before = arranger_selections_clone (
            (ArrangerSelections *) TL_SELECTIONS);

          Track * midi_track = tracklist_find_track_by_name (
            TRACKLIST, MIDI_TRACK_NAME);
          Track * audio_track = tracklist_find_track_by_name (
            TRACKLIST, AUDIO_TRACK_NAME);
          Track * new_midi_track =
            tracklist_find_track_by_name (
              TRACKLIST, TARGET_MIDI_TRACK_NAME);
          Track * new_audio_track =
            tracklist_find_track_by_name (
              TRACKLIST, TARGET_AUDIO_TRACK_NAME);
          region_move_to_track (
            midi_track->lanes[MIDI_REGION_LANE]->regions[0],
            new_midi_track, -1, -1);
          region_move_to_track (
            audio_track->lanes[AUDIO_REGION_LANE]->regions[0],
            new_audio_track, -1, -1);
        }
      else
        {
          sel_before = arranger_selections_clone (
            (ArrangerSelections *) TL_SELECTIONS);
        }

      /* do move ticks */
      arranger_selections_add_ticks (
        (ArrangerSelections *) TL_SELECTIONS, MOVE_TICKS);

      /* do link */
      arranger_selections_action_perform_link (
        sel_before, (ArrangerSelections *) TL_SELECTIONS,
        MOVE_TICKS, i > 0 ? 2 : 0, 0, F_ALREADY_MOVED, NULL);

      /* check */
      check_after_duplicate_timeline (i, true);

      if (track_diff)
        {
          g_assert_cmpint (
            REGION_LINK_GROUP_MANAGER->num_groups, ==, 2);
        }
      else
        {
          g_assert_cmpint (
            REGION_LINK_GROUP_MANAGER->num_groups, ==, 4);
          g_assert_cmpint (
            REGION_LINK_GROUP_MANAGER->groups[2]->num_ids, ==,
            2);
        }

      g_message ("-----before duplicate");
      region_link_group_manager_validate (
        REGION_LINK_GROUP_MANAGER);
      region_link_group_manager_print (
        REGION_LINK_GROUP_MANAGER);

      /* duplicate and check that the new objects
       * are not links */
      if (track_diff)
        {
          Track * midi_track = tracklist_find_track_by_name (
            TRACKLIST, MIDI_TRACK_NAME);
          Track * audio_track = tracklist_find_track_by_name (
            TRACKLIST, AUDIO_TRACK_NAME);
          Track * new_midi_track =
            tracklist_find_track_by_name (
              TRACKLIST, TARGET_MIDI_TRACK_NAME);
          Track * new_audio_track =
            tracklist_find_track_by_name (
              TRACKLIST, TARGET_AUDIO_TRACK_NAME);
          ZRegion * r =
            midi_track->lanes[MIDI_REGION_LANE]->regions[0];
          region_move_to_track (r, new_midi_track, -1, -1);
          arranger_object_select (
            (ArrangerObject *) r, F_SELECT, F_NO_APPEND,
            F_NO_PUBLISH_EVENTS);
          r =
            audio_track->lanes[AUDIO_REGION_LANE]->regions[0];
          region_move_to_track (r, new_audio_track, -1, -1);
          arranger_object_select (
            (ArrangerObject *) r, F_SELECT, F_APPEND,
            F_NO_PUBLISH_EVENTS);
        }
      else
        {
          select_audio_and_midi_regions_only ();
        }

      /* do move ticks */
      arranger_selections_add_ticks (
        (ArrangerSelections *) TL_SELECTIONS, MOVE_TICKS);

      if (track_diff)
        {
          g_assert_cmpint (
            REGION_LINK_GROUP_MANAGER->num_groups, ==, 2);
        }
      else
        {
          g_assert_cmpint (
            REGION_LINK_GROUP_MANAGER->num_groups, ==, 4);
          g_assert_cmpint (
            REGION_LINK_GROUP_MANAGER->groups[2]->num_ids, ==,
            2);
        }

      g_message ("-----before duplicate perform");
      region_link_group_manager_validate (
        REGION_LINK_GROUP_MANAGER);
      region_link_group_manager_print (
        REGION_LINK_GROUP_MANAGER);
      /*g_warn_if_reached ();*/

      /* do duplicate */
      arranger_selections_action_perform_duplicate_timeline (
        TL_SELECTIONS, MOVE_TICKS, i > 0 ? 2 : 0, 0, NULL,
        F_ALREADY_MOVED, NULL);

      /* check that new objects have no links */
      for (int j = 0; j < TL_SELECTIONS->num_regions; j++)
        {
          ZRegion * r = TL_SELECTIONS->regions[j];
          g_assert_nonnull (r);
          g_assert_cmpint (r->id.link_group, ==, -1);
          g_assert_false (region_has_link_group (r));
        }

      if (track_diff)
        {
          g_assert_cmpint (
            REGION_LINK_GROUP_MANAGER->num_groups, ==, 2);
        }
      else
        {
          g_assert_cmpint (
            REGION_LINK_GROUP_MANAGER->num_groups, ==, 4);
          g_assert_cmpint (
            REGION_LINK_GROUP_MANAGER->groups[2]->num_ids, ==,
            2);
        }

      test_project_save_and_reload ();

      /* add a midi note to a linked midi region */
      ZRegion * r = NULL;
      if (track_diff)
        {
          Track * new_midi_track =
            tracklist_find_track_by_name (
              TRACKLIST, TARGET_MIDI_TRACK_NAME);
          r =
            new_midi_track->lanes[MIDI_REGION_LANE]->regions[0];
          g_assert_true (IS_REGION_AND_NONNULL (r));
        }
      else
        {
          Track * midi_track = tracklist_find_track_by_name (
            TRACKLIST, MIDI_TRACK_NAME);
          r = midi_track->lanes[MIDI_REGION_LANE]->regions[1];
          g_assert_true (IS_REGION_AND_NONNULL (r));
        }
      g_assert_true (region_has_link_group (r));
      Position start, end;
      position_set_to_bar (&start, 1);
      position_set_to_bar (&end, 2);
      MidiNote * mn =
        midi_note_new (&r->id, &start, &end, 45, 45);
      midi_region_add_midi_note (r, mn, F_NO_PUBLISH_EVENTS);
      arranger_object_select (
        (ArrangerObject *) mn, F_SELECT, F_NO_APPEND,
        F_NO_PUBLISH_EVENTS);
      arranger_selections_action_perform_create (
        (ArrangerSelections *) MA_SELECTIONS, NULL);

      /* undo MIDI note */
      undo_manager_undo (UNDO_MANAGER, NULL);

      /* undo duplicate */
      undo_manager_undo (UNDO_MANAGER, NULL);

      test_project_save_and_reload ();

      /* undo and check that the objects are at
       * their original state*/
      undo_manager_undo (UNDO_MANAGER, NULL);
      check_timeline_objects_vs_original_state (0, 0, false);

      test_project_save_and_reload ();

      /* redo and check that the objects are moved
       * again */
      undo_manager_redo (UNDO_MANAGER, NULL);
      check_after_duplicate_timeline (i, true);

      test_project_save_and_reload ();

      /* undo again to prepare for next test */
      undo_manager_undo (UNDO_MANAGER, NULL);
      check_timeline_objects_vs_original_state (0, 0, false);

      g_assert_cmpint (
        REGION_LINK_GROUP_MANAGER->num_groups, ==, 0);
    }

  test_helper_zrythm_cleanup ();
}

static void
test_edit_marker (void)
{
  rebootstrap_timeline ();

  /* create marker with name "aa" */
  Marker *         marker = marker_new ("aa");
  ArrangerObject * marker_obj = (ArrangerObject *) marker;
  arranger_object_select (
    marker_obj, F_SELECT, F_NO_APPEND, F_NO_PUBLISH_EVENTS);
  bool success = arranger_object_add_to_project (
    marker_obj, F_NO_PUBLISH_EVENTS, NULL);
  g_assert_true (success);
  arranger_selections_action_perform_create (
    TL_SELECTIONS, NULL);

  /* change name */
  ArrangerSelections * clone_sel = arranger_selections_clone (
    (ArrangerSelections *) TL_SELECTIONS);
  Marker * m = ((TimelineSelections *) clone_sel)->markers[0];
  arranger_object_set_name (
    (ArrangerObject *) m, "bb", F_NO_PUBLISH_EVENTS);
  arranger_selections_action_perform_edit (
    (ArrangerSelections *) TL_SELECTIONS, clone_sel,
    ARRANGER_SELECTIONS_ACTION_EDIT_NAME,
    F_NOT_ALREADY_EDITED, NULL);

  /* assert name changed */
  g_assert_true (string_is_equal (marker->name, "bb"));

  undo_manager_undo (UNDO_MANAGER, NULL);
  g_assert_true (string_is_equal (marker->name, "aa"));

  /* undo again and check that all objects are at
   * original state */
  undo_manager_undo (UNDO_MANAGER, NULL);

  /* redo and check that the name is changed */
  undo_manager_redo (UNDO_MANAGER, NULL);
  undo_manager_redo (UNDO_MANAGER, NULL);

  /* return to original state */
  undo_manager_undo (UNDO_MANAGER, NULL);
  undo_manager_undo (UNDO_MANAGER, NULL);

  test_helper_zrythm_cleanup ();
}

static void
test_mute (void)
{
  rebootstrap_timeline ();

  Track * midi_track = TRACKLIST->tracks[5];
  g_assert_true (midi_track->type == TRACK_TYPE_MIDI);

  ZRegion * r =
    midi_track->lanes[MIDI_REGION_LANE]->regions[0];
  ArrangerObject * obj = (ArrangerObject *) r;
  g_assert_true (IS_ARRANGER_OBJECT (obj));

  arranger_selections_action_perform_edit (
    (ArrangerSelections *) TL_SELECTIONS, NULL,
    ARRANGER_SELECTIONS_ACTION_EDIT_MUTE,
    F_NOT_ALREADY_EDITED, NULL);

  /* assert muted */
  g_assert_true (IS_ARRANGER_OBJECT (obj));
  g_assert_true (obj->muted);

  undo_manager_undo (UNDO_MANAGER, NULL);
  g_assert_true (IS_ARRANGER_OBJECT (obj));
  g_assert_false (obj->muted);

  /* redo and recheck */
  undo_manager_redo (UNDO_MANAGER, NULL);
  g_assert_true (IS_ARRANGER_OBJECT (obj));
  g_assert_true (obj->muted);

  /* return to original state */
  undo_manager_undo (UNDO_MANAGER, NULL);
  g_assert_true (IS_ARRANGER_OBJECT (obj));
  g_assert_false (obj->muted);

  test_helper_zrythm_cleanup ();
}

static void
test_split (void)
{
  rebootstrap_timeline ();

  g_assert_cmpint (P_CHORD_TRACK->num_chord_regions, ==, 1);

  Position pos, end_pos;
  position_set_to_bar (&pos, 2);
  position_set_to_bar (&end_pos, 4);
  ZRegion *        r = chord_region_new (&pos, &end_pos, 0);
  ArrangerObject * r_obj = (ArrangerObject *) r;
  GError *         err = NULL;
  bool             success = track_add_region (
    P_CHORD_TRACK, r, NULL, -1, F_GEN_NAME,
    F_NO_PUBLISH_EVENTS, &err);
  g_assert_true (success);
  arranger_object_select (
    r_obj, F_SELECT, F_NO_APPEND, F_NO_PUBLISH_EVENTS);

  arranger_selections_action_perform_create (
    TL_SELECTIONS, NULL);

  g_assert_cmpint (P_CHORD_TRACK->num_chord_regions, ==, 2);

  position_set_to_bar (&pos, 3);
  arranger_selections_action_perform_split (
    (ArrangerSelections *) TL_SELECTIONS, &pos, NULL);

  g_assert_cmpint (P_CHORD_TRACK->num_chord_regions, ==, 3);

  Track * track2 = tracklist_find_track_by_name (
    TRACKLIST, AUDIO_TRACK_NAME);
  TrackLane * lane2 = track2->lanes[3];
  g_assert_cmpint (lane2->num_regions, ==, 1);

  ZRegion * region2 = lane2->regions[0];
  region_validate (region2, false, 0);

  undo_manager_undo (UNDO_MANAGER, NULL);
  undo_manager_redo (UNDO_MANAGER, NULL);
  undo_manager_undo (UNDO_MANAGER, NULL);
  undo_manager_undo (UNDO_MANAGER, NULL);

  /* --- test audio region split --- */

  Track * track = tracklist_find_track_by_name (
    TRACKLIST, AUDIO_TRACK_NAME);
  TrackLane * lane = track->lanes[3];
  g_assert_cmpint (lane->num_regions, ==, 1);

  ZRegion * region = lane->regions[0];
  region_validate (region, false, 0);
  r_obj = (ArrangerObject *) region;
  arranger_object_select (
    r_obj, F_SELECT, F_NO_APPEND, F_NO_PUBLISH_EVENTS);
  AudioClip * clip = audio_region_get_clip (region);
  float       first_frame = clip->frames[0];

  arranger_object_print (r_obj);

  position_set_to_bar (&pos, 2);
  position_print (&pos);
  position_add_beats (&pos, 1);
  position_print (&pos);
  arranger_selections_action_perform_split (
    (ArrangerSelections *) TL_SELECTIONS, &pos, NULL);

  test_project_save_and_reload ();

  /* check that clip frames are the same as
   * before */
  track = tracklist_find_track_by_name (
    TRACKLIST, AUDIO_TRACK_NAME);
  lane = track->lanes[3];
  region = lane->regions[0];
  r_obj = (ArrangerObject *) region;
  clip = audio_region_get_clip (region);
  g_assert_cmpfloat_with_epsilon (
    first_frame, clip->frames[0], 0.000001f);

  undo_manager_undo (UNDO_MANAGER, NULL);

  test_helper_zrythm_cleanup ();
}

static void
test_split_large_audio_file (void)
{
#ifdef TEST_SINE_OGG_30MIN
  test_helper_zrythm_init ();

  Track * track = track_new (
    TRACK_TYPE_AUDIO, TRACKLIST->num_tracks, "test track",
    F_WITH_LANE);
  tracklist_append_track (
    TRACKLIST, track, F_NO_PUBLISH_EVENTS, F_NO_RECALC_GRAPH);
  unsigned int track_name_hash = track_get_name_hash (track);
  Position     pos;
  position_set_to_bar (&pos, 3);
  ZRegion * r = audio_region_new (
    -1, TEST_SINE_OGG_30MIN, true, NULL, 0, NULL, 0, 0, &pos,
    track_name_hash, AUDIO_REGION_LANE, 0);
  AudioClip * clip = audio_region_get_clip (r);
  g_assert_cmpuint (clip->num_frames, ==, 79380000);
  GError * err = NULL;
  bool     success = track_add_region (
    track, r, NULL, 0, F_GEN_NAME, F_NO_PUBLISH_EVENTS, &err);
  g_assert_true (success);
  arranger_selections_add_object (
    (ArrangerSelections *) TL_SELECTIONS,
    (ArrangerObject *) r);

  /* attempt split */
  position_set_to_bar (&pos, 4);
  arranger_selections_action_perform_split (
    (ArrangerSelections *) TL_SELECTIONS, &pos, NULL);

  test_helper_zrythm_cleanup ();
#endif
}

static void
test_quantize (void)
{
  rebootstrap_timeline ();

  Track * audio_track = tracklist_find_track_by_name (
    TRACKLIST, AUDIO_TRACK_NAME);
  g_assert_true (audio_track->type == TRACK_TYPE_AUDIO);

  arranger_selections_action_perform_quantize (
    (ArrangerSelections *) AUDIO_SELECTIONS,
    QUANTIZE_OPTIONS_EDITOR, NULL);

  /* TODO test audio/MIDI quantization */
  TrackLane * lane = audio_track->lanes[3];
  ZRegion *   region = lane->regions[0];
  g_assert_true (IS_ARRANGER_OBJECT (region));

  /* return to original state */
  undo_manager_undo (UNDO_MANAGER, NULL);

  test_helper_zrythm_cleanup ();
}

static void
verify_audio_function (float * frames, size_t max_frames)
{
  Track * track = tracklist_find_track_by_name (
    TRACKLIST, AUDIO_TRACK_NAME);
  TrackLane * lane = track->lanes[3];
  ZRegion *   region = lane->regions[0];
  AudioClip * clip = audio_region_get_clip (region);
  size_t      num_frames =
    MIN (max_frames, (size_t) clip->num_frames);
  for (size_t i = 0; i < num_frames; i++)
    {
      for (size_t j = 0; j < clip->channels; j++)
        {
          g_assert_cmpfloat_with_epsilon (
            frames[clip->channels * i + j],
            clip->frames[clip->channels * i + j], 0.0001f);
        }
    }
}

static void
test_audio_functions (void)
{
  rebootstrap_timeline ();

  Track * track = tracklist_find_track_by_name (
    TRACKLIST, AUDIO_TRACK_NAME);
  TrackLane * lane = track->lanes[3];
  g_assert_cmpint (lane->num_regions, ==, 1);

  ZRegion *        region = lane->regions[0];
  ArrangerObject * r_obj = (ArrangerObject *) region;
  arranger_object_select (
    r_obj, F_SELECT, F_NO_APPEND, F_NO_PUBLISH_EVENTS);
  AUDIO_SELECTIONS->region_id = region->id;
  AUDIO_SELECTIONS->has_selection = true;
  AUDIO_SELECTIONS->sel_start = r_obj->pos;
  AUDIO_SELECTIONS->sel_end = r_obj->end_pos;

  AudioClip * orig_clip = audio_region_get_clip (region);
  size_t      channels = orig_clip->channels;
  size_t frames_per_channel = (size_t) orig_clip->num_frames;
  size_t total_frames =
    (size_t) orig_clip->num_frames * channels;
  float * orig_frames = object_new_n (total_frames, float);
  float * inverted_frames = object_new_n (total_frames, float);
  dsp_copy (orig_frames, orig_clip->frames, total_frames);
  dsp_copy (inverted_frames, orig_clip->frames, total_frames);
  dsp_mul_k2 (inverted_frames, -1.f, total_frames);

  verify_audio_function (orig_frames, frames_per_channel);

  /* invert */
  arranger_selections_action_perform_edit_audio_function (
    (ArrangerSelections *) AUDIO_SELECTIONS,
    AUDIO_FUNCTION_INVERT, NULL, NULL);

  verify_audio_function (inverted_frames, frames_per_channel);

  test_project_save_and_reload ();

  undo_manager_undo (UNDO_MANAGER, NULL);

  verify_audio_function (orig_frames, frames_per_channel);

  undo_manager_redo (UNDO_MANAGER, NULL);

  /* verify that frames are edited again */
  verify_audio_function (inverted_frames, frames_per_channel);

  undo_manager_undo (UNDO_MANAGER, NULL);

  free (orig_frames);
  free (inverted_frames);

  test_helper_zrythm_cleanup ();
}

static void
test_automation_fill (void)
{
  rebootstrap_timeline ();

  /* check automation region */
  AutomationTracklist * atl =
    track_get_automation_tracklist (P_MASTER_TRACK);
  g_assert_nonnull (atl);
  AutomationTrack * at = channel_get_automation_track (
    P_MASTER_TRACK->channel, PORT_FLAG_STEREO_BALANCE);
  g_assert_nonnull (at);
  g_assert_cmpint (at->num_regions, ==, 1);

  Position start_pos, end_pos;
  position_init (&start_pos);
  position_set_to_bar (&end_pos, 4);
  ZRegion * r1 = at->regions[0];

  ZRegion * r1_clone =
    (ZRegion *) arranger_object_clone ((ArrangerObject *) r1);

  AutomationPoint * ap =
    automation_point_new_float (0.5f, 0.5f, &start_pos);
  automation_region_add_ap (r1, ap, F_NO_PUBLISH_EVENTS);
  arranger_object_select (
    (ArrangerObject *) ap, F_SELECT, F_NO_APPEND,
    F_NO_PUBLISH_EVENTS);
  position_add_frames (&start_pos, 14);
  ap = automation_point_new_float (0.6f, 0.6f, &start_pos);
  automation_region_add_ap (r1, ap, F_NO_PUBLISH_EVENTS);
  arranger_object_select (
    (ArrangerObject *) ap, F_SELECT, F_APPEND,
    F_NO_PUBLISH_EVENTS);
  arranger_selections_action_perform_automation_fill (
    r1_clone, r1, F_ALREADY_EDITED, NULL);

  undo_manager_undo (UNDO_MANAGER, NULL);
  undo_manager_redo (UNDO_MANAGER, NULL);

  test_helper_zrythm_cleanup ();
}

static void
test_duplicate_midi_regions_to_track_below (void)
{
  rebootstrap_timeline ();

  Track * midi_track =
    tracklist_find_track_by_name (TRACKLIST, MIDI_TRACK_NAME);
  TrackLane * lane = midi_track->lanes[0];
  g_assert_cmpint (lane->num_regions, ==, 0);

  Position pos, end_pos;
  position_set_to_bar (&pos, 2);
  position_set_to_bar (&end_pos, 4);
  ZRegion * r1 = midi_region_new (
    &pos, &end_pos, track_get_name_hash (midi_track), 0,
    lane->num_regions);
  GError * err = NULL;
  bool     success = track_add_region (
    midi_track, r1, NULL, lane->pos, F_GEN_NAME,
    F_NO_PUBLISH_EVENTS, &err);
  g_assert_true (success);
  arranger_object_select (
    (ArrangerObject *) r1, F_SELECT, F_NO_APPEND,
    F_PUBLISH_EVENTS);
  arranger_selections_action_perform_create (
    TL_SELECTIONS, NULL);
  g_assert_cmpint (lane->num_regions, ==, 1);

  position_set_to_bar (&pos, 5);
  position_set_to_bar (&end_pos, 7);
  ZRegion * r2 = midi_region_new (
    &pos, &end_pos, track_get_name_hash (midi_track), 0,
    lane->num_regions);
  success = track_add_region (
    midi_track, r2, NULL, lane->pos, F_GEN_NAME,
    F_NO_PUBLISH_EVENTS, &err);
  g_assert_true (success);
  arranger_object_select (
    (ArrangerObject *) r2, F_SELECT, F_NO_APPEND,
    F_PUBLISH_EVENTS);
  arranger_selections_action_perform_create (
    TL_SELECTIONS, NULL);
  g_assert_cmpint (lane->num_regions, ==, 2);

  /* select the regions */
  arranger_object_select (
    (ArrangerObject *) r2, F_SELECT, F_NO_APPEND,
    F_PUBLISH_EVENTS);
  arranger_object_select (
    (ArrangerObject *) r1, F_SELECT, F_APPEND,
    F_PUBLISH_EVENTS);

  Track * new_midi_track = tracklist_find_track_by_name (
    TRACKLIST, TARGET_MIDI_TRACK_NAME);
  TrackLane * target_lane = new_midi_track->lanes[0];
  g_assert_cmpint (target_lane->num_regions, ==, 0);

  double ticks = 100.0;

  /* replicate the logic from the arranger */
  timeline_selections_set_index_in_prev_lane (TL_SELECTIONS);
  timeline_selections_move_regions_to_new_tracks (
    TL_SELECTIONS, new_midi_track->pos - midi_track->pos);
  arranger_selections_add_ticks (
    (ArrangerSelections *) TL_SELECTIONS, ticks);
  g_assert_cmpint (target_lane->num_regions, ==, 2);
  g_assert_cmpint (TL_SELECTIONS->num_regions, ==, 2);

  g_debug ("selections:");
  region_print (TL_SELECTIONS->regions[0]);
  region_print (TL_SELECTIONS->regions[1]);

  arranger_selections_action_perform_duplicate (
    TL_SELECTIONS, ticks, 0, 0,
    new_midi_track->pos - midi_track->pos, 0, 0, NULL,
    F_ALREADY_MOVED, NULL);

  /* check that new regions are created */
  g_assert_cmpint (target_lane->num_regions, ==, 2);

  undo_manager_undo (UNDO_MANAGER, NULL);

  /* check that new regions are deleted */
  g_assert_cmpint (target_lane->num_regions, ==, 0);

  undo_manager_redo (UNDO_MANAGER, NULL);

  /* check that new regions are created */
  g_assert_cmpint (target_lane->num_regions, ==, 2);

  test_helper_zrythm_cleanup ();
}

static void
test_midi_region_split (void)
{
  rebootstrap_timeline ();

  Track * midi_track =
    tracklist_find_track_by_name (TRACKLIST, MIDI_TRACK_NAME);
  TrackLane * lane = midi_track->lanes[0];
  g_assert_cmpint (lane->num_regions, ==, 0);

  Position pos, end_pos;
  position_set_to_bar (&pos, 1);
  position_set_to_bar (&end_pos, 5);
  ZRegion * r = midi_region_new (
    &pos, &end_pos, track_get_name_hash (midi_track), 0,
    lane->num_regions);
  GError * err = NULL;
  bool     success = track_add_region (
    midi_track, r, NULL, lane->pos, F_GEN_NAME,
    F_NO_PUBLISH_EVENTS, &err);
  g_assert_true (success);
  arranger_object_select (
    (ArrangerObject *) r, F_SELECT, F_NO_APPEND,
    F_PUBLISH_EVENTS);
  arranger_selections_action_perform_create (
    TL_SELECTIONS, NULL);
  g_assert_cmpint (lane->num_regions, ==, 1);

  /* create some MIDI notes */
  for (int i = 0; i < 4; i++)
    {
      position_set_to_bar (&pos, i + 1);
      position_set_to_bar (&end_pos, i + 2);
      MidiNote * mn =
        midi_note_new (&r->id, &pos, &end_pos, 34 + i, 70);
      midi_region_add_midi_note (r, mn, F_NO_PUBLISH_EVENTS);
      arranger_object_select (
        (ArrangerObject *) mn, F_SELECT, F_NO_APPEND,
        F_NO_PUBLISH_EVENTS);
      arranger_selections_action_perform_create (
        MA_SELECTIONS, NULL);
      g_assert_cmpint (r->num_midi_notes, ==, i + 1);
    }

  /* select the region */
  arranger_object_select (
    (ArrangerObject *) r, F_SELECT, F_NO_APPEND,
    F_NO_PUBLISH_EVENTS);

  /* split at bar 2 */
  position_set_to_bar (&pos, 2);
  arranger_selections_action_perform_split (
    (ArrangerSelections *) TL_SELECTIONS, &pos, NULL);
  g_assert_cmpint (lane->num_regions, ==, 2);
  r = lane->regions[1];
  g_assert_cmpint (pos.frames, ==, r->base.pos.frames);
  position_set_to_bar (&pos, 5);
  g_assert_cmpint (pos.frames, ==, r->base.end_pos.frames);

  /* split at bar 4 */
  position_set_to_bar (&pos, 4);
  arranger_object_select (
    (ArrangerObject *) r, F_SELECT, F_NO_APPEND,
    F_NO_PUBLISH_EVENTS);
  arranger_selections_action_perform_split (
    (ArrangerSelections *) TL_SELECTIONS, &pos, NULL);
  g_assert_cmpint (lane->num_regions, ==, 3);

  r = lane->regions[0];
  position_set_to_bar (&pos, 1);
  g_assert_cmpint (pos.frames, ==, r->base.pos.frames);
  position_set_to_bar (&pos, 2);
  g_assert_cmpint (pos.frames, ==, r->base.end_pos.frames);

  r = lane->regions[1];
  position_set_to_bar (&pos, 2);
  g_assert_cmpint (pos.frames, ==, r->base.pos.frames);
  position_set_to_bar (&pos, 4);
  g_assert_cmpint (pos.frames, ==, r->base.end_pos.frames);

  r = lane->regions[2];
  position_set_to_bar (&pos, 4);
  g_assert_cmpint (pos.frames, ==, r->base.pos.frames);
  position_set_to_bar (&pos, 5);
  g_assert_cmpint (pos.frames, ==, r->base.end_pos.frames);

  /* split at bar 3 */
  r = lane->regions[1];
  arranger_object_select (
    (ArrangerObject *) r, F_SELECT, F_NO_APPEND,
    F_NO_PUBLISH_EVENTS);
  position_set_to_bar (&pos, 3);
  arranger_object_select (
    (ArrangerObject *) r, F_SELECT, F_NO_APPEND,
    F_NO_PUBLISH_EVENTS);
  arranger_selections_action_perform_split (
    (ArrangerSelections *) TL_SELECTIONS, &pos, NULL);
  g_assert_cmpint (lane->num_regions, ==, 4);

  r = lane->regions[0];
  position_set_to_bar (&pos, 1);
  g_assert_cmpint (pos.frames, ==, r->base.pos.frames);
  position_set_to_bar (&pos, 2);
  g_assert_cmpint (pos.frames, ==, r->base.end_pos.frames);

  r = lane->regions[1];
  position_set_to_bar (&pos, 4);
  g_assert_cmpint (pos.frames, ==, r->base.pos.frames);
  position_set_to_bar (&pos, 5);
  g_assert_cmpint (pos.frames, ==, r->base.end_pos.frames);

  r = lane->regions[2];
  position_set_to_bar (&pos, 2);
  g_assert_cmpint (pos.frames, ==, r->base.pos.frames);
  position_set_to_bar (&pos, 3);
  g_assert_cmpint (pos.frames, ==, r->base.end_pos.frames);

  r = lane->regions[3];
  position_set_to_bar (&pos, 3);
  g_assert_cmpint (pos.frames, ==, r->base.pos.frames);
  position_set_to_bar (&pos, 4);
  g_assert_cmpint (pos.frames, ==, r->base.end_pos.frames);

  /* undo and verify */
  undo_manager_undo (UNDO_MANAGER, NULL);
  g_assert_cmpint (lane->num_regions, ==, 3);

  r = lane->regions[0];
  position_set_to_bar (&pos, 1);
  g_assert_cmpint (pos.frames, ==, r->base.pos.frames);
  position_set_to_bar (&pos, 2);
  g_assert_cmpint (pos.frames, ==, r->base.end_pos.frames);

  r = lane->regions[1];
  position_set_to_bar (&pos, 2);
  g_assert_cmpint (pos.frames, ==, r->base.pos.frames);
  position_set_to_bar (&pos, 4);
  g_assert_cmpint (pos.frames, ==, r->base.end_pos.frames);

  r = lane->regions[2];
  position_set_to_bar (&pos, 4);
  g_assert_cmpint (pos.frames, ==, r->base.pos.frames);
  position_set_to_bar (&pos, 5);
  g_assert_cmpint (pos.frames, ==, r->base.end_pos.frames);

  /* undo and verify */
  undo_manager_undo (UNDO_MANAGER, NULL);
  g_assert_cmpint (lane->num_regions, ==, 2);

  r = lane->regions[0];
  position_set_to_bar (&pos, 1);
  g_assert_cmpint (pos.frames, ==, r->base.pos.frames);
  position_set_to_bar (&pos, 2);
  g_assert_cmpint (pos.frames, ==, r->base.end_pos.frames);

  r = lane->regions[1];
  position_set_to_bar (&pos, 2);
  g_assert_cmpint (pos.frames, ==, r->base.pos.frames);
  position_set_to_bar (&pos, 5);
  g_assert_cmpint (pos.frames, ==, r->base.end_pos.frames);

  /* undo and verify */
  undo_manager_undo (UNDO_MANAGER, NULL);
  g_assert_cmpint (lane->num_regions, ==, 1);

  r = lane->regions[0];
  position_set_to_bar (&pos, 1);
  g_assert_cmpint (pos.frames, ==, r->base.pos.frames);
  position_set_to_bar (&pos, 5);
  g_assert_cmpint (pos.frames, ==, r->base.end_pos.frames);

  /* redo to bring 3 regions back */
  undo_manager_redo (UNDO_MANAGER, NULL);
  g_assert_cmpint (lane->num_regions, ==, 2);

  r = lane->regions[0];
  position_set_to_bar (&pos, 1);
  g_assert_cmpint (pos.frames, ==, r->base.pos.frames);
  position_set_to_bar (&pos, 2);
  g_assert_cmpint (pos.frames, ==, r->base.end_pos.frames);

  r = lane->regions[1];
  position_set_to_bar (&pos, 2);
  g_assert_cmpint (pos.frames, ==, r->base.pos.frames);
  position_set_to_bar (&pos, 5);
  g_assert_cmpint (pos.frames, ==, r->base.end_pos.frames);

  undo_manager_redo (UNDO_MANAGER, NULL);
  g_assert_cmpint (lane->num_regions, ==, 3);

  r = lane->regions[0];
  position_set_to_bar (&pos, 1);
  g_assert_cmpint (pos.frames, ==, r->base.pos.frames);
  position_set_to_bar (&pos, 2);
  g_assert_cmpint (pos.frames, ==, r->base.end_pos.frames);

  r = lane->regions[1];
  position_set_to_bar (&pos, 2);
  g_assert_cmpint (pos.frames, ==, r->base.pos.frames);
  position_set_to_bar (&pos, 4);
  g_assert_cmpint (pos.frames, ==, r->base.end_pos.frames);

  r = lane->regions[2];
  position_set_to_bar (&pos, 4);
  g_assert_cmpint (pos.frames, ==, r->base.pos.frames);
  position_set_to_bar (&pos, 5);
  g_assert_cmpint (pos.frames, ==, r->base.end_pos.frames);

  /* delete middle cut */
  r = lane->regions[1];
  arranger_object_select (
    (ArrangerObject *) r, F_SELECT, F_NO_APPEND,
    F_NO_PUBLISH_EVENTS);
  arranger_selections_action_perform_delete (
    (ArrangerSelections *) TL_SELECTIONS, NULL);

  g_assert_cmpint (lane->num_regions, ==, 2);

  r = lane->regions[0];
  position_set_to_bar (&pos, 1);
  g_assert_cmpint (pos.frames, ==, r->base.pos.frames);
  position_set_to_bar (&pos, 2);
  g_assert_cmpint (pos.frames, ==, r->base.end_pos.frames);

  r = lane->regions[1];
  position_set_to_bar (&pos, 4);
  g_assert_cmpint (pos.frames, ==, r->base.pos.frames);
  position_set_to_bar (&pos, 5);
  g_assert_cmpint (pos.frames, ==, r->base.end_pos.frames);

  /* undo to bring it back */
  undo_manager_undo (UNDO_MANAGER, NULL);
  g_assert_cmpint (lane->num_regions, ==, 3);

  r = lane->regions[0];
  position_set_to_bar (&pos, 1);
  g_assert_cmpint (pos.frames, ==, r->base.pos.frames);
  position_set_to_bar (&pos, 2);
  g_assert_cmpint (pos.frames, ==, r->base.end_pos.frames);

  r = lane->regions[1];
  position_set_to_bar (&pos, 2);
  g_assert_cmpint (pos.frames, ==, r->base.pos.frames);
  position_set_to_bar (&pos, 4);
  g_assert_cmpint (pos.frames, ==, r->base.end_pos.frames);

  r = lane->regions[2];
  position_set_to_bar (&pos, 4);
  g_assert_cmpint (pos.frames, ==, r->base.pos.frames);
  position_set_to_bar (&pos, 5);
  g_assert_cmpint (pos.frames, ==, r->base.end_pos.frames);

  test_helper_zrythm_cleanup ();
}

static void
test_pin_unpin (void)
{
  rebootstrap_timeline ();

  ZRegion * r = P_CHORD_TRACK->chord_regions[0];
  track_select (
    P_CHORD_TRACK, F_SELECT, F_EXCLUSIVE, F_NO_PUBLISH_EVENTS);
  tracklist_selections_action_perform_unpin (
    TRACKLIST_SELECTIONS, PORT_CONNECTIONS_MGR, NULL);

  g_assert_cmpuint (
    r->id.track_name_hash, ==,
    track_get_name_hash (P_CHORD_TRACK));

  /* TODO more tests */

  test_helper_zrythm_cleanup ();
}

static void
test_delete_markers (void)
{
  rebootstrap_timeline ();

  Marker *m = NULL, *m_c = NULL, *m_d = NULL;

  /* create markers A B C D */
  const char * names[4] = { "A", "B", "C", "D" };
  for (int i = 0; i < 4; i++)
    {
      m = marker_new (names[i]);
      marker_track_add_marker (P_MARKER_TRACK, m);
      arranger_object_select (
        (ArrangerObject *) m, F_SELECT, F_NO_APPEND,
        F_NO_PUBLISH_EVENTS);
      arranger_selections_action_perform_create (
        TL_SELECTIONS, NULL);

      if (i == 2)
        {
          m_c = m;
        }
      else if (i == 3)
        {
          m_d = m;
        }
    }

  /* delete C */
  arranger_object_select (
    (ArrangerObject *) m_c, F_SELECT, F_NO_APPEND,
    F_NO_PUBLISH_EVENTS);
  arranger_selections_action_perform_delete (
    TL_SELECTIONS, NULL);

  /* delete D */
  arranger_object_select (
    (ArrangerObject *) m_d, F_SELECT, F_NO_APPEND,
    F_NO_PUBLISH_EVENTS);
  arranger_selections_action_perform_delete (
    TL_SELECTIONS, NULL);

  for (int i = 0; i < 6; i++)
    {
      undo_manager_undo (UNDO_MANAGER, NULL);
    }

  test_helper_zrythm_cleanup ();
}

static void
test_delete_scale_objects (void)
{
  rebootstrap_timeline ();

  ScaleObject *m = NULL, *m_c = NULL, *m_d = NULL;

  /* create markers A B C D */
  for (int i = 0; i < 4; i++)
    {
      MusicalScale * ms = musical_scale_new (i, i);
      m = scale_object_new (ms);
      chord_track_add_scale (P_CHORD_TRACK, m);
      arranger_object_select (
        (ArrangerObject *) m, F_SELECT, F_NO_APPEND,
        F_NO_PUBLISH_EVENTS);
      arranger_selections_action_perform_create (
        TL_SELECTIONS, NULL);

      if (i == 2)
        {
          m_c = m;
        }
      else if (i == 3)
        {
          m_d = m;
        }
    }

  /* delete C */
  arranger_object_select (
    (ArrangerObject *) m_c, F_SELECT, F_NO_APPEND,
    F_NO_PUBLISH_EVENTS);
  arranger_selections_action_perform_delete (
    TL_SELECTIONS, NULL);

  /* delete D */
  arranger_object_select (
    (ArrangerObject *) m_d, F_SELECT, F_NO_APPEND,
    F_NO_PUBLISH_EVENTS);
  arranger_selections_action_perform_delete (
    TL_SELECTIONS, NULL);

  for (int i = 0; i < 6; i++)
    {
      undo_manager_undo (UNDO_MANAGER, NULL);
    }

  test_helper_zrythm_cleanup ();
}

static void
test_delete_chord_objects (void)
{
  rebootstrap_timeline ();

  ChordObject *m = NULL, *m_c = NULL, *m_d = NULL;

  Position pos1, pos2;
  position_set_to_bar (&pos1, 1);
  position_set_to_bar (&pos2, 4);
  ZRegion * r = chord_region_new (&pos1, &pos2, 0);
  GError *  err = NULL;
  bool      success = track_add_region (
    P_CHORD_TRACK, r, NULL, 0, F_GEN_NAME, 0, &err);
  g_assert_true (success);
  arranger_selections_add_object (
    (ArrangerSelections *) TL_SELECTIONS,
    (ArrangerObject *) r);
  arranger_selections_action_perform_create (
    TL_SELECTIONS, NULL);

  /* create markers A B C D */
  for (int i = 0; i < 4; i++)
    {
      m = chord_object_new (&r->id, i, i);
      chord_region_add_chord_object (
        r, m, F_NO_PUBLISH_EVENTS);
      arranger_object_select (
        (ArrangerObject *) m, F_SELECT, F_NO_APPEND,
        F_NO_PUBLISH_EVENTS);
      arranger_selections_action_perform_create (
        CHORD_SELECTIONS, NULL);

      if (i == 2)
        {
          m_c = m;
        }
      else if (i == 3)
        {
          m_d = m;
        }
    }

  /* delete C */
  arranger_object_select (
    (ArrangerObject *) m_c, F_SELECT, F_NO_APPEND,
    F_NO_PUBLISH_EVENTS);
  arranger_selections_action_perform_delete (
    CHORD_SELECTIONS, NULL);

  /* delete D */
  arranger_object_select (
    (ArrangerObject *) m_d, F_SELECT, F_NO_APPEND,
    F_NO_PUBLISH_EVENTS);
  arranger_selections_action_perform_delete (
    CHORD_SELECTIONS, NULL);

  for (int i = 0; i < 6; i++)
    {
      undo_manager_undo (UNDO_MANAGER, NULL);
    }

  test_helper_zrythm_cleanup ();
}

static void
test_delete_automation_points (void)
{
  rebootstrap_timeline ();

  AutomationPoint *m = NULL, *m_c = NULL, *m_d = NULL;

  Position pos1, pos2;
  position_set_to_bar (&pos1, 1);
  position_set_to_bar (&pos2, 4);
  AutomationTrack * at = channel_get_automation_track (
    P_MASTER_TRACK->channel, PORT_FLAG_CHANNEL_FADER);
  g_assert_nonnull (at);
  ZRegion * r = automation_region_new (
    &pos1, &pos2, track_get_name_hash (P_MASTER_TRACK),
    at->index, 0);
  GError * err = NULL;
  bool     success = track_add_region (
    P_MASTER_TRACK, r, at, 0, F_GEN_NAME, 0, &err);
  g_assert_true (success);
  arranger_selections_add_object (
    (ArrangerSelections *) TL_SELECTIONS,
    (ArrangerObject *) r);
  arranger_selections_action_perform_create (
    TL_SELECTIONS, NULL);

  /* create markers A B C D */
  for (int i = 0; i < 4; i++)
    {
      m = automation_point_new_float (1.f, 1.f, &pos1);
      automation_region_add_ap (r, m, F_NO_PUBLISH_EVENTS);
      arranger_object_select (
        (ArrangerObject *) m, F_SELECT, F_NO_APPEND,
        F_NO_PUBLISH_EVENTS);
      arranger_selections_action_perform_create (
        AUTOMATION_SELECTIONS, NULL);

      if (i == 2)
        {
          m_c = m;
        }
      else if (i == 3)
        {
          m_d = m;
        }
    }

  /* delete C */
  arranger_object_select (
    (ArrangerObject *) m_c, F_SELECT, F_NO_APPEND,
    F_NO_PUBLISH_EVENTS);
  arranger_selections_action_perform_delete (
    AUTOMATION_SELECTIONS, NULL);

  /* delete D */
  arranger_object_select (
    (ArrangerObject *) m_d, F_SELECT, F_NO_APPEND,
    F_NO_PUBLISH_EVENTS);
  arranger_selections_action_perform_delete (
    AUTOMATION_SELECTIONS, NULL);

  for (int i = 0; i < 6; i++)
    {
      undo_manager_undo (UNDO_MANAGER, NULL);
    }

  test_helper_zrythm_cleanup ();
}

static void
test_duplicate_audio_regions (void)
{
  test_helper_zrythm_init ();

  char audio_file_path[2000];
  sprintf (
    audio_file_path, "%s%s%s", TESTS_SRCDIR,
    G_DIR_SEPARATOR_S, "test.wav");

  /* create audio track with region */
  Position pos1;
  position_init (&pos1);
  int             track_pos = TRACKLIST->num_tracks;
  SupportedFile * file =
    supported_file_new_from_path (audio_file_path);
  Track * track = track_create_with_action (
    TRACK_TYPE_AUDIO, NULL, file, &pos1, track_pos, 1, NULL);

  arranger_object_select (
    (ArrangerObject *) track->lanes[0]->regions[0], F_SELECT,
    F_NO_APPEND, F_NO_PUBLISH_EVENTS);
  arranger_selections_action_perform_duplicate_timeline (
    TL_SELECTIONS, MOVE_TICKS, 0, 0, NULL,
    F_NOT_ALREADY_MOVED, NULL);

  test_project_save_and_reload ();

  test_helper_zrythm_cleanup ();
}

static void
test_undo_moving_midi_region_to_other_lane (void)
{
  test_helper_zrythm_init ();

  /* create midi track with region */
  track_create_empty_with_action (TRACK_TYPE_MIDI, NULL);
  Track * midi_track = tracklist_get_last_track (
    TRACKLIST, TRACKLIST_PIN_OPTION_BOTH, false);
  g_assert_true (midi_track->type == TRACK_TYPE_MIDI);

  ZRegion * r = NULL;
  for (int i = 0; i < 4; i++)
    {
      Position start, end;
      position_init (&start);
      position_init (&end);
      position_add_bars (&end, 1);
      int lane_pos = (i == 3) ? 2 : 0;
      int idx_inside_lane = (i == 3) ? 0 : i;
      r = midi_region_new (
        &start, &end, track_get_name_hash (midi_track),
        lane_pos, idx_inside_lane);
      bool success = track_add_region (
        midi_track, r, NULL, lane_pos, F_GEN_NAME,
        F_NO_PUBLISH_EVENTS, NULL);
      g_assert_true (success);
      arranger_object_select (
        (ArrangerObject *) r, F_SELECT, F_NO_APPEND,
        F_NO_PUBLISH_EVENTS);
      arranger_selections_action_perform_create (
        TL_SELECTIONS, NULL);
    }

  /* move last region to top lane */
  arranger_object_select (
    (ArrangerObject *) r, F_SELECT, F_NO_APPEND,
    F_NO_PUBLISH_EVENTS);
  arranger_selections_action_perform_move_timeline (
    TL_SELECTIONS, MOVE_TICKS, 0, -2, NULL,
    F_NOT_ALREADY_MOVED, NULL);

  undo_manager_undo (UNDO_MANAGER, NULL);
  undo_manager_redo (UNDO_MANAGER, NULL);
  undo_manager_undo (UNDO_MANAGER, NULL);

  test_helper_zrythm_cleanup ();
}

static void
test_delete_multiple_regions (void)
{
  test_helper_zrythm_init ();

  track_create_empty_with_action (TRACK_TYPE_MIDI, NULL);
  Track * midi_track = tracklist_get_last_track (
    TRACKLIST, TRACKLIST_PIN_OPTION_BOTH, false);
  g_assert_true (midi_track->type == TRACK_TYPE_MIDI);

  TrackLane * lane = midi_track->lanes[0];
  for (int i = 0; i < 6; i++)
    {
      Position pos, end_pos;
      position_set_to_bar (&pos, 2 + i);
      position_set_to_bar (&end_pos, 4 + i);
      ZRegion * r1 = midi_region_new (
        &pos, &end_pos, track_get_name_hash (midi_track), 0,
        lane->num_regions);
      bool success = track_add_region (
        midi_track, r1, NULL, lane->pos, F_GEN_NAME,
        F_NO_PUBLISH_EVENTS, NULL);
      g_assert_true (success);
      arranger_object_select (
        (ArrangerObject *) r1, F_SELECT, F_NO_APPEND,
        F_NO_PUBLISH_EVENTS);
      arranger_selections_action_perform_create (
        TL_SELECTIONS, NULL);
      g_assert_cmpint (lane->num_regions, ==, i + 1);
    }

  /* select multiple and delete */
  for (int i = 0; i < 100; i++)
    {
      int idx1 = rand () % 6;
      int idx2 = rand () % 6;
      arranger_object_select (
        (ArrangerObject *) lane->regions[idx1], F_SELECT,
        F_NO_APPEND, F_NO_PUBLISH_EVENTS);
      arranger_object_select (
        (ArrangerObject *) (ArrangerObject *)
          lane->regions[idx2],
        F_SELECT, F_APPEND, F_NO_PUBLISH_EVENTS);

      /* do delete */
      GError * err = NULL;
      bool ret = arranger_selections_action_perform_delete (
        TL_SELECTIONS, &err);
      g_assert_true (ret);

      clip_editor_get_region (CLIP_EDITOR);

      int iret = undo_manager_undo (UNDO_MANAGER, &err);
      g_assert_true (iret == 0);

      g_assert_nonnull (clip_editor_get_region (CLIP_EDITOR));
    }

  g_assert_nonnull (clip_editor_get_region (CLIP_EDITOR));

  test_helper_zrythm_cleanup ();
}

static void
test_split_and_merge_midi_unlooped (void)
{
  test_helper_zrythm_init ();

  Position pos, end_pos, tmp;

  track_create_empty_with_action (TRACK_TYPE_MIDI, NULL);
  Track * midi_track = tracklist_get_last_track (
    TRACKLIST, TRACKLIST_PIN_OPTION_BOTH, false);
  g_assert_true (midi_track->type == TRACK_TYPE_MIDI);

  TrackLane * lane = midi_track->lanes[0];
  position_set_to_bar (&pos, 2);
  position_set_to_bar (&end_pos, 10);
  ZRegion * r1 = midi_region_new (
    &pos, &end_pos, track_get_name_hash (midi_track), 0,
    lane->num_regions);
  bool success = track_add_region (
    midi_track, r1, NULL, lane->pos, F_GEN_NAME,
    F_NO_PUBLISH_EVENTS, NULL);
  g_assert_true (success);
  arranger_object_select (
    (ArrangerObject *) r1, F_SELECT, F_NO_APPEND,
    F_NO_PUBLISH_EVENTS);
  arranger_selections_action_perform_create (
    TL_SELECTIONS, NULL);
  g_assert_cmpint (lane->num_regions, ==, 1);

  for (int i = 0; i < 2; i++)
    {
      if (i == 0)
        {
          position_set_to_bar (&pos, 1);
          position_set_to_bar (&end_pos, 2);
        }
      else
        {
          position_set_to_bar (&pos, 5);
          position_set_to_bar (&end_pos, 6);
        }
      MidiNote * mn =
        midi_note_new (&r1->id, &pos, &end_pos, 45, 45);
      midi_region_add_midi_note (r1, mn, F_NO_PUBLISH_EVENTS);
      arranger_object_select (
        (ArrangerObject *) mn, F_SELECT, F_NO_APPEND,
        F_NO_PUBLISH_EVENTS);
      arranger_selections_action_perform_create (
        (ArrangerSelections *) MA_SELECTIONS, NULL);
    }

  /* split */
  ZRegion *        r = lane->regions[0];
  ArrangerObject * r_obj = (ArrangerObject *) r;
  position_set_to_bar (&tmp, 2);
  g_assert_cmppos (&r_obj->pos, &tmp);
  arranger_object_select (
    r_obj, F_SELECT, F_NO_APPEND, F_NO_PUBLISH_EVENTS);
  GError * err = NULL;
  Position split_pos;
  position_set_to_bar (&split_pos, 4);
  bool ret = arranger_selections_action_perform_split (
    (ArrangerSelections *) TL_SELECTIONS, &split_pos, &err);
  g_assert_true (ret);

  /* check r1 positions */
  r1 = lane->regions[0];
  ArrangerObject * r1_obj = (ArrangerObject *) r1;
  position_set_to_bar (&tmp, 2);
  g_assert_cmppos (&r1_obj->pos, &tmp);
  position_set_to_bar (&tmp, 1);
  g_assert_cmppos (&r1_obj->clip_start_pos, &tmp);
  position_set_to_bar (&tmp, 1);
  g_assert_cmppos (&r1_obj->loop_start_pos, &tmp);
  position_set_to_bar (&tmp, 4);
  g_assert_cmppos (&r1_obj->end_pos, &tmp);
  position_set_to_bar (&tmp, 3);
  g_assert_cmppos (&r1_obj->loop_end_pos, &tmp);

  /* check r1 midi note positions */
  g_assert_cmpint (r1->num_midi_notes, ==, 1);
  MidiNote *       mn = r1->midi_notes[0];
  ArrangerObject * mn_obj = (ArrangerObject *) mn;
  position_set_to_bar (&tmp, 1);
  g_assert_cmppos (&mn_obj->pos, &tmp);
  position_set_to_bar (&tmp, 2);
  g_assert_cmppos (&mn_obj->end_pos, &tmp);

  /* check r2 positions */
  ZRegion *        r2 = lane->regions[1];
  ArrangerObject * r2_obj = (ArrangerObject *) r2;
  position_set_to_bar (&tmp, 4);
  g_assert_cmppos (&r2_obj->pos, &tmp);
  position_set_to_bar (&tmp, 1);
  g_assert_cmppos (&r2_obj->clip_start_pos, &tmp);
  position_set_to_bar (&tmp, 1);
  g_assert_cmppos (&r2_obj->loop_start_pos, &tmp);
  position_set_to_bar (&tmp, 10);
  g_assert_cmppos (&r2_obj->end_pos, &tmp);
  position_set_to_bar (&tmp, 7);
  g_assert_cmppos (&r2_obj->loop_end_pos, &tmp);

  /* check r2 midi note positions */
  g_assert_cmpint (r2->num_midi_notes, ==, 1);
  mn = r2->midi_notes[0];
  mn_obj = (ArrangerObject *) mn;
  position_set_to_bar (&tmp, 3);
  g_assert_cmppos (&mn_obj->pos, &tmp);
  position_set_to_bar (&tmp, 4);
  g_assert_cmppos (&mn_obj->end_pos, &tmp);

  /* merge */
  arranger_object_select (
    (ArrangerObject *) lane->regions[0], F_SELECT,
    F_NO_APPEND, F_NO_PUBLISH_EVENTS);
  arranger_object_select (
    (ArrangerObject *) (ArrangerObject *) lane->regions[1],
    F_SELECT, F_APPEND, F_NO_PUBLISH_EVENTS);
  err = NULL;
  ret = arranger_selections_action_perform_merge (
    (ArrangerSelections *) TL_SELECTIONS, &err);
  g_assert_true (ret);

  /* verify positions */
  g_assert_cmpint (lane->num_regions, ==, 1);
  r = lane->regions[0];
  r_obj = (ArrangerObject *) r;
  position_set_to_bar (&tmp, 2);
  g_assert_cmppos (&r_obj->pos, &tmp);
  position_set_to_bar (&tmp, 10);
  g_assert_cmppos (&r_obj->end_pos, &tmp);
  position_set_to_bar (&tmp, 1);
  g_assert_cmppos (&r_obj->loop_start_pos, &tmp);
  position_set_to_bar (&tmp, 1);
  g_assert_cmppos (&r_obj->clip_start_pos, &tmp);
  position_set_to_bar (&tmp, 9);
  g_assert_cmppos (&r_obj->loop_end_pos, &tmp);

  clip_editor_get_region (CLIP_EDITOR);

  int iret = undo_manager_undo (UNDO_MANAGER, &err);
  g_assert_true (iret == 0);

  /* check r1 positions */
  r1 = lane->regions[0];
  r1_obj = (ArrangerObject *) r1;
  position_set_to_bar (&tmp, 2);
  g_assert_cmppos (&r1_obj->pos, &tmp);
  position_set_to_bar (&tmp, 1);
  g_assert_cmppos (&r1_obj->clip_start_pos, &tmp);
  position_set_to_bar (&tmp, 1);
  g_assert_cmppos (&r1_obj->loop_start_pos, &tmp);
  position_set_to_bar (&tmp, 4);
  g_assert_cmppos (&r1_obj->end_pos, &tmp);
  position_set_to_bar (&tmp, 3);
  g_assert_cmppos (&r1_obj->loop_end_pos, &tmp);

  /* check r1 midi note positions */
  g_assert_cmpint (r1->num_midi_notes, ==, 1);
  mn = r1->midi_notes[0];
  mn_obj = (ArrangerObject *) mn;
  position_set_to_bar (&tmp, 1);
  g_assert_cmppos (&mn_obj->pos, &tmp);
  position_set_to_bar (&tmp, 2);
  g_assert_cmppos (&mn_obj->end_pos, &tmp);

  /* check r2 positions */
  r2 = lane->regions[1];
  r2_obj = (ArrangerObject *) r2;
  position_set_to_bar (&tmp, 4);
  g_assert_cmppos (&r2_obj->pos, &tmp);
  position_set_to_bar (&tmp, 1);
  g_assert_cmppos (&r2_obj->clip_start_pos, &tmp);
  position_set_to_bar (&tmp, 1);
  g_assert_cmppos (&r2_obj->loop_start_pos, &tmp);
  position_set_to_bar (&tmp, 10);
  g_assert_cmppos (&r2_obj->end_pos, &tmp);
  position_set_to_bar (&tmp, 7);
  g_assert_cmppos (&r2_obj->loop_end_pos, &tmp);

  /* check r2 midi note positions */
  g_assert_cmpint (r2->num_midi_notes, ==, 1);
  mn = r2->midi_notes[0];
  mn_obj = (ArrangerObject *) mn;
  position_set_to_bar (&tmp, 3);
  g_assert_cmppos (&mn_obj->pos, &tmp);
  position_set_to_bar (&tmp, 4);
  g_assert_cmppos (&mn_obj->end_pos, &tmp);

  /* undo split */
  iret = undo_manager_undo (UNDO_MANAGER, &err);
  g_assert_true (iret == 0);

  /* verify region */
  r = lane->regions[0];
  r_obj = (ArrangerObject *) r;
  position_set_to_bar (&tmp, 2);
  g_assert_cmppos (&r_obj->pos, &tmp);
  position_set_to_bar (&tmp, 10);
  g_assert_cmppos (&r_obj->end_pos, &tmp);
  position_set_to_bar (&tmp, 9);
  g_assert_cmppos (&r_obj->loop_end_pos, &tmp);
  position_set_to_bar (&tmp, 1);
  g_assert_cmppos (&r_obj->clip_start_pos, &tmp);
  position_set_to_bar (&tmp, 1);
  g_assert_cmppos (&r_obj->loop_start_pos, &tmp);

  /* verify midi notes are back to start */
  g_assert_cmpint (r->num_midi_notes, ==, 2);
  mn = r->midi_notes[0];
  mn_obj = (ArrangerObject *) mn;
  position_set_to_bar (&tmp, 1);
  g_assert_cmppos (&mn_obj->pos, &tmp);
  position_set_to_bar (&tmp, 2);
  g_assert_cmppos (&mn_obj->end_pos, &tmp);
  mn = r->midi_notes[1];
  mn_obj = (ArrangerObject *) mn;
  position_set_to_bar (&tmp, 5);
  g_assert_cmppos (&mn_obj->pos, &tmp);
  position_set_to_bar (&tmp, 6);
  g_assert_cmppos (&mn_obj->end_pos, &tmp);

  g_assert_nonnull (clip_editor_get_region (CLIP_EDITOR));

  iret = undo_manager_redo (UNDO_MANAGER, &err);
  g_assert_true (iret == 0);
  iret = undo_manager_redo (UNDO_MANAGER, &err);
  g_assert_true (iret == 0);
  iret = undo_manager_undo (UNDO_MANAGER, &err);
  g_assert_true (iret == 0);
  iret = undo_manager_undo (UNDO_MANAGER, &err);
  g_assert_true (iret == 0);

  test_helper_zrythm_cleanup ();
}

static void
test_split_and_merge_audio_unlooped (void)
{
  test_helper_zrythm_init ();

  Position pos, tmp;

  char audio_file_path[2000];
  sprintf (
    audio_file_path, "%s%s%s", TESTS_SRCDIR,
    G_DIR_SEPARATOR_S, "test.wav");
  SupportedFile * file_descr =
    supported_file_new_from_path (audio_file_path);
  position_set_to_bar (&pos, 2);
  Track * audio_track = track_create_with_action (
    TRACK_TYPE_AUDIO, NULL, file_descr, &pos,
    TRACKLIST->num_tracks, 1, NULL);
  int audio_track_pos = audio_track->pos;
  supported_file_free (file_descr);
  g_assert_nonnull (audio_track);
  g_assert_cmpint (audio_track->num_lanes, ==, 2);
  g_assert_cmpint (audio_track->lanes[0]->num_regions, ==, 1);
  g_assert_cmpint (audio_track->lanes[1]->num_regions, ==, 0);

  /* <2.1.1.0> to around <4.1.1.0> (around 2 bars
   * long) */
  TrackLane *      lane = audio_track->lanes[0];
  ZRegion *        r = lane->regions[0];
  ArrangerObject * r_obj = (ArrangerObject *) r;
  g_assert_cmppos (&r_obj->pos, &pos);

  /* remember frames */
  AudioClip *      clip = audio_region_get_clip (r);
  unsigned_frame_t num_frames = clip->num_frames;
  float            l_frames[num_frames];
  dsp_copy (l_frames, clip->ch_frames[0], (size_t) num_frames);

  /* split */
  r = lane->regions[0];
  r_obj = (ArrangerObject *) r;
  position_set_to_bar (&tmp, 2);
  g_assert_cmppos (&r_obj->pos, &tmp);
  arranger_object_select (
    r_obj, F_SELECT, F_NO_APPEND, F_NO_PUBLISH_EVENTS);
  GError * err = NULL;
  Position split_pos;
  position_set_to_bar (&split_pos, 3);
  bool ret = arranger_selections_action_perform_split (
    (ArrangerSelections *) TL_SELECTIONS, &split_pos, &err);
  g_assert_true (ret);

  /* check r1 positions */
  ZRegion *        r1 = lane->regions[0];
  ArrangerObject * r1_obj = (ArrangerObject *) r1;
  position_set_to_bar (&tmp, 2);
  g_assert_cmppos (&r1_obj->pos, &tmp);
  position_set_to_bar (&tmp, 1);
  g_assert_cmppos (&r1_obj->clip_start_pos, &tmp);
  position_set_to_bar (&tmp, 1);
  g_assert_cmppos (&r1_obj->loop_start_pos, &tmp);
  position_set_to_bar (&tmp, 3);
  g_assert_cmppos (&r1_obj->end_pos, &tmp);
  position_set_to_bar (&tmp, 2);
  g_assert_cmppos (&r1_obj->loop_end_pos, &tmp);

  unsigned_frame_t frames_per_bar =
    (unsigned_frame_t)
    (AUDIO_ENGINE->frames_per_tick *
       (double) TRANSPORT->ticks_per_bar);

  /* check r1 audio positions */
  AudioClip * r1_clip = audio_region_get_clip (r1);
  g_assert_cmpuint (r1_clip->num_frames, ==, frames_per_bar);
  g_assert_true (audio_frames_equal (
    r1_clip->ch_frames[0], &l_frames[0],
    (size_t) r1_clip->num_frames, 0.0001f));

  /* check r2 positions */
  ZRegion *        r2 = lane->regions[1];
  ArrangerObject * r2_obj = (ArrangerObject *) r2;
  position_set_to_bar (&tmp, 3);
  g_assert_cmppos (&r2_obj->pos, &tmp);
  position_set_to_bar (&tmp, 1);
  g_assert_cmppos (&r2_obj->clip_start_pos, &tmp);
  position_set_to_bar (&tmp, 1);
  g_assert_cmppos (&r2_obj->loop_start_pos, &tmp);
  g_assert_cmpuint (
    (unsigned_frame_t) r2_obj->end_pos.frames, ==,
    /* total previous frames + started at bar 2
     * (1 bar) */
    num_frames + frames_per_bar);
  g_assert_cmpuint (
    (unsigned_frame_t) r2_obj->loop_end_pos.frames, ==,
    /* total previous frames - r1 frames */
    num_frames - r1_clip->num_frames);

  /* check r2 audio positions */
  AudioClip * r2_clip = audio_region_get_clip (r2);
  g_assert_cmpuint (
    r2_clip->num_frames, ==,
    (unsigned_frame_t) r2_obj->loop_end_pos.frames);
  g_assert_true (audio_frames_equal (
    r2_clip->ch_frames[0], &l_frames[frames_per_bar],
    (size_t) r2_clip->num_frames, 0.0001f));

  /* merge */
  arranger_object_select (
    (ArrangerObject *) lane->regions[0], F_SELECT,
    F_NO_APPEND, F_NO_PUBLISH_EVENTS);
  arranger_object_select (
    (ArrangerObject *) (ArrangerObject *) lane->regions[1],
    F_SELECT, F_APPEND, F_NO_PUBLISH_EVENTS);
  err = NULL;
  ret = arranger_selections_action_perform_merge (
    (ArrangerSelections *) TL_SELECTIONS, &err);
  g_assert_true (ret);

  /* verify positions */
  g_assert_cmpint (lane->num_regions, ==, 1);
  r = lane->regions[0];
  r_obj = (ArrangerObject *) r;
  position_set_to_bar (&tmp, 2);
  g_assert_cmppos (&r_obj->pos, &tmp);
  position_set_to_bar (&tmp, 2);
  position_add_frames (&tmp, (signed_frame_t) num_frames);
  g_assert_cmppos (&r_obj->end_pos, &tmp);
  position_set_to_bar (&tmp, 1);
  g_assert_cmppos (&r_obj->loop_start_pos, &tmp);
  position_set_to_bar (&tmp, 1);
  g_assert_cmppos (&r_obj->clip_start_pos, &tmp);
  position_from_frames (&tmp, (signed_frame_t) num_frames);
  g_assert_cmppos (&r_obj->loop_end_pos, &tmp);

  clip_editor_get_region (CLIP_EDITOR);

  test_project_save_and_reload ();
  audio_track = TRACKLIST->tracks[audio_track_pos];
  lane = audio_track->lanes[0];

  /* undo merge */
  int iret = undo_manager_undo (UNDO_MANAGER, &err);
  g_assert_true (iret == 0);
  test_project_save_and_reload ();
  audio_track = TRACKLIST->tracks[audio_track_pos];
  lane = audio_track->lanes[0];

  /* check r1 positions */
  r1 = lane->regions[0];
  r1_obj = (ArrangerObject *) r1;
  position_set_to_bar (&tmp, 2);
  g_assert_cmppos (&r1_obj->pos, &tmp);
  position_set_to_bar (&tmp, 1);
  g_assert_cmppos (&r1_obj->clip_start_pos, &tmp);
  position_set_to_bar (&tmp, 1);
  g_assert_cmppos (&r1_obj->loop_start_pos, &tmp);
  position_set_to_bar (&tmp, 3);
  g_assert_cmppos (&r1_obj->end_pos, &tmp);
  position_set_to_bar (&tmp, 2);
  g_assert_cmppos (&r1_obj->loop_end_pos, &tmp);

  /* check r1 audio positions */
  r1_clip = audio_region_get_clip (r1);
  g_assert_cmpuint (r1_clip->num_frames, ==, frames_per_bar);
  g_assert_true (audio_frames_equal (
    r1_clip->ch_frames[0], &l_frames[0],
    (size_t) r1_clip->num_frames, 0.0001f));

  /* check r2 positions */
  r2 = lane->regions[1];
  r2_obj = (ArrangerObject *) r2;
  position_set_to_bar (&tmp, 3);
  g_assert_cmppos (&r2_obj->pos, &tmp);
  position_set_to_bar (&tmp, 1);
  g_assert_cmppos (&r2_obj->clip_start_pos, &tmp);
  position_set_to_bar (&tmp, 1);
  g_assert_cmppos (&r2_obj->loop_start_pos, &tmp);
  g_assert_cmpuint (
    (unsigned_frame_t) r2_obj->end_pos.frames, ==,
    /* total previous frames + started at bar 2
     * (1 bar) */
    num_frames + frames_per_bar);
  g_assert_cmpuint (
    (unsigned_frame_t) r2_obj->loop_end_pos.frames, ==,
    /* total previous frames - r1 frames */
    num_frames - r1_clip->num_frames);

  /* check r2 audio positions */
  r2_clip = audio_region_get_clip (r2);
  g_assert_cmpint (
    (signed_frame_t) r2_clip->num_frames, ==,
    r2_obj->loop_end_pos.frames);
  g_assert_true (audio_frames_equal (
    r2_clip->ch_frames[0], &l_frames[frames_per_bar],
    (size_t) r2_clip->num_frames, 0.0001f));

  /* undo split */
  iret = undo_manager_undo (UNDO_MANAGER, &err);
  g_assert_true (iret == 0);
  test_project_save_and_reload ();
  audio_track = TRACKLIST->tracks[audio_track_pos];
  lane = audio_track->lanes[0];

  /* verify region */
  r = lane->regions[0];
  r_obj = (ArrangerObject *) r;
  position_set_to_bar (&tmp, 2);
  g_assert_cmppos (&r_obj->pos, &tmp);
  position_set_to_bar (&tmp, 1);
  g_assert_cmppos (&r_obj->clip_start_pos, &tmp);
  position_set_to_bar (&tmp, 1);
  g_assert_cmppos (&r_obj->loop_start_pos, &tmp);
  g_assert_cmpuint (
    (unsigned_frame_t) r_obj->end_pos.frames, ==,
    /* total previous frames + started at bar 2
     * (1 bar) */
    num_frames + frames_per_bar);
  g_assert_cmpint (
    r_obj->loop_end_pos.frames, ==,
    /* total previous frames */
    (signed_frame_t) num_frames);

  /* check frames */
  clip = audio_region_get_clip (r);
  g_assert_cmpuint (clip->num_frames, ==, num_frames);
  g_assert_true (audio_frames_equal (
    clip->ch_frames[0], l_frames, (size_t) num_frames,
    0.0001f));

  g_assert_nonnull (clip_editor_get_region (CLIP_EDITOR));

  test_project_save_and_reload ();

  /* redo split */
  iret = undo_manager_redo (UNDO_MANAGER, &err);
  g_assert_true (iret == 0);
  test_project_save_and_reload ();

  /* redo merge */
  iret = undo_manager_redo (UNDO_MANAGER, &err);
  g_assert_true (iret == 0);
  test_project_save_and_reload ();

  /* undo merge */
  iret = undo_manager_undo (UNDO_MANAGER, &err);
  g_assert_true (iret == 0);
  test_project_save_and_reload ();

  /* undo split */
  iret = undo_manager_undo (UNDO_MANAGER, &err);
  g_assert_true (iret == 0);
  test_project_save_and_reload ();

  test_helper_zrythm_cleanup ();
}

static void
test_resize_loop_l (void)
{
  test_helper_zrythm_init ();

  Position pos, tmp;

  char audio_file_path[2000];
  sprintf (
    audio_file_path, "%s%s%s", TESTS_SRCDIR,
    G_DIR_SEPARATOR_S, "test.wav");
  SupportedFile * file_descr =
    supported_file_new_from_path (audio_file_path);
  position_set_to_bar (&pos, 3);
  Track * audio_track = track_create_with_action (
    TRACK_TYPE_AUDIO, NULL, file_descr, &pos,
    TRACKLIST->num_tracks, 1, NULL);
  int audio_track_pos = audio_track->pos;
  supported_file_free (file_descr);
  g_assert_nonnull (audio_track);
  g_assert_cmpint (audio_track->num_lanes, ==, 2);
  g_assert_cmpint (audio_track->lanes[0]->num_regions, ==, 1);
  g_assert_cmpint (audio_track->lanes[1]->num_regions, ==, 0);

  /* <3.1.1.0> to around <5.1.1.0> (around 2 bars
   * long) */
  TrackLane *      lane = audio_track->lanes[0];
  ZRegion *        r = lane->regions[0];
  ArrangerObject * r_obj = (ArrangerObject *) r;
  g_assert_cmppos (&r_obj->pos, &pos);

  /* remember end pos */
  Position end_pos;
  position_set_to_pos (&end_pos, &r_obj->end_pos);

  /* remember loop length */
  double loop_len_ticks =
    arranger_object_get_loop_length_in_ticks (r_obj);

  /* resize L */
  arranger_object_select (
    r_obj, F_SELECT, F_NO_APPEND, F_NO_PUBLISH_EVENTS);
  const double move_ticks = 100.0;
  arranger_selections_action_perform_resize (
    (ArrangerSelections *) TL_SELECTIONS,
    ARRANGER_SELECTIONS_ACTION_RESIZE_L_LOOP, -move_ticks,
    F_NOT_ALREADY_EDITED, NULL);

  Position new_pos;

  /* test start pos */
  position_set_to_pos (&new_pos, &pos);
  position_add_ticks (&new_pos, -move_ticks);
  g_assert_cmppos (&r_obj->pos, &new_pos);

  /* test loop start pos */
  position_init (&new_pos);
  g_assert_cmppos (&r_obj->loop_start_pos, &new_pos);

  /* test end pos */
  g_assert_cmppos (&r_obj->end_pos, &end_pos);

  g_assert_cmpfloat_with_epsilon (
    loop_len_ticks,
    arranger_object_get_loop_length_in_ticks (r_obj), 0.0001);

  (void) audio_track_pos;
  (void) tmp;

  test_helper_zrythm_cleanup ();
}

static void
test_delete_midi_notes (void)
{
  test_helper_zrythm_init ();

  Track * midi_track =
    track_create_empty_with_action (TRACK_TYPE_MIDI, NULL);

  /* create region */
  Position start, end;
  position_set_to_bar (&start, 1);
  position_set_to_bar (&end, 6);
  ZRegion * r = midi_region_new (
    &start, &end, track_get_name_hash (midi_track), 0, 0);
  ArrangerObject * r_obj = (ArrangerObject *) r;
  GError *         err = NULL;
  bool             success = track_add_region (
    midi_track, r, NULL, 0, F_GEN_NAME, F_NO_PUBLISH_EVENTS,
    &err);
  g_assert_true (success);
  arranger_object_select (
    r_obj, F_SELECT, F_NO_APPEND, F_NO_PUBLISH_EVENTS);
  arranger_selections_action_perform_create (
    (ArrangerSelections *) TL_SELECTIONS, NULL);

  /* create 4 MIDI notes */
  for (int i = 0; i < 4; i++)
    {
      position_set_to_bar (&start, 1 + i);
      position_set_to_bar (&end, 2 + i);
      MidiNote * mn =
        midi_note_new (&r->id, &start, &end, 45, 45);
      midi_region_add_midi_note (r, mn, F_NO_PUBLISH_EVENTS);
      arranger_object_select (
        (ArrangerObject *) mn, F_SELECT, F_NO_APPEND,
        F_NO_PUBLISH_EVENTS);
      arranger_selections_action_perform_create (
        (ArrangerSelections *) MA_SELECTIONS, NULL);
    }

#define CHECK_INDICES \
  for (int i = 0; i < r->num_midi_notes; i++) \
    { \
      MidiNote * mn = r->midi_notes[i]; \
      g_assert_cmpint (mn->pos, ==, i); \
    }

  CHECK_INDICES;

  for (int j = 0; j < 20; j++)
    {
      /* delete random MIDI notes */
      {
        int        idx = rand () % r->num_midi_notes;
        MidiNote * mn = r->midi_notes[idx];
        arranger_object_select (
          (ArrangerObject *) mn, F_SELECT, F_NO_APPEND,
          F_NO_PUBLISH_EVENTS);
        idx = rand () % r->num_midi_notes;
        mn = r->midi_notes[idx];
        arranger_object_select (
          (ArrangerObject *) mn, F_SELECT, F_APPEND,
          F_NO_PUBLISH_EVENTS);
        arranger_selections_action_perform_delete (
          (ArrangerSelections *) MA_SELECTIONS, NULL);
      }

      CHECK_INDICES;

      /* undo */
      undo_manager_undo (UNDO_MANAGER, NULL);

      CHECK_INDICES;

      /* redo delete midi note */
      undo_manager_redo (UNDO_MANAGER, NULL);

      CHECK_INDICES;

      undo_manager_undo (UNDO_MANAGER, NULL);
    }

#undef CHECK_INDICES

  test_helper_zrythm_cleanup ();
}

static void
test_cut_automation_region (void)
{
  test_helper_zrythm_init ();

  /* create master fader automation region */
  Position pos1, pos2;
  position_set_to_bar (&pos1, 1);
  position_set_to_bar (&pos2, 8);
  AutomationTrack * at = channel_get_automation_track (
    P_MASTER_TRACK->channel, PORT_FLAG_CHANNEL_FADER);
  g_assert_nonnull (at);
  ZRegion * r = automation_region_new (
    &pos1, &pos2, track_get_name_hash (P_MASTER_TRACK),
    at->index, 0);
  bool success = track_add_region (
    P_MASTER_TRACK, r, at, 0, F_GEN_NAME, 0, NULL);
  g_assert_true (success);
  arranger_selections_add_object (
    (ArrangerSelections *) TL_SELECTIONS,
    (ArrangerObject *) r);
  arranger_selections_action_perform_create (
    TL_SELECTIONS, NULL);

  /* create 2 points spanning the split point */
  for (int i = 0; i < 2; i++)
    {
      position_set_to_bar (&pos1, i == 0 ? 3 : 5);
      AutomationPoint * ap =
        automation_point_new_float (1.f, 1.f, &pos1);
      automation_region_add_ap (r, ap, F_NO_PUBLISH_EVENTS);
      arranger_object_select (
        (ArrangerObject *) ap, F_SELECT, F_NO_APPEND,
        F_NO_PUBLISH_EVENTS);
      arranger_selections_action_perform_create (
        AUTOMATION_SELECTIONS, NULL);
    }

  /* split between the 2 points */
  arranger_object_select (
    (ArrangerObject *) r, F_SELECT, F_NO_APPEND,
    F_NO_PUBLISH_EVENTS);
  position_set_to_bar (&pos1, 4);
  arranger_selections_action_perform_split (
    (ArrangerSelections *) TL_SELECTIONS, &pos1, NULL);

  undo_manager_undo (UNDO_MANAGER, NULL);
  undo_manager_redo (UNDO_MANAGER, NULL);
  undo_manager_undo (UNDO_MANAGER, NULL);
  undo_manager_redo (UNDO_MANAGER, NULL);
  undo_manager_undo (UNDO_MANAGER, NULL);

  /* split before the first point */
  r = at->regions[0];
  arranger_object_select (
    (ArrangerObject *) r, F_SELECT, F_NO_APPEND,
    F_NO_PUBLISH_EVENTS);
  position_set_to_bar (&pos1, 2);
  arranger_selections_action_perform_split (
    (ArrangerSelections *) TL_SELECTIONS, &pos1, NULL);

  undo_manager_undo (UNDO_MANAGER, NULL);
  undo_manager_redo (UNDO_MANAGER, NULL);
  undo_manager_undo (UNDO_MANAGER, NULL);

  test_helper_zrythm_cleanup ();
}

static void
test_copy_and_move_automation_regions (void)
{
  test_helper_zrythm_init ();

  /* create a new track */
  Track * audio_track =
    track_create_empty_with_action (TRACK_TYPE_AUDIO, NULL);

  Position pos1, pos2;
  position_set_to_bar (&pos1, 2);
  position_set_to_bar (&pos2, 4);
  AutomationTrack * fader_at = channel_get_automation_track (
    P_MASTER_TRACK->channel, PORT_FLAG_CHANNEL_FADER);
  g_assert_nonnull (fader_at);
  ZRegion * r = automation_region_new (
    &pos1, &pos2, track_get_name_hash (P_MASTER_TRACK),
    fader_at->index, 0);
  GError * err = NULL;
  bool     success = track_add_region (
    P_MASTER_TRACK, r, fader_at, 0, F_GEN_NAME, 0, &err);
  g_assert_true (success);
  arranger_selections_add_object (
    (ArrangerSelections *) TL_SELECTIONS,
    (ArrangerObject *) r);
  arranger_selections_action_perform_create (
    TL_SELECTIONS, NULL);

  AutomationPoint * ap =
    automation_point_new_float (0.5f, 0.5f, &pos1);
  automation_region_add_ap (r, ap, F_NO_PUBLISH_EVENTS);
  arranger_object_select (
    (ArrangerObject *) ap, F_SELECT, F_NO_APPEND,
    F_NO_PUBLISH_EVENTS);
  arranger_selections_action_perform_create (
    AUTOMATION_SELECTIONS, NULL);
  ap = automation_point_new_float (0.6f, 0.6f, &pos2);
  automation_region_add_ap (r, ap, F_NO_PUBLISH_EVENTS);
  arranger_object_select (
    (ArrangerObject *) ap, F_SELECT, F_NO_APPEND,
    F_NO_PUBLISH_EVENTS);
  arranger_selections_action_perform_create (
    AUTOMATION_SELECTIONS, NULL);

  AutomationTrack * mute_at = channel_get_automation_track (
    audio_track->channel, PORT_FLAG_FADER_MUTE);
  g_assert_nonnull (mute_at);

  /* 1st test */

  /* when i == 1 we are copying */
  for (int i = 0; i < 2; i++)
    {
      bool copy = i == 1;

      g_assert_cmpint (fader_at->num_regions, ==, 1);
      g_assert_cmpint (mute_at->num_regions, ==, 0);

      if (copy)
        {
          arranger_selections_action_perform_duplicate_timeline (
            TL_SELECTIONS, 0, 0, 0, &mute_at->port_id,
            F_NOT_ALREADY_MOVED, NULL);
          g_assert_cmpint (fader_at->num_regions, ==, 1);
          g_assert_cmpint (mute_at->num_regions, ==, 1);
          undo_manager_undo (UNDO_MANAGER, NULL);
          g_assert_cmpint (fader_at->num_regions, ==, 1);
          g_assert_cmpint (mute_at->num_regions, ==, 0);
          undo_manager_redo (UNDO_MANAGER, NULL);
          g_assert_cmpint (fader_at->num_regions, ==, 1);
          g_assert_cmpint (mute_at->num_regions, ==, 1);
          undo_manager_undo (UNDO_MANAGER, NULL);
        }
      else
        {
          arranger_selections_action_perform_move_timeline (
            TL_SELECTIONS, 0, 0, 0, &mute_at->port_id,
            F_NOT_ALREADY_MOVED, NULL);
          g_assert_cmpint (fader_at->num_regions, ==, 0);
          g_assert_cmpint (mute_at->num_regions, ==, 1);
          undo_manager_undo (UNDO_MANAGER, NULL);
          g_assert_cmpint (fader_at->num_regions, ==, 1);
          g_assert_cmpint (mute_at->num_regions, ==, 0);
          undo_manager_redo (UNDO_MANAGER, NULL);
          g_assert_cmpint (fader_at->num_regions, ==, 0);
          g_assert_cmpint (mute_at->num_regions, ==, 1);
          undo_manager_undo (UNDO_MANAGER, NULL);
        }

      g_assert_cmpint (fader_at->num_regions, ==, 1);
      g_assert_cmpint (mute_at->num_regions, ==, 0);
    }

  /* 2nd test */

  /* create 2 copies in the empty lane a bit behind */
  arranger_object_select (
    (ArrangerObject *) fader_at->regions[0], F_SELECT,
    F_NO_APPEND, F_NO_PUBLISH_EVENTS);
  arranger_selections_action_perform_duplicate_timeline (
    TL_SELECTIONS, -200, 0, 0, &mute_at->port_id,
    F_NOT_ALREADY_MOVED, NULL);
  g_assert_cmpint (fader_at->num_regions, ==, 1);
  g_assert_cmpint (mute_at->num_regions, ==, 1);
  arranger_object_select (
    (ArrangerObject *) fader_at->regions[0], F_SELECT,
    F_NO_APPEND, F_NO_PUBLISH_EVENTS);
  arranger_selections_action_perform_duplicate_timeline (
    TL_SELECTIONS, -400, 0, 0, &mute_at->port_id,
    F_NOT_ALREADY_MOVED, NULL);
  g_assert_cmpint (fader_at->num_regions, ==, 1);
  g_assert_cmpint (mute_at->num_regions, ==, 2);

  /* move the copy to the first lane */
  arranger_object_select (
    (ArrangerObject *) mute_at->regions[0], F_SELECT,
    F_NO_APPEND, F_NO_PUBLISH_EVENTS);
  arranger_selections_action_perform_move_timeline (
    TL_SELECTIONS, 0, 0, 0, &fader_at->port_id,
    F_NOT_ALREADY_MOVED, NULL);
  g_assert_cmpint (fader_at->num_regions, ==, 2);
  g_assert_cmpint (mute_at->num_regions, ==, 1);

  /* undo and verify all ok */
  undo_manager_undo (UNDO_MANAGER, NULL);
  g_assert_cmpint (fader_at->num_regions, ==, 1);
  g_assert_cmpint (mute_at->num_regions, ==, 2);
  undo_manager_redo (UNDO_MANAGER, NULL);
  g_assert_cmpint (fader_at->num_regions, ==, 2);
  g_assert_cmpint (mute_at->num_regions, ==, 1);
  undo_manager_undo (UNDO_MANAGER, NULL);
  g_assert_cmpint (fader_at->num_regions, ==, 1);
  g_assert_cmpint (mute_at->num_regions, ==, 2);

  test_helper_zrythm_cleanup ();
}

static void
test_move_region_from_lane_3_to_lane_1 (void)
{
  test_helper_zrythm_init ();

  Position pos, end_pos;
  position_init (&pos);
  Track * track = track_create_with_action (
    TRACK_TYPE_MIDI, NULL, NULL, &pos, TRACKLIST->num_tracks,
    1, NULL);
  TrackLane * lane = track->lanes[0];
  g_assert_cmpint (lane->num_regions, ==, 0);

  /* create region */
  position_set_to_bar (&pos, 2);
  position_set_to_bar (&end_pos, 4);
  ZRegion * r1 = midi_region_new (
    &pos, &end_pos, track_get_name_hash (track), 0,
    lane->num_regions);
  bool success = track_add_region (
    track, r1, NULL, lane->pos, F_GEN_NAME,
    F_NO_PUBLISH_EVENTS, NULL);
  g_assert_true (success);
  arranger_object_select (
    (ArrangerObject *) r1, F_SELECT, F_NO_APPEND,
    F_PUBLISH_EVENTS);
  arranger_selections_action_perform_create (
    TL_SELECTIONS, NULL);
  g_assert_cmpint (lane->num_regions, ==, 1);

  /* move to lane 3 */
  success = arranger_selections_action_perform_move_timeline (
    TL_SELECTIONS, 0, 0, 2, NULL, F_NOT_ALREADY_MOVED, NULL);
  g_assert_true (success);
  g_assert_cmpint (lane->num_regions, ==, 0);
  lane = track->lanes[2];
  g_assert_cmpint (lane->num_regions, ==, 1);

  /* duplicate track */
  success = tracklist_selections_action_perform_copy (
    TRACKLIST_SELECTIONS, PORT_CONNECTIONS_MGR,
    track->pos + 1, NULL);
  g_assert_true (success);

  /* move new region to lane 1 */
  track = tracklist_get_last_track (
    TRACKLIST, TRACKLIST_PIN_OPTION_BOTH, true);
  g_assert_cmpint (
    track->num_lanes, <=, (int) track->lanes_size);
  lane = track->lanes[2];
  r1 = lane->regions[0];
  arranger_object_select (
    (ArrangerObject *) r1, F_SELECT, F_NO_APPEND,
    F_PUBLISH_EVENTS);
  success = arranger_selections_action_perform_move_timeline (
    TL_SELECTIONS, 0, 0, -2, NULL, F_NOT_ALREADY_MOVED, NULL);
  g_assert_true (success);
  lane = track->lanes[0];
  g_assert_cmpint (lane->num_regions, ==, 1);

  test_helper_zrythm_cleanup ();
}

int
main (int argc, char * argv[])
{
  g_test_init (&argc, &argv, NULL);

#define TEST_PREFIX "/actions/arranger_selections/"

  g_test_add_func (
    TEST_PREFIX "test delete automation points",
    (GTestFunc) test_delete_automation_points);
  g_test_add_func (
    TEST_PREFIX "test move region from lane 3 to lane 1",
    (GTestFunc) test_move_region_from_lane_3_to_lane_1);
  g_test_add_func (
    TEST_PREFIX "test copy and move automation regions",
    (GTestFunc) test_copy_and_move_automation_regions);
  g_test_add_func (
    TEST_PREFIX "test audio functions",
    (GTestFunc) test_audio_functions);
  g_test_add_func (
    TEST_PREFIX "test move audio_region_and lower samplerate",
    (GTestFunc) test_move_audio_region_and_lower_samplerate);
  g_test_add_func (
    TEST_PREFIX "test cut automation region",
    (GTestFunc) test_cut_automation_region);
  g_test_add_func (
    TEST_PREFIX "test split large audio file",
    (GTestFunc) test_split_large_audio_file);
  g_test_add_func (
    TEST_PREFIX "test move audio_region_and lower bpm",
    (GTestFunc) test_move_audio_region_and_lower_bpm);
  g_test_add_func (
    TEST_PREFIX "test move timeline",
    (GTestFunc) test_move_timeline);
  g_test_add_func (
    TEST_PREFIX "test delete midi notes",
    (GTestFunc) test_delete_midi_notes);
  g_test_add_func (
    TEST_PREFIX "test resize loop l",
    (GTestFunc) test_resize_loop_l);
  g_test_add_func (
    TEST_PREFIX "test split and merge audio unlooped",
    (GTestFunc) test_split_and_merge_audio_unlooped);
  g_test_add_func (
    TEST_PREFIX "test split and merge midi unlooped",
    (GTestFunc) test_split_and_merge_midi_unlooped);
  g_test_add_func (
    TEST_PREFIX "test delete multiple regions",
    (GTestFunc) test_delete_multiple_regions);
  g_test_add_func (
    TEST_PREFIX "test delete timeline",
    (GTestFunc) test_delete_timeline);
  g_test_add_func (
    TEST_PREFIX "test link timeline",
    (GTestFunc) test_link_timeline);
  g_test_add_func (
    TEST_PREFIX "test undo moving midi_region to other lane",
    (GTestFunc) test_undo_moving_midi_region_to_other_lane);
  g_test_add_func (
    TEST_PREFIX "test duplicate audio regions",
    (GTestFunc) test_duplicate_audio_regions);
  g_test_add_func (
    TEST_PREFIX "test link then duplicate",
    (GTestFunc) test_link_then_duplicate);
  g_test_add_func (
    TEST_PREFIX "test delete chord objects",
    (GTestFunc) test_delete_chord_objects);
  g_test_add_func (
    TEST_PREFIX "test delete scale objects",
    (GTestFunc) test_delete_scale_objects);
  g_test_add_func (
    TEST_PREFIX "test delete markers",
    (GTestFunc) test_delete_markers);
  g_test_add_func (
    TEST_PREFIX "test split", (GTestFunc) test_split);
  g_test_add_func (
    TEST_PREFIX "test pin unpin", (GTestFunc) test_pin_unpin);
  g_test_add_func (
    TEST_PREFIX "test delete chords",
    (GTestFunc) test_delete_chords);
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
    TEST_PREFIX "test duplicate timeline",
    (GTestFunc) test_duplicate_timeline);
  g_test_add_func (
    TEST_PREFIX "test edit marker",
    (GTestFunc) test_edit_marker);
  g_test_add_func (
    TEST_PREFIX "test mute", (GTestFunc) test_mute);
  g_test_add_func (
    TEST_PREFIX "test quantize", (GTestFunc) test_quantize);
  g_test_add_func (
    TEST_PREFIX "test automation fill",
    (GTestFunc) test_automation_fill);
  g_test_add_func (
    TEST_PREFIX "test duplicate automation region",
    (GTestFunc) test_duplicate_automation_region);

  return g_test_run ();
}
