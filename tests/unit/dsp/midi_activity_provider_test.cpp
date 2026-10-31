// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <memory>
#include <vector>

#include "dsp/midi_activity_provider.h"
#include "dsp/midi_event.h"
#include "dsp/midi_port.h"
#include "dsp/port_observation_manager.h"
#include "dsp/port_observer.h"
#include "utils/object_registry.h"
#include "utils/registry_utils.h"

#include <QSignalSpy>

#include "helpers/scoped_qcoreapplication.h"

#include "graph_helpers.h"
#include <gtest/gtest.h>

namespace zrythm::dsp
{

using graph_test::MockTransport;
using MidiEventKind = MidiActivityProvider::MidiEventKind;
using MidiEventKinds = MidiActivityProvider::MidiEventKinds;

class MidiActivityProviderTest : public ::testing::Test
{
protected:
  test_helpers::ScopedQCoreApplication app_;
  utils::ObjectRegistry                registry_;
  PortObservationManager               manager_{ registry_ };
  MockTransport                        mock_transport_;
  units::sample_rate_t sample_rate_{ units::sample_rate (48000) };
  units::sample_u32_t  block_length_{ units::samples (256u) };
  TempoMap             tempo_map_{ sample_rate_ };

  graph::ProcessBlockInfo default_time_nfo () const
  {
    return {
      .transport_position_ = units::samples (0),
      .buffer_offset_ = units::samples (0),
      .nframes_ = block_length_
    };
  }

  void drain_and_update (
    MidiActivityProvider          &provider,
    PortObserver *                 observer,
    const graph::ProcessBlockInfo &time_nfo)
  {
    observer->process_block (time_nfo, mock_transport_, tempo_map_);
    manager_.drain_all ();
    provider.update ();
  }

  void push_and_drain (
    MidiPort             &port,
    MidiActivityProvider &provider,
    PortObserver *        observer)
  {
    drain_and_update (provider, observer, default_time_nfo ());
    port.buffer_.clear ();
  }

  std::shared_ptr<std::vector<MidiActivityProvider::MidiEventKinds>>
  record_event_batches (MidiActivityProvider &provider)
  {
    auto batches =
      std::make_shared<std::vector<MidiActivityProvider::MidiEventKinds>> ();
    QObject::connect (
      &provider, &MidiActivityProvider::midiEventsReceived, &provider,
      [batches] (MidiActivityProvider::MidiEventKinds kinds) {
        batches->push_back (kinds);
      });
    return batches;
  }
};

void
push_note_on (
  MidiPort   &port,
  midi_byte_t channel,
  midi_byte_t pitch,
  midi_byte_t velocity = 100)
{
  const auto ev =
    midi_event::make_note_on (channel, pitch, velocity, units::samples (0));
  port.buffer_.push_back (ev.time_, ev.data ());
}

void
push_note_off (MidiPort &port, midi_byte_t channel, midi_byte_t pitch)
{
  const auto ev = midi_event::make_note_off_with_default_velocity (
    channel, pitch, units::samples (0));
  port.buffer_.push_back (ev.time_, ev.data ());
}

void
push_cc (MidiPort &port, midi_byte_t channel, midi_byte_t cc, midi_byte_t value)
{
  const auto ev =
    midi_event::make_control_change (channel, cc, value, units::samples (0));
  port.buffer_.push_back (ev.time_, ev.data ());
}

void
push_all_notes_off (MidiPort &port, midi_byte_t channel)
{
  const auto ev = midi_event::make_control_change (
    channel, utils::midi::MIDI_ALL_NOTES_OFF, 0, units::samples (0));
  port.buffer_.push_back (ev.time_, ev.data ());
}

TEST_F (MidiActivityProviderTest, InitiallyNoActivity)
{
  auto port_ref =
    utils::create_object<MidiPort> (registry_, u8"Test", PortFlow::Output);
  auto * port = port_ref.get_object_as<MidiPort> ();

  MidiActivityProvider provider;
  provider.setPort (port);
  provider.setPortObservationManager (&manager_);

  EXPECT_FALSE (provider.hasActivity ());
  for (int i = 0; i < 128; ++i)
    EXPECT_FALSE (provider.isNoteActive (i));
}

TEST_F (MidiActivityProviderTest, SingleNoteOnOff)
{
  auto port_ref =
    utils::create_object<MidiPort> (registry_, u8"Test", PortFlow::Output);
  auto * port = port_ref.get_object_as<MidiPort> ();
  port->prepare_for_processing (nullptr, sample_rate_, block_length_);

  MidiActivityProvider provider;
  provider.setPort (port);
  provider.setPortObservationManager (&manager_);

  auto * observer = manager_.get_observer (*port);
  ASSERT_NE (observer, nullptr);
  observer->prepare_for_processing (nullptr, sample_rate_, block_length_);

  push_note_on (*port, 0, 60);
  drain_and_update (provider, observer, default_time_nfo ());
  port->buffer_.clear ();

  EXPECT_TRUE (provider.isNoteActive (60));
  EXPECT_TRUE (provider.hasActivity ());

  push_note_off (*port, 0, 60);
  drain_and_update (provider, observer, default_time_nfo ());
  port->buffer_.clear ();

  EXPECT_FALSE (provider.isNoteActive (60));
}

TEST_F (MidiActivityProviderTest, OverlappingNoteOns)
{
  auto port_ref =
    utils::create_object<MidiPort> (registry_, u8"Test", PortFlow::Output);
  auto * port = port_ref.get_object_as<MidiPort> ();
  port->prepare_for_processing (nullptr, sample_rate_, block_length_);

  MidiActivityProvider provider;
  provider.setPort (port);
  provider.setPortObservationManager (&manager_);

  auto * observer = manager_.get_observer (*port);
  ASSERT_NE (observer, nullptr);
  observer->prepare_for_processing (nullptr, sample_rate_, block_length_);

  push_note_on (*port, 0, 60);
  push_note_on (*port, 1, 60);
  drain_and_update (provider, observer, default_time_nfo ());
  port->buffer_.clear ();

  EXPECT_TRUE (provider.isNoteActive (60));

  push_note_off (*port, 0, 60);
  drain_and_update (provider, observer, default_time_nfo ());
  port->buffer_.clear ();

  EXPECT_TRUE (provider.isNoteActive (60));

  push_note_off (*port, 1, 60);
  drain_and_update (provider, observer, default_time_nfo ());
  port->buffer_.clear ();

  EXPECT_FALSE (provider.isNoteActive (60));
}

TEST_F (MidiActivityProviderTest, NoteOnVelocityZeroIsNoteOff)
{
  auto port_ref =
    utils::create_object<MidiPort> (registry_, u8"Test", PortFlow::Output);
  auto * port = port_ref.get_object_as<MidiPort> ();
  port->prepare_for_processing (nullptr, sample_rate_, block_length_);

  MidiActivityProvider provider;
  provider.setPort (port);
  provider.setPortObservationManager (&manager_);

  auto * observer = manager_.get_observer (*port);
  ASSERT_NE (observer, nullptr);
  observer->prepare_for_processing (nullptr, sample_rate_, block_length_);

  push_note_on (*port, 0, 64);
  drain_and_update (provider, observer, default_time_nfo ());
  port->buffer_.clear ();

  EXPECT_TRUE (provider.isNoteActive (64));

  const std::array<midi_byte_t, 3> note_on_vel0{
    static_cast<midi_byte_t> (utils::midi::MIDI_CH1_NOTE_ON), 64, 0
  };
  port->buffer_.push_back (units::samples (0), note_on_vel0);
  drain_and_update (provider, observer, default_time_nfo ());
  port->buffer_.clear ();

  EXPECT_FALSE (provider.isNoteActive (64));
}

TEST_F (MidiActivityProviderTest, NonNoteMessagesSetActivityOnly)
{
  auto port_ref =
    utils::create_object<MidiPort> (registry_, u8"Test", PortFlow::Output);
  auto * port = port_ref.get_object_as<MidiPort> ();
  port->prepare_for_processing (nullptr, sample_rate_, block_length_);

  MidiActivityProvider provider;
  provider.setPort (port);
  provider.setPortObservationManager (&manager_);

  auto * observer = manager_.get_observer (*port);
  ASSERT_NE (observer, nullptr);
  observer->prepare_for_processing (nullptr, sample_rate_, block_length_);

  push_cc (*port, 0, 1, 64);
  drain_and_update (provider, observer, default_time_nfo ());
  port->buffer_.clear ();

  EXPECT_TRUE (provider.hasActivity ());
  for (int i = 0; i < 128; ++i)
    EXPECT_FALSE (provider.isNoteActive (i));
}

TEST_F (MidiActivityProviderTest, HasActivityResetsWhenNoEvents)
{
  auto port_ref =
    utils::create_object<MidiPort> (registry_, u8"Test", PortFlow::Output);
  auto * port = port_ref.get_object_as<MidiPort> ();
  port->prepare_for_processing (nullptr, sample_rate_, block_length_);

  MidiActivityProvider provider;
  provider.setPort (port);
  provider.setPortObservationManager (&manager_);

  auto * observer = manager_.get_observer (*port);
  ASSERT_NE (observer, nullptr);
  observer->prepare_for_processing (nullptr, sample_rate_, block_length_);

  push_note_on (*port, 0, 60);
  drain_and_update (provider, observer, default_time_nfo ());
  port->buffer_.clear ();
  EXPECT_TRUE (provider.hasActivity ());

  drain_and_update (provider, observer, default_time_nfo ());
  EXPECT_FALSE (provider.hasActivity ());
}

TEST_F (MidiActivityProviderTest, PortChangeResetsState)
{
  auto port_ref =
    utils::create_object<MidiPort> (registry_, u8"Test", PortFlow::Output);
  auto * port = port_ref.get_object_as<MidiPort> ();
  port->prepare_for_processing (nullptr, sample_rate_, block_length_);

  MidiActivityProvider provider;
  provider.setPort (port);
  provider.setPortObservationManager (&manager_);

  auto * observer = manager_.get_observer (*port);
  ASSERT_NE (observer, nullptr);
  observer->prepare_for_processing (nullptr, sample_rate_, block_length_);

  push_note_on (*port, 0, 60);
  drain_and_update (provider, observer, default_time_nfo ());
  port->buffer_.clear ();
  EXPECT_TRUE (provider.isNoteActive (60));

  auto port2_ref =
    utils::create_object<MidiPort> (registry_, u8"Test2", PortFlow::Output);
  auto * port2 = port2_ref.get_object_as<MidiPort> ();
  provider.setPort (port2);

  EXPECT_FALSE (provider.isNoteActive (60));
}

TEST_F (MidiActivityProviderTest, IsNoteActiveOutOfBoundsReturnsFalse)
{
  MidiActivityProvider provider;
  EXPECT_FALSE (provider.isNoteActive (-1));
  EXPECT_FALSE (provider.isNoteActive (128));
  EXPECT_FALSE (provider.isNoteActive (999));
}

TEST_F (MidiActivityProviderTest, AllNotesOffClearsAll)
{
  auto port_ref =
    utils::create_object<MidiPort> (registry_, u8"Test", PortFlow::Output);
  auto * port = port_ref.get_object_as<MidiPort> ();
  port->prepare_for_processing (nullptr, sample_rate_, block_length_);

  MidiActivityProvider provider;
  provider.setPort (port);
  provider.setPortObservationManager (&manager_);

  auto * observer = manager_.get_observer (*port);
  ASSERT_NE (observer, nullptr);
  observer->prepare_for_processing (nullptr, sample_rate_, block_length_);

  push_note_on (*port, 0, 60);
  push_note_on (*port, 0, 64);
  push_note_on (*port, 0, 72);
  drain_and_update (provider, observer, default_time_nfo ());
  port->buffer_.clear ();

  EXPECT_TRUE (provider.isNoteActive (60));
  EXPECT_TRUE (provider.isNoteActive (64));
  EXPECT_TRUE (provider.isNoteActive (72));

  QSignalSpy notes_spy (&provider, &MidiActivityProvider::activeNotesChanged);
  push_all_notes_off (*port, 0);
  drain_and_update (provider, observer, default_time_nfo ());
  port->buffer_.clear ();

  EXPECT_EQ (notes_spy.count (), 1);
  EXPECT_FALSE (provider.isNoteActive (60));
  EXPECT_FALSE (provider.isNoteActive (64));
  EXPECT_FALSE (provider.isNoteActive (72));
  for (int i = 0; i < 128; ++i)
    EXPECT_FALSE (provider.isNoteActive (i));
}

TEST_F (MidiActivityProviderTest, NoteOnAndOffEachEmitClassifiedKinds)
{
  auto port_ref =
    utils::create_object<MidiPort> (registry_, u8"Test", PortFlow::Output);
  auto * port = port_ref.get_object_as<MidiPort> ();
  port->prepare_for_processing (nullptr, sample_rate_, block_length_);

  MidiActivityProvider provider;
  provider.setPort (port);
  provider.setPortObservationManager (&manager_);

  auto * observer = manager_.get_observer (*port);
  ASSERT_NE (observer, nullptr);
  observer->prepare_for_processing (nullptr, sample_rate_, block_length_);

  auto batches = record_event_batches (provider);

  push_note_on (*port, 0, 60);
  push_and_drain (*port, provider, observer);
  ASSERT_EQ (batches->size (), 1);
  EXPECT_EQ (batches->front (), MidiEventKind::NoteOn);

  push_note_off (*port, 0, 60);
  push_and_drain (*port, provider, observer);
  ASSERT_EQ (batches->size (), 2);
  EXPECT_EQ (batches->back (), MidiEventKind::NoteOff);
}

TEST_F (MidiActivityProviderTest, NoteOffWithoutNoteOnEmitsNoteOff)
{
  auto port_ref =
    utils::create_object<MidiPort> (registry_, u8"Test", PortFlow::Output);
  auto * port = port_ref.get_object_as<MidiPort> ();
  port->prepare_for_processing (nullptr, sample_rate_, block_length_);

  MidiActivityProvider provider;
  provider.setPort (port);
  provider.setPortObservationManager (&manager_);

  auto * observer = manager_.get_observer (*port);
  ASSERT_NE (observer, nullptr);
  observer->prepare_for_processing (nullptr, sample_rate_, block_length_);

  auto batches = record_event_batches (provider);

  push_note_off (*port, 0, 60);
  push_and_drain (*port, provider, observer);
  ASSERT_EQ (batches->size (), 1);
  EXPECT_EQ (batches->front (), MidiEventKind::NoteOff);
}

TEST_F (MidiActivityProviderTest, VelocityZeroNoteOnIsClassifiedAsNoteOff)
{
  auto port_ref =
    utils::create_object<MidiPort> (registry_, u8"Test", PortFlow::Output);
  auto * port = port_ref.get_object_as<MidiPort> ();
  port->prepare_for_processing (nullptr, sample_rate_, block_length_);

  MidiActivityProvider provider;
  provider.setPort (port);
  provider.setPortObservationManager (&manager_);

  auto * observer = manager_.get_observer (*port);
  ASSERT_NE (observer, nullptr);
  observer->prepare_for_processing (nullptr, sample_rate_, block_length_);

  auto batches = record_event_batches (provider);

  const std::array<midi_byte_t, 3> note_on_vel0{
    static_cast<midi_byte_t> (utils::midi::MIDI_CH1_NOTE_ON), 64, 0
  };
  port->buffer_.push_back (units::samples (0), note_on_vel0);
  push_and_drain (*port, provider, observer);
  ASSERT_EQ (batches->size (), 1);
  EXPECT_EQ (batches->front (), MidiEventKind::NoteOff);
  EXPECT_FALSE (provider.isNoteActive (64));
}

TEST_F (MidiActivityProviderTest, MixedNoteBatchEmitsUnionOnce)
{
  auto port_ref =
    utils::create_object<MidiPort> (registry_, u8"Test", PortFlow::Output);
  auto * port = port_ref.get_object_as<MidiPort> ();
  port->prepare_for_processing (nullptr, sample_rate_, block_length_);

  MidiActivityProvider provider;
  provider.setPort (port);
  provider.setPortObservationManager (&manager_);

  auto * observer = manager_.get_observer (*port);
  ASSERT_NE (observer, nullptr);
  observer->prepare_for_processing (nullptr, sample_rate_, block_length_);

  auto batches = record_event_batches (provider);

  push_note_on (*port, 0, 60);
  push_note_on (*port, 0, 64);
  push_note_off (*port, 0, 60);
  push_and_drain (*port, provider, observer);
  ASSERT_EQ (batches->size (), 1);
  EXPECT_EQ (batches->front (), MidiEventKind::NoteOn | MidiEventKind::NoteOff);

  push_note_on (*port, 0, 67);
  push_and_drain (*port, provider, observer);
  ASSERT_EQ (batches->size (), 2);
  EXPECT_EQ (batches->back (), MidiEventKind::NoteOn);
}

TEST_F (MidiActivityProviderTest, SameTimestampNoteOffAndRetriggerEmitUnionOnce)
{
  auto port_ref =
    utils::create_object<MidiPort> (registry_, u8"Test", PortFlow::Output);
  auto * port = port_ref.get_object_as<MidiPort> ();
  port->prepare_for_processing (nullptr, sample_rate_, block_length_);

  MidiActivityProvider provider;
  provider.setPort (port);
  provider.setPortObservationManager (&manager_);

  auto * observer = manager_.get_observer (*port);
  ASSERT_NE (observer, nullptr);
  observer->prepare_for_processing (nullptr, sample_rate_, block_length_);

  push_note_on (*port, 0, 60);
  push_and_drain (*port, provider, observer);

  auto batches = record_event_batches (provider);

  push_note_off (*port, 0, 60);
  push_note_on (*port, 0, 60);
  push_and_drain (*port, provider, observer);
  ASSERT_EQ (batches->size (), 1);
  EXPECT_EQ (batches->front (), MidiEventKind::NoteOn | MidiEventKind::NoteOff);
  EXPECT_TRUE (provider.isNoteActive (60));
}

TEST_F (MidiActivityProviderTest, NonNoteMessagesEmitTheirKinds)
{
  auto port_ref =
    utils::create_object<MidiPort> (registry_, u8"Test", PortFlow::Output);
  auto * port = port_ref.get_object_as<MidiPort> ();
  port->prepare_for_processing (nullptr, sample_rate_, block_length_);

  MidiActivityProvider provider;
  provider.setPort (port);
  provider.setPortObservationManager (&manager_);

  auto * observer = manager_.get_observer (*port);
  ASSERT_NE (observer, nullptr);
  observer->prepare_for_processing (nullptr, sample_rate_, block_length_);

  auto batches = record_event_batches (provider);

  push_cc (*port, 0, 1, 64);
  const std::array<midi_byte_t, 3> pitch_bend{
    static_cast<midi_byte_t> (0xE0), 0, 64
  };
  port->buffer_.push_back (units::samples (0), pitch_bend);
  const std::array<midi_byte_t, 2> program_change{
    static_cast<midi_byte_t> (0xC0), 5
  };
  port->buffer_.push_back (units::samples (0), program_change);
  push_and_drain (*port, provider, observer);

  ASSERT_EQ (batches->size (), 1);
  EXPECT_EQ (
    batches->front (),
    MidiEventKind::ControlChange | MidiEventKind::PitchBend
      | MidiEventKind::ProgramChange);
  EXPECT_TRUE (provider.hasActivity ());
}

TEST_F (MidiActivityProviderTest, PressureAndSystemMessagesEmitTheirKinds)
{
  auto port_ref =
    utils::create_object<MidiPort> (registry_, u8"Test", PortFlow::Output);
  auto * port = port_ref.get_object_as<MidiPort> ();
  port->prepare_for_processing (nullptr, sample_rate_, block_length_);

  MidiActivityProvider provider;
  provider.setPort (port);
  provider.setPortObservationManager (&manager_);

  auto * observer = manager_.get_observer (*port);
  ASSERT_NE (observer, nullptr);
  observer->prepare_for_processing (nullptr, sample_rate_, block_length_);

  auto batches = record_event_batches (provider);

  const std::array<midi_byte_t, 3> poly_aftertouch{
    static_cast<midi_byte_t> (0xA0), 60, 40
  };
  port->buffer_.push_back (units::samples (0), poly_aftertouch);
  const std::array<midi_byte_t, 2> channel_pressure{
    static_cast<midi_byte_t> (0xD0), 40
  };
  port->buffer_.push_back (units::samples (0), channel_pressure);
  const std::array<midi_byte_t, 1> timing_clock{
    static_cast<midi_byte_t> (0xF8)
  };
  port->buffer_.push_back (units::samples (0), timing_clock);
  push_and_drain (*port, provider, observer);

  ASSERT_EQ (batches->size (), 1);
  EXPECT_EQ (
    batches->front (),
    MidiEventKind::PolyAftertouch | MidiEventKind::ChannelPressure
      | MidiEventKind::Other);
}

TEST_F (MidiActivityProviderTest, AllNotesOffEmitsAllNotesOffKind)
{
  auto port_ref =
    utils::create_object<MidiPort> (registry_, u8"Test", PortFlow::Output);
  auto * port = port_ref.get_object_as<MidiPort> ();
  port->prepare_for_processing (nullptr, sample_rate_, block_length_);

  MidiActivityProvider provider;
  provider.setPort (port);
  provider.setPortObservationManager (&manager_);

  auto * observer = manager_.get_observer (*port);
  ASSERT_NE (observer, nullptr);
  observer->prepare_for_processing (nullptr, sample_rate_, block_length_);

  push_note_on (*port, 0, 60);
  push_and_drain (*port, provider, observer);

  auto batches = record_event_batches (provider);
  EXPECT_TRUE (batches->empty ());

  push_all_notes_off (*port, 0);
  push_and_drain (*port, provider, observer);
  ASSERT_EQ (batches->size (), 1);
  EXPECT_EQ (batches->front (), MidiEventKind::AllNotesOff);
  EXPECT_FALSE (provider.isNoteActive (60));
}

TEST_F (MidiActivityProviderTest, EmptyBatchDoesNotEmitEvents)
{
  auto port_ref =
    utils::create_object<MidiPort> (registry_, u8"Test", PortFlow::Output);
  auto * port = port_ref.get_object_as<MidiPort> ();
  port->prepare_for_processing (nullptr, sample_rate_, block_length_);

  MidiActivityProvider provider;
  provider.setPort (port);
  provider.setPortObservationManager (&manager_);

  auto * observer = manager_.get_observer (*port);
  ASSERT_NE (observer, nullptr);
  observer->prepare_for_processing (nullptr, sample_rate_, block_length_);

  push_note_on (*port, 0, 60);
  push_and_drain (*port, provider, observer);

  auto batches = record_event_batches (provider);

  drain_and_update (provider, observer, default_time_nfo ());
  EXPECT_TRUE (batches->empty ());
}

}
