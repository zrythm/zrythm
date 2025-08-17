// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/tracks/piano_roll_track.h"

#include <QObject>
#include <QSignalSpy>

#include <gtest/gtest.h>

namespace zrythm::structure::tracks
{

class PianoRollTrackTest : public ::testing::Test
{
protected:
  void SetUp () override { track_ = std::make_unique<PianoRollTrackMixin> (); }

  void TearDown () override { track_.reset (); }

  std::unique_ptr<PianoRollTrackMixin> track_;
};

TEST_F (PianoRollTrackTest, ConstructionAndInitialState)
{
  EXPECT_NE (track_, nullptr);
  EXPECT_EQ (track_->midiChannel (), 1);
  EXPECT_FALSE (track_->drumMode ());
  EXPECT_FALSE (track_->passthroughMidiInput ());
}

TEST_F (PianoRollTrackTest, MidiChannelProperty)
{
  // Test getter/setter
  track_->setMidiChannel (5);
  EXPECT_EQ (track_->midiChannel (), 5);

  // Test signal emission
  QSignalSpy spy (track_.get (), &PianoRollTrackMixin::midiChannelChanged);
  track_->setMidiChannel (10);
  EXPECT_EQ (spy.count (), 1);
  EXPECT_EQ (spy.takeFirst ().at (0).toUInt (), 10u);

  // Test no signal on same value
  spy.clear ();
  track_->setMidiChannel (10);
  EXPECT_EQ (spy.count (), 0);

  // Test edge cases
  track_->setMidiChannel (0);
  EXPECT_EQ (track_->midiChannel (), 1);
  track_->setMidiChannel (255);
  EXPECT_EQ (track_->midiChannel (), 16);
}

TEST_F (PianoRollTrackTest, DrumModeProperty)
{
  // Test getter/setter
  track_->setDrumMode (true);
  EXPECT_TRUE (track_->drumMode ());

  // Test signal emission
  QSignalSpy spy (track_.get (), &PianoRollTrackMixin::drumModeChanged);
  track_->setDrumMode (false);
  EXPECT_EQ (spy.count (), 1);
  EXPECT_FALSE (spy.takeFirst ().at (0).toBool ());

  // Test no signal on same value
  spy.clear ();
  track_->setDrumMode (false);
  EXPECT_EQ (spy.count (), 0);
}

TEST_F (PianoRollTrackTest, PassthroughMidiInputProperty)
{
  // Test getter/setter
  track_->setPassthroughMidiInput (true);
  EXPECT_TRUE (track_->passthroughMidiInput ());

  // Test signal emission
  QSignalSpy spy (
    track_.get (), &PianoRollTrackMixin::passthroughMidiInputChanged);
  track_->setPassthroughMidiInput (false);
  EXPECT_EQ (spy.count (), 1);
  EXPECT_FALSE (spy.takeFirst ().at (0).toBool ());

  // Test no signal on same value
  spy.clear ();
  track_->setPassthroughMidiInput (false);
  EXPECT_EQ (spy.count (), 0);
}

TEST_F (PianoRollTrackTest, QmlProperties)
{
  // Test QML property access
  EXPECT_EQ (track_->property ("midiChannel").toUInt (), 1u);
  EXPECT_EQ (track_->property ("drumMode").toBool (), false);
  EXPECT_EQ (track_->property ("passthroughMidiInput").toBool (), false);

  // Test setting via QML properties
  track_->setProperty ("midiChannel", 7);
  track_->setProperty ("drumMode", true);
  track_->setProperty ("passthroughMidiInput", true);

  EXPECT_EQ (track_->midiChannel (), 7);
  EXPECT_TRUE (track_->drumMode ());
  EXPECT_TRUE (track_->passthroughMidiInput ());
}

TEST_F (PianoRollTrackTest, MultiplePropertiesChangedSignals)
{
  QSignalSpy midi_spy (track_.get (), &PianoRollTrackMixin::midiChannelChanged);
  QSignalSpy drum_spy (track_.get (), &PianoRollTrackMixin::drumModeChanged);
  QSignalSpy passthrough_spy (
    track_.get (), &PianoRollTrackMixin::passthroughMidiInputChanged);

  // Change multiple properties
  track_->setMidiChannel (5);
  track_->setDrumMode (true);
  track_->setPassthroughMidiInput (true);

  EXPECT_EQ (midi_spy.count (), 1);
  EXPECT_EQ (drum_spy.count (), 1);
  EXPECT_EQ (passthrough_spy.count (), 1);

  // Change to same values - no signals
  midi_spy.clear ();
  drum_spy.clear ();
  passthrough_spy.clear ();

  track_->setMidiChannel (5);
  track_->setDrumMode (true);
  track_->setPassthroughMidiInput (true);

  EXPECT_EQ (midi_spy.count (), 0);
  EXPECT_EQ (drum_spy.count (), 0);
  EXPECT_EQ (passthrough_spy.count (), 0);
}

TEST_F (PianoRollTrackTest, JsonSerializationRoundtrip)
{
  // Set all publicly accessible properties
  track_->setMidiChannel (12);
  track_->setDrumMode (true);
  track_->setPassthroughMidiInput (true);

  // Serialize to JSON
  nlohmann::json j = *track_;

  // Create new track from JSON
  auto deserialized_track = std::make_unique<PianoRollTrackMixin> ();
  from_json (j, *deserialized_track);

  // Verify all publicly accessible properties
  EXPECT_EQ (deserialized_track->midiChannel (), 12);
  EXPECT_TRUE (deserialized_track->drumMode ());
  EXPECT_TRUE (deserialized_track->passthroughMidiInput ());
}

TEST_F (PianoRollTrackTest, JsonSerializationDefaultState)
{
  nlohmann::json j = *track_;

  // Create new track from JSON
  auto deserialized_track = std::make_unique<PianoRollTrackMixin> ();
  from_json (j, *deserialized_track);

  // Verify default state
  EXPECT_EQ (deserialized_track->midiChannel (), 1);
  EXPECT_FALSE (deserialized_track->drumMode ());
  EXPECT_FALSE (deserialized_track->passthroughMidiInput ());
}

TEST_F (PianoRollTrackTest, TransformMidiInputsFunc)
{
  dsp::MidiEventVector events;

  // Add some MIDI events with different channels
  events.add_simple (0x90, 60, 100, 0); // Note on, channel 1
  events.add_simple (0x91, 61, 100, 1); // Note on, channel 2
  events.add_simple (0x92, 62, 100, 2); // Note on, channel 3

  // Test with passthrough disabled (default) - should change all channels to
  // track's MIDI channel
  track_->setMidiChannel (5);
  track_->setPassthroughMidiInput (false);
  track_->transform_midi_inputs_func (events);

  // All events should now have channel 5
  EXPECT_EQ (events.size (), 3);
  EXPECT_EQ (events.at (0).raw_buffer_[0], 0x94); // 0x90 | 0x04 (channel 5)
  EXPECT_EQ (events.at (1).raw_buffer_[0], 0x94); // 0x90 | 0x04 (channel 5)
  EXPECT_EQ (events.at (2).raw_buffer_[0], 0x94); // 0x90 | 0x04 (channel 5)

  // Test with passthrough enabled - should leave channels unchanged
  events.clear ();
  events.add_simple (0x90, 60, 100, 0); // Note on, channel 1
  events.add_simple (0x91, 61, 100, 1); // Note on, channel 2
  events.add_simple (0x92, 62, 100, 2); // Note on, channel 3

  track_->setPassthroughMidiInput (true);
  track_->transform_midi_inputs_func (events);

  // Channels should remain unchanged
  EXPECT_EQ (events.size (), 3);
  EXPECT_EQ (events.at (0).raw_buffer_[0], 0x90); // Channel 1
  EXPECT_EQ (events.at (1).raw_buffer_[0], 0x91); // Channel 2
  EXPECT_EQ (events.at (2).raw_buffer_[0], 0x92); // Channel 3

  // Test with different track MIDI channel
  events.clear ();
  events.add_simple (0x90, 60, 100, 0); // Note on, channel 1

  track_->setMidiChannel (12);
  track_->setPassthroughMidiInput (false);
  track_->transform_midi_inputs_func (events);

  // Event should now have channel 12
  EXPECT_EQ (events.at (0).raw_buffer_[0], 0x9B); // 0x90 | 0x0B (channel 12)
}
} // namespace zrythm::structure::tracks
