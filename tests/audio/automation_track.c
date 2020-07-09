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

#include "audio/channel.h"
#include "audio/automation_region.h"
#include "audio/automation_track.h"
#include "project.h"
#include "utils/arrays.h"
#include "zrythm.h"

#include "tests/helpers/zrythm.h"

#include <glib.h>
#include <locale.h>

typedef struct
{
  Channel *         ch;
  Automatable *     a;
  AutomationTrack * at;
} AutomationTrackFixture;

static void
at_fixture_set_up (
  AutomationTrackFixture * fixture)
{
  /*AutomationPoint * ap;*/
  /*AutomationCurve * ac;*/

  /* needed to set TRANSPORT */
  /*transport_init (TRANSPORT, 0);*/
  engine_update_frames_per_tick (
    AUDIO_ENGINE, 4, 140, 44000);

  fixture->at =
    calloc (1, sizeof (AutomationTrack));
  /*ap =*/
    /*calloc (1, sizeof (AutomationPoint));*/
  /*position_set_to_bar (&ap->pos, 2);*/
  /*array_append (fixture->at->aps,*/
                /*fixture->at->num_aps,*/
                /*ap);*/
  /*ac =*/
    /*calloc (1, sizeof (AutomationCurve));*/
  /*position_set_to_bar (&ap->pos, 3);*/
  /*array_append (fixture->at->acs,*/
                /*fixture->at->num_acs,*/
                /*ac);*/
  /*ap =*/
    /*calloc (1, sizeof (AutomationPoint));*/
  /*position_set_to_bar (&ap->pos, 4);*/
  /*array_append (fixture->at->aps,*/
                /*fixture->at->num_aps,*/
                /*ap);*/
  /*ac =*/
    /*calloc (1, sizeof (AutomationCurve));*/
  /*position_set_to_bar (&ap->pos, 5);*/
  /*array_append (fixture->at->acs,*/
                /*fixture->at->num_acs,*/
                /*ac);*/
  /*ap =*/
    /*calloc (1, sizeof (AutomationPoint));*/
  /*position_set_to_bar (&ap->pos, 6);*/
  /*array_append (fixture->at->aps,*/
                /*fixture->at->num_aps,*/
                /*ap);*/
  /*ac =*/
    /*calloc (1, sizeof (AutomationCurve));*/
  /*position_set_to_bar (&ap->pos, 7);*/
  /*array_append (fixture->at->acs,*/
                /*fixture->at->num_acs,*/
                /*ac);*/
  /*ap =*/
    /*calloc (1, sizeof (AutomationPoint));*/
  /*position_set_to_bar (&ap->pos, 8);*/
  /*array_append (fixture->at->aps,*/
                /*fixture->at->num_acs,*/
                /*ap);*/
}

static void
get_x_relevant_to_pos ()
{
  AutomationTrackFixture _fixture;
  AutomationTrackFixture * fixture =
    &_fixture;
  at_fixture_set_up (fixture);

  /*Position pos;*/
  /*AutomationPoint * ap;*/

  /*[> test when pos is before first ap <]*/
  /*position_set_to_bar (&pos, 1);*/
  /*ap =*/
    /*automation_track_get_ap_before_pos (*/
      /*fixture->at, &pos);*/
  /*g_assert_null (ap);*/

  /*[> test when pos is before 3rd ap <]*/
  /*position_set_to_pos (*/
    /*&pos, &fixture->at->aps[1]->pos);*/
  /*position_add_ticks (*/
    /*&pos, 1);*/
  /*ap =*/
    /*automation_track_get_ap_before_pos (*/
      /*fixture->at, &pos);*/
  /*g_assert_true (*/
    /*ap == fixture->at->aps[1]);*/

  /*[> test when pos is after all aps <]*/
  /*position_set_to_pos (*/
    /*&pos, &fixture->at->aps[*/
            /*fixture->at->num_aps - 1]->*/
              /*pos);*/
  /*position_add_ticks (*/
    /*&pos, 1);*/
  /*ap =*/
    /*automation_track_get_ap_before_pos (*/
      /*fixture->at, &pos);*/
  /*g_assert_true (*/
    /*ap == fixture->at->aps[*/
            /*fixture->at->num_aps - 1]);*/

  /*[> test when there are no APs <]*/
  /*[>int tmp = fixture->at->num_aps;<]*/
  /*fixture->at->num_aps = 0;*/
  /*ap =*/
    /*automation_track_get_ap_before_pos (*/
      /*fixture->at, &pos);*/
  /*g_assert_null (ap);*/


}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  test_helper_zrythm_init ();

#define TEST_PREFIX "/audio/automation_track/"

  g_test_add_func (
    TEST_PREFIX "get x relevant to pos",
    (GTestFunc) get_x_relevant_to_pos);

  return g_test_run ();
}
