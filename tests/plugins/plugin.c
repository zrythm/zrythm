/*
 * Copyright (C) 2020-2022 Alexandros Theodotou <alex at zrythm dot org>
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
#include "audio/midi_event.h"
#include "audio/router.h"
#include "utils/math.h"

#include <glib.h>

#include "tests/helpers/plugin_manager.h"
#include "tests/helpers/zrythm.h"

#include <lilv/lilv.h>

#ifdef HAVE_HELM
static void
_test_loading_non_existing_plugin (
  const char * pl_bundle,
  const char * pl_uri,
  bool         with_carla)
{
  /* create instrument track */
  test_plugin_manager_create_tracks_from_plugin (
    pl_bundle, pl_uri, true, with_carla, 1);

  /* save the project */
  char * prj_file = test_project_save ();
  g_assert_nonnull (prj_file);

  /* unload bundle so plugin can't be found */
  LilvNode * path = lilv_new_uri (LILV_WORLD, pl_bundle);
  lilv_world_unload_bundle (LILV_WORLD, path);
  lilv_node_free (path);

  /* reload project and expect messages */
  LOG->use_structured_for_console = false;
  LOG->min_log_level_for_test_console = G_LOG_LEVEL_WARNING;
  g_test_expect_message (
    G_LOG_DOMAIN, G_LOG_LEVEL_WARNING,
    "*Instantiation failed for plugin *");

  test_project_reload (prj_file);
  g_free (prj_file);

  /* let engine run */
  engine_wait_n_cycles (AUDIO_ENGINE, 8);

  /* assert expected messages */
  g_test_assert_expected_messages ();
}
#endif

static void
test_loading_non_existing_plugin (void)
{
  test_helper_zrythm_init ();

#ifdef HAVE_HELM
  _test_loading_non_existing_plugin (
    HELM_BUNDLE, HELM_URI, false);
#endif

  test_helper_zrythm_cleanup ();
}

static Port *
get_skew_duty_port (void)
{
  Track * track = TRACKLIST->tracks[TRACKLIST->num_tracks - 1];
  Plugin * pl = track->channel->instrument;
  Port *   port = NULL;
  for (int i = 0; i < pl->num_in_ports; i++)
    {
      Port * cur_port = pl->in_ports[i];
      if (string_is_equal (
            cur_port->id.label, "OscA Skew/Duty"))
        {
          port = cur_port;
          break;
        }
    }
  return port;
}

static void
test_loading_fully_bridged_plugin (void)
{
  test_helper_zrythm_init ();

#ifdef HAVE_CARLA
#  ifdef HAVE_CHIPWAVE
  test_plugin_manager_create_tracks_from_plugin (
    CHIPWAVE_BUNDLE, CHIPWAVE_URI, true, true, 1);

  Port * port = get_skew_duty_port ();
  g_return_if_fail (port);
  float val_before = port->control;
  float val_after = 1.f;
  g_assert_cmpfloat_with_epsilon (val_before, 0.5f, 0.0001f);
  port_set_control_value (
    port, val_after, F_NORMALIZED, F_PUBLISH_EVENTS);
  g_assert_cmpfloat_with_epsilon (
    port->control, val_after, 0.0001f);

  /* save project and reload and check the
   * value is correct */
  test_project_save_and_reload ();

  port = get_skew_duty_port ();
  g_return_if_fail (port);
  g_assert_cmpfloat_with_epsilon (
    port->control, val_after, 0.0001f);
#  endif
#endif

  test_helper_zrythm_cleanup ();
}

static void
test_loading_plugins_needing_bridging (void)
{
  test_helper_zrythm_init ();

#ifdef HAVE_CARLA
#  ifdef HAVE_CALF_MONOSYNTH
  PluginSetting * setting =
    test_plugin_manager_get_plugin_setting (
      CALF_MONOSYNTH_BUNDLE, CALF_MONOSYNTH_URI, false);
  g_return_if_fail (setting);
  g_assert_true (setting->open_with_carla);
  g_assert_true (setting->bridge_mode == CARLA_BRIDGE_FULL);

  test_project_save_and_reload ();
#  endif
#endif

  test_helper_zrythm_cleanup ();
}

static void
test_bypass_state_after_project_load (void)
{
#ifdef HAVE_LSP_COMPRESSOR
  test_helper_zrythm_init ();

  for (int i = 0;
#  ifdef HAVE_CARLA
       i < 2;
#  else
       i < 1;
#  endif
       i++)
    {
      /* create fx track */
      test_plugin_manager_create_tracks_from_plugin (
        LSP_COMPRESSOR_BUNDLE, LSP_COMPRESSOR_URI, false,
        i == 1, 1);
      Track * track =
        TRACKLIST->tracks[TRACKLIST->num_tracks - 1];
      Plugin * pl = track->channel->inserts[0];
      g_assert_true (IS_PLUGIN_AND_NONNULL (pl));

      /* set bypass */
      plugin_set_enabled (
        pl, F_NOT_ENABLED, F_NO_PUBLISH_EVENTS);
      g_assert_false (plugin_is_enabled (pl, false));

      /* reload project */
      test_project_save_and_reload ();

      track = TRACKLIST->tracks[TRACKLIST->num_tracks - 1];
      pl = track->channel->inserts[0];
      g_assert_true (IS_PLUGIN_AND_NONNULL (pl));

      /* check bypass */
      g_assert_false (plugin_is_enabled (pl, false));
    }

  test_helper_zrythm_cleanup ();
#endif
}

static void
test_plugin_without_outputs (void)
{
#ifdef HAVE_KXSTUDIO_LFO
  test_helper_zrythm_init ();

  test_plugin_manager_create_tracks_from_plugin (
    KXSTUDIO_LFO_BUNDLE, KXSTUDIO_LFO_URI, false, true, 1);
  Track * track = TRACKLIST->tracks[TRACKLIST->num_tracks - 1];
  Plugin * pl = track->channel->inserts[0];
  g_assert_true (IS_PLUGIN_AND_NONNULL (pl));

  /* reload project */
  test_project_save_and_reload ();

  test_helper_zrythm_cleanup ();
#endif
}

int
main (int argc, char * argv[])
{
  g_test_init (&argc, &argv, NULL);

  (void) get_skew_duty_port;

#define TEST_PREFIX "/plugins/plugin/"

  g_test_add_func (
    TEST_PREFIX "test plugin without outputs",
    (GTestFunc) test_plugin_without_outputs);
  g_test_add_func (
    TEST_PREFIX "test bypass state after project load",
    (GTestFunc) test_bypass_state_after_project_load);
#if 0
  /* test does not work with carla */
  g_test_add_func (
    TEST_PREFIX "test loading non-existing plugin",
    (GTestFunc) test_loading_non_existing_plugin);
#endif
  g_test_add_func (
    TEST_PREFIX "test loading fully bridged plugin",
    (GTestFunc) test_loading_fully_bridged_plugin);
  g_test_add_func (
    TEST_PREFIX "test loading plugins needing bridging",
    (GTestFunc) test_loading_plugins_needing_bridging);

  (void) test_loading_non_existing_plugin;

  return g_test_run ();
}
