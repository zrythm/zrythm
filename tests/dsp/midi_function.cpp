// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#include "dsp/midi_function.h"
#include "dsp/tracklist.h"

#include <glib.h>

#include "tests/helpers/zrythm_helper.h"

static void
test_crescendo (void)
{
  test_helper_zrythm_init ();

  bool success = track_create_with_action (
    TrackType::TRACK_TYPE_MIDI, NULL, NULL, NULL, TRACKLIST->tracks.size (), 1,
    -1, NULL, NULL);
  g_assert_true (success);
  Track *  midi_track = TRACKLIST->tracks[TRACKLIST->tracks.size () - 1];
  Position pos, end_pos;
  position_set_to_bar (&pos, 1);
  position_set_to_bar (&end_pos, 4);
  Region * r1 =
    midi_region_new (&pos, &end_pos, track_get_name_hash (*midi_track), 0, 0);
  GError * err = NULL;
  success = track_add_region (
    midi_track, r1, NULL, 0, F_GEN_NAME, F_NO_PUBLISH_EVENTS, &err);
  g_assert_true (success);
  arranger_object_select (
    (ArrangerObject *) r1, F_SELECT, F_NO_APPEND, F_PUBLISH_EVENTS);
  arranger_selections_action_perform_create (TL_SELECTIONS, NULL);

  MidiNote * mn;

  position_set_to_bar (&pos, 1);
  position_set_to_bar (&end_pos, 2);
  mn = midi_note_new (&r1->id, &pos, &end_pos, 34, 50);
  midi_region_add_midi_note (r1, mn, F_NO_PUBLISH_EVENTS);
  arranger_object_select (
    (ArrangerObject *) mn, F_SELECT, F_NO_APPEND, F_NO_PUBLISH_EVENTS);
  arranger_selections_action_perform_create (MA_SELECTIONS, NULL);
  MidiNote * mn1 = mn;

  position_set_to_bar (&pos, 2);
  position_set_to_bar (&end_pos, 3);
  mn = midi_note_new (&r1->id, &pos, &end_pos, 34, 50);
  midi_region_add_midi_note (r1, mn, F_NO_PUBLISH_EVENTS);
  arranger_object_select (
    (ArrangerObject *) mn, F_SELECT, F_NO_APPEND, F_NO_PUBLISH_EVENTS);
  arranger_selections_action_perform_create (MA_SELECTIONS, NULL);
  MidiNote * mn2 = mn;

  arranger_object_select (
    (ArrangerObject *) mn1, F_SELECT, F_NO_APPEND, F_NO_PUBLISH_EVENTS);
  arranger_object_select (
    (ArrangerObject *) mn2, F_SELECT, F_APPEND, F_NO_PUBLISH_EVENTS);

  MidiFunctionOpts opts = { 0 };
  opts.curve_algo = CurveAlgorithm::EXPONENT;
  opts.curviness = 0.5;
  int res;

  /* test crescendo */
  opts.start_vel = 30;
  opts.end_vel = 90;
  res = midi_function_apply (
    (ArrangerSelections *) MA_SELECTIONS,
    MidiFunctionType::MIDI_FUNCTION_CRESCENDO, opts, NULL);
  g_assert_cmpint (res, ==, 0);
  g_assert_cmpuint (mn1->vel->vel, ==, 30);
  g_assert_cmpuint (mn2->vel->vel, ==, 90);

  /* test diminuendo */
  opts.start_vel = 90;
  opts.end_vel = 30;
  res = midi_function_apply (
    (ArrangerSelections *) MA_SELECTIONS,
    MidiFunctionType::MIDI_FUNCTION_CRESCENDO, opts, NULL);
  g_assert_cmpint (res, ==, 0);
  g_assert_cmpuint (mn1->vel->vel, ==, 90);
  g_assert_cmpuint (mn2->vel->vel, ==, 30);

  test_helper_zrythm_cleanup ();
}

int
main (int argc, char * argv[])
{
  g_test_init (&argc, &argv, NULL);

#define TEST_PREFIX "/dsp/midi_function/"

  g_test_add_func (TEST_PREFIX "test crescendo", (GTestFunc) test_crescendo);

  return g_test_run ();
}
