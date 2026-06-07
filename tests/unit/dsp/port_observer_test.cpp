// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/audio_port.h"
#include "dsp/cv_port.h"
#include "dsp/midi_event.h"
#include "dsp/midi_port.h"
#include "dsp/port_observer.h"
#include "utils/object_registry.h"
#include "utils/registry_utils.h"

#include "helpers/scoped_qcoreapplication.h"

#include "graph_helpers.h"
#include <gtest/gtest.h>

namespace zrythm::dsp
{

class PortObserverTest : public ::testing::Test
{
protected:
  test_helpers::ScopedQCoreApplication app_;
  utils::ObjectRegistry                registry_;
  units::sample_rate_t sample_rate_{ units::sample_rate (48000) };
  units::sample_u32_t  block_length_{ units::samples (256u) };

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
};

TEST_F (PortObserverTest, AudioCaptureToRingBuffer)
{
  auto port_ref = utils::create_object<AudioPort> (
    registry_, u8"Test Audio", PortFlow::Output, AudioPort::BusLayout::Stereo,
    2);
  auto * port = port_ref.get_object_as<AudioPort> ();
  port->prepare_for_processing (nullptr, sample_rate_, block_length_);

  auto observer = std::make_unique<PortObserver> (registry_, *port);
  observer->prepare_for_processing (nullptr, sample_rate_, block_length_);

  port->buffers ()->clear ();
  port->buffers ()->getWritePointer (0)[0] = 0.5f;
  port->buffers ()->getWritePointer (1)[0] = -0.8f;

  observer->process_block (default_time_nfo (), mock_transport_, tempo_map_);

  EXPECT_GT (observer->audio_ring (0).read_space (), 0u);
  EXPECT_GT (observer->audio_ring (1).read_space (), 0u);

  float sample = 0.f;
  observer->audio_ring (0).read (sample);
  EXPECT_FLOAT_EQ (sample, 0.5f);
  observer->audio_ring (1).read (sample);
  EXPECT_FLOAT_EQ (sample, -0.8f);
}

TEST_F (PortObserverTest, AudioMultiChannel)
{
  auto port_ref = utils::create_object<AudioPort> (
    registry_, u8"Stereo", PortFlow::Output, AudioPort::BusLayout::Stereo, 2);
  auto * port = port_ref.get_object_as<AudioPort> ();
  port->prepare_for_processing (nullptr, sample_rate_, block_length_);

  auto observer = std::make_unique<PortObserver> (registry_, *port);
  observer->prepare_for_processing (nullptr, sample_rate_, block_length_);

  port->buffers ()->clear ();
  port->buffers ()->getWritePointer (0)[0] = 0.25f;
  port->buffers ()->getWritePointer (1)[0] = 0.75f;

  observer->process_block (default_time_nfo (), mock_transport_, tempo_map_);

  EXPECT_EQ (observer->num_channels (), 2);

  float sample = 0.f;
  observer->audio_ring (0).read (sample);
  EXPECT_FLOAT_EQ (sample, 0.25f);
  observer->audio_ring (1).read (sample);
  EXPECT_FLOAT_EQ (sample, 0.75f);
}

TEST_F (PortObserverTest, AudioEmptyBufferNoop)
{
  auto port_ref = utils::create_object<AudioPort> (
    registry_, u8"Empty", PortFlow::Output, AudioPort::BusLayout::Mono, 1);
  auto * port = port_ref.get_object_as<AudioPort> ();

  auto observer = std::make_unique<PortObserver> (registry_, *port);
  observer->prepare_for_processing (nullptr, sample_rate_, block_length_);

  observer->process_block (default_time_nfo (), mock_transport_, tempo_map_);

  EXPECT_EQ (observer->audio_ring (0).read_space (), 0u);
}

TEST_F (PortObserverTest, MidiCaptureToRingBuffer)
{
  auto port_ref =
    utils::create_object<MidiPort> (registry_, u8"Test MIDI", PortFlow::Output);
  auto * port = port_ref.get_object_as<MidiPort> ();
  port->prepare_for_processing (nullptr, sample_rate_, block_length_);

  auto observer = std::make_unique<PortObserver> (registry_, *port);
  observer->prepare_for_processing (nullptr, sample_rate_, block_length_);

  observer->process_block (default_time_nfo (), mock_transport_, tempo_map_);
  EXPECT_EQ (observer->midi_ring ().read_space (), 0u);

  {
    const auto _ev = midi_event::make_note_on (0, 60, 100, units::samples (0));
    port->buffer_.push_back (_ev.time_, _ev.data ());
  }
  observer->process_block (default_time_nfo (), mock_transport_, tempo_map_);
  EXPECT_EQ (observer->midi_ring ().read_space (), 1u);

  RealtimeMidiEvent event;
  observer->midi_ring ().read (event);
  EXPECT_TRUE (utils::midi::midi_is_note_on (event.data ()));
}

TEST_F (PortObserverTest, MidiMultipleEvents)
{
  auto port_ref =
    utils::create_object<MidiPort> (registry_, u8"Test MIDI", PortFlow::Output);
  auto * port = port_ref.get_object_as<MidiPort> ();
  port->prepare_for_processing (nullptr, sample_rate_, block_length_);

  auto observer = std::make_unique<PortObserver> (registry_, *port);
  observer->prepare_for_processing (nullptr, sample_rate_, block_length_);

  {
    const auto _ev1 = midi_event::make_note_on (0, 60, 100, units::samples (0));
    port->buffer_.push_back (_ev1.time_, _ev1.data ());
  }
  {
    const auto _ev2 = midi_event::make_note_off (0, 60, units::samples (100));
    port->buffer_.push_back (_ev2.time_, _ev2.data ());
  }
  {
    const auto _ev3 =
      midi_event::make_control_change (0, 1, 64, units::samples (50));
    port->buffer_.push_back (_ev3.time_, _ev3.data ());
  }

  observer->process_block (default_time_nfo (), mock_transport_, tempo_map_);
  EXPECT_EQ (observer->midi_ring ().read_space (), 3u);
}

TEST_F (PortObserverTest, MidiEmptyBufferNoop)
{
  auto port_ref = utils::create_object<MidiPort> (
    registry_, u8"Empty MIDI", PortFlow::Output);
  auto * port = port_ref.get_object_as<MidiPort> ();
  port->prepare_for_processing (nullptr, sample_rate_, block_length_);

  auto observer = std::make_unique<PortObserver> (registry_, *port);
  observer->prepare_for_processing (nullptr, sample_rate_, block_length_);

  observer->process_block (default_time_nfo (), mock_transport_, tempo_map_);
  EXPECT_EQ (observer->midi_ring ().read_space (), 0u);
}

TEST_F (PortObserverTest, CVCaptureToRingBuffer)
{
  auto port_ref =
    utils::create_object<CVPort> (registry_, u8"Test CV", PortFlow::Output);
  auto * port = port_ref.get_object_as<CVPort> ();
  port->prepare_for_processing (nullptr, sample_rate_, block_length_);

  auto observer = std::make_unique<PortObserver> (registry_, *port);
  observer->prepare_for_processing (nullptr, sample_rate_, block_length_);

  port->buf_.resize (256);
  port->buf_[0] = 0.5f;
  port->buf_[1] = -0.3f;

  observer->process_block (default_time_nfo (), mock_transport_, tempo_map_);

  EXPECT_GT (observer->audio_ring (0).read_space (), 0u);

  float sample = 0.f;
  observer->audio_ring (0).read (sample);
  EXPECT_FLOAT_EQ (sample, 0.5f);
}

TEST_F (PortObserverTest, ObserverStoresPortUuid)
{
  auto port_ref = utils::create_object<AudioPort> (
    registry_, u8"Test", PortFlow::Output, AudioPort::BusLayout::Mono, 1);
  auto * port = port_ref.get_object_as<AudioPort> ();

  PortObserver observer (registry_, *port);
  EXPECT_EQ (observer.observed_port_uuid (), port->get_uuid ());
}

TEST_F (PortObserverTest, HasRingBuffersByPortType)
{
  auto audio_ref = utils::create_object<AudioPort> (
    registry_, u8"Audio", PortFlow::Output, AudioPort::BusLayout::Mono, 1);
  auto * audio_port = audio_ref.get_object_as<AudioPort> ();

  PortObserver audio_obs (registry_, *audio_port);
  audio_obs.prepare_for_processing (nullptr, sample_rate_, block_length_);
  EXPECT_TRUE (audio_obs.has_audio_rings ());
  EXPECT_FALSE (audio_obs.has_midi_ring ());

  auto midi_ref =
    utils::create_object<MidiPort> (registry_, u8"MIDI", PortFlow::Output);
  auto * midi_port = midi_ref.get_object_as<MidiPort> ();

  PortObserver midi_obs (registry_, *midi_port);
  midi_obs.prepare_for_processing (nullptr, sample_rate_, block_length_);
  EXPECT_FALSE (midi_obs.has_audio_rings ());
  EXPECT_TRUE (midi_obs.has_midi_ring ());
}

TEST_F (PortObserverTest, ReleaseResourcesClearsRings)
{
  auto port_ref = utils::create_object<AudioPort> (
    registry_, u8"Test", PortFlow::Output, AudioPort::BusLayout::Mono, 1);
  auto * port = port_ref.get_object_as<AudioPort> ();

  auto observer = std::make_unique<PortObserver> (registry_, *port);
  observer->prepare_for_processing (nullptr, sample_rate_, block_length_);
  EXPECT_TRUE (observer->has_audio_rings ());

  observer->release_resources ();
  EXPECT_FALSE (observer->has_audio_rings ());
}

}
