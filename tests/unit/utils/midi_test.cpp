// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "utils/gtest_wrapper.h"
#include "utils/midi.h"

using namespace zrythm::utils::midi;

TEST (MidiTest, NoteConversion)
{
  // Test frequency to note number
  float   freq = 440.0f; // A4
  uint8_t note = midi_frequency_to_note_number (freq);
  EXPECT_EQ (note, 69);

  // Test note number to frequency
  float freq_back = midi_note_number_to_frequency (note);
  EXPECT_NEAR (freq_back, freq, 0.01f);
}

TEST (MidiTest, MessageTypes)
{
  midi_byte_t note_on[3] = { MIDI_CH1_NOTE_ON, 60, 100 };
  EXPECT_TRUE (midi_is_note_on (note_on));
  EXPECT_FALSE (midi_is_note_off (note_on));
  EXPECT_EQ (midi_get_note_number (note_on), 60);
  EXPECT_EQ (midi_get_velocity (note_on), 100);

  midi_byte_t note_off[3] = { MIDI_CH1_NOTE_OFF, 60, 0 };
  EXPECT_TRUE (midi_is_note_off (note_off));
  EXPECT_FALSE (midi_is_note_on (note_off));
}

TEST (MidiTest, ControllerMessages)
{
  midi_byte_t cc[3] = { MIDI_CH1_CTRL_CHANGE, 7, 100 }; // Volume
  EXPECT_TRUE (midi_is_controller (cc));
  EXPECT_EQ (midi_get_controller_number (cc), 7);
  EXPECT_EQ (midi_get_controller_value (cc), 100);
}

TEST (MidiTest, PitchWheel)
{
  midi_byte_t pitch[3] = { MIDI_CH1_PITCH_WHEEL_RANGE, 0, 64 };
  EXPECT_TRUE (midi_is_pitch_wheel (pitch));
  EXPECT_EQ (midi_get_pitchwheel_value (pitch), 8192); // Center position
}

TEST (MidiTest, ChannelInfo)
{
  midi_byte_t msg[3] = { MIDI_CH1_NOTE_ON, 60, 100 };
  EXPECT_EQ (midi_get_channel_0_to_15 (msg), 0);
  EXPECT_EQ (midi_get_channel_1_to_16 (msg), 1);
}

TEST (MidiTest, SystemMessages)
{
  midi_byte_t clock[3] = { MIDI_CLOCK_START, 0, 0 };
  EXPECT_TRUE (midi_is_start (clock));

  midi_byte_t stop[3] = { MIDI_CLOCK_STOP, 0, 0 };
  EXPECT_TRUE (midi_is_stop (stop));

  midi_byte_t continue_msg[3] = { MIDI_CLOCK_CONTINUE, 0, 0 };
  EXPECT_TRUE (midi_is_continue (continue_msg));
}

TEST (MidiTest, MessageLength)
{
  EXPECT_EQ (midi_get_msg_length (MIDI_CH1_NOTE_ON), 3);
  EXPECT_EQ (midi_get_msg_length (MIDI_CH1_PROG_CHANGE), 2);
  EXPECT_EQ (midi_get_msg_length (MIDI_CLOCK_START), 1);
}

TEST (MidiTest, NoteNames)
{
  std::array<midi_byte_t, 3> note_msg{ MIDI_CH1_NOTE_ON, 60, 100 }; // Middle C
  const auto note_name = midi_get_note_name_with_octave (note_msg);
  EXPECT_EQ (note_name, "C3");
}

TEST (MidiTest, CombinedValues)
{
  midi_byte_t lsb, msb;
  uint32_t    combined = 8192; // Center position for pitch wheel
  midi_get_bytes_from_combined (combined, &lsb, &msb);
  EXPECT_EQ (lsb, 0);
  EXPECT_EQ (msb, 64);

  midi_get_bytes_from_combined (12280, &lsb, &msb);
  EXPECT_EQ (lsb, 120);
  EXPECT_EQ (msb, 95);

  std::array<midi_byte_t, 3> buf{ MIDI_CH1_PITCH_WHEEL_RANGE, 120, 95 };
  EXPECT_EQ (midi_get_14_bit_value (buf), 12280);
}

TEST (MidiTest, MetaEventDetection)
{
  std::vector<midi_byte_t> meta{ 0xFF, 0x01, 0x03, 'a', 'b', 'c' };
  EXPECT_TRUE (midi_is_meta_event (meta));
  EXPECT_FALSE (midi_is_short_msg (meta));

  std::vector<midi_byte_t> sysex{ 0xF0, 0x01, 0x02 };
  EXPECT_TRUE (midi_is_sysex (sysex));

  std::array<midi_byte_t, 3> note_on{ 0x90, 0x40, 0x64 };
  EXPECT_FALSE (midi_is_meta_event (note_on));
  EXPECT_TRUE (midi_is_short_msg (note_on));

  std::vector<midi_byte_t> too_short_1{ 0xFF };
  EXPECT_FALSE (midi_is_meta_event (too_short_1));

  std::vector<midi_byte_t> too_short_2{ 0xFF, 0x01 };
  EXPECT_FALSE (midi_is_meta_event (too_short_2));
}

TEST (MidiTest, MetaEventTypeChecks)
{
  std::vector<midi_byte_t> set_tempo{ 0xFF, 0x51, 0x03, 0x07, 0xA1, 0x20 };
  EXPECT_TRUE (midi_is_meta_event_of_type (set_tempo, 0x51));
  EXPECT_FALSE (midi_is_meta_event_of_type (set_tempo, 0x58));
  EXPECT_EQ (midi_get_meta_event_type (set_tempo), 0x51);

  std::vector<midi_byte_t> end_of_track{ 0xFF, 0x2F, 0x00 };
  EXPECT_EQ (midi_get_meta_event_type (end_of_track), 0x2F);
}

TEST (MidiTest, MetaEventDataSimpleSingleByteLength)
{
  std::vector<midi_byte_t> msg{ 0xFF, 0x01, 0x03, 'A', 'B', 'C' };
  const midi_byte_t * data = nullptr;
  size_t len = midi_get_meta_event_data (&data, msg.data (), msg.size ());
  EXPECT_EQ (len, 3u);
  ASSERT_NE (data, nullptr);
  EXPECT_EQ (data[0], 'A');
  EXPECT_EQ (data[1], 'B');
  EXPECT_EQ (data[2], 'C');
}

TEST (MidiTest, MetaEventDataZeroLength)
{
  std::vector<midi_byte_t> msg{ 0xFF, 0x2F, 0x00 };
  const midi_byte_t * data = nullptr;
  size_t              len = midi_get_meta_event_data (&data, msg.data (), msg.size ());
  EXPECT_EQ (len, 0u);
}

TEST (MidiTest, MetaEventDataSetTempo)
{
  std::vector<midi_byte_t> msg{ 0xFF, 0x51, 0x03, 0x07, 0xA1, 0x20 };
  const midi_byte_t * data = nullptr;
  size_t              len = midi_get_meta_event_data (&data, msg.data (), msg.size ());
  EXPECT_EQ (len, 3u);
  ASSERT_NE (data, nullptr);
  EXPECT_EQ (data[0], 0x07);
  EXPECT_EQ (data[1], 0xA1);
  EXPECT_EQ (data[2], 0x20);
}

TEST (MidiTest, MetaEventDataTimeSignature)
{
  std::vector<midi_byte_t> msg{ 0xFF, 0x58, 0x04, 0x04, 0x02, 0x18, 0x08 };
  const midi_byte_t * data = nullptr;
  size_t              len = midi_get_meta_event_data (&data, msg.data (), msg.size ());
  EXPECT_EQ (len, 4u);
  ASSERT_NE (data, nullptr);
  EXPECT_EQ (data[0], 0x04);
  EXPECT_EQ (data[1], 0x02);
}

TEST (MidiTest, MetaEventDataKeySignature)
{
  std::vector<midi_byte_t> msg{ 0xFF, 0x59, 0x02, 0x00, 0x00 };
  const midi_byte_t * data = nullptr;
  size_t              len = midi_get_meta_event_data (&data, msg.data (), msg.size ());
  EXPECT_EQ (len, 2u);
  ASSERT_NE (data, nullptr);
  EXPECT_EQ (data[0], 0x00);
  EXPECT_EQ (data[1], 0x00);
}

TEST (MidiTest, MetaEventDataTwoByteVLQ)
{
  std::vector<midi_byte_t> msg (4 + 128, 0);
  msg[0] = 0xFF;
  msg[1] = 0x01;
  msg[2] = 0x81;
  msg[3] = 0x00;
  const midi_byte_t * data = nullptr;
  size_t len = midi_get_meta_event_data (&data, msg.data (), msg.size ());
  EXPECT_EQ (len, 128u);
  ASSERT_NE (data, nullptr);
  EXPECT_EQ (data, &msg[4]);
}

TEST (MidiTest, MetaEventDataThreeByteVLQ)
{
  const size_t        content_len = 16384;
  std::vector<midi_byte_t> msg (5 + content_len, 0);
  msg[0] = 0xFF;
  msg[1] = 0x01;
  msg[2] = 0x81;
  msg[3] = 0x80;
  msg[4] = 0x00;
  const midi_byte_t * data = nullptr;
  size_t              len = midi_get_meta_event_data (&data, msg.data (), msg.size ());
  EXPECT_EQ (len, content_len);
  ASSERT_NE (data, nullptr);
  EXPECT_EQ (data, &msg[5]);
}

TEST (MidiTest, MetaEventDataMalformedTooShort)
{
  std::vector<midi_byte_t> msg{ 0xFF, 0x01, 0x03 };
  const midi_byte_t * data = nullptr;
  size_t              len = midi_get_meta_event_data (&data, msg.data (), msg.size ());
  EXPECT_EQ (len, 0u);
}

TEST (MidiTest, MetaEventDataMalformedVLQNoTerminator)
{
  std::vector<midi_byte_t> msg{ 0xFF, 0x01, 0x80, 0x80, 0x80 };
  const midi_byte_t * data = nullptr;
  size_t              len = midi_get_meta_event_data (&data, msg.data (), msg.size ());
  EXPECT_EQ (len, 0u);
}

TEST (MidiTest, MetaEventDataMalformedVLQExceedsMsgSize)
{
  std::vector<midi_byte_t> msg{ 0xFF, 0x01, 0x80 };
  const midi_byte_t * data = nullptr;
  size_t              len = midi_get_meta_event_data (&data, msg.data (), msg.size ());
  EXPECT_EQ (len, 0u);
}

TEST (MidiTest, MetaEventDataMalformedContentTruncated)
{
  std::vector<midi_byte_t> msg{ 0xFF, 0x01, 0x0A, 'a', 'b' };
  const midi_byte_t * data = nullptr;
  size_t              len = midi_get_meta_event_data (&data, msg.data (), msg.size ());
  EXPECT_EQ (len, 0u);
}

TEST (MidiTest, MetaEventTypeNameLookup)
{
  EXPECT_EQ (midi_get_meta_event_type_name (0x00), "Sequence number");
  EXPECT_EQ (midi_get_meta_event_type_name (0x01), "Text");
  EXPECT_EQ (midi_get_meta_event_type_name (0x03), "Track name");
  EXPECT_EQ (midi_get_meta_event_type_name (0x2F), "End of track");
  EXPECT_EQ (midi_get_meta_event_type_name (0x51), "Set tempo");
  EXPECT_EQ (midi_get_meta_event_type_name (0x58), "Time signature");
  EXPECT_EQ (midi_get_meta_event_type_name (0x59), "Key signature");
  EXPECT_EQ (midi_get_meta_event_type_name (0x7F), "Sequencer specific");
}
