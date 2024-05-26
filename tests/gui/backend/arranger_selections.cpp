// SPDX-FileCopyrightText: © 2021-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#include "dsp/midi_region.h"
#include "gui/backend/arranger_selections.h"
#include "project.h"
#include "utils/flags.h"
#include "zrythm.h"

#include <glib.h>

#include "tests/helpers/zrythm.h"

#include <cmath>
#include <locale.h>

static void
test_region_length_in_ticks (Track * track, int bar_start, int bar_end)
{
  Position p1, p2;
  position_set_to_bar (&p1, bar_start);
  position_set_to_bar (&p2, bar_end);
  ZRegion * r = midi_region_new (&p1, &p2, track_get_name_hash (track), 0, 0);
  ArrangerObject * r_obj = (ArrangerObject *) r;
  GError *         err = NULL;
  bool             success =
    track_add_region (track, r, NULL, 0, F_GEN_NAME, F_NO_PUBLISH_EVENTS, &err);
  g_assert_true (success);

  arranger_object_select (r_obj, F_SELECT, F_NO_APPEND, F_NO_PUBLISH_EVENTS);

  double length = arranger_selections_get_length_in_ticks (
    (ArrangerSelections *) TL_SELECTIONS);
  g_assert_cmpfloat_with_epsilon (
    length, TRANSPORT->ticks_per_bar * (bar_end - bar_start), 0.00001);
}

static void
test_get_length_in_ticks (void)
{
  test_helper_zrythm_init ();

  Track * track =
    track_create_empty_with_action (TrackType::TRACK_TYPE_MIDI, NULL);

  test_region_length_in_ticks (track, 3, 4);
  test_region_length_in_ticks (track, 100, 102);
  test_region_length_in_ticks (track, 1000, 1010);

  test_helper_zrythm_cleanup ();
}

static void
test_get_last_object (void)
{
  test_helper_zrythm_init ();

  Track * track =
    track_create_empty_with_action (TrackType::TRACK_TYPE_MIDI, NULL);

  Position p1, p2;
  position_set_to_bar (&p1, 3);
  position_set_to_bar (&p2, 4);
  ZRegion * r = midi_region_new (&p1, &p2, track_get_name_hash (track), 0, 0);
  GError *  err = NULL;
  bool      success =
    track_add_region (track, r, NULL, 0, F_GEN_NAME, F_NO_PUBLISH_EVENTS, &err);
  g_assert_true (success);

  position_from_frames (&p1, -40000);
  position_from_frames (&p2, -4000);
  MidiNote *       mn = midi_note_new (&r->id, &p1, &p2, 60, 60);
  ArrangerObject * mn_obj = (ArrangerObject *) mn;
  midi_region_add_midi_note (r, mn, F_NO_PUBLISH_EVENTS);

  arranger_object_select (mn_obj, F_SELECT, F_NO_APPEND, F_NO_PUBLISH_EVENTS);

  ArrangerObject * last_obj = arranger_selections_get_last_object (
    (ArrangerSelections *) MA_SELECTIONS, true);
  g_assert_nonnull (last_obj);
  g_assert_true (last_obj == mn_obj);

  test_helper_zrythm_cleanup ();
}

static void
test_contains_object_with_property (void)
{
  test_helper_zrythm_init ();

  Track * track =
    track_create_empty_with_action (TrackType::TRACK_TYPE_MIDI, NULL);

  Position p1, p2;
  position_set_to_bar (&p1, 3);
  position_set_to_bar (&p2, 4);
  ZRegion * r = midi_region_new (&p1, &p2, track_get_name_hash (track), 0, 0);
  GError *  err = NULL;
  bool      success =
    track_add_region (track, r, NULL, 0, F_GEN_NAME, F_NO_PUBLISH_EVENTS, &err);
  g_assert_true (success);

  position_from_frames (&p1, -40000);
  position_from_frames (&p2, -4000);
  MidiNote *       mn = midi_note_new (&r->id, &p1, &p2, 60, 60);
  ArrangerObject * mn_obj = (ArrangerObject *) mn;
  midi_region_add_midi_note (r, mn, F_NO_PUBLISH_EVENTS);

  arranger_object_select (mn_obj, F_SELECT, F_NO_APPEND, F_NO_PUBLISH_EVENTS);

  bool ret = arranger_selections_contains_object_with_property (
    (ArrangerSelections *) MA_SELECTIONS,
    ArrangerSelectionsProperty::ARRANGER_SELECTIONS_PROPERTY_HAS_LENGTH, true);
  g_assert_true (ret);
  ret = arranger_selections_contains_object_with_property (
    (ArrangerSelections *) MA_SELECTIONS,
    ArrangerSelectionsProperty::ARRANGER_SELECTIONS_PROPERTY_HAS_LENGTH, false);
  g_assert_false (ret);

  test_helper_zrythm_cleanup ();
}

int
main (int argc, char * argv[])
{
  g_test_init (&argc, &argv, NULL);

#define TEST_PREFIX "/gui/backend/arranger selections/"

  g_test_add_func (
    TEST_PREFIX "test contains object with property",
    (GTestFunc) test_contains_object_with_property);
  g_test_add_func (
    TEST_PREFIX "test get last object", (GTestFunc) test_get_last_object);
  g_test_add_func (
    TEST_PREFIX "test get length in ticks",
    (GTestFunc) test_get_length_in_ticks);

  return g_test_run ();
}
