// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <array>
#include <vector>

#include "dsp/midi_event.h"
#include "dsp/poly_voice_manager.h"

#include <gtest/gtest.h>

namespace zrythm::dsp
{

/** Records calls for verification. Vectors are pre-reserved so recording is
 * allocation-free in RT context. */
class MockVoice : public SynthVoice
{
public:
  MockVoice () { render_calls_.reserve (64); }

  void note_on (
    int           channel,
    int           pitch,
    float         velocity,
    std::uint32_t note_sequence) noexcept override
  {
    SynthVoice::note_on (channel, pitch, velocity, note_sequence);
    ++start_count_;
    last_pitch_ = pitch;
    last_velocity_ = velocity;
  }

  void note_off () noexcept override
  {
    // Keep active (simulates an envelope voice with a release tail)
    ++note_off_count_;
  }

  void cut () noexcept override
  {
    SynthVoice::cut ();
    ++cut_count_;
  }

  void pitch_bend (int value) noexcept override { last_pitch_bend_ = value; }

  void render (
    juce::AudioBuffer<float> &output,
    int                       start_sample,
    int                       num_samples) noexcept override
  {
    render_calls_.push_back ({ start_sample, num_samples });
  }

  int   start_count_{};
  int   note_off_count_{};
  int   cut_count_{};
  int   last_pitch_{ -1 };
  float last_velocity_{};
  int   last_pitch_bend_{ -1 };

  std::vector<std::pair<int, int>> render_calls_;
};

class PolyVoiceManagerTest : public ::testing::Test
{
protected:
  MockVoice * add_mock_voice ()
  {
    auto   voice = std::make_unique<MockVoice> ();
    auto * ptr = voice.get ();
    manager_.add_voice (std::move (voice));
    return ptr;
  }

  static MidiEventBuffer make_buffer ()
  {
    MidiEventBuffer buf;
    buf.reserve (4096);
    return buf;
  }

  template <typename TimeType>
  static void push_event (MidiEventBuffer &buf, const MidiEvent<TimeType> &ev)
  {
    buf.push_back (ev.time_, ev.data ());
  }

  PolyVoiceManager         manager_;
  juce::AudioBuffer<float> output_{ 2, 512 };
};

TEST_F (PolyVoiceManagerTest, NoteOnAllocatesFreeVoicesInOrder)
{
  auto * v0 = add_mock_voice ();
  auto * v1 = add_mock_voice ();

  auto buf = make_buffer ();
  push_event (buf, midi_event::make_note_on (0, 60, 100, units::samples (0u)));
  push_event (buf, midi_event::make_note_on (0, 62, 100, units::samples (1u)));
  manager_.process (output_, buf, units::samples (0u), units::samples (256u));

  EXPECT_TRUE (v0->is_active ());
  EXPECT_EQ (v0->current_note (), 60);
  EXPECT_TRUE (v1->is_active ());
  EXPECT_EQ (v1->current_note (), 62);
  EXPECT_FLOAT_EQ (v0->last_velocity_, 100.f / 127.f);
}

TEST_F (PolyVoiceManagerTest, StealsOldestVoiceWhenFull)
{
  auto * v0 = add_mock_voice ();
  auto * v1 = add_mock_voice ();

  auto buf = make_buffer ();
  push_event (buf, midi_event::make_note_on (0, 60, 100, units::samples (0u)));
  push_event (buf, midi_event::make_note_on (0, 62, 100, units::samples (1u)));
  push_event (buf, midi_event::make_note_on (0, 64, 100, units::samples (2u)));
  manager_.process (output_, buf, units::samples (0u), units::samples (256u));

  // v0 was the oldest - stolen for the third note
  EXPECT_EQ (v0->current_note (), 64);
  EXPECT_EQ (v0->start_count_, 2);
  EXPECT_EQ (v0->cut_count_, 1);
  EXPECT_EQ (v1->current_note (), 62);
  EXPECT_EQ (v1->start_count_, 1);
}

TEST_F (PolyVoiceManagerTest, NoteOffMatchesPitchAndChannel)
{
  auto * v0 = add_mock_voice ();
  auto * v1 = add_mock_voice ();

  auto buf = make_buffer ();
  // Same pitch on two channels
  push_event (buf, midi_event::make_note_on (0, 60, 100, units::samples (0u)));
  push_event (buf, midi_event::make_note_on (1, 60, 100, units::samples (1u)));
  manager_.process (output_, buf, units::samples (0u), units::samples (256u));
  ASSERT_TRUE (v0->is_active ());
  ASSERT_TRUE (v1->is_active ());

  // Note-off on channel 0 only releases the channel-0 voice
  auto buf2 = make_buffer ();
  push_event (
    buf2,
    midi_event::make_note_off_with_default_velocity (0, 60, units::samples (0u)));
  manager_.process (output_, buf2, units::samples (0u), units::samples (256u));

  EXPECT_EQ (v0->note_off_count_, 1);
  // Tail-off is the voice's choice: the mock stays active after note_off
  EXPECT_TRUE (v0->is_active ());
  EXPECT_EQ (v1->note_off_count_, 0);
}

TEST_F (PolyVoiceManagerTest, NoteOnWithZeroVelocityActsAsNoteOff)
{
  auto * v0 = add_mock_voice ();

  auto buf = make_buffer ();
  push_event (buf, midi_event::make_note_on (0, 60, 100, units::samples (0u)));
  manager_.process (output_, buf, units::samples (0u), units::samples (256u));
  ASSERT_TRUE (v0->is_active ());

  // A note-on with velocity 0 is a note-off (construct raw - make_note_on
  // asserts velocity != 0)
  auto                             buf2 = make_buffer ();
  const std::array<midi_byte_t, 3> note_on_zero_vel = { 0x90, 60, 0 };
  buf2.push_back (units::samples (0u), note_on_zero_vel);
  manager_.process (output_, buf2, units::samples (0u), units::samples (256u));
  EXPECT_EQ (v0->note_off_count_, 1);
}

TEST_F (PolyVoiceManagerTest, RendersSampleAccuratelyAroundEvents)
{
  auto * v0 = add_mock_voice ();

  auto buf = make_buffer ();
  push_event (buf, midi_event::make_note_on (0, 60, 100, units::samples (64u)));
  push_event (buf, midi_event::make_pitchbend (0, 4000, units::samples (128u)));
  manager_.process (output_, buf, units::samples (0u), units::samples (256u));

  // Render starts at the note-on, then splits at the pitch bend
  ASSERT_EQ (v0->render_calls_.size (), 2u);
  EXPECT_EQ (v0->render_calls_[0], (std::pair<int, int>{ 64, 64 }));
  EXPECT_EQ (v0->render_calls_[1], (std::pair<int, int>{ 128, 128 }));
}

TEST_F (PolyVoiceManagerTest, EventsOutsideBlockAreIgnored)
{
  auto * v0 = add_mock_voice ();

  auto buf = make_buffer ();
  // Event before the block start
  push_event (buf, midi_event::make_note_on (0, 60, 100, units::samples (100u)));
  manager_.process (output_, buf, units::samples (512u), units::samples (256u));

  EXPECT_FALSE (v0->is_active ());
  EXPECT_TRUE (v0->render_calls_.empty ());
}

TEST_F (PolyVoiceManagerTest, PitchWheelBroadcastAndTrackedForNewNotes)
{
  auto * v0 = add_mock_voice ();
  auto * v1 = add_mock_voice ();

  auto buf = make_buffer ();
  push_event (buf, midi_event::make_note_on (0, 60, 100, units::samples (0u)));
  push_event (buf, midi_event::make_pitchbend (0, 4000, units::samples (64u)));
  manager_.process (output_, buf, units::samples (0u), units::samples (256u));

  // Active voice received the bend
  EXPECT_EQ (v0->last_pitch_bend_, 4000);
  EXPECT_EQ (v1->last_pitch_bend_, 4000);

  // A note started later inherits the channel's current bend
  auto buf2 = make_buffer ();
  push_event (buf2, midi_event::make_note_on (0, 62, 100, units::samples (0u)));
  manager_.process (output_, buf2, units::samples (0u), units::samples (256u));
  EXPECT_EQ (v1->last_pitch_bend_, 4000);
}

TEST_F (PolyVoiceManagerTest, AllNotesOffDeactivatesImmediately)
{
  auto * v0 = add_mock_voice ();
  auto * v1 = add_mock_voice ();

  auto buf = make_buffer ();
  push_event (buf, midi_event::make_note_on (0, 60, 100, units::samples (0u)));
  push_event (buf, midi_event::make_note_on (0, 62, 100, units::samples (1u)));
  manager_.process (output_, buf, units::samples (0u), units::samples (256u));
  ASSERT_TRUE (v0->is_active ());
  ASSERT_TRUE (v1->is_active ());

  manager_.all_notes_off ();
  EXPECT_FALSE (v0->is_active ());
  EXPECT_FALSE (v1->is_active ());
  EXPECT_EQ (v0->cut_count_, 1);
  EXPECT_EQ (v1->cut_count_, 1);
}

TEST_F (PolyVoiceManagerTest, AllNotesOffCcReleasesChannelVoices)
{
  auto * v0 = add_mock_voice ();
  auto * v1 = add_mock_voice ();

  auto buf = make_buffer ();
  push_event (buf, midi_event::make_note_on (0, 60, 100, units::samples (0u)));
  push_event (buf, midi_event::make_note_on (1, 62, 100, units::samples (1u)));
  manager_.process (output_, buf, units::samples (0u), units::samples (256u));
  ASSERT_TRUE (v0->is_active ());
  ASSERT_TRUE (v1->is_active ());

  // CC 123 on channel 0 releases only the channel-0 voice
  auto buf2 = make_buffer ();
  push_event (buf2, midi_event::make_all_notes_off (0, units::samples (0u)));
  manager_.process (output_, buf2, units::samples (0u), units::samples (256u));

  EXPECT_EQ (v0->note_off_count_, 1);
  EXPECT_EQ (v0->cut_count_, 0);
  // Tail-off is the voice's choice: the mock stays active after note_off
  EXPECT_TRUE (v0->is_active ());
  EXPECT_EQ (v1->note_off_count_, 0);
  EXPECT_TRUE (v1->is_active ());
}

TEST_F (PolyVoiceManagerTest, AllSoundOffCcCutsChannelVoices)
{
  auto * v0 = add_mock_voice ();
  auto * v1 = add_mock_voice ();

  auto buf = make_buffer ();
  push_event (buf, midi_event::make_note_on (0, 60, 100, units::samples (0u)));
  push_event (buf, midi_event::make_note_on (1, 62, 100, units::samples (1u)));
  manager_.process (output_, buf, units::samples (0u), units::samples (256u));
  ASSERT_TRUE (v0->is_active ());
  ASSERT_TRUE (v1->is_active ());

  // CC 120 on channel 0 cuts only the channel-0 voice
  auto                             buf2 = make_buffer ();
  const std::array<midi_byte_t, 3> all_sound_off = { 0xb0, 120, 0 };
  buf2.push_back (units::samples (0u), all_sound_off);
  manager_.process (output_, buf2, units::samples (0u), units::samples (256u));

  EXPECT_EQ (v0->cut_count_, 1);
  EXPECT_FALSE (v0->is_active ());
  EXPECT_EQ (v1->cut_count_, 0);
  EXPECT_TRUE (v1->is_active ());
}

} // namespace zrythm::dsp
