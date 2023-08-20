// SPDX-FileCopyrightText: © 2020-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#include "actions/range_action.h"
#include "actions/tracklist_selections.h"
#include "actions/undo_manager.h"
#include "actions/undoable_action.h"
#include "dsp/audio_region.h"
#include "dsp/automation_region.h"
#include "dsp/chord_region.h"
#include "dsp/control_port.h"
#include "dsp/master_track.h"
#include "dsp/midi_note.h"
#include "dsp/region.h"
#include "project.h"
#include "utils/flags.h"
#include "zrythm.h"

#include <glib.h>

#include "tests/helpers/project.h"

#define RANGE_START_BAR 4
#define RANGE_END_BAR 10
#define RANGE_SIZE_IN_BARS \
  (RANGE_END_BAR - RANGE_START_BAR) /* 6 */

/* cue is before the range */
#define CUE_BEFORE (RANGE_START_BAR - 1)

/* playhead is after the range */
#define PLAYHEAD_BEFORE (RANGE_END_BAR + 2)
#define LOOP_START_BEFORE 1

/* loop end is inside the range */
#define LOOP_END_BEFORE (RANGE_START_BAR + 1)

/* audio region starts before the range start and
 * ends in the middle of the range */
#define AUDIO_REGION_START_BAR (RANGE_START_BAR - 1)
#define AUDIO_REGION_END_BAR (RANGE_START_BAR + 1)

/* midi region starting after the range end */
#define MIDI_REGION_START_BAR (RANGE_END_BAR + 2)
#define MIDI_REGION_END_BAR (MIDI_REGION_START_BAR + 2)

/* midi region starting even further after range
 * end */
#define MIDI_REGION2_START_BAR (MIDI_REGION_START_BAR + 10)
#define MIDI_REGION2_END_BAR (MIDI_REGION2_START_BAR + 8)

/* midi region starting before the range and ending
 * inside the range */
#define MIDI_REGION3_START_BAR (RANGE_START_BAR - 1)
#define MIDI_REGION3_END_BAR (RANGE_START_BAR + 2)

/* another midi region starting before the range and
 * ending inside the range */
#define MIDI_REGION4_START_BAR (RANGE_START_BAR - 2)
#define MIDI_REGION4_END_BAR (RANGE_START_BAR + 1)

/* midi region starting before the range and ending
 * after the range */
#define MIDI_REGION5_START_BAR (RANGE_START_BAR - 1)
#define MIDI_REGION5_END_BAR (RANGE_END_BAR + 1)

/* midi region starting inside the range and ending
 * after the range */
#define MIDI_REGION6_START_BAR (RANGE_START_BAR + 1)
#define MIDI_REGION6_END_BAR (RANGE_END_BAR + 1)

/* midi region ending before the range start */
#define MIDI_REGION7_START_BAR 1
#define MIDI_REGION7_END_BAR 2

static int midi_track_pos = -1;
static int audio_track_pos = -1;

static void
test_prepare_common (void)
{
  test_helper_zrythm_init ();

  /* create MIDI track with region */
  Track * midi_track =
    track_create_empty_with_action (TRACK_TYPE_MIDI, NULL);
  midi_track_pos = midi_track->pos;

  bool success;

#define ADD_MREGION(start_bar, end_bar) \
  position_set_to_bar (&start, start_bar); \
  position_set_to_bar (&end, end_bar); \
  midi_region = midi_region_new ( \
    &start, &end, track_get_name_hash (midi_track), 0, \
    midi_track->lanes[0]->num_regions); \
  success = track_add_region ( \
    midi_track, midi_region, NULL, 0, F_GEN_NAME, \
    F_NO_PUBLISH_EVENTS, NULL); \
  g_assert_true (success); \
  g_assert_cmpint (midi_region->id.idx, >=, 0); \
  arranger_object_select ( \
    (ArrangerObject *) midi_region, F_SELECT, F_NO_APPEND, \
    F_NO_PUBLISH_EVENTS); \
  arranger_selections_action_perform_create ( \
    (ArrangerSelections *) TL_SELECTIONS, NULL)

  /* add all the midi regions */
  Position  start, end;
  ZRegion * midi_region;
  ADD_MREGION (MIDI_REGION_START_BAR, MIDI_REGION_END_BAR);
  ADD_MREGION (MIDI_REGION2_START_BAR, MIDI_REGION2_END_BAR);
  ADD_MREGION (MIDI_REGION3_START_BAR, MIDI_REGION3_END_BAR);
  ADD_MREGION (MIDI_REGION4_START_BAR, MIDI_REGION4_END_BAR);
  ADD_MREGION (MIDI_REGION5_START_BAR, MIDI_REGION5_END_BAR);
  ADD_MREGION (MIDI_REGION6_START_BAR, MIDI_REGION6_END_BAR);
  ADD_MREGION (MIDI_REGION7_START_BAR, MIDI_REGION7_END_BAR);

  /* create audio track with region */
  char * filepath =
    g_build_filename (TESTS_SRCDIR, "test.wav", NULL);
  SupportedFile * file =
    supported_file_new_from_path (filepath);
  g_free (filepath);
  position_set_to_bar (&start, AUDIO_REGION_START_BAR);
  position_set_to_bar (&end, AUDIO_REGION_END_BAR);
  track_create_with_action (
    TRACK_TYPE_AUDIO, NULL, file, &start,
    TRACKLIST->num_tracks, 1, -1, NULL, NULL);
  Track * audio_track = tracklist_get_last_track (
    TRACKLIST, TRACKLIST_PIN_OPTION_BOTH, false);
  audio_track_pos = audio_track->pos;
  ZRegion * audio_region = audio_track->lanes[0]->regions[0];

  /* resize audio region */
  arranger_object_select (
    (ArrangerObject *) audio_region, F_SELECT, F_NO_APPEND,
    F_NO_PUBLISH_EVENTS);
  double audio_region_size_ticks =
    arranger_object_get_length_in_ticks (
      (ArrangerObject *) audio_region);
  double missing_ticks =
    (end.ticks - start.ticks) - audio_region_size_ticks;
  success = arranger_object_resize (
    (ArrangerObject *) audio_region, false,
    ARRANGER_OBJECT_RESIZE_LOOP, missing_ticks, false, NULL);
  g_assert_true (success);
  arranger_selections_action_perform_resize (
    (ArrangerSelections *) TL_SELECTIONS,
    ARRANGER_SELECTIONS_ACTION_RESIZE_R_LOOP, missing_ticks,
    F_ALREADY_EDITED, NULL);
  g_assert_cmppos (&end, &audio_region->base.end_pos);

  /* set transport positions */
  position_set_to_bar (&TRANSPORT->cue_pos, CUE_BEFORE);
  position_set_to_bar (
    &TRANSPORT->playhead_pos, PLAYHEAD_BEFORE);
  position_set_to_bar (
    &TRANSPORT->loop_start_pos, LOOP_START_BEFORE);
  position_set_to_bar (
    &TRANSPORT->loop_end_pos, LOOP_END_BEFORE);
}

static void
check_start_end_markers (void)
{
  /* check that start/end markers exist */
  Marker * start_m =
    marker_track_get_start_marker (P_MARKER_TRACK);
  g_assert_true (start_m->type == MARKER_TYPE_START);
  ArrangerObject * start_obj = (ArrangerObject *) start_m;
  Marker *         end_m =
    marker_track_get_end_marker (P_MARKER_TRACK);
  g_assert_true (end_m->type == MARKER_TYPE_END);
  ArrangerObject * end_obj = (ArrangerObject *) end_m;
  g_assert_nonnull (start_m);
  g_assert_nonnull (end_m);

  /* check positions are valid */
  Position init_pos;
  position_init (&init_pos);
  g_assert_true (
    position_is_after_or_equal (&start_obj->pos, &init_pos));
  g_assert_true (
    position_is_after_or_equal (&end_obj->pos, &init_pos));
}

static void
check_before_insert (void)
{
  Track * midi_track = TRACKLIST->tracks[midi_track_pos];
  g_assert_cmpint (midi_track->lanes[0]->num_regions, ==, 7);

#define GET_MIDI_REGION(name, idx) \
  ZRegion *        name = midi_track->lanes[0]->regions[idx]; \
  ArrangerObject * name##_obj = (ArrangerObject *) name

  GET_MIDI_REGION (midi_region, 0);
  GET_MIDI_REGION (midi_region2, 1);
  GET_MIDI_REGION (midi_region3, 2);
  GET_MIDI_REGION (midi_region4, 3);
  GET_MIDI_REGION (midi_region5, 4);
  GET_MIDI_REGION (midi_region6, 5);
  GET_MIDI_REGION (midi_region7, 6);

#undef GET_MIDI_REGION

  Track *   audio_track = TRACKLIST->tracks[audio_track_pos];
  ZRegion * audio_region = audio_track->lanes[0]->regions[0];
  g_assert_cmpint (audio_track->lanes[0]->num_regions, ==, 1);
  ArrangerObject * audio_region_obj =
    (ArrangerObject *) audio_region;

#define CHECK_POS(obj, start_bar, end_bar) \
  position_set_to_bar (&start, start_bar); \
  position_set_to_bar (&end, end_bar); \
  g_assert_cmppos (&obj->pos, &start); \
  g_assert_cmppos (&obj->end_pos, &end)

  Position start, end;
  CHECK_POS (
    midi_region_obj, MIDI_REGION_START_BAR,
    MIDI_REGION_END_BAR);
  CHECK_POS (
    midi_region2_obj, MIDI_REGION2_START_BAR,
    MIDI_REGION2_END_BAR);
  CHECK_POS (
    midi_region3_obj, MIDI_REGION3_START_BAR,
    MIDI_REGION3_END_BAR);
  CHECK_POS (
    midi_region4_obj, MIDI_REGION4_START_BAR,
    MIDI_REGION4_END_BAR);
  CHECK_POS (
    midi_region5_obj, MIDI_REGION5_START_BAR,
    MIDI_REGION5_END_BAR);
  CHECK_POS (
    midi_region6_obj, MIDI_REGION6_START_BAR,
    MIDI_REGION6_END_BAR);
  CHECK_POS (
    midi_region7_obj, MIDI_REGION7_START_BAR,
    MIDI_REGION7_END_BAR);
  CHECK_POS (
    audio_region_obj, AUDIO_REGION_START_BAR,
    AUDIO_REGION_END_BAR);

#undef CHECK_POS

  check_start_end_markers ();
}

static void
check_after_insert (void)
{

#define GET_MIDI_REGION(name, idx) \
  ZRegion *        name = midi_track->lanes[0]->regions[idx]; \
  ArrangerObject * name##_obj = (ArrangerObject *) name

  /* get objects */
  Track * midi_track = TRACKLIST->tracks[midi_track_pos];
  g_assert_cmpint (midi_track->lanes[0]->num_regions, ==, 10);
  GET_MIDI_REGION (midi_region1, 0);
  GET_MIDI_REGION (midi_region2, 1);
  GET_MIDI_REGION (midi_region3, 2);
  GET_MIDI_REGION (midi_region4, 3);
  GET_MIDI_REGION (midi_region5, 4);
  GET_MIDI_REGION (midi_region6, 5);
  GET_MIDI_REGION (midi_region7, 6);
  GET_MIDI_REGION (midi_region8, 7);
  GET_MIDI_REGION (midi_region9, 8);
  GET_MIDI_REGION (midi_region10, 9);

#undef GET_MIDI_REGION

  /* get new audio regions */
  Track * audio_track = TRACKLIST->tracks[audio_track_pos];
  g_assert_cmpint (audio_track->lanes[0]->num_regions, ==, 2);
  ZRegion * audio_region1 = audio_track->lanes[0]->regions[0];
  ZRegion * audio_region2 = audio_track->lanes[0]->regions[1];
  ArrangerObject * audio_region_obj1 =
    (ArrangerObject *) audio_region1;
  ArrangerObject * audio_region_obj2 =
    (ArrangerObject *) audio_region2;

#define CHECK_POS(obj, start_bar, end_bar) \
  position_set_to_bar (&start_expected, start_bar); \
  position_set_to_bar (&end_expected, end_bar); \
  g_assert_cmppos (&obj->pos, &start_expected); \
  g_assert_cmppos (&obj->end_pos, &end_expected)

  /* check midi region positions */
  Position start_expected, end_expected;
  CHECK_POS (
    midi_region1_obj,
    MIDI_REGION_START_BAR + RANGE_SIZE_IN_BARS,
    MIDI_REGION_END_BAR + RANGE_SIZE_IN_BARS);
  CHECK_POS (
    midi_region2_obj,
    MIDI_REGION2_START_BAR + RANGE_SIZE_IN_BARS,
    MIDI_REGION2_END_BAR + RANGE_SIZE_IN_BARS);

  /* orig 3 */
  CHECK_POS (
    midi_region9_obj, MIDI_REGION3_START_BAR, RANGE_START_BAR);
  CHECK_POS (
    midi_region10_obj, RANGE_END_BAR,
    MIDI_REGION3_END_BAR + RANGE_SIZE_IN_BARS);

  /* orig 4 */
  CHECK_POS (
    midi_region7_obj, MIDI_REGION4_START_BAR, RANGE_START_BAR);
  CHECK_POS (
    midi_region8_obj, RANGE_END_BAR,
    MIDI_REGION4_END_BAR + RANGE_SIZE_IN_BARS);

  /* orig 5 */
  CHECK_POS (
    midi_region5_obj, MIDI_REGION5_START_BAR, RANGE_START_BAR);
  CHECK_POS (
    midi_region6_obj, RANGE_END_BAR,
    MIDI_REGION5_END_BAR + RANGE_SIZE_IN_BARS);

  /* orig 6 */
  CHECK_POS (
    midi_region3_obj,
    MIDI_REGION6_START_BAR + RANGE_SIZE_IN_BARS,
    MIDI_REGION6_END_BAR + RANGE_SIZE_IN_BARS);

  /* orig 7 */
  CHECK_POS (
    midi_region4_obj, MIDI_REGION7_START_BAR,
    MIDI_REGION7_END_BAR);

#undef CHECK_POS

  /* check audio region positions */
  Position audio_region1_end_after_expected,
    audio_region2_start_after_expected,
    audio_region2_end_after_expected;
  position_set_to_bar (
    &audio_region1_end_after_expected, RANGE_START_BAR);
  position_set_to_bar (
    &audio_region2_start_after_expected, RANGE_END_BAR);
  position_set_to_bar (
    &audio_region2_end_after_expected,
    AUDIO_REGION_END_BAR + RANGE_SIZE_IN_BARS);
  g_assert_cmppos (
    &audio_region_obj1->end_pos,
    &audio_region1_end_after_expected);
  g_assert_cmppos (
    &audio_region_obj2->pos,
    &audio_region2_start_after_expected);
  g_assert_cmppos (
    &audio_region_obj2->end_pos,
    &audio_region2_end_after_expected);

#define CHECK_TPOS(name, expected_bar) \
  position_set_to_bar (&start_expected, expected_bar); \
  g_assert_cmppos (&TRANSPORT->name, &start_expected);

  /* check transport positions */
  CHECK_TPOS (
    playhead_pos, PLAYHEAD_BEFORE + RANGE_SIZE_IN_BARS);
  CHECK_TPOS (
    loop_end_pos, LOOP_END_BEFORE + RANGE_SIZE_IN_BARS);
  CHECK_TPOS (cue_pos, CUE_BEFORE);

#undef CHECK_TPOS

  check_start_end_markers ();
}

static void
test_insert_silence (void)
{
  test_prepare_common ();

  /* create inset silence action */
  Position start, end;
  position_set_to_bar (&start, RANGE_START_BAR);
  position_set_to_bar (&end, RANGE_END_BAR);
  UndoableAction * ua =
    range_action_new_insert_silence (&start, &end, NULL);

  /* verify that number of objects is as expected */
  RangeAction * ra = (RangeAction *) ua;
  g_assert_cmpint (
    arranger_selections_get_num_objects (
      (ArrangerSelections *) ra->sel_before),
    ==, 7);

  check_before_insert ();

  /* perform action */
  undo_manager_perform (UNDO_MANAGER, ua, NULL);

  check_after_insert ();

  /* undo and verify things are back to previous
   * state */
  undo_manager_undo (UNDO_MANAGER, NULL);

  check_before_insert ();

  undo_manager_redo (UNDO_MANAGER, NULL);

  check_after_insert ();

  test_helper_zrythm_cleanup ();
}

static void
check_after_remove (void)
{
#define GET_MIDI_REGION(name, idx) \
  ZRegion *        name = midi_track->lanes[0]->regions[idx]; \
  ArrangerObject * name##_obj = (ArrangerObject *) name

  test_project_save_and_reload ();

  /* get objects */
  Track * midi_track = TRACKLIST->tracks[midi_track_pos];
  g_assert_cmpint (midi_track->lanes[0]->num_regions, ==, 8);
  GET_MIDI_REGION (midi_region1, 0);
  GET_MIDI_REGION (midi_region2, 1);
  GET_MIDI_REGION (midi_region3, 2);
  GET_MIDI_REGION (midi_region4, 3);
  GET_MIDI_REGION (midi_region5, 4);
  GET_MIDI_REGION (midi_region6, 5);
  GET_MIDI_REGION (midi_region7, 6);
  GET_MIDI_REGION (midi_region8, 7);

  /* get new audio regions */
  Track * audio_track = TRACKLIST->tracks[audio_track_pos];
  g_assert_cmpint (audio_track->lanes[0]->num_regions, ==, 1);
  ZRegion * audio_region = audio_track->lanes[0]->regions[0];
  ArrangerObject * audio_region_obj =
    (ArrangerObject *) audio_region;

#define CHECK_POS(obj, start_bar, end_bar) \
  position_set_to_bar (&start_expected, start_bar); \
  position_set_to_bar (&end_expected, end_bar); \
  g_assert_cmppos (&obj->pos, &start_expected); \
  g_assert_cmppos (&obj->end_pos, &end_expected)

  /* check midi region positions */
  Position start_expected, end_expected;

  /* orig 1 */
  CHECK_POS (
    midi_region1_obj,
    MIDI_REGION_START_BAR - RANGE_SIZE_IN_BARS,
    MIDI_REGION_END_BAR - RANGE_SIZE_IN_BARS);

  /* orig 2 */
  CHECK_POS (
    midi_region2_obj,
    MIDI_REGION2_START_BAR - RANGE_SIZE_IN_BARS,
    MIDI_REGION2_END_BAR - RANGE_SIZE_IN_BARS);

  /* orig 3 */
  CHECK_POS (
    midi_region8_obj, MIDI_REGION3_START_BAR, RANGE_START_BAR);

  /* orig 4 */
  CHECK_POS (
    midi_region7_obj, MIDI_REGION4_START_BAR, RANGE_START_BAR);

  /* orig 5 */
  CHECK_POS (
    midi_region5_obj, MIDI_REGION5_START_BAR, RANGE_START_BAR);
  CHECK_POS (
    midi_region6_obj, RANGE_START_BAR,
    MIDI_REGION5_END_BAR - RANGE_SIZE_IN_BARS);

  /* orig 6 */
  CHECK_POS (
    midi_region4_obj, RANGE_START_BAR,
    MIDI_REGION6_END_BAR - RANGE_SIZE_IN_BARS);

  /* orig 7 */
  CHECK_POS (
    midi_region3_obj, MIDI_REGION7_START_BAR,
    MIDI_REGION7_END_BAR);

#undef CHECK_POS

  /* check audio region positions */
  Position audio_region_start_after_expected,
    audio_region_end_after_expected;
  position_set_to_bar (
    &audio_region_start_after_expected,
    AUDIO_REGION_START_BAR);
  position_set_to_bar (
    &audio_region_end_after_expected, RANGE_START_BAR);
  g_assert_cmppos (
    &audio_region_obj->pos,
    &audio_region_start_after_expected);
  g_assert_cmppos (
    &audio_region_obj->end_pos,
    &audio_region_end_after_expected);

#define CHECK_TPOS(name, expected_bar) \
  position_set_to_bar (&start_expected, expected_bar); \
  g_assert_cmppos (&TRANSPORT->name, &start_expected);

  /* check transport positions */
  CHECK_TPOS (
    playhead_pos, PLAYHEAD_BEFORE - RANGE_SIZE_IN_BARS);
  CHECK_TPOS (loop_end_pos, RANGE_START_BAR);
  CHECK_TPOS (cue_pos, CUE_BEFORE);

#undef CHECK_TPOS

  check_start_end_markers ();
}

static void
test_remove_range (void)
{
  test_prepare_common ();

  /* create remove range action */
  Position start, end;
  position_set_to_bar (&start, RANGE_START_BAR);
  position_set_to_bar (&end, RANGE_END_BAR);

  check_before_insert ();

  range_action_perform_remove (&start, &end, NULL);

  check_after_remove ();

  /* undo and verify things are back to previous
   * state */
  undo_manager_undo (UNDO_MANAGER, NULL);

  check_before_insert ();

  undo_manager_redo (UNDO_MANAGER, NULL);

  check_after_remove ();

  test_helper_zrythm_cleanup ();
}

static void
test_remove_range_w_start_marker (void)
{
  test_helper_zrythm_init ();

  /* create audio track */
  audio_track_pos = TRACKLIST->num_tracks;
  SupportedFile * file =
    supported_file_new_from_path (TEST_WAV2);
  track_create_with_action (
    TRACK_TYPE_AUDIO, NULL, file, NULL, audio_track_pos, 1,
    -1, NULL, NULL);

  /* remove range */
  Position start, end;
  position_set_to_bar (&start, 1);
  position_set_to_bar (&end, 3);
  range_action_perform_remove (&start, &end, NULL);

  check_start_end_markers ();

  /* undo */
  undo_manager_undo (UNDO_MANAGER, NULL);

  check_start_end_markers ();

  test_helper_zrythm_cleanup ();
}

static void
test_remove_range_w_objects_inside (void)
{
  test_helper_zrythm_init ();

  /* create midi track with region */
  midi_track_pos = TRACKLIST->num_tracks;
  char * filepath = g_build_filename (
    TESTS_SRCDIR, "1_track_with_data.mid", NULL);
  SupportedFile * file =
    supported_file_new_from_path (filepath);
  track_create_with_action (
    TRACK_TYPE_MIDI, NULL, file, NULL, midi_track_pos, 1, -1,
    NULL, NULL);
  Track * midi_track =
    tracklist_get_track (TRACKLIST, midi_track_pos);

  /* create scale object */
  MusicalScale * ms = musical_scale_new (0, 0);
  ScaleObject *  so = scale_object_new (ms);
  chord_track_add_scale (P_CHORD_TRACK, so);
  arranger_object_select (
    (ArrangerObject *) so, F_SELECT, F_NO_APPEND,
    F_NO_PUBLISH_EVENTS);
  arranger_selections_action_perform_create (
    TL_SELECTIONS, NULL);

  /* remove range */
  Position start, end;
  position_set_to_bar (&start, 1);
  position_set_to_bar (&end, 14);
  range_action_perform_remove (&start, &end, NULL);

  check_start_end_markers ();

  /* check scale and midi region removed */
  g_assert_cmpint (midi_track->lanes[0]->num_regions, ==, 0);
  g_assert_cmpint (P_CHORD_TRACK->num_scales, ==, 0);

  /* undo */
  undo_manager_undo (UNDO_MANAGER, NULL);

  check_start_end_markers ();

  /* check scale and midi region added */
  g_assert_cmpint (midi_track->lanes[0]->num_regions, ==, 1);
  g_assert_cmpint (P_CHORD_TRACK->num_scales, ==, 1);

  test_helper_zrythm_cleanup ();
}

int
main (int argc, char * argv[])
{
  g_test_init (&argc, &argv, NULL);

#define TEST_PREFIX "/actions/range/"

  g_test_add_func (
    TEST_PREFIX "test remove range w objects inside",
    (GTestFunc) test_remove_range_w_objects_inside);
  g_test_add_func (
    TEST_PREFIX "test remove range w start marker",
    (GTestFunc) test_remove_range_w_start_marker);
  g_test_add_func (
    TEST_PREFIX "test insert silence",
    (GTestFunc) test_insert_silence);
  g_test_add_func (
    TEST_PREFIX "test remove range",
    (GTestFunc) test_remove_range);

  return g_test_run ();
}
