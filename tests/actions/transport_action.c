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

#include "actions/transport_action.h"
#include "audio/control_port.h"
#include "audio/transport.h"
#include "project.h"
#include "utils/flags.h"
#include "zrythm.h"

#include "tests/helpers/plugin_manager.h"
#include "tests/helpers/project.h"

#include <glib.h>

static void
test_change_bpm_and_time_sig (void)
{
  test_helper_zrythm_init ();

  /* import audio */
  char audio_file_path[2000];
  sprintf (
    audio_file_path, "%s%s%s",
    TESTS_SRCDIR, G_DIR_SEPARATOR_S,
    "test.wav");
  SupportedFile * file_descr =
    supported_file_new_from_path (audio_file_path);
  Position pos;
  position_init (&pos);
  Track * audio_track =
    track_create_with_action (
      TRACK_TYPE_AUDIO, NULL, file_descr, &pos,
      TRACKLIST->num_tracks, 1, NULL);
  int audio_track_pos = audio_track->pos;
  (void) audio_track_pos;
  supported_file_free (file_descr);

  /* print region before the change */
  ZRegion * r = audio_track->lanes[0]->regions[0];
  ArrangerObject * r_obj = (ArrangerObject *) r;
  g_assert_true (IS_REGION_AND_NONNULL (r));
  arranger_object_print (r_obj);

  /* change time sig to 4/16 */
  ControlPortChange change = { 0 };
  change.flag2 = PORT_FLAG2_BEAT_UNIT;
  change.beat_unit = BEAT_UNIT_16;
  router_queue_control_port_change (ROUTER, &change);

  engine_wait_n_cycles (AUDIO_ENGINE, 3);

  test_helper_zrythm_cleanup ();
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

#define TEST_PREFIX "/actions/transport/"

  g_test_add_func (
    TEST_PREFIX "test change BPM and time sig",
    (GTestFunc) test_change_bpm_and_time_sig);

  return g_test_run ();
}

