// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/midi_event.h"
#include "dsp/midi_port.h"
#include "dsp/port_observation_manager.h"
#include "dsp/port_observer.h"
#include "dsp/tempo_map.h"
#include "gui/qquick/track_event_activity_provider.h"
#include "structure/tracks/channel.h"
#include "structure/tracks/track_factory.h"
#include "utils/midi.h"
#include "utils/object_registry.h"
#include "utils/registry_utils.h"

#include "helpers/scoped_qcoreapplication.h"

#include "../../dsp/graph_helpers.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace zrythm::gui::qquick
{

using dsp::graph_test::MockTransport;
using structure::tracks::Track;
using structure::tracks::TrackFactory;

class TrackEventActivityProviderTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    tempo_map_ = std::make_unique<dsp::TempoMap> (sample_rate_);
    tempo_map_wrapper_ = std::make_unique<dsp::TempoMapWrapper> (*tempo_map_);
    mock_transport_ = std::make_unique<MockTransport> ();

    structure::tracks::FinalTrackDependencies deps{
      *tempo_map_wrapper_, registry_, [] { return false; }, {}
    };
    factory_ = std::make_unique<TrackFactory> (
      [deps] () -> structure::tracks::FinalTrackDependencies { return deps; });
  }

  dsp::graph::ProcessBlockInfo time_nfo () const
  {
    return {
      .transport_position_ = units::samples (0),
      .buffer_offset_ = units::samples (0),
      .nframes_ = block_length_
    };
  }

  // Pushes a raw MIDI message into the track's post-MIDI-FX port and runs one
  // observation cycle through the facade's note provider.
  void send_message (
    TrackEventActivityProvider  &facade,
    std::span<const midi_byte_t> message,
    dsp::PortObservationManager &manager)
  {
    auto * note_provider = facade.midiActivityProvider ();
    auto * port = qobject_cast<dsp::MidiPort *> (note_provider->port ());
    ASSERT_NE (port, nullptr);
    port->prepare_for_processing (nullptr, sample_rate_, block_length_);

    auto * observer = manager.get_observer (*port);
    ASSERT_NE (observer, nullptr);
    observer->prepare_for_processing (nullptr, sample_rate_, block_length_);

    port->buffer_.push_back (units::samples (0), message);
    observer->process_block (time_nfo (), *mock_transport_, *tempo_map_);
    manager.drain_all ();
    note_provider->update ();
    port->buffer_.clear ();
  }

  void send_message (
    TrackEventActivityProvider  &facade,
    std::span<const midi_byte_t> message)
  {
    send_message (facade, message, manager_);
  }

  void send_note (TrackEventActivityProvider &facade, midi_byte_t pitch)
  {
    const auto ev =
      dsp::midi_event::make_note_on (0, pitch, 100, units::samples (0));
    send_message (facade, ev.data ());
  }

  void send_note (
    TrackEventActivityProvider  &facade,
    midi_byte_t                  pitch,
    dsp::PortObservationManager &manager)
  {
    const auto ev =
      dsp::midi_event::make_note_on (0, pitch, 100, units::samples (0));
    send_message (facade, ev.data (), manager);
  }

  void
  send_control_change (TrackEventActivityProvider &facade, midi_byte_t controller)
  {
    const auto ev = dsp::midi_event::make_control_change (
      0, controller, 64, units::samples (0));
    send_message (facade, ev.data ());
  }

  void send_all_notes_off (TrackEventActivityProvider &facade)
  {
    const auto ev = dsp::midi_event::make_control_change (
      0, utils::midi::MIDI_ALL_NOTES_OFF, 0, units::samples (0));
    send_message (facade, ev.data ());
  }

  test_helpers::ScopedQCoreApplication  app_;
  utils::ObjectRegistry                 registry_;
  dsp::PortObservationManager           manager_{ registry_ };
  std::unique_ptr<dsp::TempoMap>        tempo_map_;
  std::unique_ptr<dsp::TempoMapWrapper> tempo_map_wrapper_;
  std::unique_ptr<MockTransport>        mock_transport_;
  std::unique_ptr<TrackFactory>         factory_;

  units::sample_rate_t sample_rate_{ units::sample_rate (48000) };
  units::sample_u32_t  block_length_{ units::samples (256u) };
};

TEST_F (TrackEventActivityProviderTest, InitiallyHasNoEventSource)
{
  TrackEventActivityProvider facade;
  EXPECT_FALSE (facade.hasEventSource ());
  EXPECT_EQ (facade.track (), nullptr);
  EXPECT_EQ (facade.portObservationManager (), nullptr);
  EXPECT_EQ (facade.midiActivityProvider ()->port (), nullptr);
}

TEST_F (TrackEventActivityProviderTest, NoteTracksHaveEventSource)
{
  auto instrument_ref = factory_->create_empty_track (Track::Type::Instrument);
  auto midi_ref = factory_->create_empty_track (Track::Type::Midi);
  auto chord_ref = factory_->create_empty_track (Track::Type::Chord);
  auto audio_ref = factory_->create_empty_track (Track::Type::Audio);
  auto audio_bus_ref = factory_->create_empty_track (Track::Type::AudioBus);
  auto folder_ref = factory_->create_empty_track (Track::Type::Folder);

  const auto verify = [&] (Track &track, bool expected) {
    TrackEventActivityProvider facade;
    facade.setPortObservationManager (&manager_);
    facade.setTrack (&track);
    EXPECT_EQ (facade.hasEventSource (), expected);

    const auto * expected_port =
      track.channel () != nullptr ? track.channel ()->postMidiFxPort () : nullptr;
    EXPECT_EQ (facade.midiActivityProvider ()->port (), expected_port);
  };

  verify (*instrument_ref.get (), true);
  verify (*midi_ref.get (), true);
  verify (*chord_ref.get (), true);
  verify (*audio_ref.get (), false);
  verify (*audio_bus_ref.get (), false);
  verify (*folder_ref.get (), false);
}

TEST_F (TrackEventActivityProviderTest, ForwardsNoteEventsFromPostMidiFxPort)
{
  auto instrument_ref = factory_->create_empty_track (Track::Type::Instrument);

  TrackEventActivityProvider facade;
  facade.setPortObservationManager (&manager_);
  facade.setTrack (instrument_ref.get ());

  QSignalSpy spy (&facade, &TrackEventActivityProvider::eventReceived);
  EXPECT_EQ (spy.count (), 0);

  send_note (facade, 60);
  EXPECT_EQ (spy.count (), 1);

  send_note (facade, 64);
  EXPECT_EQ (spy.count (), 2);
}

TEST_F (TrackEventActivityProviderTest, ForwardsAllNotesOffFromPostMidiFxPort)
{
  auto instrument_ref = factory_->create_empty_track (Track::Type::Instrument);

  TrackEventActivityProvider facade;
  facade.setPortObservationManager (&manager_);
  facade.setTrack (instrument_ref.get ());

  QSignalSpy spy (&facade, &TrackEventActivityProvider::eventReceived);
  send_all_notes_off (facade);
  EXPECT_EQ (spy.count (), 1);
}

TEST_F (TrackEventActivityProviderTest, IgnoresNonNoteMidiEvents)
{
  auto instrument_ref = factory_->create_empty_track (Track::Type::Instrument);

  TrackEventActivityProvider facade;
  facade.setPortObservationManager (&manager_);
  facade.setTrack (instrument_ref.get ());

  QSignalSpy spy (&facade, &TrackEventActivityProvider::eventReceived);
  send_control_change (facade, 1);
  EXPECT_EQ (spy.count (), 0);
  EXPECT_TRUE (facade.midiActivityProvider ()->hasActivity ());
}

TEST_F (TrackEventActivityProviderTest, AttachmentDoesNotReplayPastEvents)
{
  auto instrument_ref = factory_->create_empty_track (Track::Type::Instrument);

  TrackEventActivityProvider first;
  first.setPortObservationManager (&manager_);
  first.setTrack (instrument_ref.get ());
  send_note (first, 60);

  QSignalSpy first_spy (&first, &TrackEventActivityProvider::eventReceived);
  EXPECT_EQ (first_spy.count (), 0);

  // a second facade attached after the fact starts from a clean baseline
  TrackEventActivityProvider second;
  second.setPortObservationManager (&manager_);
  second.setTrack (instrument_ref.get ());

  QSignalSpy second_spy (&second, &TrackEventActivityProvider::eventReceived);
  second.midiActivityProvider ()->update ();
  EXPECT_EQ (second_spy.count (), 0);

  // but new events reach it
  send_note (second, 67);
  EXPECT_EQ (second_spy.count (), 1);
}

TEST_F (TrackEventActivityProviderTest, TrackReplacementRebindsSource)
{
  auto instrument_ref = factory_->create_empty_track (Track::Type::Instrument);
  auto audio_ref = factory_->create_empty_track (Track::Type::Audio);
  auto midi_ref = factory_->create_empty_track (Track::Type::Midi);

  TrackEventActivityProvider facade;
  facade.setPortObservationManager (&manager_);

  facade.setTrack (instrument_ref.get ());
  EXPECT_TRUE (facade.hasEventSource ());
  auto * instrument_port = facade.midiActivityProvider ()->port ();
  ASSERT_NE (instrument_port, nullptr);

  facade.setTrack (audio_ref.get ());
  EXPECT_FALSE (facade.hasEventSource ());
  EXPECT_EQ (facade.midiActivityProvider ()->port (), nullptr);

  facade.setTrack (midi_ref.get ());
  EXPECT_TRUE (facade.hasEventSource ());
  EXPECT_NE (facade.midiActivityProvider ()->port (), nullptr);
  EXPECT_NE (facade.midiActivityProvider ()->port (), instrument_port);
}

TEST_F (
  TrackEventActivityProviderTest,
  ManagerDestructionClearsProvidersAndAllowsRebind)
{
  auto instrument_ref = factory_->create_empty_track (Track::Type::Instrument);
  auto audio_ref = factory_->create_empty_track (Track::Type::Audio);

  utils::ObjectRegistry manager_registry;
  auto * manager = new dsp::PortObservationManager (manager_registry);

  TrackEventActivityProvider facade;
  facade.setPortObservationManager (manager);
  facade.setTrack (instrument_ref.get ());

  QSignalSpy event_spy (&facade, &TrackEventActivityProvider::eventReceived);
  send_note (facade, 60, *manager);
  EXPECT_EQ (event_spy.count (), 1);
  EXPECT_TRUE (facade.midiActivityProvider ()->hasActivity ());

  QSignalSpy manager_spy (
    &facade, &TrackEventActivityProvider::portObservationManagerChanged);
  delete manager;
  ASSERT_EQ (facade.portObservationManager (), nullptr);
  EXPECT_EQ (facade.midiActivityProvider ()->portObservationManager (), nullptr);
  EXPECT_EQ (manager_spy.count (), 1);
  EXPECT_FALSE (facade.midiActivityProvider ()->hasActivity ());

  facade.setTrack (audio_ref.get ());
  facade.setTrack (instrument_ref.get ());

  dsp::PortObservationManager replacement{ manager_registry };
  facade.setPortObservationManager (&replacement);
  send_note (facade, 64, replacement);
  EXPECT_EQ (event_spy.count (), 2);
}

TEST_F (TrackEventActivityProviderTest, MissingManagerIsSafe)
{
  auto instrument_ref = factory_->create_empty_track (Track::Type::Instrument);

  TrackEventActivityProvider facade;
  facade.setTrack (instrument_ref.get ());

  // the source exists even without an observation manager; no events flow
  EXPECT_TRUE (facade.hasEventSource ());
  EXPECT_EQ (facade.portObservationManager (), nullptr);
  facade.midiActivityProvider ()->update ();

  // a manager set later activates the pipeline
  facade.setPortObservationManager (&manager_);
  QSignalSpy spy (&facade, &TrackEventActivityProvider::eventReceived);
  send_note (facade, 60);
  EXPECT_EQ (spy.count (), 1);
}

TEST_F (TrackEventActivityProviderTest, NullTrackHasNoEventSource)
{
  TrackEventActivityProvider facade;
  facade.setPortObservationManager (&manager_);

  EXPECT_FALSE (facade.hasEventSource ());
  facade.midiActivityProvider ()->update ();
  EXPECT_FALSE (facade.hasEventSource ());
}

} // namespace zrythm::gui::qquick
