/*
 * Copyright (C) 2021 Alexandros Theodotou <alex at zrythm dot org>
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

#include <math.h>

#include "audio/midi_region.h"
#include "gui/backend/arranger_selections.h"
#include "project.h"
#include "utils/flags.h"
#include "zrythm.h"

#include "tests/helpers/zrythm.h"

#include <glib.h>
#include <locale.h>

static void
test_region_length_in_ticks (
  Track * track,
  int     bar_start,
  int     bar_end)
{
  Position p1, p2;
  position_set_to_bar (&p1, bar_start);
  position_set_to_bar (&p2, bar_end);
  ZRegion * r =
    midi_region_new (
      &p1, &p2, track->pos, 0, 0);
  ArrangerObject * r_obj = (ArrangerObject *) r;
  track_add_region (
    track, r, NULL, 0, F_GEN_NAME,
    F_NO_PUBLISH_EVENTS);

  arranger_object_select (
    r_obj, F_SELECT, F_NO_APPEND, F_NO_PUBLISH_EVENTS);

  double length =
    arranger_selections_get_length_in_ticks (
      (ArrangerSelections *) TL_SELECTIONS);
  g_assert_cmpfloat_with_epsilon (
    length,
    TRANSPORT->ticks_per_bar * (bar_end - bar_start),
    0.00001);
}

static void
test_get_length_in_ticks ()
{
  test_helper_zrythm_init ();

  Track * track =
    track_create_empty_with_action (
      TRACK_TYPE_MIDI);

  test_region_length_in_ticks (track, 3, 4);
  test_region_length_in_ticks (track, 100, 102);
  test_region_length_in_ticks (track, 1000, 1010);

  test_helper_zrythm_cleanup ();
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

#define TEST_PREFIX "/gui/backend/arranger selections/"

  g_test_add_func (
    TEST_PREFIX "test get length in ticks",
    (GTestFunc) test_get_length_in_ticks);

  return g_test_run ();
}
