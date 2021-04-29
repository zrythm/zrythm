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

#include <stdlib.h>

#include "plugins/lv2/lv2_state.h"
#include "plugins/lv2_plugin.h"
#include "utils/string.h"

#include "tests/helpers/plugin_manager.h"

#include <lv2/presets/presets.h>

#include <glib.h>

static void
test_lilv_instance_activation ()
{
#ifdef HAVE_HELM
  for (int i = 0; i < 20; i++)
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

static void
test_save_state_w_files (void)
{
#ifdef HAVE_LSP_MULTISAMPLER_24_DO
  test_helper_zrythm_init ();

  test_plugin_manager_create_tracks_from_plugin (
    LSP_MULTISAMPLER_24_DO_BUNDLE,
    LSP_MULTISAMPLER_24_DO_URI, true, false, 1);

  Plugin * pl =
    TRACKLIST->tracks[TRACKLIST->num_tracks - 1]->
      channel->instrument;
  g_assert_true (IS_PLUGIN_AND_NONNULL (pl));

  /* set a sample */
  /*pl->lv2, port, strlen (filename),*/
  /*pl->lv2, pl->lv2->forge.Path, filename)*/

  char * pset_bundle_path =
    g_build_filename (
      TESTS_SRCDIR, "presets",
      "LSP_Multi_Sampler_x24_DirectOut_test.preset.lv2",
      NULL);
  char * pset_bundle_path_uri =
    g_strdup_printf (
      "file://%s/", pset_bundle_path);
  LilvNode * pset_path_uri_node =
    lilv_new_uri (
      LILV_WORLD, pset_bundle_path_uri);
  lilv_world_load_bundle (
    LILV_WORLD, pset_path_uri_node);
  lilv_node_free (pset_path_uri_node);
  lv2_state_load_presets (
    pl->lv2, NULL, NULL);

  LilvNodes* presets =
    lilv_plugin_get_related (
      pl->lv2->lilv_plugin,
      PM_GET_NODE (LV2_PRESETS__Preset));
  LILV_FOREACH(nodes, i, presets)
    {
      const LilvNode* preset =
        lilv_nodes_get(presets, i);
      lilv_world_load_resource(LILV_WORLD, preset);

        g_message (
          "Preset <%s>",
          lilv_node_as_string(preset));

      LilvNodes* labels =
        lilv_world_find_nodes (
          LILV_WORLD, preset,
          PM_GET_NODE (LILV_NS_RDFS "label"), NULL);
      if (labels) {
        const LilvNode* label = lilv_nodes_get_first(labels);
        g_message ("label: %s",
          lilv_node_as_string (label));
        lilv_nodes_free(labels);
      } else {
        g_message (
          "Preset <%s> has no rdfs:label\n",
          lilv_node_as_string(preset));
      }
    }
  lilv_nodes_free(presets);

  char * pset_uri =
    g_strdup_printf (
      "%s%s",
      pset_bundle_path_uri, "test2.ttl");
  LilvNode * pset_uri_node =
    lilv_new_uri (LILV_WORLD, pset_uri);
  g_assert_nonnull (pset_uri_node);
  lv2_state_apply_preset (
    pl->lv2, pset_uri_node);
  lilv_node_free (pset_uri_node);

  lv2_state_save_to_file (pl->lv2, false);

  char * state_dir =
    plugin_get_abs_state_dir (pl, false);
  char * state_file =
    g_build_filename (
      state_dir, "state.ttl", NULL);
  GError *err = NULL;
  char * content = NULL;
  g_file_get_contents (
    state_file, &content, NULL, &err);
  g_assert_null (err);

  g_assert_true (
    string_contains_substr (
      content,
      "http://lsp-plug.in/plugins/lv2/multisampler_x24_do/ports#sf_0_0"));

  g_free (pset_bundle_path);
  g_free (pset_bundle_path_uri);
  g_free (content);
  g_free (state_file);
  g_free (state_dir);

  test_helper_zrythm_cleanup ();
#endif
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

#define TEST_PREFIX "/plugins/lv2_plugin/"

  g_test_add_func (
    TEST_PREFIX "test save state with files",
    (GTestFunc) test_save_state_w_files);
  g_test_add_func (
    TEST_PREFIX "test lilv instance activation",
    (GTestFunc) test_lilv_instance_activation);

  return g_test_run ();
}
