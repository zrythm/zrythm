// SPDX-FileCopyrightText: © 2021-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#include "audio/fader.h"
#include "audio/router.h"
#include "plugins/carla/carla_discovery.h"
#include "plugins/carla_native_plugin.h"
#include "utils/math.h"

#include <glib.h>

#include "tests/helpers/plugin_manager.h"
#include "tests/helpers/zrythm.h"

static void
test_mono_plugin (void)
{
#if defined(HAVE_CARLA) && defined(HAVE_LSP_COMPRESSOR_MONO)

  test_helper_zrythm_init ();

  /* stop dummy audio engine processing so we can
   * process manually */
  AUDIO_ENGINE->stop_dummy_audio_thread = true;
  g_usleep (1000000);

  /* create an audio track */
  char * filepath =
    g_build_filename (TESTS_SRCDIR, "test.wav", NULL);
  SupportedFile * file =
    supported_file_new_from_path (filepath);
  Track * audio_track = track_create_with_action (
    TRACK_TYPE_AUDIO, NULL, file, PLAYHEAD,
    TRACKLIST->num_tracks, 1, NULL);
  supported_file_free (file);

  /* hard pan right */
  GError *         err = NULL;
  UndoableAction * ua =
    tracklist_selections_action_new_edit_single_float (
      EDIT_TRACK_ACTION_TYPE_PAN, audio_track, 0.5f, 1.f,
      false, &err);
  undo_manager_perform (UNDO_MANAGER, ua, NULL);

  /* add a mono insert */
  PluginSetting * setting =
    test_plugin_manager_get_plugin_setting (
      LSP_COMPRESSOR_MONO_BUNDLE, LSP_COMPRESSOR_MONO_URI,
      true);
  g_return_if_fail (setting);

  bool ret = mixer_selections_action_perform_create (
    PLUGIN_SLOT_INSERT, track_get_name_hash (audio_track), 0,
    setting, 1, NULL);
  g_assert_true (ret);

  Plugin * pl = audio_track->channel->inserts[0];
  g_assert_true (IS_PLUGIN_AND_NONNULL (pl));

  engine_process (AUDIO_ENGINE, AUDIO_ENGINE->block_length);
  engine_process (AUDIO_ENGINE, AUDIO_ENGINE->block_length);
  engine_process (AUDIO_ENGINE, AUDIO_ENGINE->block_length);

  /* bounce */
  ExportSettings settings;
  memset (&settings, 0, sizeof (ExportSettings));
  export_settings_set_bounce_defaults (
    &settings, EXPORT_FORMAT_WAV, NULL, __func__);
  settings.time_range = TIME_RANGE_LOOP;
  settings.bounce_with_parents = true;
  settings.mode = EXPORT_MODE_FULL;

  EngineState state;
  GPtrArray * conns =
    exporter_prepare_tracks_for_export (&settings, &state);

  /* start exporting in a new thread */
  GThread * thread = g_thread_new (
    "bounce_thread",
    (GThreadFunc) exporter_generic_export_thread, &settings);

  g_thread_join (thread);

  exporter_post_export (&settings, conns, &state);

  g_assert_false (audio_file_is_silent (settings.file_uri));

  export_settings_free_members (&settings);

  test_helper_zrythm_cleanup ();
#endif
}

#if 0
static void
test_has_custom_ui (void)
{
  test_helper_zrythm_init ();

#  ifdef HAVE_CARLA
#    ifdef HAVE_HELM
  PluginSetting * setting =
    test_plugin_manager_get_plugin_setting (
      HELM_BUNDLE, HELM_URI, false);
  g_assert_nonnull (setting);
  g_assert_true (
    carla_native_plugin_has_custom_ui (
      setting->descr));
#    endif
#  endif

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
  g_return_if_fail (setting);
  setting->bridge_mode = CARLA_BRIDGE_FULL;

  /* create a track from the plugin */
  track_create_for_plugin_at_idx_w_action (
    TRACK_TYPE_AUDIO_BUS, setting, TRACKLIST->num_tracks,
    NULL);

  Plugin * pl =
    TRACKLIST->tracks[TRACKLIST->num_tracks - 1]
      ->channel->inserts[0];
  g_assert_true (IS_PLUGIN_AND_NONNULL (pl));

  engine_process (AUDIO_ENGINE, AUDIO_ENGINE->block_length);
  engine_process (AUDIO_ENGINE, AUDIO_ENGINE->block_length);
  engine_process (AUDIO_ENGINE, AUDIO_ENGINE->block_length);

  test_helper_zrythm_cleanup ();
#endif
}

/**
 * Test process.
 */
static void
test_process (void)
{
#if defined(HAVE_TEST_SIGNAL) && defined(HAVE_CARLA)

  test_helper_zrythm_init ();

  test_plugin_manager_create_tracks_from_plugin (
    TEST_SIGNAL_BUNDLE, TEST_SIGNAL_URI, false, true, 1);

  Track * track = TRACKLIST->tracks[TRACKLIST->num_tracks - 1];
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
    .g_start_frame = 0, .local_offset = 0, .nframes = local_offset
  };
  carla_native_plugin_process (pl->carla, &time_nfo);
  for (nframes_t i = 1; i < local_offset; i++)
    {
      g_assert_true (fabsf (out->buf[i]) > 1e-10f);
    }
  time_nfo.g_start_frame = local_offset;
  time_nfo.local_offset = local_offset;
  time_nfo.nframes = AUDIO_ENGINE->block_length - local_offset;
  carla_native_plugin_process (pl->carla, &time_nfo);
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

#define TEST_PREFIX "/plugins/carla native plugin/"

  g_test_add_func (
    TEST_PREFIX "test mono plugin",
    (GTestFunc) test_mono_plugin);
  g_test_add_func (
    TEST_PREFIX "test process", (GTestFunc) test_process);
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
