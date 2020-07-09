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

#include "audio/track.h"
#include "project.h"
#include "utils/flags.h"
#include "zrythm.h"

#include "tests/helpers/project.h"

#include <glib.h>
#include <locale.h>

typedef struct
{
  Track *           ins_track;
} TrackFixture;

static void
fixture_set_up (
  TrackFixture * fixture)
{
  fixture->ins_track =
    track_new (
      TRACK_TYPE_INSTRUMENT,
      TRACKLIST->num_tracks,
      "Test Instrument Track 1",
      F_WITH_LANE);
}

static void
test_new_track ()
{
  TrackFixture _fixture;
  TrackFixture * fixture =
    &_fixture;
  fixture_set_up (fixture);

  Track * track =
    fixture->ins_track;

  g_assert_nonnull (track->name);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  test_helper_zrythm_init ();

#define TEST_PREFIX "/audio/track/"

  g_test_add_func (
    TEST_PREFIX "test new track",
    (GTestFunc) test_new_track);

  return g_test_run ();
}

