/*
 * Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
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

#include "actions/range_action.h"
#include "actions/undoable_action.h"
#include "actions/undo_manager.h"
#include "audio/audio_region.h"
#include "audio/automation_region.h"
#include "audio/chord_region.h"
#include "audio/control_port.h"
#include "audio/master_track.h"
#include "audio/midi_note.h"
#include "audio/region.h"
#include "project.h"
#include "utils/flags.h"
#include "zrythm.h"

#include "tests/helpers/project.h"

#include <glib.h>

#define PLAYHEAD_BEFORE 6
#define LOOP_START_BEFORE 1
#define LOOP_END_BEFORE 5

#define RANGE_START_BAR 3
#define RANGE_END_BAR 4
#define RANGE_SIZE_IN_BARS \
  (RANGE_END_BAR - RANGE_START_BAR)

#define MIDI_REGION_START_BAR \
  (RANGE_START_BAR + 2)
#define MIDI_REGION_END_BAR \
  (MIDI_REGION_START_BAR + 2)

static int midi_track_pos = -1;

static void
test_prepare_common (void)
{
  test_helper_zrythm_init ();

  /* create MIDI track with region */
  UndoableAction * ua =
    create_tracks_action_new (
      TRACK_TYPE_MIDI, NULL, NULL,
      TRACKLIST->num_tracks, NULL, 1);
  undo_manager_perform (UNDO_MANAGER, ua);
  midi_track_pos = TRACKLIST->num_tracks - 1;
  Track * midi_track =
    TRACKLIST->tracks[midi_track_pos];

  Position start, end;
  position_set_to_bar (
    &start, MIDI_REGION_START_BAR);
  position_set_to_bar (
    &end, MIDI_REGION_END_BAR);
  ZRegion * midi_region =
    midi_region_new (
      &start, &end, midi_track_pos, 0, 0);
  track_add_region (
    midi_track, midi_region, NULL, 0, F_GEN_NAME,
    F_NO_PUBLISH_EVENTS);
  arranger_object_select (
    (ArrangerObject *) midi_region, F_SELECT,
    F_NO_APPEND);
  ua =
    arranger_selections_action_new_create (
      (ArrangerSelections *) TL_SELECTIONS);
  undo_manager_perform (UNDO_MANAGER, ua);

  /* set transport positions */
  position_set_to_bar (
    &TRANSPORT->playhead_pos, PLAYHEAD_BEFORE);
  position_set_to_bar (
    &TRANSPORT->loop_start_pos, LOOP_START_BEFORE);
  position_set_to_bar (
    &TRANSPORT->loop_end_pos, LOOP_END_BEFORE);
}

static void
check_after_common (void)
{
  /* get expected transport positions */
  Position playhead_after_expected,
           loop_end_after_expected;
  position_set_to_bar (
    &playhead_after_expected,
    PLAYHEAD_BEFORE + RANGE_SIZE_IN_BARS);
  position_set_to_bar (
    &loop_end_after_expected,
    LOOP_END_BEFORE + RANGE_SIZE_IN_BARS);

  /* check that they match with actual positions */
  g_assert_cmppos (
    &playhead_after_expected,
    &TRANSPORT->playhead_pos);
  g_assert_cmppos (
    &loop_end_after_expected,
    &TRANSPORT->loop_end_pos);
}

static void
test_insert_silence_no_intersections (void)
{
  test_prepare_common ();

  /* get object */
  Track * midi_track =
    TRACKLIST->tracks[midi_track_pos];
  ZRegion * midi_region =
    midi_track->lanes[0]->regions[0];
  ArrangerObject * midi_region_obj =
    (ArrangerObject *) midi_region;

  /* get object info */
  Position midi_region_start_before,
           midi_region_end_before;
  position_set_to_pos (
    &midi_region_start_before,
    &midi_region_obj->pos);
  position_set_to_pos (
    &midi_region_end_before,
    &midi_region_obj->end_pos);

  /* create inset silence action */
  Position start, end;
  position_set_to_bar (
    &start, RANGE_START_BAR);
  position_set_to_bar (
    &end, RANGE_END_BAR);
  UndoableAction * ua =
    range_action_new_insert_silence (&start, &end);

  /* verify that number of objects is as expected */
  RangeAction * ra = (RangeAction *) ua;
  g_assert_cmpint (
    arranger_selections_get_num_objects (
      (ArrangerSelections *) ra->sel_before), ==, 1);

  /* perform action */
  undo_manager_perform (UNDO_MANAGER, ua);

  /* get expected positions */
  Position midi_region_start_after_expected,
           midi_region_end_after_expected;
  position_set_to_pos (
    &midi_region_start_after_expected,
    &midi_region_start_before);
  position_add_bars (
    &midi_region_start_after_expected,
    RANGE_SIZE_IN_BARS);
  position_set_to_pos (
    &midi_region_end_after_expected,
    &midi_region_end_before);
  position_add_bars (
    &midi_region_end_after_expected,
    RANGE_SIZE_IN_BARS);

  /* check start and end as expected */
  g_assert_cmppos (
    &midi_region_obj->pos,
    &midi_region_start_after_expected);
  g_assert_cmppos (
    &midi_region_obj->end_pos,
    &midi_region_end_after_expected);

  check_after_common ();

  test_helper_zrythm_cleanup ();
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

#define TEST_PREFIX "/actions/range/"

  g_test_add_func (
    TEST_PREFIX
    "test insert silence no interesections",
    (GTestFunc)
    test_insert_silence_no_intersections);

  return g_test_run ();
}
