// SPDX-FileCopyrightText: © 2021-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "dsp/fader.h"
#include "dsp/router.h"
#include "plugins/carla_discovery.h"
#include "plugins/carla_native_plugin.h"
#include "utils/math.h"

#include "tests/helpers/plugin_manager.h"
#include "tests/helpers/project_helper.h"
#include "tests/helpers/zrythm_helper.h"

TEST_SUITE_BEGIN ("plugins/carla native plugin");

TEST_CASE_FIXTURE (ZrythmFixture, "vst instrument makes sound")
{
#ifdef HAVE_CARLA
  for (int i = 0; i < 2; i++)
    {
      const char * pl_path = NULL;
#  ifdef HAVE_NOIZEMAKER
      if (i == 0)
        {
          /* vst2 */
          pl_path = NOIZEMAKER_PATH;
        }
#  endif
#  ifdef HAVE_SURGE_XT
      if (i == 1)
        {
          /* vst3 */
          pl_path = SURGE_XT_PATH;
          /* TODO: remove this line after Surge XT issue is fixed in Carla 2.6.0
           */
          pl_path = NULL;
        }
#  endif
      if (!pl_path)
        continue;

      SUBCASE ("iteration")
      {
        /* stop dummy audio engine processing so we can
         * process manually */
        test_project_stop_dummy_engine ();

        /* create instrument track */
        test_plugin_manager_create_tracks_from_plugin (
          pl_path, nullptr, true, true, 1);
        auto track = TRACKLIST->get_last_track<InstrumentTrack> ();

        /* create midi note */
        Position pos, end_pos;
        pos.zero ();
        end_pos.set_to_bar (3);
        auto r = std::make_shared<MidiRegion> (
          pos, end_pos, track->get_name_hash (), 0, 0);
        track->add_region (r, nullptr, 0, true, false);
        auto mn = std::make_shared<MidiNote> (r->id_, pos, end_pos, 57, 120);
        r->append_object (mn);

        AUDIO_ENGINE->process (AUDIO_ENGINE->block_length_);
        AUDIO_ENGINE->process (AUDIO_ENGINE->block_length_);
        AUDIO_ENGINE->process (AUDIO_ENGINE->block_length_);

        /* bounce */
        ExportSettings * settings = export_settings_new ();
        export_settings_set_bounce_defaults (
          settings, Exporter::Format::WAV, nullptr, __func__);
        settings->time_range = ExportTimeRange::TIME_RANGE_LOOP;
        settings->bounce_with_parents = true;
        settings->mode = ExportMode::EXPORT_MODE_FULL;

        EngineState state;
        GPtrArray * conns =
          exporter_prepare_tracks_for_export (settings, &state);

        /* start exporting in a new thread */
        GThread * thread = g_thread_new (
          "bounce_thread", (GThreadFunc) exporter_generic_export_thread,
          settings);

        g_thread_join (thread);

        exporter_post_export (settings, conns, &state);

        REQUIRE_FALSE (audio_file_is_silent (settings->file_uri));

        export_settings_free (settings);
      }
    }
#endif
}

TEST_CASE_FIXTURE (ZrythmFixture, "mono plugin")
{
#if defined(HAVE_CARLA) && defined(HAVE_LSP_COMPRESSOR_MONO)
  /* stop dummy audio engine processing so we can
   * process manually */
  test_project_stop_dummy_engine ();

  /* create an audio track */
  char * filepath = g_build_filename (TESTS_SRCDIR, "test.wav", nullptr);
  FileDescriptor file = FileDescriptor (filepath);
  track_create_with_action (
    Track::Type::Audio, nullptr, &file, PLAYHEAD, TRACKLIST->tracks.size (), 1,
    -1, nullptr, nullptr);
  Track * audio_track = tracklist_get_last_track (
    TRACKLIST, TracklistPinOption::TRACKLIST_PIN_OPTION_BOTH, false);

  /* hard pan right */
  GError *         err = NULL;
  UndoableAction * ua = tracklist_selections_action_new_edit_single_float (
    EditTrackActionType::EDIT_TRACK_ACTION_TYPE_PAN, audio_track, 0.5f, 1.f,
    false, &err);
  undo_manager_perform (UNDO_MANAGER, ua, nullptr);

  /* add a mono insert */
  PluginSetting * setting = test_plugin_manager_get_plugin_setting (
    LSP_COMPRESSOR_MONO_BUNDLE, LSP_COMPRESSOR_MONO_URI, true);
  z_return_if_fail (setting);

  bool ret = mixer_selections_action_perform_create (
    PluginSlotType::Insert, audio_track->get_name_hash (), 0, setting, 1,
    nullptr);
  REQUIRE (ret);

  Plugin * pl = audio_track->channel->inserts[0];
  REQUIRE (IS_PLUGIN_AND_NONNULL (pl));

  AUDIO_ENGINE->process (AUDIO_ENGINE->block_length_);
  AUDIO_ENGINE->process (AUDIO_ENGINE->block_length_);
  AUDIO_ENGINE->process (AUDIO_ENGINE->block_length_);

  /* bounce */
  ExportSettings * settings = export_settings_new ();
  export_settings_set_bounce_defaults (
    settings, Exporter::Format::WAV, nullptr, __func__);
  settings->time_range = ExportTimeRange::TIME_RANGE_LOOP;
  settings->bounce_with_parents = true;
  settings->mode = ExportMode::EXPORT_MODE_FULL;

  EngineState state;
  GPtrArray * conns = exporter_prepare_tracks_for_export (settings, &state);

  /* start exporting in a new thread */
  GThread * thread = g_thread_new (
    "bounce_thread", (GThreadFunc) exporter_generic_export_thread, settings);

  g_thread_join (thread);

  exporter_post_export (settings, conns, &state);

  REQUIRE_FALSE (audio_file_is_silent (settings->file_uri));

  export_settings_free (settings);
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
  REQUIRE_NONNULL (setting);
  REQUIRE (
    carla_native_plugin_has_custom_ui (
      setting->descr));
#    endif
#  endif

  test_helper_zrythm_cleanup ();
}
#endif

TEST_CASE_FIXTURE (ZrythmFixture, "crash handling")
{
#ifdef HAVE_CARLA
  /* stop dummy audio engine processing so we can
   * process manually */
  test_project_stop_dummy_engine ();

  PluginSetting * setting = test_plugin_manager_get_plugin_setting (
    SIGABRT_BUNDLE_URI, SIGABRT_URI, true);
  z_return_if_fail (setting);
  setting->bridge_mode_ = CarlaBridgeMode::Full;

  /* create a track from the plugin */
  track_create_for_plugin_at_idx_w_action (
    Track::Type::AudioBus, setting, TRACKLIST->tracks.size (), nullptr);

  Plugin * pl =
    TRACKLIST->tracks[TRACKLIST->tracks.size () - 1]->channel->inserts[0];
  REQUIRE (IS_PLUGIN_AND_NONNULL (pl));

  AUDIO_ENGINE->process (AUDIO_ENGINE->block_length_);
  AUDIO_ENGINE->process (AUDIO_ENGINE->block_length_);
  AUDIO_ENGINE->process (AUDIO_ENGINE->block_length_);
#endif
}

TEST_CASE_FIXTURE (ZrythmFixture, "process")
{
#if defined(HAVE_TEST_SIGNAL) && defined(HAVE_CARLA)
  test_plugin_manager_create_tracks_from_plugin (
    TEST_SIGNAL_BUNDLE, TEST_SIGNAL_URI, false, true, 1);

  Track *  track = TRACKLIST->tracks[TRACKLIST->tracks.size () - 1];
  Plugin * pl = track->channel->inserts[0];
  REQUIRE (IS_PLUGIN_AND_NONNULL (pl));

  /* stop dummy audio engine processing so we can
   * process manually */
  test_project_stop_dummy_engine ();

  /* run plugin and check that output is filled */
  Port *                out = pl->out_ports[0];
  nframes_t             local_offset = 60;
  EngineProcessTimeInfo time_nfo = {
    .g_start_frame = 0,
    .g_start_frame_w_offset = 0,
    .local_offset = 0,
    .nframes = local_offset
  };
  carla_native_plugin_process (pl->carla, &time_nfo);
  for (nframes_t i = 1; i < local_offset; i++)
    {
      REQUIRE (fabsf (out->buf_[i]) > 1e-10f);
    }
  time_nfo.g_start_frame = 0;
  time_nfo.g_start_frame_w_offset = local_offset;
  time_nfo.local_offset = local_offset;
  time_nfo.nframes_ = AUDIO_ENGINE->block_length_ - local_offset;
  carla_native_plugin_process (pl->carla, &time_nfo);
  for (nframes_t i = local_offset; i < AUDIO_ENGINE->block_length_; i++)
    {
      REQUIRE (fabsf (out->buf_[i]) > 1e-10f);
    }
#endif
}

TEST_SUITE_END;