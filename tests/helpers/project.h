// SPDX-FileCopyrightText: © 2019-2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Project helper.
 */

#ifndef __TEST_HELPERS_PROJECT_H__
#define __TEST_HELPERS_PROJECT_H__

#include "zrythm-test-config.h"

#include "dsp/audio_region.h"
#include "dsp/automation_region.h"
#include "dsp/chord_region.h"
#include "dsp/chord_track.h"
#include "dsp/engine_dummy.h"
#include "dsp/marker_track.h"
#include "dsp/master_track.h"
#include "dsp/midi_note.h"
#include "dsp/modulator_track.h"
#include "dsp/recording_manager.h"
#include "dsp/region.h"
#include "dsp/router.h"
#include "dsp/tempo_track.h"
#include "dsp/tracklist.h"
#include "project.h"
#include "utils/cairo.h"
#include "utils/flags.h"
#include "utils/objects.h"
#include "utils/ui.h"
#include "zrythm.h"

#include <glib.h>
#include <glib/gi18n.h>

#include "tests/helpers/zrythm.h"

#include "project/project_init_flow_manager.h"

/**
 * @addtogroup tests
 *
 * @{
 */

/** MidiNote value to use. */
#define MN_VAL 78
/** MidiNote velocity to use. */
#define MN_VEL 23

/** First AP value. */
#define AP_VAL1 0.6f
/** Second AP value. */
#define AP_VAL2 0.9f

/** Marker name. */
#define MARKER_NAME "Marker name"

#define MUSICAL_SCALE_TYPE SCALE_IONIAN
#define MUSICAL_SCALE_ROOT NOTE_A

#define MOVE_TICKS 400

#define TOTAL_TL_SELECTIONS 6

#define MIDI_REGION_NAME "Midi region"
#define AUDIO_REGION_NAME "Audio region"
#define MIDI_TRACK_NAME "Midi track"
#define AUDIO_TRACK_NAME "Audio track"

/* initial positions */
#define MIDI_REGION_LANE 2
#define AUDIO_REGION_LANE 3

/* target positions */
#define TARGET_MIDI_TRACK_NAME "Target midi tr"
#define TARGET_AUDIO_TRACK_NAME "Target audio tr"

/* TODO test moving lanes */
#define TARGET_MIDI_REGION_LANE 0
#define TARGET_AUDIO_REGION_LANE 5

char *
test_project_save (void);

COLD void
test_project_reload (const char * prj_file);

void
test_project_save_and_reload (void);

/**
 * Stop dummy audio engine processing so we can
 * process manually.
 */
void
test_project_stop_dummy_engine (void);

void
test_project_check_vs_original_state (
  Position * p1,
  Position * p2,
  int        check_selections);

void
test_project_rebootstrap_timeline (Position * p1, Position * p2);

char *
test_project_save (void)
{
  /* save the project */
  GError * err = NULL;
  bool success = project_save (PROJECT, PROJECT->dir, 0, 0, F_NO_ASYNC, &err);
  g_assert_true (success);
  char * prj_file = g_build_filename (PROJECT->dir, PROJECT_FILE, NULL);

  object_free_w_func_and_null (project_free, PROJECT);

  return prj_file;
}

void
test_project_reload (const char * prj_file)
{
  project_init_flow_manager_load_or_create_default_project (
    prj_file, false, _test_helper_project_init_done_cb, NULL);
}

void
test_project_save_and_reload (void)
{
  /* save the project */
  char * prj_file = test_project_save ();
  g_assert_nonnull (prj_file);

  /* recreate the recording manager to drop any
   * events */
  object_free_w_func_and_null (
    recording_manager_free, ZRYTHM->recording_manager);
  ZRYTHM->recording_manager = recording_manager_new ();

  /* reload it */
  test_project_reload (prj_file);
  g_free (prj_file);
}

/**
 * Checks that the objects are back to their original
 * state.
 * @param check_selections Also checks that the
 *   selections are back to where they were.
 */
void
test_project_check_vs_original_state (
  Position * p1,
  Position * p2,
  int        check_selections)
{
  bool after_reload = false;

check_vs_orig_state:

  if (check_selections)
    {
      g_assert_cmpint (TL_SELECTIONS->num_regions, ==, 4);
      g_assert_cmpint (TL_SELECTIONS->num_markers, ==, 1);
      g_assert_cmpint (TL_SELECTIONS->num_scale_objects, ==, 1);
    }

  Track * midi_track = tracklist_find_track_by_name (TRACKLIST, MIDI_TRACK_NAME);
  g_assert_nonnull (midi_track);
  Track * audio_track =
    tracklist_find_track_by_name (TRACKLIST, AUDIO_TRACK_NAME);
  g_assert_nonnull (audio_track);

  Position p1_before_move, p2_before_move;
  p1_before_move = *p1;
  p2_before_move = *p2;
  position_add_ticks (&p1_before_move, -MOVE_TICKS);
  position_add_ticks (&p2_before_move, -MOVE_TICKS);

  /* check midi region */
  g_assert_cmpint (midi_track->lanes[MIDI_REGION_LANE]->num_regions, ==, 1);
  ArrangerObject * obj =
    (ArrangerObject *) midi_track->lanes[MIDI_REGION_LANE]->regions[0];
  g_assert_cmppos (&obj->pos, p1);
  g_assert_cmppos (&obj->end_pos, p2);
  ZRegion * r = (ZRegion *) obj;
  g_assert_cmpint (r->num_midi_notes, ==, 1);
  MidiNote * mn = r->midi_notes[0];
  obj = (ArrangerObject *) mn;
  g_assert_cmpuint (mn->val, ==, MN_VAL);
  g_assert_cmpuint (mn->vel->vel, ==, MN_VEL);
  g_assert_cmppos (&obj->pos, p1);
  g_assert_cmppos (&obj->end_pos, p2);
  g_assert_true (region_identifier_is_equal (&obj->region_id, &r->id));

  /* check audio region */
  g_assert_cmpint (audio_track->lanes[AUDIO_REGION_LANE]->num_regions, ==, 1);
  obj = (ArrangerObject *) audio_track->lanes[AUDIO_REGION_LANE]->regions[0];
  g_assert_cmppos (&obj->pos, p1);

  /* check automation region */
  AutomationTracklist * atl = track_get_automation_tracklist (P_MASTER_TRACK);
  g_assert_nonnull (atl);
  AutomationTrack * at = channel_get_automation_track (
    P_MASTER_TRACK->channel, PORT_FLAG_STEREO_BALANCE);
  g_assert_nonnull (at);
  g_assert_cmpint (at->num_regions, ==, 1);
  obj = (ArrangerObject *) at->regions[0];
  g_assert_cmppos (&obj->pos, p1);
  g_assert_cmppos (&obj->end_pos, p2);
  r = (ZRegion *) obj;
  g_assert_cmpint (r->num_aps, ==, 2);
  AutomationPoint * ap = r->aps[0];
  obj = (ArrangerObject *) ap;
  g_assert_cmppos (&obj->pos, p1);
  g_assert_cmpfloat_with_epsilon (ap->fvalue, AP_VAL1, 0.000001f);
  ap = r->aps[1];
  obj = (ArrangerObject *) ap;
  g_assert_cmppos (&obj->pos, p2);
  g_assert_cmpfloat_with_epsilon (ap->fvalue, AP_VAL2, 0.000001f);

  /* check marker */
  Marker * m;
  g_assert_cmpint (P_MARKER_TRACK->num_markers, ==, 3);
  obj = (ArrangerObject *) P_MARKER_TRACK->markers[0];
  m = (Marker *) obj;
  g_assert_true (m);
  g_assert_cmpstr (m->name, ==, "[start]");
  obj = (ArrangerObject *) P_MARKER_TRACK->markers[2];
  m = (Marker *) obj;
  g_assert_cmppos (&obj->pos, p1);
  g_assert_cmpstr (m->name, ==, MARKER_NAME);

  /* check scale object */
  g_assert_cmpint (P_CHORD_TRACK->num_scales, ==, 1);
  obj = (ArrangerObject *) P_CHORD_TRACK->scales[0];
  ScaleObject * s = (ScaleObject *) obj;
  g_assert_cmppos (&obj->pos, p1);
  g_assert_cmpint (s->scale->type, ==, MUSICAL_SCALE_TYPE);
  g_assert_cmpint (s->scale->root_key, ==, MUSICAL_SCALE_ROOT);

  /* save the project and reopen it. some callers
   * undo after this step so this checks if the undo
   * history works after reopening the project */
  if (!after_reload)
    {
      test_project_save_and_reload ();
      after_reload = true;
      goto check_vs_orig_state;
    }
}

/**
 * Bootstraps the test with test data.
 */
void
test_project_rebootstrap_timeline (Position * p1, Position * p2)
{
  GError * err = NULL;

  test_helper_zrythm_init ();

  /* pause engine */
  EngineState state;
  engine_wait_for_pause (AUDIO_ENGINE, &state, false, true);

  /* remove any previous work */
  chord_track_clear (P_CHORD_TRACK);
  marker_track_clear (P_MARKER_TRACK);
  tempo_track_clear (P_TEMPO_TRACK);
  //modulator_track_clear (P_MODULATOR_TRACK);
  for (int i = TRACKLIST->num_tracks - 1; i > P_MASTER_TRACK->pos; i--)
    {
      Track * track = TRACKLIST->tracks[i];
      tracklist_remove_track (
        TRACKLIST, track, F_REMOVE_PL, F_FREE, F_NO_PUBLISH_EVENTS,
        F_NO_RECALC_GRAPH);
    }
  track_clear (P_MASTER_TRACK);

  /* Create and add a MidiRegion with a MidiNote */
  position_set_to_bar (p1, 2);
  position_set_to_bar (p2, 4);
  Track * track = track_new (
    TRACK_TYPE_MIDI, TRACKLIST->num_tracks, MIDI_TRACK_NAME, F_WITH_LANE);
  tracklist_append_track (
    TRACKLIST, track, F_NO_PUBLISH_EVENTS, F_NO_RECALC_GRAPH);
  unsigned int track_name_hash = track_get_name_hash (track);
  ZRegion *    r =
    midi_region_new (p1, p2, track_get_name_hash (track), MIDI_REGION_LANE, 0);
  ArrangerObject * r_obj = (ArrangerObject *) r;
  bool success = track_add_region (track, r, NULL, MIDI_REGION_LANE, 1, 0, &err);
  g_assert_true (success);
  arranger_object_set_name (r_obj, MIDI_REGION_NAME, F_NO_PUBLISH_EVENTS);
  g_assert_cmpuint (r->id.track_name_hash, ==, track_name_hash);
  g_assert_cmpint (r->id.lane_pos, ==, MIDI_REGION_LANE);
  g_assert_cmpint (r->id.type, ==, REGION_TYPE_MIDI);
  arranger_selections_add_object (
    (ArrangerSelections *) TL_SELECTIONS, (ArrangerObject *) r);

  /* add a midi note to the region */
  MidiNote * mn = midi_note_new (&r->id, p1, p2, MN_VAL, MN_VEL);
  midi_region_add_midi_note (r, mn, 0);
  ArrangerObject * mn_obj = (ArrangerObject *) mn;
  g_assert_true (region_identifier_is_equal (&mn_obj->region_id, &r->id));
  arranger_selections_add_object (
    (ArrangerSelections *) MA_SELECTIONS, (ArrangerObject *) mn);

  /* Create and add an automation region with
   * 2 AutomationPoint's */
  AutomationTrack * at = channel_get_automation_track (
    P_MASTER_TRACK->channel, PORT_FLAG_STEREO_BALANCE);
  track_name_hash = track_get_name_hash (P_MASTER_TRACK);
  r = automation_region_new (p1, p2, track_name_hash, at->index, 0);
  success = track_add_region (P_MASTER_TRACK, r, at, 0, F_GEN_NAME, 0, &err);
  g_assert_true (success);
  g_assert_cmpuint (r->id.track_name_hash, ==, track_name_hash);
  g_assert_cmpint (r->id.at_idx, ==, at->index);
  g_assert_cmpint (r->id.type, ==, REGION_TYPE_AUTOMATION);
  arranger_selections_add_object (
    (ArrangerSelections *) TL_SELECTIONS, (ArrangerObject *) r);

  /* add 2 automation points to the region */
  AutomationPoint * ap = automation_point_new_float (AP_VAL1, AP_VAL1, p1);
  automation_region_add_ap (r, ap, F_NO_PUBLISH_EVENTS);
  arranger_selections_add_object (
    (ArrangerSelections *) AUTOMATION_SELECTIONS, (ArrangerObject *) ap);
  ap = automation_point_new_float (AP_VAL2, AP_VAL1, p2);
  automation_region_add_ap (r, ap, F_NO_PUBLISH_EVENTS);
  arranger_selections_add_object (
    (ArrangerSelections *) AUTOMATION_SELECTIONS, (ArrangerObject *) ap);

  /* Create and add a chord region with
   * 2 Chord's */
  r = chord_region_new (p1, p2, 0);
  success = track_add_region (
    P_CHORD_TRACK, r, NULL, 0, F_GEN_NAME, F_NO_PUBLISH_EVENTS, &err);
  g_assert_true (success);
  arranger_selections_add_object (
    (ArrangerSelections *) TL_SELECTIONS, (ArrangerObject *) r);

  /* add 2 chords to the region */
  ChordObject * c = chord_object_new (&r->id, 0, 1);
  chord_region_add_chord_object (r, c, F_NO_PUBLISH_EVENTS);
  arranger_object_pos_setter ((ArrangerObject *) c, p1);
  arranger_selections_add_object (
    (ArrangerSelections *) CHORD_SELECTIONS, (ArrangerObject *) c);
  c = chord_object_new (&r->id, 0, 1);
  chord_region_add_chord_object (r, c, F_NO_PUBLISH_EVENTS);
  arranger_object_pos_setter ((ArrangerObject *) c, p2);
  arranger_selections_add_object (
    (ArrangerSelections *) CHORD_SELECTIONS, (ArrangerObject *) c);

  /* create and add a Marker */
  Marker * m = marker_new (MARKER_NAME);
  marker_track_add_marker (P_MARKER_TRACK, m);
  g_assert_cmpint (m->index, ==, 2);
  arranger_selections_add_object (
    (ArrangerSelections *) TL_SELECTIONS, (ArrangerObject *) m);
  arranger_object_pos_setter ((ArrangerObject *) m, p1);

  /* create and add a ScaleObject */
  MusicalScale * ms = musical_scale_new (MUSICAL_SCALE_TYPE, MUSICAL_SCALE_ROOT);
  ScaleObject * s = scale_object_new (ms);
  chord_track_add_scale (P_CHORD_TRACK, s);
  arranger_selections_add_object (
    (ArrangerSelections *) TL_SELECTIONS, (ArrangerObject *) s);
  arranger_object_pos_setter ((ArrangerObject *) s, p1);

  /* Create and add an audio region */
  position_set_to_bar (p1, 2);
  track = track_new (
    TRACK_TYPE_AUDIO, TRACKLIST->num_tracks, AUDIO_TRACK_NAME, F_WITH_LANE);
  tracklist_append_track (
    TRACKLIST, track, F_NO_PUBLISH_EVENTS, F_NO_RECALC_GRAPH);
  char audio_file_path[2000];
  sprintf (
    audio_file_path, "%s%s%s", TESTS_SRCDIR, G_DIR_SEPARATOR_S, "test.wav");
  track_name_hash = track_get_name_hash (track);
  r = audio_region_new (
    -1, audio_file_path, true, NULL, 0, NULL, 0, 0, p1, track_name_hash,
    AUDIO_REGION_LANE, 0, NULL);
  AudioClip * clip = audio_region_get_clip (r);
  g_assert_cmpuint (clip->num_frames, >, 151000);
  g_assert_cmpuint (clip->num_frames, <, 152000);
  success = track_add_region (track, r, NULL, AUDIO_REGION_LANE, 1, 0, &err);
  g_assert_true (success);
  r_obj = (ArrangerObject *) r;
  arranger_object_set_name (r_obj, AUDIO_REGION_NAME, F_NO_PUBLISH_EVENTS);
  g_assert_cmpuint (r->id.track_name_hash, ==, track_name_hash);
  g_assert_cmpint (r->id.lane_pos, ==, AUDIO_REGION_LANE);
  g_assert_cmpint (r->id.type, ==, REGION_TYPE_AUDIO);
  arranger_selections_add_object (
    (ArrangerSelections *) TL_SELECTIONS, (ArrangerObject *) r);

  /* create the target tracks */
  track = track_new (
    TRACK_TYPE_MIDI, TRACKLIST->num_tracks, TARGET_MIDI_TRACK_NAME, F_WITH_LANE);
  tracklist_append_track (
    TRACKLIST, track, F_NO_PUBLISH_EVENTS, F_NO_RECALC_GRAPH);
  track = track_new (
    TRACK_TYPE_AUDIO, TRACKLIST->num_tracks, TARGET_AUDIO_TRACK_NAME,
    F_WITH_LANE);
  tracklist_append_track (
    TRACKLIST, track, F_NO_PUBLISH_EVENTS, F_NO_RECALC_GRAPH);

  int   beats_per_bar = tempo_track_get_beats_per_bar (P_TEMPO_TRACK);
  bpm_t bpm = tempo_track_get_current_bpm (P_TEMPO_TRACK);
  engine_update_frames_per_tick (
    AUDIO_ENGINE, beats_per_bar, bpm, AUDIO_ENGINE->sample_rate, true, true,
    false);

  router_recalc_graph (ROUTER, F_NOT_SOFT);

  engine_resume (AUDIO_ENGINE, &state);

  test_project_save_and_reload ();
}

/**
 * Stop dummy audio engine processing so we can
 * process manually.
 */
void
test_project_stop_dummy_engine (void)
{
  AUDIO_ENGINE->stop_dummy_audio_thread = true;
  g_usleep (100000);
}

/**
 * @}
 */

#endif
