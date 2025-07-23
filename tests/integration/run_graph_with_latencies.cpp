// SPDX-FileCopyrightText: © 2020-2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/* This test was written because of an issue with not
 * post-processing nodes properly when no roll was
 * needed for them. This test will fail if it happens
 * again */

#include "zrythm-test-config.h"

#include "dsp/graph.h"
#include "engine/session/graph_dispatcher.h"
#include "gui/backend/backend/actions/mixer_selections_action.h"
#include "gui/backend/backend/actions/undo_manager.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"
#include "gui/dsp/control_port.h"
#include "structure/tracks/tempo_track.h"

#include "tests/helpers/plugin_manager.h"
#include "tests/helpers/project_helper.h"

#ifdef HAVE_NO_DELAY_LINE
static ControlPort *
get_delay_port (Plugin * pl)
{
  for (auto &port : pl->in_ports_)
    {
      if (port->id_.label_ == "Delay Time")
        {
          return dynamic_cast<ControlPort *> (port.get ());
        }
    }
  EXPECT_UNREACHABLE ();
  z_return_val_if_reached (nullptr);
}

static void
_test (
  const char * pl_bundle,
  const char * pl_uri,
  bool         is_instrument,
  bool         with_carla)
{
  /* 1. create instrument track */
  test_plugin_manager_create_tracks_from_plugin (
    pl_bundle, pl_uri, is_instrument, with_carla, 1);

  auto track = TRACKLIST->get_last_track<ChannelTrack> ();
  int  orig_track_pos = track->pos_;

  auto setting = test_plugin_manager_get_plugin_setting (
    NO_DELAY_LINE_BUNDLE, NO_DELAY_LINE_URI, with_carla);

  /* 2. add no delay line */
  UNDO_MANAGER->perform (
    std::make_unique<MixerSelectionsCreateAction> (
      zrythm::plugins::PluginSlotType::Insert, *track, 0, setting, 1));

  /* 3. set delay to high value */
  auto pl = track->channel_->inserts_[0].get ();
  auto port = get_delay_port (pl);
  port->set_val_from_normalized (0.1f, false);

  /* let the engine run */
  std::this_thread::sleep_for (std::chrono::milliseconds (1000000));
  nframes_t latency = pl->latency_;
  ASSERT_GT (latency, 0);

  /* recalculate graph to update latencies */
  ROUTER->recalc_graph (true);
  auto node = ROUTER->graph_->find_node_from_track (track, false);
  ASSERT_NONNULL (node);
  ASSERT_EQ (latency, node->route_playback_latency_);

  /* let the engine run */
  std::this_thread::sleep_for (std::chrono::milliseconds (1000000));
  ASSERT_EQ (latency, node->route_playback_latency_);

  /* 3. start playback */
  TRANSPORT->requestRoll (true);

  /* let the engine run */
  std::this_thread::sleep_for (std::chrono::milliseconds (4000000));

  /* 4. reload project */
  test_project_save_and_reload ();

  /* let the engine run */
  std::this_thread::sleep_for (std::chrono::milliseconds (1000000));

  /* add another track with latency and check if
   * OK */
  setting = test_plugin_manager_get_plugin_setting (
    NO_DELAY_LINE_BUNDLE, NO_DELAY_LINE_URI, with_carla);
  auto new_track = Track::create_for_plugin_at_idx_w_action<AudioBusTrack> (
    &setting, TRACKLIST->get_num_tracks ());

  pl = new_track->channel_->inserts_[0].get ();
  port = get_delay_port (pl);
  port->set_val_from_normalized (0.2f, false);

  /* let the engine run */
  std::this_thread::sleep_for (std::chrono::milliseconds (1000000));
  nframes_t latency2 = pl->latency_;
  ASSERT_GT (latency2, 0);
  ASSERT_GT (latency2, latency);

  /* recalculate graph to update latencies */
  ROUTER->recalc_graph (true);
  node = ROUTER->graph_->find_node_from_track (P_TEMPO_TRACK, false);
  ASSERT_TRUE (node);
  ASSERT_EQ (latency2, node->route_playback_latency_);
  node = ROUTER->graph_->find_node_from_track (new_track, false);
  ASSERT_TRUE (node);
  ASSERT_EQ (latency2, node->route_playback_latency_);

  /* let the engine run */
  std::this_thread::sleep_for (std::chrono::milliseconds (1000000));
  ASSERT_EQ (latency2, node->route_playback_latency_);

  /* 3. start playback */
  TRANSPORT->requestRoll (true);

  /* let the engine run */
  std::this_thread::sleep_for (std::chrono::milliseconds (4000000));

  /* set latencies to 0 and verify that the updated
   * latency for tempo is 0 */
  pl = new_track->channel_->inserts_[0].get ();
  port = get_delay_port (pl);
  port->set_val_from_normalized (0.f, false);
  track = TRACKLIST->get_track<ChannelTrack> (orig_track_pos);
  pl = track->channel_->inserts_[0].get ();
  port = get_delay_port (pl);
  port->set_val_from_normalized (0.f, false);

  std::this_thread::sleep_for (std::chrono::milliseconds (1000000));
  ASSERT_EQ (pl->latency_, 0);

  /* recalculate graph to update latencies */
  ROUTER->recalc_graph (true);
  node = ROUTER->graph_->find_node_from_track (P_TEMPO_TRACK, false);
  ASSERT_TRUE (node);
  ASSERT_EQ (node->route_playback_latency_, 0);
}
#endif

TEST_F (ZrythmFixture, RunGraphWithLatencies)
{
#ifdef HAVE_NO_DELAY_LINE
#  ifdef HAVE_HELM
  _test (HELM_BUNDLE, HELM_URI, true, false);
#  endif
#endif
#if 0
#  ifdef HAVE_NO_DELAY_LINE
  _test (
    NO_DELAY_LINE_BUNDLE, NO_DELAY_LINE_URI, false,
    false);
#  endif
#endif
}
