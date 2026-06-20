// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/audio_port.h"
#include "dsp/midi_event.h"
#include "dsp/midi_port.h"
#include "dsp/port_observation_cache.h"
#include "dsp/port_observation_manager.h"
#include "dsp/port_observation_token.h"
#include "dsp/port_observer.h"
#include "utils/object_registry.h"
#include "utils/registry_utils.h"

#include "helpers/scoped_qcoreapplication.h"

#include "graph_helpers.h"
#include <gtest/gtest.h>

namespace zrythm::dsp
{

class PortObservationManagerTest : public ::testing::Test
{
protected:
  test_helpers::ScopedQCoreApplication app_;
  utils::ObjectRegistry                registry_;
  units::sample_rate_t sample_rate_{ units::sample_rate (48000) };
  units::sample_u32_t  block_length_{ units::samples (256u) };

  int                    recalc_count_ = 0;
  PortObservationManager manager_{ registry_ };

  graph_test::MockTransport mock_transport_;
  TempoMap                  tempo_map_{ sample_rate_ };

  dsp::graph::ProcessBlockInfo default_time_nfo () const
  {
    return {
      .transport_position_ = units::samples (0),
      .buffer_offset_ = units::samples (0),
      .nframes_ = block_length_
    };
  }

  void SetUp () override
  {
    QObject::connect (
      &manager_, &PortObservationManager::observationChanged, &manager_,
      [this] () { ++recalc_count_; });
  }
};

// ============================================================
// Manager core tests
// ============================================================

TEST_F (PortObservationManagerTest, InitiallyEmpty)
{
  EXPECT_TRUE (manager_.observers ().empty ());
}

TEST_F (PortObservationManagerTest, GetObserverReturnsNullForUnknown)
{
  auto port_ref = utils::create_object<AudioPort> (
    registry_, u8"Test", PortFlow::Output, AudioPort::BusLayout::Mono, 1);
  auto * port = port_ref.get_object_as<AudioPort> ();
  EXPECT_EQ (manager_.get_observer (*port), nullptr);
}

TEST_F (PortObservationManagerTest, ObservesCorrectPort)
{
  auto port_ref = utils::create_object<AudioPort> (
    registry_, u8"Test", PortFlow::Output, AudioPort::BusLayout::Mono, 1);
  auto * port = port_ref.get_object_as<AudioPort> ();

  ObservationToken token (manager_, *port);

  auto * observer = manager_.get_observer (*port);
  ASSERT_NE (observer, nullptr);
  EXPECT_EQ (observer->observed_port_uuid (), port->get_uuid ());
}

TEST_F (PortObservationManagerTest, MultiplePortsObservedIndependently)
{
  auto port1_ref = utils::create_object<AudioPort> (
    registry_, u8"Port1", PortFlow::Output, AudioPort::BusLayout::Mono, 1);
  auto * port1 = port1_ref.get_object_as<AudioPort> ();

  auto port2_ref = utils::create_object<AudioPort> (
    registry_, u8"Port2", PortFlow::Output, AudioPort::BusLayout::Mono, 1);
  auto * port2 = port2_ref.get_object_as<AudioPort> ();

  ObservationToken token1 (manager_, *port1);
  ObservationToken token2 (manager_, *port2);

  EXPECT_EQ (manager_.observers ().size (), 2u);
  EXPECT_NE (manager_.get_observer (*port1), nullptr);
  EXPECT_NE (manager_.get_observer (*port2), nullptr);
  EXPECT_NE (manager_.get_observer (*port1), manager_.get_observer (*port2));
}

TEST_F (PortObservationManagerTest, SignalOnlyOnFirstAndLastToken)
{
  auto port_ref = utils::create_object<AudioPort> (
    registry_, u8"Test", PortFlow::Output, AudioPort::BusLayout::Mono, 1);
  auto * port = port_ref.get_object_as<AudioPort> ();

  auto token1 = std::make_unique<ObservationToken> (manager_, *port);
  EXPECT_EQ (recalc_count_, 1);

  auto token2 = std::make_unique<ObservationToken> (manager_, *port);
  EXPECT_EQ (recalc_count_, 1);

  token2.reset ();
  EXPECT_EQ (recalc_count_, 1);

  token1.reset ();
  EXPECT_EQ (recalc_count_, 2);
}

// ============================================================
// Observers map reflects registrations (endpoint rebuild contract)
// ============================================================

TEST_F (PortObservationManagerTest, ObserversMapReflectsRegistrations)
{
  EXPECT_TRUE (manager_.observers ().empty ());

  auto port1_ref = utils::create_object<AudioPort> (
    registry_, u8"P1", PortFlow::Output, AudioPort::BusLayout::Mono, 1);
  auto * port1 = port1_ref.get_object_as<AudioPort> ();

  auto port2_ref = utils::create_object<AudioPort> (
    registry_, u8"P2", PortFlow::Output, AudioPort::BusLayout::Mono, 1);
  auto * port2 = port2_ref.get_object_as<AudioPort> ();

  auto token1 = std::make_unique<ObservationToken> (manager_, *port1);
  EXPECT_EQ (manager_.observers ().size (), 1u);
  EXPECT_NE (manager_.get_observer (*port1), nullptr);
  EXPECT_EQ (manager_.get_observer (*port2), nullptr);

  auto token2 = std::make_unique<ObservationToken> (manager_, *port2);
  EXPECT_EQ (manager_.observers ().size (), 2u);

  auto token3 = std::make_unique<ObservationToken> (manager_, *port1);
  EXPECT_EQ (manager_.observers ().size (), 2u);

  token3.reset ();
  EXPECT_EQ (manager_.observers ().size (), 2u);

  token1.reset ();
  EXPECT_EQ (manager_.observers ().size (), 1u);
  EXPECT_EQ (manager_.get_observer (*port1), nullptr);
  EXPECT_NE (manager_.get_observer (*port2), nullptr);

  token2.reset ();
  EXPECT_TRUE (manager_.observers ().empty ());
}

// ============================================================
// Drain tests
// ============================================================

TEST_F (PortObservationManagerTest, DrainFillsMidiCache)
{
  auto port_ref =
    utils::create_object<MidiPort> (registry_, u8"Test MIDI", PortFlow::Output);
  auto * port = port_ref.get_object_as<MidiPort> ();
  port->prepare_for_processing (nullptr, sample_rate_, block_length_);

  ObservationToken token (manager_, *port);
  auto *           observer = manager_.get_observer (*port);
  ASSERT_NE (observer, nullptr);
  observer->prepare_for_processing (nullptr, sample_rate_, block_length_);

  {
    const auto _ev = midi_event::make_note_on (0, 64, 127, units::samples (0));
    port->buffer_.push_back (_ev.time_, _ev.data ());
  }

  observer->process_block (default_time_nfo (), mock_transport_, tempo_map_);

  manager_.drain_all ();

  auto &cache = token.cache ();
  ASSERT_EQ (cache.midi.size (), 1u);
  EXPECT_TRUE (utils::midi::midi_is_note_on (cache.midi[0].data ()));
}

// A drain batch larger than kMaxAudioSamples landing in an empty cache is
// trimmed to the cap.
TEST_F (PortObservationManagerTest, DrainTrimsAudioCacheWhenBatchExceedsCap)
{
  auto port_ref = utils::create_object<AudioPort> (
    registry_, u8"Test", PortFlow::Output, AudioPort::BusLayout::Mono, 1);
  auto * port = port_ref.get_object_as<AudioPort> ();
  port->prepare_for_processing (nullptr, sample_rate_, block_length_);

  ObservationToken token (manager_, *port);
  auto *           observer = manager_.get_observer (*port);
  ASSERT_NE (observer, nullptr);
  observer->prepare_for_processing (nullptr, sample_rate_, block_length_);

  const auto         time_nfo = default_time_nfo ();
  const unsigned int block_size =
    time_nfo.nframes_.in<unsigned int> (units::samples);
  const size_t blocks_needed =
    PortObservationCache::kMaxAudioSamples / block_size + 1;

  // Accumulate more samples than the cap without draining, so the next
  // drain reads a single batch larger than kMaxAudioSamples.
  for (size_t b = 0; b < blocks_needed; ++b)
    {
      port->buffers ()->clear ();
      float * data = port->buffers ()->getWritePointer (0);
      for (unsigned int i = 0; i < block_size; ++i)
        data[i] = 1.0f;
      observer->process_block (time_nfo, mock_transport_, tempo_map_);
    }

  manager_.drain_all ();

  const auto &result = token.cache ();
  ASSERT_EQ (result.audio[0].size (), PortObservationCache::kMaxAudioSamples);
  EXPECT_FLOAT_EQ (result.audio[0].front (), 1.0f);
  EXPECT_FLOAT_EQ (result.audio[0].back (), 1.0f);
}

// A single MIDI drain batch larger than kMaxMidiEvents is trimmed to the cap.
TEST_F (PortObservationManagerTest, DrainTrimsMidiCacheWhenBatchExceedsCap)
{
  auto port_ref =
    utils::create_object<MidiPort> (registry_, u8"Test MIDI", PortFlow::Output);
  auto * port = port_ref.get_object_as<MidiPort> ();
  port->prepare_for_processing (nullptr, sample_rate_, block_length_);

  ObservationToken token (manager_, *port);
  auto *           observer = manager_.get_observer (*port);
  ASSERT_NE (observer, nullptr);
  observer->prepare_for_processing (nullptr, sample_rate_, block_length_);

  const auto   time_nfo = default_time_nfo ();
  const auto   ev = midi_event::make_note_on (0, 64, 127, units::samples (0));
  const size_t events_needed = PortObservationCache::kMaxMidiEvents + 1;

  // Accumulate more events than the cap without draining.
  for (size_t e = 0; e < events_needed; ++e)
    {
      port->buffer_.clear ();
      port->buffer_.push_back (ev.time_, ev.data ());
      observer->process_block (time_nfo, mock_transport_, tempo_map_);
    }

  manager_.drain_all ();

  ASSERT_EQ (token.cache ().midi.size (), PortObservationCache::kMaxMidiEvents);
}

// ============================================================
// Token tests
// ============================================================

TEST_F (PortObservationManagerTest, TokenRAII)
{
  auto port_ref = utils::create_object<AudioPort> (
    registry_, u8"Test", PortFlow::Output, AudioPort::BusLayout::Mono, 1);
  auto * port = port_ref.get_object_as<AudioPort> ();

  EXPECT_EQ (recalc_count_, 0);
  EXPECT_EQ (manager_.get_observer (*port), nullptr);

  {
    ObservationToken token (manager_, *port);
    EXPECT_EQ (recalc_count_, 1);
    ASSERT_NE (manager_.get_observer (*port), nullptr);
  }

  EXPECT_EQ (recalc_count_, 2);
  EXPECT_EQ (manager_.get_observer (*port), nullptr);
}

TEST_F (PortObservationManagerTest, TokenReturnsCorrectPortUuid)
{
  auto port_ref = utils::create_object<AudioPort> (
    registry_, u8"Test", PortFlow::Output, AudioPort::BusLayout::Mono, 1);
  auto * port = port_ref.get_object_as<AudioPort> ();
  auto   port_uuid = port->get_uuid ();

  ObservationToken token (manager_, *port);

  EXPECT_EQ (token.port_uuid (), port_uuid);
}

TEST_F (PortObservationManagerTest, TokenCacheInitiallyEmpty)
{
  auto port_ref = utils::create_object<AudioPort> (
    registry_, u8"Test", PortFlow::Output, AudioPort::BusLayout::Mono, 1);
  auto * port = port_ref.get_object_as<AudioPort> ();

  ObservationToken token (manager_, *port);

  auto &cache = token.cache ();
  EXPECT_TRUE (cache.audio.empty ());
  EXPECT_TRUE (cache.midi.empty ());
}

TEST_F (PortObservationManagerTest, DrainCopiesAudioToCache)
{
  auto port_ref = utils::create_object<AudioPort> (
    registry_, u8"Test", PortFlow::Output, AudioPort::BusLayout::Mono, 1);
  auto * port = port_ref.get_object_as<AudioPort> ();
  port->prepare_for_processing (nullptr, sample_rate_, block_length_);

  ObservationToken token (manager_, *port);
  auto *           observer = manager_.get_observer (*port);
  ASSERT_NE (observer, nullptr);
  observer->prepare_for_processing (nullptr, sample_rate_, block_length_);

  port->buffers ()->clear ();
  float * data = port->buffers ()->getWritePointer (0);
  for (int i = 0; i < 256; ++i)
    data[i] = 1.0f;

  observer->process_block (default_time_nfo (), mock_transport_, tempo_map_);

  manager_.drain_all ();

  auto &cache = token.cache ();
  ASSERT_FALSE (cache.audio[0].empty ());
  EXPECT_FLOAT_EQ (cache.audio[0].front (), 1.0f);
}

TEST_F (PortObservationManagerTest, TokenCacheClear)
{
  auto port_ref = utils::create_object<AudioPort> (
    registry_, u8"Test", PortFlow::Output, AudioPort::BusLayout::Mono, 1);
  auto * port = port_ref.get_object_as<AudioPort> ();
  port->prepare_for_processing (nullptr, sample_rate_, block_length_);

  ObservationToken token (manager_, *port);
  auto *           observer = manager_.get_observer (*port);
  ASSERT_NE (observer, nullptr);
  observer->prepare_for_processing (nullptr, sample_rate_, block_length_);

  port->buffers ()->clear ();
  port->buffers ()->getWritePointer (0)[0] = 0.5f;

  observer->process_block (default_time_nfo (), mock_transport_, tempo_map_);
  manager_.drain_all ();

  EXPECT_FALSE (token.cache ().audio[0].empty ());

  token.cache ().clear ();
  EXPECT_TRUE (token.cache ().audio[0].empty ());
  EXPECT_TRUE (token.cache ().midi.empty ());
}

TEST_F (PortObservationManagerTest, MultipleTokensForSamePortGetIndependentCaches)
{
  auto port_ref = utils::create_object<AudioPort> (
    registry_, u8"Test", PortFlow::Output, AudioPort::BusLayout::Mono, 1);
  auto * port = port_ref.get_object_as<AudioPort> ();
  port->prepare_for_processing (nullptr, sample_rate_, block_length_);

  ObservationToken token1 (manager_, *port);
  ObservationToken token2 (manager_, *port);

  auto * observer = manager_.get_observer (*port);
  ASSERT_NE (observer, nullptr);
  observer->prepare_for_processing (nullptr, sample_rate_, block_length_);

  port->buffers ()->clear ();
  port->buffers ()->getWritePointer (0)[0] = 0.42f;

  observer->process_block (default_time_nfo (), mock_transport_, tempo_map_);
  manager_.drain_all ();

  EXPECT_FALSE (token1.cache ().audio[0].empty ());
  EXPECT_FALSE (token2.cache ().audio[0].empty ());

  token1.cache ().clear ();
  EXPECT_TRUE (token1.cache ().audio[0].empty ());
  EXPECT_FALSE (token2.cache ().audio[0].empty ());
}

TEST_F (PortObservationManagerTest, TokenMoveTransfersOwnership)
{
  auto port_ref = utils::create_object<AudioPort> (
    registry_, u8"Test", PortFlow::Output, AudioPort::BusLayout::Mono, 1);
  auto * port = port_ref.get_object_as<AudioPort> ();

  EXPECT_EQ (recalc_count_, 0);

  auto token = std::make_unique<ObservationToken> (manager_, *port);
  EXPECT_EQ (recalc_count_, 1);
  ASSERT_NE (manager_.get_observer (*port), nullptr);

  auto moved_token = std::move (*token);
  token.reset ();

  EXPECT_EQ (recalc_count_, 1);
  ASSERT_NE (manager_.get_observer (*port), nullptr);

  auto * observer = manager_.get_observer (*port);
  ASSERT_NE (observer, nullptr);
  observer->prepare_for_processing (nullptr, sample_rate_, block_length_);

  port->prepare_for_processing (nullptr, sample_rate_, block_length_);
  port->buffers ()->clear ();
  port->buffers ()->getWritePointer (0)[0] = 0.9f;
  observer->process_block (default_time_nfo (), mock_transport_, tempo_map_);

  manager_.drain_all ();

  EXPECT_FALSE (moved_token.cache ().audio[0].empty ());
  EXPECT_FLOAT_EQ (moved_token.cache ().audio[0].front (), 0.9f);

  moved_token.cache ().clear ();
}

TEST_F (PortObservationManagerTest, TokenMoveAssignmentTransfersOwnership)
{
  auto port1_ref = utils::create_object<AudioPort> (
    registry_, u8"P1", PortFlow::Output, AudioPort::BusLayout::Mono, 1);
  auto * port1 = port1_ref.get_object_as<AudioPort> ();

  auto port2_ref = utils::create_object<AudioPort> (
    registry_, u8"P2", PortFlow::Output, AudioPort::BusLayout::Mono, 1);
  auto * port2 = port2_ref.get_object_as<AudioPort> ();

  auto token1 = std::make_unique<ObservationToken> (manager_, *port1);
  EXPECT_EQ (recalc_count_, 1);
  EXPECT_NE (manager_.get_observer (*port1), nullptr);

  auto token2 = std::make_unique<ObservationToken> (manager_, *port2);
  EXPECT_EQ (recalc_count_, 2);
  EXPECT_NE (manager_.get_observer (*port2), nullptr);

  *token1 = std::move (*token2);
  token2.reset ();

  EXPECT_EQ (recalc_count_, 3);
  EXPECT_EQ (manager_.get_observer (*port1), nullptr);
  EXPECT_NE (manager_.get_observer (*port2), nullptr);

  token1.reset ();
  EXPECT_EQ (recalc_count_, 4);
  EXPECT_TRUE (manager_.observers ().empty ());
}

TEST_F (PortObservationManagerTest, MultiCycleDrainAccumulatesData)
{
  auto port_ref = utils::create_object<AudioPort> (
    registry_, u8"Test", PortFlow::Output, AudioPort::BusLayout::Mono, 1);
  auto * port = port_ref.get_object_as<AudioPort> ();
  port->prepare_for_processing (nullptr, sample_rate_, block_length_);

  ObservationToken token (manager_, *port);

  auto * observer = manager_.get_observer (*port);
  ASSERT_NE (observer, nullptr);
  observer->prepare_for_processing (nullptr, sample_rate_, block_length_);

  const auto time_nfo = default_time_nfo ();

  port->buffers ()->clear ();
  port->buffers ()->getWritePointer (0)[0] = 0.1f;

  observer->process_block (time_nfo, mock_transport_, tempo_map_);
  manager_.drain_all ();

  EXPECT_FALSE (token.cache ().audio[0].empty ());

  token.cache ().clear ();

  port->buffers ()->getWritePointer (0)[0] = 0.2f;

  observer->process_block (time_nfo, mock_transport_, tempo_map_);
  manager_.drain_all ();

  ASSERT_FALSE (token.cache ().audio[0].empty ());
  EXPECT_FLOAT_EQ (token.cache ().audio[0].front (), 0.2f);
}

}
