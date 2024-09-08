// SPDX-FileCopyrightText: © 2021-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#include "actions/tracklist_selections.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "actions/mixer_selections_action.h"
#include "dsp/exporter.h"
#include "dsp/fader.h"
#include "dsp/router.h"
#include "plugins/carla_native_plugin.h"

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
        Exporter::Settings settings;
        settings.set_bounce_defaults (Exporter::Format::WAV, "", __func__);
        settings.time_range_ = Exporter::TimeRange::Loop;
        settings.bounce_with_parents_ = true;
        settings.mode_ = Exporter::Mode::Full;

        Exporter exporter (settings);
        exporter.prepare_tracks_for_export (*AUDIO_ENGINE, *TRANSPORT);

        /* start exporting in a new thread */
        exporter.begin_generic_thread ();
        exporter.join_generic_thread ();
        exporter.post_export ();

        ASSERT_FALSE (
          audio_file_is_silent (exporter.get_exported_path ().c_str ()));
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
  FileDescriptor file (fs::path (TESTS_SRCDIR) / "test.wav");
  Track::create_with_action (
    Track::Type::Audio, nullptr, &file, &PLAYHEAD, TRACKLIST->get_num_tracks (),
    1, -1, nullptr);
  auto audio_track = TRACKLIST->get_last_track<AudioTrack> ();

  /* hard pan right */
  UNDO_MANAGER->perform (std::make_unique<SingleTrackFloatAction> (
    SingleTrackFloatAction::EditType::Pan, audio_track, 0.5f, 1.f, false));

  /* add a mono insert */
  auto setting = test_plugin_manager_get_plugin_setting (
    LSP_COMPRESSOR_MONO_BUNDLE, LSP_COMPRESSOR_MONO_URI, true);

  UNDO_MANAGER->perform (std::make_unique<MixerSelectionsCreateAction> (
    PluginSlotType::Insert, *audio_track, 0, setting, 1));

  const auto &pl = audio_track->channel_->inserts_[0];
  ASSERT_NONNULL (pl);

  AUDIO_ENGINE->process (AUDIO_ENGINE->block_length_);
  AUDIO_ENGINE->process (AUDIO_ENGINE->block_length_);
  AUDIO_ENGINE->process (AUDIO_ENGINE->block_length_);

  /* bounce */
  Exporter::Settings settings;
  settings.set_bounce_defaults (Exporter::Format::WAV, "", __func__);
  settings.time_range_ = Exporter::TimeRange::Loop;
  settings.bounce_with_parents_ = true;
  settings.mode_ = Exporter::Mode::Full;

  Exporter exporter (settings);
  exporter.prepare_tracks_for_export (*AUDIO_ENGINE, *TRANSPORT);

  /* start exporting in a new thread */
  exporter.begin_generic_thread ();
  exporter.join_generic_thread ();
  exporter.post_export ();

  ASSERT_FALSE (audio_file_is_silent (exporter.get_exported_path ().c_str ()));
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
  ASSERT_NONNULL (setting);
  ASSERT_TRUE (
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

  auto setting = test_plugin_manager_get_plugin_setting (
    SIGABRT_BUNDLE_URI, SIGABRT_URI, true);
  setting.bridge_mode_ = CarlaBridgeMode::Full;

  /* create a track from the plugin */
  auto track = Track::create_for_plugin_at_idx_w_action<AudioBusTrack> (
    &setting, TRACKLIST->get_num_tracks ());

  const auto &pl = track->channel_->inserts_[0];
  ASSERT_NONNULL (pl);

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

  auto        track = TRACKLIST->get_last_track<ChannelTrack> ();
  const auto &pl = track->channel_->inserts_[0];
  ASSERT_NONNULL (pl);

  /* stop dummy audio engine processing so we can process manually */
  test_project_stop_dummy_engine ();

  /* run plugin and check that output is filled */
  const auto           &out = pl->out_ports_[0];
  nframes_t             local_offset = 60;
  EngineProcessTimeInfo time_nfo = {
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = local_offset
  };
  pl->process (time_nfo);
  for (nframes_t i = 1; i < local_offset; i++)
    {
      ASSERT_TRUE (fabsf (out->buf_[i]) > 1e-10f);
    }
  time_nfo.g_start_frame_ = 0;
  time_nfo.g_start_frame_w_offset_ = local_offset;
  time_nfo.local_offset_ = local_offset;
  time_nfo.nframes_ = AUDIO_ENGINE->block_length_ - local_offset;
  pl->process (time_nfo);
  for (nframes_t i = local_offset; i < AUDIO_ENGINE->block_length_; i++)
    {
      ASSERT_TRUE (fabsf (out->buf_[i]) > 1e-10f);
    }
#endif
}

TEST_SUITE_END;