/*
 * Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
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

#include <lilv/lilv.h>

#include "audio/fader.h"
#include "audio/midi_event.h"
#include "audio/router.h"
#include "utils/math.h"

#include "tests/helpers/plugin_manager.h"
#include "tests/helpers/zrythm.h"

#include <glib.h>

static void
_test_loading_non_existing_plugin (
  const char * pl_bundle,
  const char * pl_uri,
  bool         with_carla)
{
  PluginDescriptor * descr =
    test_plugin_manager_get_plugin_descriptor (
      pl_bundle, pl_uri, with_carla);

  /* create instrument track */
  UndoableAction * ua =
    tracklist_selections_action_new_create (
      TRACK_TYPE_INSTRUMENT,
      descr, NULL, TRACKLIST->num_tracks, NULL, 1);
  undo_manager_perform (UNDO_MANAGER, ua);

  /* unload bundle so plugin can't be found */
  LilvNode * path =
    lilv_new_uri (LILV_WORLD, pl_bundle);
  lilv_world_unload_bundle (LILV_WORLD, path);
  lilv_node_free (path);

  /* reload project and expect messages */
  LOG->use_structured_for_console = false;
  LOG->min_log_level_for_test_console =
    G_LOG_LEVEL_WARNING;
  g_test_expect_message (
    G_LOG_DOMAIN, G_LOG_LEVEL_WARNING,
    "*lv2 instantiate failed*");
  g_test_expect_message (
    G_LOG_DOMAIN, G_LOG_LEVEL_WARNING,
    "*Instantiation failed for plugin *");
  test_project_save_and_reload ();

  /* let engine run */
  g_usleep (600000);

  /* assert expected messages */
  g_test_assert_expected_messages ();
}

static void
test_loading_non_existing_plugin ()
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
  Track * track =
    TRACKLIST->tracks[TRACKLIST->num_tracks - 1];
  Plugin * pl = track->channel->instrument;
  Port * port = NULL;
  for (int i = 0; i < pl->num_in_ports; i++)
    {
      Port * cur_port =  pl->in_ports[i];
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
#ifdef HAVE_CHIPWAVE
  for (int i = 0; i < 2; i++)
    {
      test_plugin_manager_create_tracks_from_plugin (
        CHIPWAVE_BUNDLE, CHIPWAVE_URI, true, true,
        1);

      Port * port = get_skew_duty_port ();
      g_assert_nonnull (port);
      float val_before = port->control;
      float val_after = 1.f;
      g_assert_cmpfloat_with_epsilon (
        val_before, 0.5f, 0.0001f);
      port_set_control_value (
        port, val_after, F_NORMALIZED,
        i);

      /* save project and reload and check the
       * value is correct */
      test_project_save_and_reload ();

      port = get_skew_duty_port ();
      g_assert_nonnull (port);
      g_assert_cmpfloat_with_epsilon (
        port->control, val_after, 0.0001f);
    }
#endif
#endif

  test_helper_zrythm_cleanup ();
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  (void) get_skew_duty_port;

#define TEST_PREFIX "/plugins/plugin/"

  g_test_add_func (
    TEST_PREFIX "test loading non-existing plugin",
    (GTestFunc) test_loading_non_existing_plugin);
  g_test_add_func (
    TEST_PREFIX "test loading fully bridged plugin",
    (GTestFunc) test_loading_fully_bridged_plugin);

  return g_test_run ();
}
