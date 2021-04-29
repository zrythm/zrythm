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

#include "audio/fader.h"
#include "audio/router.h"
#include "plugins/carla_native_plugin.h"
#include "plugins/carla/carla_discovery.h"
#include "utils/math.h"

#include "tests/helpers/plugin_manager.h"
#include "tests/helpers/zrythm.h"

#include <glib.h>

#if 0
static void
test_has_custom_ui (void)
{
  test_helper_zrythm_init ();

#ifdef HAVE_CARLA
#ifdef HAVE_HELM
  PluginSetting * setting =
    test_plugin_manager_get_plugin_setting (
      HELM_BUNDLE, HELM_URI, false);
  g_assert_nonnull (setting);
  g_assert_true (
    carla_native_plugin_has_custom_ui (
      setting->descr));
#endif
#endif

  test_helper_zrythm_cleanup ();
}
#endif

static void
test_crash_handling (void)
{
#ifdef HAVE_CARLA
  test_helper_zrythm_init ();

  /* stop dummy audio engine processing so we can
   * process manually */
  AUDIO_ENGINE->stop_dummy_audio_thread = true;
  g_usleep (1000000);

  PluginSetting * setting =
    test_plugin_manager_get_plugin_setting (
    SIGABRT_BUNDLE_URI, SIGABRT_URI, true);
  setting->bridge_mode = CARLA_BRIDGE_FULL;

  /* create a track from the plugin */
  UndoableAction * ua =
    tracklist_selections_action_new_create (
      TRACK_TYPE_AUDIO_BUS, setting, NULL,
      TRACKLIST->num_tracks, NULL, 1);
  undo_manager_perform (UNDO_MANAGER, ua);

  Plugin * pl =
    TRACKLIST->tracks[TRACKLIST->num_tracks - 1]->
      channel->inserts[0];
  g_assert_true (IS_PLUGIN_AND_NONNULL (pl));

  engine_process (
    AUDIO_ENGINE, AUDIO_ENGINE->block_length);
  engine_process (
    AUDIO_ENGINE, AUDIO_ENGINE->block_length);
  engine_process (
    AUDIO_ENGINE, AUDIO_ENGINE->block_length);

  test_helper_zrythm_cleanup ();
#endif
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

#define TEST_PREFIX "/plugins/carla native plugin/"

#if 0
  g_test_add_func (
    TEST_PREFIX "test has custom UI",
    (GTestFunc) test_has_custom_ui);
#endif
  g_test_add_func (
    TEST_PREFIX "test crash handling",
    (GTestFunc) test_crash_handling);

  return g_test_run ();
}
