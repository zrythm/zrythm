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

#include "actions/arranger_selections.h"
#include "actions/undoable_action.h"
#include "audio/audio_region.h"
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

Position p1, p2;

/**
 * Bootstraps the test with test data.
 */
static void
rebootstrap_timeline ()
{
  /* remove any previous work */
  chord_track_clear (P_CHORD_TRACK);
  marker_track_clear (P_MARKER_TRACK);
  for (int i = TRACKLIST->num_tracks - 1;
       i >= 3; i--)
    {
      Track * track = TRACKLIST->tracks[i];
      tracklist_remove_track (
        TRACKLIST, track, 1, 1, 0, 0);
    }
  track_clear (P_MASTER_TRACK);

  /* Create and add a MidiRegion with a MidiNote */
  position_set_to_bar (&p1, 2);
  position_set_to_bar (&p2, 4);
  Track * track =
    track_new (
      TRACK_TYPE_MIDI, TRACKLIST->num_tracks,
      MIDI_TRACK_NAME, 1);
  tracklist_append_track (
    TRACKLIST, track, F_NO_PUBLISH_EVENTS,
    F_NO_RECALC_GRAPH);
  ZRegion * r =
    midi_region_new (
      &p1, &p2, track->pos, MIDI_REGION_LANE, 0);
  track_add_region (
    track, r, NULL, MIDI_REGION_LANE, 1, 0);
  region_set_name (r, MIDI_REGION_NAME, 0);
  g_assert_cmpint (r->id.track_pos, ==, track->pos);
  g_assert_cmpint (
    r->id.lane_pos, ==, MIDI_REGION_LANE);
  g_assert_cmpint (
    r->id.type, ==, REGION_TYPE_MIDI);
  arranger_selections_add_object (
    (ArrangerSelections *) TL_SELECTIONS,
    (ArrangerObject *) r);

  /* add a midi note to the region */
  MidiNote * mn =
    midi_note_new (
      &r->id, &p1, &p2, MN_VAL, MN_VEL);
  midi_region_add_midi_note (r, mn, 0);
  g_assert_true (
    region_identifier_is_equal (
      &mn->region_id, &r->id));
  arranger_selections_add_object (
    (ArrangerSelections *) MA_SELECTIONS,
    (ArrangerObject *) mn);

  /* Create and add an automation region with
   * 2 AutomationPoint's */
  AutomationTrack * at =
    channel_get_automation_track (
      P_MASTER_TRACK->channel,
      PORT_FLAG_STEREO_BALANCE);
  r =
    automation_region_new (
      &p1, &p2, P_MASTER_TRACK->pos, at->index, 0);
  track_add_region (
    P_MASTER_TRACK, r, at, 0, 1, 0);
  g_assert_cmpint (
    r->id.track_pos, ==, P_MASTER_TRACK->pos);
  g_assert_cmpint (r->id.at_idx, ==, at->index);
  g_assert_cmpint (
    r->id.type, ==, REGION_TYPE_AUTOMATION);
  arranger_selections_add_object (
    (ArrangerSelections *) TL_SELECTIONS,
    (ArrangerObject *) r);

  /* add 2 automation points to the region */
  AutomationPoint * ap =
    automation_point_new_float (
      AP_VAL1, AP_VAL1, &p1);
  automation_region_add_ap (
    r, ap, F_NO_PUBLISH_EVENTS);
  arranger_selections_add_object (
    (ArrangerSelections *) AUTOMATION_SELECTIONS,
    (ArrangerObject *) ap);
  ap =
    automation_point_new_float (
      AP_VAL2, AP_VAL1, &p2);
  automation_region_add_ap (
    r, ap, F_NO_PUBLISH_EVENTS);
  arranger_selections_add_object (
    (ArrangerSelections *) AUTOMATION_SELECTIONS,
    (ArrangerObject *) ap);

  /* Create and add a chord region with
   * 2 Chord's */
  r =
    chord_region_new (&p1, &p2, 0);
  track_add_region (
    P_CHORD_TRACK, r, NULL, 0, 1, 0);
  arranger_selections_add_object (
    (ArrangerSelections *) TL_SELECTIONS,
    (ArrangerObject *) r);

  /* add 2 chords to the region */
  ChordObject * c =
    chord_object_new (&r->id, 0, 1);
  chord_region_add_chord_object (
    r, c);
  arranger_object_pos_setter (
    (ArrangerObject *) c, &p1);
  arranger_selections_add_object (
    (ArrangerSelections *) CHORD_SELECTIONS,
    (ArrangerObject *) c);
  c =
    chord_object_new (&r->id, 0, 1);
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

  /* Create and add an audio region */
  position_set_to_bar (&p1, 2);
  track =
    track_new (
      TRACK_TYPE_AUDIO, TRACKLIST->num_tracks,
      AUDIO_TRACK_NAME, 1);
  tracklist_append_track (
    TRACKLIST, track, F_NO_PUBLISH_EVENTS,
    F_NO_RECALC_GRAPH);
  char audio_file_path[2000];
  sprintf (
    audio_file_path, "%s%s%s",
    TESTS_SRCDIR, G_DIR_SEPARATOR_S,
    "test.wav");
  r =
    audio_region_new (
      -1, audio_file_path, NULL, 0, 0,
      &p1, track->pos, AUDIO_REGION_LANE, 0);
  AudioClip * clip =
    audio_region_get_clip (r);
  g_assert_cmpint (clip->num_frames, >, 290000);
  g_assert_cmpint (clip->num_frames, <, 294000);
  track_add_region (
    track, r, NULL, AUDIO_REGION_LANE, 1, 0);
  region_set_name (r, AUDIO_REGION_NAME, 0);
  g_assert_cmpint (r->id.track_pos, ==, track->pos);
  g_assert_cmpint (
    r->id.lane_pos, ==, AUDIO_REGION_LANE);
  g_assert_cmpint (
    r->id.type, ==, REGION_TYPE_AUDIO);
  arranger_selections_add_object (
    (ArrangerSelections *) TL_SELECTIONS,
    (ArrangerObject *) r);

  /* create the target tracks */
  track =
    track_new (
      TRACK_TYPE_MIDI, TRACKLIST->num_tracks,
      TARGET_MIDI_TRACK_NAME, 1);
  tracklist_append_track (
    TRACKLIST, track, F_NO_PUBLISH_EVENTS,
    F_NO_RECALC_GRAPH);
  track =
    track_new (
      TRACK_TYPE_AUDIO, TRACKLIST->num_tracks,
      TARGET_AUDIO_TRACK_NAME, 1);
  tracklist_append_track (
    TRACKLIST, track, F_NO_PUBLISH_EVENTS,
    F_NO_RECALC_GRAPH);
}

static void
select_audio_and_midi_regions_only ()
{
  arranger_selections_clear (
    (ArrangerSelections *) TL_SELECTIONS);
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
 * Checks that the objects are back to their original
 * state.
 * @param check_selections Also checks that the
 *   selections are back to where they were.
 */
static void
check_timeline_objects_vs_original_state (
  const int check_selections)
{
  if (check_selections)
    {
      g_assert_cmpint (
        TL_SELECTIONS->num_regions, ==, 4);
      g_assert_cmpint (
        TL_SELECTIONS->num_markers, ==, 1);
      g_assert_cmpint (
        TL_SELECTIONS->num_scale_objects, ==, 1);
    }

  Track * midi_track =
    tracklist_find_track_by_name (
      TRACKLIST, MIDI_TRACK_NAME);
  g_assert_nonnull (midi_track);
  Track * audio_track =
    tracklist_find_track_by_name (
      TRACKLIST, AUDIO_TRACK_NAME);
  g_assert_nonnull (audio_track);

  Position p1_before_move, p2_before_move;
  p1_before_move = p1;
  p2_before_move = p2;
  position_add_ticks (
    &p1_before_move, - MOVE_TICKS);
  position_add_ticks (
    &p2_before_move, - MOVE_TICKS);

  /* check midi region */
  g_assert_cmpint (
    midi_track->lanes[MIDI_REGION_LANE]->
      num_regions, ==, 1);
  ArrangerObject * obj =
    (ArrangerObject *)
    midi_track->lanes[MIDI_REGION_LANE]->regions[0];
  g_assert_cmppos (&obj->pos, &p1);
  g_assert_cmppos (&obj->end_pos, &p2);
  ZRegion * r = (ZRegion *) obj;
  g_assert_cmpint (r->num_midi_notes, ==, 1);
  MidiNote * mn = r->midi_notes[0];
  obj = (ArrangerObject *) mn;
  g_assert_cmpuint (mn->val, ==, MN_VAL);
  g_assert_cmpuint (mn->vel->vel, ==, MN_VEL);
  g_assert_cmppos (&obj->pos, &p1);
  g_assert_cmppos (&obj->end_pos, &p2);
  g_assert_true (
    region_identifier_is_equal (
      &mn->region_id, &r->id));

  /* check audio region */
  g_assert_cmpint (
    audio_track->lanes[AUDIO_REGION_LANE]->
      num_regions, ==, 1);
  obj =
    (ArrangerObject *)
    audio_track->lanes[AUDIO_REGION_LANE]->
      regions[0];
  g_assert_cmppos (&obj->pos, &p1);

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
  obj =
    (ArrangerObject *) at->regions[0];
  g_assert_cmppos (&obj->pos, &p1);
  g_assert_cmppos (&obj->end_pos, &p2);
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
    P_MARKER_TRACK->num_markers, ==, 3);
  obj =
    (ArrangerObject *) P_MARKER_TRACK->markers[2];
  Marker * m = (Marker *) obj;
  g_assert_cmppos (&obj->pos, &p1);
  g_assert_cmpstr (m->name, ==, MARKER_NAME);

  /* check scale object */
  g_assert_cmpint (
    P_CHORD_TRACK->num_scales, ==, 1);
  obj =
    (ArrangerObject *)
    P_CHORD_TRACK->scales[0];
  ScaleObject * s = (ScaleObject *) obj;
  g_assert_cmppos (&obj->pos, &p1);
  g_assert_cmpint (
    s->scale->type, ==, MUSICAL_SCALE_TYPE);
  g_assert_cmpint (
    s->scale->root_key, ==, MUSICAL_SCALE_ROOT);
}
/**
 * Checks that the objects are deleted.
 */
static void
check_timeline_objects_deleted ()
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
    midi_track->lanes[MIDI_REGION_LANE]->
      num_regions, ==, 0);

  /* check audio region */
  g_assert_cmpint (
    audio_track->lanes[AUDIO_REGION_LANE]->
      num_regions, ==, 0);

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
}

static void
test_create_timeline ()
{
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
  check_timeline_objects_vs_original_state (1);

  /* undo and check that the objects are deleted */
  undo_manager_undo (UNDO_MANAGER);
  g_assert_cmpint (
    arranger_selections_get_num_objects (
      (ArrangerSelections *) MA_SELECTIONS), ==, 0);
  check_timeline_objects_deleted ();

  /* redo and check that the objects are there */
  undo_manager_redo (UNDO_MANAGER);
  g_assert_cmpint (
    arranger_selections_get_num_objects (
      (ArrangerSelections *) TL_SELECTIONS), ==,
    TOTAL_TL_SELECTIONS);
  check_timeline_objects_vs_original_state (1);
}

static void
test_delete_timeline ()
{
  /* do delete */
  UndoableAction * ua =
    arranger_selections_action_new_delete (
      TL_SELECTIONS);
  undo_manager_perform (UNDO_MANAGER, ua);

  /* check */
  check_timeline_objects_deleted ();
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
  check_timeline_objects_vs_original_state (1);

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
  check_timeline_objects_deleted ();

  /* undo again to prepare for next test */
  undo_manager_undo (UNDO_MANAGER);
  g_assert_cmpint (
    arranger_selections_get_num_objects (
      (ArrangerSelections *) TL_SELECTIONS), ==,
    TOTAL_TL_SELECTIONS);
  check_timeline_objects_vs_original_state (1);
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
      &mn->region_id, &r->id));

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
            new_midi_track);
          region_move_to_track (
            audio_track->lanes[AUDIO_REGION_LANE]->
              regions[0],
            new_audio_track);
        }
      /* do move ticks */
      arranger_selections_add_ticks (
        (ArrangerSelections *) TL_SELECTIONS,
        MOVE_TICKS, 0);

      UndoableAction * ua =
        arranger_selections_action_new_move_timeline (
          TL_SELECTIONS, MOVE_TICKS, track_diff, 0);
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
        i ? 0 : 1);

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
        1);
    }
}

/**
 * @param new_tracks Whether midi/audio regions
 *   were moved to new tracks.
 */
static void
check_after_duplicate_timeline (
  int  new_tracks)
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
      &mn->region_id, &r->id));
  g_assert_cmpuint (mn->val, ==, MN_VAL);
  g_assert_cmpuint (mn->vel->vel, ==, MN_VEL);
  g_assert_cmppos (&obj->pos, &p1);
  g_assert_cmppos (&obj->end_pos, &p2);

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
      &mn->region_id, &r->id));
  g_assert_cmpuint (mn->val, ==, MN_VAL);
  g_assert_cmpuint (mn->vel->vel, ==, MN_VEL);
  g_assert_cmppos (&obj->pos, &p1);
  g_assert_cmppos (&obj->end_pos, &p2);

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

  if (!new_tracks)
    {
      /* check automation region */
      AutomationTracklist * atl =
        track_get_automation_tracklist (P_MASTER_TRACK);
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
        MOVE_TICKS, 0);

      /* do duplicate */
      UndoableAction * ua =
        arranger_selections_action_new_duplicate_timeline (
          TL_SELECTIONS, MOVE_TICKS,
          i > 0 ? 2 : 0, 0);
      undo_manager_perform (UNDO_MANAGER, ua);

      /* check */
      check_after_duplicate_timeline (i);

      /* undo and check that the objects are at
       * their original state*/
      undo_manager_undo (UNDO_MANAGER);
      check_timeline_objects_vs_original_state (
        0);

      /* redo and check that the objects are moved
       * again */
      undo_manager_redo (UNDO_MANAGER);
      check_after_duplicate_timeline (i);

      /* undo again to prepare for next test */
      undo_manager_undo (UNDO_MANAGER);
      check_timeline_objects_vs_original_state (
        0);
    }
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

  return g_test_run ();
}

