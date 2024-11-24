#include "utils/gtest_wrapper.h"
#include "utils/midi.h"

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
  midi_byte_t note_msg[3] = { MIDI_CH1_NOTE_ON, 60, 100 }; // Middle C
  char        note_name[128];
  midi_get_note_name_with_octave (note_msg, note_name);
  EXPECT_STREQ (note_name, "C3");
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
  EXPECT_EQ (midi_get_14_bit_value (buf.data ()), 12280);
}
