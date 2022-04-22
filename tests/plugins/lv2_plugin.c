/*
 * Copyright (C) 2021-2022 Alexandros Theodotou <alex at zrythm dot org>
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
#include <stdlib.h>

#include "plugins/lv2/lv2_state.h"
#include "plugins/lv2_plugin.h"
#include "utils/string.h"

#include <glib.h>

#include "tests/helpers/plugin_manager.h"

#include <lv2/presets/presets.h>

static void
test_lilv_instance_activation (void)
{
#ifdef HAVE_HELM
  for (int i = 0; i < 20; i++)
    {
      test_helper_zrythm_init ();

      test_plugin_manager_create_tracks_from_plugin (
        HELM_BUNDLE, HELM_URI, true, false, 1);

      Plugin * pl =
        TRACKLIST
          ->tracks[TRACKLIST->num_tracks - 1]
          ->channel->instrument;
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
check_state_contains_wav (void)
{
  Track * track =
    TRACKLIST->tracks[TRACKLIST->num_tracks - 1];
  Plugin * pl = track->channel->instrument;
  char *   state_dir =
    plugin_get_abs_state_dir (pl, false);
  char * state_file = g_build_filename (
    state_dir, "state.ttl", NULL);
  GError * err = NULL;
  char *   content = NULL;
  g_file_get_contents (
    state_file, &content, NULL, &err);
  g_assert_null (err);

  g_message ("state file: '%s'", state_file);
  g_assert_true (
    string_contains_substr (content, "test.wav"));

  g_free (content);
  g_free (state_file);
  g_free (state_dir);
}

static void
test_save_state_w_files (void)
{
#ifdef HAVE_LSP_MULTISAMPLER_24_DO
  test_helper_zrythm_init ();

  test_plugin_manager_create_tracks_from_plugin (
    LSP_MULTISAMPLER_24_DO_BUNDLE,
    LSP_MULTISAMPLER_24_DO_URI, true, false, 1);

  Track * track =
    TRACKLIST->tracks[TRACKLIST->num_tracks - 1];
  Plugin * pl = track->channel->instrument;
  g_assert_true (IS_PLUGIN_AND_NONNULL (pl));

  char * pset_bundle_path = g_build_filename (
    TESTS_SRCDIR, "presets",
    "LSP_Multi_Sampler_x24_DirectOut_test.preset.lv2",
    NULL);
  char * pset_bundle_path_uri = g_strdup_printf (
    "file://%s/", pset_bundle_path);
  LilvNode * pset_path_uri_node = lilv_new_uri (
    LILV_WORLD, pset_bundle_path_uri);
  lilv_world_load_bundle (
    LILV_WORLD, pset_path_uri_node);
  lilv_node_free (pset_path_uri_node);
  lv2_state_load_presets (pl->lv2, NULL, NULL);

  LilvNodes * presets = lilv_plugin_get_related (
    pl->lv2->lilv_plugin,
    PM_GET_NODE (LV2_PRESETS__Preset));
  LILV_FOREACH (nodes, i, presets)
    {
      const LilvNode * preset =
        lilv_nodes_get (presets, i);
      lilv_world_load_resource (LILV_WORLD, preset);

      g_message (
        "Preset <%s>",
        lilv_node_as_string (preset));

      LilvNodes * labels = lilv_world_find_nodes (
        LILV_WORLD, preset,
        PM_GET_NODE (LILV_NS_RDFS "label"), NULL);
      if (labels)
        {
          const LilvNode * label =
            lilv_nodes_get_first (labels);
          g_message (
            "label: %s",
            lilv_node_as_string (label));
          lilv_nodes_free (labels);
        }
      else
        {
          g_message (
            "Preset <%s> has no rdfs:label\n",
            lilv_node_as_string (preset));
        }
    }
  lilv_nodes_free (presets);

  char * pset_uri = g_strdup_printf (
    "%s%s", pset_bundle_path_uri, "test2.ttl");
  LilvNode * pset_uri_node =
    lilv_new_uri (LILV_WORLD, pset_uri);
  g_assert_nonnull (pset_uri_node);
  bool applied = lv2_state_apply_preset (
    pl->lv2, pset_uri_node, NULL, NULL);
  g_assert_true (applied);
  lilv_node_free (pset_uri_node);

  /* create midi note */
  Position pos, end_pos;
  position_init (&pos);
  position_set_to_bar (&end_pos, 3);
  ZRegion * r = midi_region_new (
    &pos, &end_pos, track_get_name_hash (track), 0,
    0);
  track_add_region (
    track, r, NULL, 0, F_GEN_NAME,
    F_NO_PUBLISH_EVENTS);
  MidiNote * mn = midi_note_new (
    &r->id, &pos, &end_pos, 57, 120);
  midi_region_add_midi_note (
    r, mn, F_NO_PUBLISH_EVENTS);

  /* stop dummy audio engine processing so we can
   * process manually */
  AUDIO_ENGINE->stop_dummy_audio_thread = true;
  g_usleep (1000000);

  /* test that plugin makes sound */
  transport_request_roll (TRANSPORT, true);
  engine_process (
    AUDIO_ENGINE, AUDIO_ENGINE->block_length);
  engine_process (
    AUDIO_ENGINE, AUDIO_ENGINE->block_length);
  engine_process (
    AUDIO_ENGINE, AUDIO_ENGINE->block_length);

  /* FIXME fails */
  /*g_assert_true (*/
  /*port_has_sound (track->channel->stereo_out->l));*/

  LilvState * state =
    lv2_state_save_to_file (pl->lv2, false);
  lilv_state_free (state);
  check_state_contains_wav ();

  project_save (
    PROJECT, PROJECT->dir, false, false,
    F_NO_ASYNC);
  check_state_contains_wav ();

  test_project_save_and_reload ();
  check_state_contains_wav ();

  track =
    TRACKLIST->tracks[TRACKLIST->num_tracks - 1];
  transport_request_roll (TRANSPORT, true);
  engine_process (
    AUDIO_ENGINE, AUDIO_ENGINE->block_length);
  engine_process (
    AUDIO_ENGINE, AUDIO_ENGINE->block_length);
  engine_process (
    AUDIO_ENGINE, AUDIO_ENGINE->block_length);

  /* FIXME fails */
  /*g_assert_true (*/
  /*port_has_sound (track->channel->stereo_out->l));*/

  g_free (pset_bundle_path);
  g_free (pset_bundle_path_uri);

  test_helper_zrythm_cleanup ();
#endif
}
#if defined(HAVE_SFIZZ) || defined(HAVE_DROPS)
static void
test_reloading_project_with_instrument (
  const char * pl_bundle,
  const char * pl_uri)
{
  test_helper_zrythm_init ();

  test_plugin_manager_create_tracks_from_plugin (
    pl_bundle, pl_uri, true, true, 1);

  Track * track =
    TRACKLIST->tracks[TRACKLIST->num_tracks - 1];
  Plugin * pl = track->channel->instrument;
  g_assert_true (IS_PLUGIN_AND_NONNULL (pl));

  /* --- run engine a few cycles --- */

  /* stop dummy audio engine processing so we can
   * process manually */
  AUDIO_ENGINE->stop_dummy_audio_thread = true;
  g_usleep (1000000);

  /* test that plugin makes sound */
  transport_request_roll (TRANSPORT, true);
  engine_process (
    AUDIO_ENGINE, AUDIO_ENGINE->block_length);
  engine_process (
    AUDIO_ENGINE, AUDIO_ENGINE->block_length);
  engine_process (
    AUDIO_ENGINE, AUDIO_ENGINE->block_length);

  test_project_save_and_reload ();

  test_helper_zrythm_cleanup ();
}
#endif

/**
 * Test a plugin with lots of parameters.
 */
static void
test_lots_of_params (void)
{
#ifdef HAVE_SFIZZ
  test_reloading_project_with_instrument (
    SFIZZ_BUNDLE, SFIZZ_URI);
#endif
}

/**
 * Test reloading drops project.
 */
static void
test_reloading_drops_project (void)
{
#ifdef HAVE_DROPS
  test_reloading_project_with_instrument (
    DROPS_BUNDLE, DROPS_URI);
#endif
}

/**
 * Test process.
 */
static void
test_process (void)
{
#ifdef HAVE_TEST_SIGNAL
  test_helper_zrythm_init ();

  test_plugin_manager_create_tracks_from_plugin (
    TEST_SIGNAL_BUNDLE, TEST_SIGNAL_URI, false,
    false, 1);

  Track * track =
    TRACKLIST->tracks[TRACKLIST->num_tracks - 1];
  Plugin * pl = track->channel->inserts[0];
  g_assert_true (IS_PLUGIN_AND_NONNULL (pl));

  /* stop dummy audio engine processing so we can
   * process manually */
  AUDIO_ENGINE->stop_dummy_audio_thread = true;
  g_usleep (1000000);

  /* run plugin and check that output is filled */
  Port *                out = pl->out_ports[0];
  nframes_t             local_offset = 60;
  EngineProcessTimeInfo time_nfo = {
    .g_start_frame = 0,
    .local_offset = 0,
    .nframes = local_offset
  };
  lv2_plugin_process (pl->lv2, &time_nfo);
  for (nframes_t i = 1; i < local_offset; i++)
    {
      g_assert_true (fabsf (out->buf[i]) > 1e-10f);
    }
  time_nfo.g_start_frame = local_offset;
  time_nfo.local_offset = local_offset;
  time_nfo.nframes =
    AUDIO_ENGINE->block_length - local_offset;
  lv2_plugin_process (pl->lv2, &time_nfo);
  for (nframes_t i = local_offset;
       i < AUDIO_ENGINE->block_length; i++)
    {
      g_assert_true (fabsf (out->buf[i]) > 1e-10f);
    }

  test_helper_zrythm_cleanup ();
#endif
}

int
main (int argc, char * argv[])
{
  g_test_init (&argc, &argv, NULL);

#define TEST_PREFIX "/plugins/lv2_plugin/"

#if 0
  /* direct LV2 no longer supported */
  g_test_add_func (
    TEST_PREFIX "test save state with files",
    (GTestFunc) test_save_state_w_files);
  g_test_add_func (
    TEST_PREFIX "test lilv instance activation",
    (GTestFunc) test_lilv_instance_activation);
  g_test_add_func (
    TEST_PREFIX "test process",
    (GTestFunc) test_process);
  g_test_add_func (
    TEST_PREFIX "test reloading drops project",
    (GTestFunc) test_reloading_drops_project);
#endif
  g_test_add_func (
    TEST_PREFIX "test lots of params",
    (GTestFunc) test_lots_of_params);

  (void) test_save_state_w_files;
  (void) test_lilv_instance_activation;
  (void) test_process;
  (void) test_reloading_drops_project;
  (void) check_state_contains_wav;

  return g_test_run ();
}
