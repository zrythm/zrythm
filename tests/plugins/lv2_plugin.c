/*
 * Copyright (C) 2021 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, version 3.
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

#include <stdlib.h>

#include "plugins/lv2_plugin.h"

#include "tests/helpers/plugin_manager.h"

#include <glib.h>

static void
test_lilv_instance_activation ()
{
#ifdef HAVE_HELM
  for (int i = 0; i < 40; i++)
    {
      test_helper_zrythm_init ();

      test_plugin_manager_create_tracks_from_plugin (
        HELM_BUNDLE, HELM_URI, true, false, 1);

      Plugin * pl =
        TRACKLIST->tracks[TRACKLIST->num_tracks - 1]->
          channel->instrument;
      g_assert_true (IS_PLUGIN_AND_NONNULL (pl));

      EngineState state;
      engine_wait_for_pause (
        AUDIO_ENGINE, &state, false);

      lilv_instance_deactivate (pl->lv2->instance);
      lilv_instance_activate (pl->lv2->instance);
      lilv_instance_deactivate (pl->lv2->instance);
      lilv_instance_activate (pl->lv2->instance);
      if (i % 2)
        {
          lilv_instance_deactivate (
            pl->lv2->instance);
        }

      test_helper_zrythm_cleanup ();
    }
#endif
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

#define TEST_PREFIX "/plugins/lv2_plugin/"

  g_test_add_func (
    TEST_PREFIX "test lilv instance activation",
    (GTestFunc) test_lilv_instance_activation);

  return g_test_run ();
}
