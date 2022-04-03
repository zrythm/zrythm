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

#include "zrythm-test-config.h"

#include "audio/midi_note.h"
#include "audio/region.h"
#include "project.h"
#include "utils/flags.h"
#include "zrythm.h"

#include <glib.h>

#if 0
typedef struct
{
  ZRegion * region;
} MidiNoteFixture;

static void
fixture_set_up (
  MidiNoteFixture * fixture)
{
  engine_update_frames_per_tick (
    AUDIO_ENGINE, 4, 140, 44000);

  Position start_pos, end_pos;
  position_set_to_bar (&start_pos, 2);
  position_set_to_bar (&end_pos, 6);
  fixture->region =
    midi_region_new (
      &start_pos, &end_pos, 0, 0, 0);
}

static void
test_new_midi_note ()
{
  MidiNoteFixture _fixture;
  MidiNoteFixture * fixture =
    &_fixture;
  fixture_set_up (fixture);

  int val = 23, vel = 90;
  Position start_pos, end_pos;
  position_set_to_bar (&start_pos, 2);
  position_set_to_bar (&end_pos, 3);
  MidiNote * mn =
    midi_note_new (
      &fixture->region->id,
      &start_pos, &end_pos, val,
      VELOCITY_DEFAULT);
  ArrangerObject * mn_obj =
    (ArrangerObject *) mn;

  g_assert_nonnull (mn);
  g_assert_true (
    position_is_equal (
      &start_pos, &mn_obj->pos));
  g_assert_true (
    position_is_equal (
      &end_pos, &mn_obj->end_pos));

  g_assert_cmpint (mn->vel->vel, ==, vel);
  g_assert_cmpint (mn->val, ==, val);
  g_assert_false (mn->muted);

  ZRegion * region =
    arranger_object_get_region (
      (ArrangerObject *) mn);
  ZRegion * r_clone =
    (ZRegion *)
    arranger_object_clone (
      (ArrangerObject *) region,
      ARRANGER_OBJECT_CLONE_COPY);
  midi_note_set_region_and_index (mn, r_clone, 0);

  g_assert_cmpint (
    region_identifier_is_equal (
      &mn_obj->region_id, &r_clone->id), ==, 1);

  MidiNote * mn_clone =
    (MidiNote *)
    arranger_object_clone (
      (ArrangerObject *) mn,
      ARRANGER_OBJECT_CLONE_COPY);
  g_assert_cmpint (
    midi_note_is_equal (mn, mn_clone), ==, 1);
}
#endif

int
main (int argc, char * argv[])
{
  g_test_init (&argc, &argv, NULL);

#define TEST_PREFIX "/audio/midi_note/"

  /*g_test_add_func (*/
  /*TEST_PREFIX "test new midi note",*/
  /*(GTestFunc) test_new_midi_note);*/

  return g_test_run ();
}
