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

#include "audio/midi_note.h"
#include "audio/region.h"
#include "project.h"
#include "utils/flags.h"
#include "zrythm.h"

#include <glib.h>

typedef struct
{
  Region * region;
} MidiNoteFixture;

static void
fixture_set_up (
  MidiNoteFixture * fixture)
{
  transport_init (TRANSPORT, 0);
  engine_update_frames_per_tick (
    4, 140, 44000);

  Position start_pos, end_pos;
  position_set_to_bar (&start_pos, 2);
  position_set_to_bar (&end_pos, 6);
  fixture->region =
    midi_region_new (
      &start_pos, &end_pos, 1);
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
      fixture->region,
      &start_pos, &end_pos, val,
      velocity_new (vel), 1);

  g_assert_nonnull (mn);
  g_assert_true (
    position_is_equal (
      &start_pos, &mn->start_pos));
  g_assert_true (
    position_is_equal (
      &end_pos, &mn->end_pos));

  g_assert_cmpint (mn->vel->vel, ==, vel);
  g_assert_cmpint (mn->val, ==, val);
  g_assert_false (mn->muted);

  MidiNote * main_midi_note =
    midi_note_get_main_midi_note (mn);
  MidiNote * main_trans_midi_note =
    midi_note_get_main_trans_midi_note (mn);

  g_assert_true (mn == main_midi_note);
  g_assert_nonnull (main_midi_note);
  g_assert_nonnull (main_trans_midi_note);

  g_assert_true (
    midi_note_is_transient (main_trans_midi_note));
  g_assert_false (
    midi_note_is_transient (main_midi_note));

  g_assert_true (
    midi_note_is_main (main_trans_midi_note));
  g_assert_true (
    midi_note_is_main (main_midi_note));

  Region * r_clone =
    region_clone (
      mn->region, REGION_CLONE_COPY);
  midi_note_set_region (mn, r_clone);

  g_assert_true (
    main_midi_note->region == r_clone);
  g_assert_true (
    main_trans_midi_note->region == r_clone);
  g_assert_cmpstr (
    r_clone->name, ==,
    main_midi_note->region_name);
  g_assert_cmpstr (
    r_clone->name, ==,
    main_trans_midi_note->region_name);

  MidiNote * mn_clone =
    midi_note_clone (mn, MIDI_NOTE_CLONE_COPY);
  g_assert_cmpint (
    midi_note_is_equal (mn, mn_clone), ==, 1);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  ZRYTHM = calloc (1, sizeof (Zrythm));
  PROJECT = calloc (1, sizeof (Project));

#define TEST_PREFIX "/audio/midi_note/"

  g_test_add_func (
    TEST_PREFIX "test new midi note",
    (GTestFunc) test_new_midi_note);

  return g_test_run ();
}
