// SPDX-FileCopyrightText: © 2020-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

# include "gui/dsp/fader.h"
# include "gui/dsp/midi_event.h"
# include "gui/dsp/router.h"
#include "utils/math.h"

#include "tests/helpers/plugin_manager.h"
#include "tests/helpers/project_helper.h"
#include "tests/helpers/zrythm_helper.h"

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
  auto prj_file = test_project_save ();
  ASSERT_NONEMPTY (prj_file);

  /* unload bundle so plugin can't be found */
  /*LilvNode * path = lilv_new_uri (LILV_WORLD, pl_bundle);*/
  /*lilv_world_unload_bundle (LILV_WORLD, path);*/
  /*lilv_node_free (path);*/

// TODO
#  if 0
  /* reload project and expect messages */
  LOG->use_structured_for_console = false;
  LOG->min_log_level_for_test_console = G_LOG_LEVEL_WARNING;
  g_test_expect_message (
    G_LOG_DOMAIN, G_LOG_LEVEL_WARNING, "*Instantiation failed for plugin *");
#  endif
  test_project_reload (prj_file);

  /* let engine run */
  AUDIO_ENGINE->wait_n_cycles (8);

// TODO
#  if 0
  /* assert expected messages */
  g_test_assert_expected_messages ();
#  endif
}
#endif

TEST_F (ZrythmFixture, LoadNonExistingPlugin)
{
#ifdef HAVE_HELM
  _test_loading_non_existing_plugin (HELM_BUNDLE, HELM_URI, false);
#endif
}

TEST_F (ZrythmFixture, LoadFullyBridgedPlugin)
{
#if HAVE_CARLA
#  ifdef HAVE_CHIPWAVE
  test_plugin_manager_create_tracks_from_plugin (
    CHIPWAVE_BUNDLE, CHIPWAVE_URI, true, true, 1);

  auto get_skew_duty_port = [] () -> ControlPort * {
    auto  track = TRACKLIST->get_last_track<InstrumentTrack> ();
    auto &pl = track->channel_->instrument_;
    auto  it = std::find_if (
      pl->in_ports_.begin (), pl->in_ports_.end (),
      [] (const auto &p) { return p->id_.label_ == "OscA Skew/Duty"; });
    ASSERT_TRUE (it != pl->in_ports_.end ());
    return dynamic_cast<ControlPort *> ((*it).get ());
  };

  Port * port = get_skew_duty_port ();
  z_return_if_fail (port);
  float val_before = port->control_;
  float val_after = 1.f;
  ASSERT_NEAR (val_before, 0.5f, 0.0001f);
  port->set_control_value (val_after, F_NORMALIZED, true);
  ASSERT_NEAR (port->control, val_after, 0.0001f);

  /* save project and reload and check the
   * value is correct */
  test_project_save_and_reload ();

  port = get_skew_duty_port ();
  z_return_if_fail (port);
  ASSERT_NEAR (port->control, val_after, 0.0001f);
#  endif
#endif
}

TEST_F (ZrythmFixture, LoadPluginsNeedingBridging)
{
#if HAVE_CARLA
#  ifdef HAVE_CALF_MONOSYNTH
  auto setting = test_plugin_manager_get_plugin_setting (
    CALF_MONOSYNTH_BUNDLE, CALF_MONOSYNTH_URI, false);
  ASSERT_TRUE (setting.open_with_carla_);
  ASSERT_EQ (setting.bridge_mode_, zrythm::gui::dsp::plugins::CarlaBridgeMode::Full);

  test_project_save_and_reload ();
#  endif
#endif
}

TEST_F (ZrythmFixture, BypassStateAfterProjectLoad)
{
#ifdef HAVE_LSP_COMPRESSOR
  for (
    int i = 0;
#  if HAVE_CARLA
    i < 2;
#  else
    i < 1;
#  endif
    i++)
    {
      /* create fx track */
      test_plugin_manager_create_tracks_from_plugin (
        LSP_COMPRESSOR_BUNDLE, LSP_COMPRESSOR_URI, false, i == 1, 1);

      {
        auto  track = TRACKLIST->get_last_track<ChannelTrack> ();
        auto &pl = track->channel_->inserts_[0];
        ASSERT_NONNULL (pl);

        /* set bypass */
        pl->set_enabled (false, false);
        ASSERT_FALSE (pl->is_enabled (false));
      }

      /* reload project */
      test_project_save_and_reload ();

      {
        auto        track = TRACKLIST->get_last_track<ChannelTrack> ();
        const auto &pl = track->channel_->inserts_[0];
        ASSERT_NONNULL (pl);

        /* check bypass */
        ASSERT_FALSE (pl->is_enabled (false));
      }
    }
#endif
}

TEST_F (ZrythmFixture, PluginWithoutOutputs)
{
#ifdef HAVE_KXSTUDIO_LFO
  test_plugin_manager_create_tracks_from_plugin (
    KXSTUDIO_LFO_BUNDLE, KXSTUDIO_LFO_URI, false, true, 1);
  auto        track = TRACKLIST->get_last_track<ChannelTrack> ();
  const auto &pl = track->channel_->inserts_[0];
  ASSERT_NONNULL (pl);

  /* reload project */
  test_project_save_and_reload ();
#endif
}
