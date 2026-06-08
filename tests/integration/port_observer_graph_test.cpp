// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/audio_port.h"
#include "dsp/graph_node.h"
#include "dsp/graph_scheduler.h"
#include "dsp/itransport.h"
#include "dsp/midi_event.h"
#include "dsp/port_observation_manager.h"
#include "dsp/port_observation_token.h"
#include "dsp/port_observer.h"
#include "dsp/tempo_map.h"
#include "utils/object_registry.h"
#include "utils/registry_utils.h"

#include "helpers/scoped_qcoreapplication.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace testing;

namespace zrythm::dsp
{

class MockTransportForIntegration : public zrythm::dsp::ITransport
{
public:
  MOCK_METHOD (
    (std::pair<units::sample_t, units::sample_t>),
    get_loop_range_positions,
    (),
    (const, noexcept, override));
  MOCK_METHOD (
    (std::pair<units::sample_t, units::sample_t>),
    get_punch_range_positions,
    (),
    (const, noexcept, override));
  MOCK_METHOD (PlayState, get_play_state, (), (const, noexcept, override));
  MOCK_METHOD (
    units::sample_t,
    get_playhead_position_in_audio_thread,
    (),
    (const, noexcept, override));
  MOCK_METHOD (bool, loop_enabled, (), (const, noexcept, override));
  MOCK_METHOD (bool, punch_enabled, (), (const, noexcept, override));
  MOCK_METHOD (bool, recording_enabled, (), (const, noexcept, override));
  MOCK_METHOD (
    units::sample_t,
    recording_preroll_frames_remaining,
    (),
    (const, noexcept, override));
  MOCK_METHOD (
    units::sample_t,
    metronome_countin_frames_remaining,
    (),
    (const, noexcept, override));
};

class PortObserverIntegrationTest : public ::testing::Test
{
protected:
  test_helpers::ScopedQCoreApplication app_;
  utils::ObjectRegistry                registry_;
  units::sample_rate_t        sample_rate_{ units::sample_rate (48000) };
  units::sample_u32_t         block_length_{ units::samples (256u) };
  TempoMap                    tempo_map_{ sample_rate_ };
  MockTransportForIntegration transport_;

  void SetUp () override
  {
    ON_CALL (transport_, get_play_state ())
      .WillByDefault (Return (ITransport::PlayState::Paused));
    ON_CALL (transport_, get_playhead_position_in_audio_thread ())
      .WillByDefault (Return (units::samples (0)));
    ON_CALL (transport_, loop_enabled ()).WillByDefault (Return (false));
    ON_CALL (transport_, punch_enabled ()).WillByDefault (Return (false));
    ON_CALL (transport_, recording_enabled ()).WillByDefault (Return (false));
    ON_CALL (transport_, recording_preroll_frames_remaining ())
      .WillByDefault (Return (units::samples (0)));
    ON_CALL (transport_, metronome_countin_frames_remaining ())
      .WillByDefault (Return (units::samples (0)));
    ON_CALL (transport_, get_loop_range_positions ())
      .WillByDefault (
        Return (std::make_pair (units::samples (0), units::samples (48000))));
    ON_CALL (transport_, get_punch_range_positions ())
      .WillByDefault (
        Return (std::make_pair (units::samples (0), units::samples (48000))));
  }
};

TEST_F (PortObserverIntegrationTest, ObserverCapturesDataThroughGraphCycle)
{
  auto port_ref = utils::create_object<AudioPort> (
    registry_, u8"Graph Audio", PortFlow::Output, AudioPort::BusLayout::Mono, 1);
  auto * port = port_ref.get_object_as<AudioPort> ();

  auto observer = std::make_unique<PortObserver> (registry_, *port);

  graph::GraphNodeCollection collection;
  auto port_node = std::make_unique<graph::GraphNode> (1, *port);
  auto obs_node = std::make_unique<graph::GraphNode> (2, *observer);
  port_node->connect_to (*obs_node);
  collection.graph_nodes_.push_back (std::move (port_node));
  collection.graph_nodes_.push_back (std::move (obs_node));
  collection.finalize_nodes ();

  auto scheduler = std::make_unique<graph::GraphScheduler> (
    [] (std::function<void ()> f) { f (); }, sample_rate_, block_length_);
  scheduler->rechain_from_node_collection (
    std::move (collection), sample_rate_, block_length_);
  scheduler->start_threads (1);

  port->buffers ()->clear ();
  port->buffers ()->getWritePointer (0)[0] = 0.75f;

  auto time_info = graph::ProcessBlockInfo::from_position_and_nframes (
    units::samples (0), block_length_);

  scheduler->run_cycle (time_info, units::samples (0), transport_, tempo_map_);
  scheduler->terminate_threads ();

  EXPECT_GT (observer->audio_ring (0).read_space (), 0u);
  float sample = 0.f;
  observer->audio_ring (0).read (sample);
  EXPECT_FLOAT_EQ (sample, 0.75f);
}

TEST_F (PortObserverIntegrationTest, ObserverDrainPopulatesTokenCache)
{
  auto port_ref = utils::create_object<AudioPort> (
    registry_, u8"Drain Audio", PortFlow::Output, AudioPort::BusLayout::Mono, 1);
  auto * port = port_ref.get_object_as<AudioPort> ();

  PortObservationManager manager (registry_);
  ObservationToken       token (manager, *port);
  auto *                 observer = manager.get_observer (*port);
  ASSERT_NE (observer, nullptr);

  graph::GraphNodeCollection collection;
  auto port_node = std::make_unique<graph::GraphNode> (1, *port);
  auto obs_node = std::make_unique<graph::GraphNode> (2, *observer);
  port_node->connect_to (*obs_node);
  collection.graph_nodes_.push_back (std::move (port_node));
  collection.graph_nodes_.push_back (std::move (obs_node));
  collection.finalize_nodes ();

  auto scheduler = std::make_unique<graph::GraphScheduler> (
    [] (std::function<void ()> f) { f (); }, sample_rate_, block_length_);
  scheduler->rechain_from_node_collection (
    std::move (collection), sample_rate_, block_length_);
  scheduler->start_threads (1);

  port->buffers ()->clear ();
  port->buffers ()->getWritePointer (0)[0] = 0.42f;

  auto time_info = graph::ProcessBlockInfo::from_position_and_nframes (
    units::samples (0), block_length_);

  scheduler->run_cycle (time_info, units::samples (0), transport_, tempo_map_);
  scheduler->terminate_threads ();

  manager.drain_all ();

  auto &cache = token.cache ();
  ASSERT_FALSE (cache.audio[0].empty ());
  EXPECT_FLOAT_EQ (cache.audio[0].front (), 0.42f);
}

}
