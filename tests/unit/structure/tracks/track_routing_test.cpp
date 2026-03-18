// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/tracks/track_routing.h"

#include "unit/structure/tracks/mock_track.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace zrythm::structure::tracks
{

class TrackRoutingTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    track_registry_ = std::make_unique<TrackRegistry> ();
    track_routing_ = std::make_unique<TrackRouting> (*track_registry_);
  }

  std::unique_ptr<MockTrack> createMockTrack (
    Track::Type   type,
    dsp::PortType in_type = dsp::PortType::Audio,
    dsp::PortType out_type = dsp::PortType::Audio)
  {
    return mock_track_factory_.createMockTrack (
      type, in_type, out_type,
      MockTrack::TrackFeatures::Automation | MockTrack::TrackFeatures::Lanes);
  }

  MockTrackFactory               mock_track_factory_;
  std::unique_ptr<TrackRegistry> track_registry_;
  std::unique_ptr<TrackRouting>  track_routing_;
};

TEST_F (TrackRoutingTest, NullSourceCannotBeRouted)
{
  auto audio_track = createMockTrack (Track::Type::Audio);

  // Null source is never valid
  EXPECT_FALSE (track_routing_->canRouteTo (nullptr, audio_track.get ()));

  // Both null is also invalid
  EXPECT_FALSE (track_routing_->canRouteTo (nullptr, nullptr));
}

TEST_F (TrackRoutingTest, NullDestinationAllowedForUnrouting)
{
  auto audio_track = createMockTrack (Track::Type::Audio);

  // Null destination is allowed (means "unroute")
  EXPECT_TRUE (track_routing_->canRouteTo (audio_track.get (), nullptr));
}

TEST_F (TrackRoutingTest, MasterTrackCannotBeRouted)
{
  auto master_track = createMockTrack (Track::Type::Master);
  auto audio_group = createMockTrack (Track::Type::AudioGroup);

  // Master cannot route to anything
  EXPECT_FALSE (
    track_routing_->canRouteTo (master_track.get (), audio_group.get ()));
}

TEST_F (TrackRoutingTest, CannotRouteToSelf)
{
  auto audio_track = createMockTrack (Track::Type::Audio);

  // A track cannot route to itself
  EXPECT_FALSE (
    track_routing_->canRouteTo (audio_track.get (), audio_track.get ()));
}

TEST_F (TrackRoutingTest, AudioToAudioRouting)
{
  auto audio_track = createMockTrack (Track::Type::Audio);
  auto audio_group = createMockTrack (Track::Type::AudioGroup);

  // Audio track (Audio out) -> Audio group (Audio in) - compatible
  EXPECT_TRUE (
    track_routing_->canRouteTo (audio_track.get (), audio_group.get ()));
}

TEST_F (TrackRoutingTest, MidiToMidiRouting)
{
  auto midi_track = createMockTrack (
    Track::Type::Midi, dsp::PortType::Midi, dsp::PortType::Midi);
  auto midi_group = createMockTrack (
    Track::Type::MidiGroup, dsp::PortType::Midi, dsp::PortType::Midi);

  // MIDI track (MIDI out) -> MIDI group (MIDI in) - compatible
  EXPECT_TRUE (
    track_routing_->canRouteTo (midi_track.get (), midi_group.get ()));
}

TEST_F (TrackRoutingTest, InstrumentTrackRouting)
{
  // Instrument tracks have MIDI input and Audio output
  auto instrument_track = createMockTrack (
    Track::Type::Instrument, dsp::PortType::Midi, dsp::PortType::Audio);
  auto audio_group = createMockTrack (Track::Type::AudioGroup);

  // Instrument (Audio out) -> Audio group (Audio in) - compatible
  EXPECT_TRUE (
    track_routing_->canRouteTo (instrument_track.get (), audio_group.get ()));
}

TEST_F (TrackRoutingTest, IncompatibleSignalTypes)
{
  auto audio_track = createMockTrack (Track::Type::Audio);
  auto midi_group = createMockTrack (
    Track::Type::MidiGroup, dsp::PortType::Midi, dsp::PortType::Midi);

  // Audio track (Audio out) -> MIDI group (Midi in) - incompatible
  EXPECT_FALSE (
    track_routing_->canRouteTo (audio_track.get (), midi_group.get ()));

  auto midi_track = createMockTrack (
    Track::Type::Midi, dsp::PortType::Midi, dsp::PortType::Midi);
  auto audio_group = createMockTrack (Track::Type::AudioGroup);

  // MIDI track (MIDI out) -> Audio group (Audio in) - incompatible
  EXPECT_FALSE (
    track_routing_->canRouteTo (midi_track.get (), audio_group.get ()));
}

TEST_F (TrackRoutingTest, AudioGroupToAudioGroupRouting)
{
  auto audio_group1 = createMockTrack (Track::Type::AudioGroup);
  auto audio_group2 = createMockTrack (Track::Type::AudioGroup);

  // Audio group (Audio out) -> Audio group (Audio in) - compatible
  EXPECT_TRUE (
    track_routing_->canRouteTo (audio_group1.get (), audio_group2.get ()));
}

TEST_F (TrackRoutingTest, MidiGroupToMidiGroupRouting)
{
  auto midi_group1 = createMockTrack (
    Track::Type::MidiGroup, dsp::PortType::Midi, dsp::PortType::Midi);
  auto midi_group2 = createMockTrack (
    Track::Type::MidiGroup, dsp::PortType::Midi, dsp::PortType::Midi);

  // MIDI group (MIDI out) -> MIDI group (Midi in) - compatible
  EXPECT_TRUE (
    track_routing_->canRouteTo (midi_group1.get (), midi_group2.get ()));
}

TEST_F (TrackRoutingTest, CannotRouteToNonGroupTarget)
{
  auto audio_track1 = createMockTrack (Track::Type::Audio);
  auto audio_track2 = createMockTrack (Track::Type::Audio);

  // Audio track cannot route to another Audio track (not a group target)
  EXPECT_FALSE (
    track_routing_->canRouteTo (audio_track1.get (), audio_track2.get ()));

  auto midi_track1 = createMockTrack (
    Track::Type::Midi, dsp::PortType::Midi, dsp::PortType::Midi);
  auto midi_track2 = createMockTrack (
    Track::Type::Midi, dsp::PortType::Midi, dsp::PortType::Midi);

  // MIDI track cannot route to another MIDI track (not a group target)
  EXPECT_FALSE (
    track_routing_->canRouteTo (midi_track1.get (), midi_track2.get ()));
}

TEST_F (TrackRoutingTest, CircularRouteDetection)
{
  // Create chain: Audio -> AudioGroup1 -> AudioGroup2
  auto audio_track = createMockTrack (Track::Type::Audio);
  auto audio_group1 = createMockTrack (Track::Type::AudioGroup);
  auto audio_group2 = createMockTrack (Track::Type::AudioGroup);

  // Set up the routing chain: Audio -> Group1 -> Group2
  track_routing_->add_or_replace_route (
    audio_track->get_uuid (), audio_group1->get_uuid ());
  track_routing_->add_or_replace_route (
    audio_group1->get_uuid (), audio_group2->get_uuid ());

  // Now Group2 cannot route to Audio (would create cycle: Audio -> Group1 ->
  // Group2 -> Audio)
  EXPECT_FALSE (
    track_routing_->canRouteTo (audio_group2.get (), audio_track.get ()));

  // Group2 cannot route to Group1 (would create cycle: Group1 -> Group2 ->
  // Group1)
  EXPECT_FALSE (
    track_routing_->canRouteTo (audio_group2.get (), audio_group1.get ()));

  // But Audio can still route to Group2 (no cycle - just a different path)
  EXPECT_TRUE (
    track_routing_->canRouteTo (audio_track.get (), audio_group2.get ()));
}

} // namespace zrythm::structure::tracks
