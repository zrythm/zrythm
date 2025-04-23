// clang-format off
// SPDX-FileCopyrightText: © 2018-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
/*
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 * ---
 *
 * CHOC is (C)2021 Tracktion Corporation, and is offered under the terms of the ISC license:
 *
 * Permission to use, copy, modify, and/or distribute this software for any purpose with or
 * without fee is hereby granted, provided that the above copyright notice and this permission
 * notice appear in all copies. THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * ---
 */
// clang-format on

#include <cinttypes>
#include <cmath>
#include <cstdlib>

#include "utils/midi.h"

/* https://www.midi.org/specifications-old/item/table-3-control-change-messages-data-bytes-2
 */
static const char * midi_cc_names[128] = {
  "Bank Select",
  "Modulation Wheel (MSB)",
  "Breath Controller (MSB)",
  "Undefined 3",
  "Foot Pedal (MSB)",
  "Portamento Time (MSB)",
  "Data Entry MSB",
  "Channel Volume (MSB)",
  "Balance (MSB)",
  "Undefined 9",
  "Pan Position (MSB)",
  "Expression (MSB)",
  "Effect Control 1 (MSB)",
  "Effect Control 2 (MSB)",
  "Undefined 14",
  "Undefined 15",
  "General Purpose Slider 1",
  "General Purpose Slider 2",
  "General Purpose Slider 3",
  "General Purpose Slider 4",
  "Undefined 20",
  "Undefined 21",
  "Undefined 22",
  "Undefined 23",
  "Undefined 24",
  "Undefined 25",
  "Undefined 26",
  "Undefined 27",
  "Undefined 28",
  "Undefined 29",
  "Undefined 30",
  "Undefined 31",
  "Bank Select (LSB)",
  "Modulation Wheel (LSB)",
  "Breath Controller (LSB)",
  "Undefined 35",
  "Foot Pedal (LSB)",
  "Portamento/Glide Time (LSB)",
  "Data Entry (LSB)",
  "Volume (LSB)",
  "Balance (LSB)",
  "Undefined 41",
  "Pan Position (LSB)",
  "Expression (LSB)",
  "Effect Control 1 (LSB)",
  "Effect Control 2 (LSB)",
  "Undefined 46",
  "Undefined 47",
  "Undefined 48",
  "Undefined 49",
  "Undefined 50",
  "Undefined 51",
  "Undefined 52",
  "Undefined 53",
  "Undefined 54",
  "Undefined 55",
  "Undefined 56",
  "Undefined 57",
  "Undefined 58",
  "Undefined 59",
  "Undefined 60",
  "Undefined 61",
  "Undefined 62",
  "Undefined 63",
  "Sustain Pedal On/Off",
  "Portamento/Glide On/Off Switch",
  "Sostenuto On/Off Switch",
  "Soft Pedal On/Off Switch",
  "Legato On/Off Switch",
  "Hold Pedal 2",
  "Sound Controller 1",
  "Sound Controller 2 - Filter Resonance",
  "Sound Controller 3 - Amp Envelope Decay",
  "Sound Controller 4 - Amp Envelope Attack",
  "Sound Controller 5 - Filter Cutoff",
  "Sound Controller 6",
  "Sound Controller 7",
  "Sound Controller 8",
  "Sound Controller 9",
  "Sound Controller 10",
  "General Purpose",
  "General Purpose",
  "General Purpose",
  "General Purpose",
  "Undefined 84",
  "Undefined 85",
  "Undefined 86",
  "Undefined 87",
  "Undefined 88",
  "Undefined 89",
  "Undefined 90",
  "Effect 1 Amount (Reverb)",
  "Effect 2 Amount (Tremolo)",
  "Effect 3 Amount (Chorus)",
  "Effect 4 Amount (Detuning)",
  "Effect 5 Amount (Phaser)",
  "Data Bound Increment (+1)",
  "Data Bound Decrement (-1)",
  "NRPN LSB",
  "NRPN MSB",
  "RPN LSB",
  "RPN MSB",
  "Undefined 102",
  "Undefined 103",
  "Undefined 104",
  "Undefined 105",
  "Undefined 106",
  "Undefined 107",
  "Undefined 108",
  "Undefined 109",
  "Undefined 110",
  "Undefined 111",
  "Undefined 112",
  "Undefined 113",
  "Undefined 114",
  "Undefined 115",
  "Undefined 116",
  "Undefined 117",
  "Undefined 118",
  "Undefined 119",
  "Channel Mute / Sound Off",
  "Reset All Controllers",
  "Local Keyboard On/Off Switch",
  "All MIDI Notes OFF",
  "OMNI Mode OFF",
  "OMNI Mode ON",
  "Mono Mode",
  "Poly Mode",
};

static const char * note_labels[12] = {
  "C",       "D\u266D", "D",       "E\u266D", "E",       "F",
  "F\u266F", "G",       "A\u266D", "A",       "B\u266D", "B"
};

namespace zrythm::utils::midi
{

/**
 * Return the name of the given cc (0-127).
 */
std::string_view
midi_get_controller_name (const midi_byte_t cc)
{
  return midi_cc_names[cc];
}

std::optional<int>
midi_ctrl_change_get_channel (std::span<const midi_byte_t> ctrl_change)
{
  /* assert the given event is a ctrl change event */
  if (ctrl_change[0] < 0xB0 || ctrl_change[0] > 0xBF)
    {
      return std::nullopt;
    }
  return (ctrl_change[0] - 0xB0) + 1;
}

std::optional<std::string>
midi_ctrl_change_get_description (std::span<const midi_byte_t> ctrl_change)
{
  /* FIXME is there a reason this is not using
   * get_controller_name() ? */

  /* assert the given event is a ctrl change event */
  if (ctrl_change[0] < 0xB0 || ctrl_change[0] > 0xBF)
    {
      return std::nullopt;
    }

  if (ctrl_change[1] >= 0x08 && ctrl_change[1] <= 0x1F)
    {
      return fmt::format ("Continuous controller #{}", ctrl_change[1]);
    }
  if (ctrl_change[1] >= 0x28 && ctrl_change[1] <= 0x3F)
    {
      return fmt::format ("Continuous controller #{}", ctrl_change[1] - 0x28);
    }

  switch (ctrl_change[1])
    {
    case 0x00:
    case 0x20:
      return std::string ("Continuous controller #0");
      break;
    case 0x01:
    case 0x21:
      return std::string ("Modulation wheel");
      break;
    case 0x02:
    case 0x22:
      return std::string ("Breath control");
      break;
    case 0x03:
    case 0x23:
      return std::string ("Continuous controller #3");
      break;
    case 0x04:
    case 0x24:
      return std::string ("Foot controller");
      break;
    case 0x05:
    case 0x25:
      return std::string ("Portamento time");
      break;
    case 0x06:
    case 0x26:
      return std::string ("Data Entry");
      break;
    case 0x07:
    case 0x27:
      return std::string ("Main Volume");
      break;
    default:
      return std::string ("Unknown");
      break;
    }
}

/**
 * Used for MIDI controls whose values are split
 * between MSB/LSB.
 *
 * @param lsb First byte (pos 1).
 * @param msb Second byte (pos 2).
 */
void
midi_get_bytes_from_combined (
  const uint32_t val,
  midi_byte_t *  lsb,
  midi_byte_t *  msb)
{
  /* https://arduino.stackexchange.com/questions/18955/how-to-send-a-pitch-bend-midi-message-using-arcore
   */
  *lsb = val & 0x7F;
  *msb = (midi_byte_t) (val >> 7);
}

/**
 * Returns the length of the MIDI message based on
 * the status byte.
 *
 * TODO move this and other functions to utils/midi,
 * split separate files for MidiEvents and MidiEvent.
 */
int
midi_get_msg_length (const uint8_t status_byte)
{
  // define these with meaningful names
  switch (status_byte & 0xf0)
    {
    case 0x80:
    case 0x90:
    case 0xa0:
    case 0xb0:
    case 0xe0:
      return 3;
    case 0xc0:
    case 0xd0:
      return 2;
    case 0xf0:
      switch (status_byte)
        {
        case 0xf0:
          return 0;
        case 0xf1:
        case 0xf3:
          return 2;
        case 0xf2:
          return 3;
        case 0xf4:
        case 0xf5:
        case 0xf7:
        case 0xfd:
          break;
        default:
          return 1;
        }
    }
  z_return_val_if_reached (-1);
}

/**
 * Returns the note name (eg, "C") for a value
 * between 0 and 127.
 */
std::string_view
midi_get_note_name (const midi_byte_t note)
{
  return { note_labels[note % 12] };
}

std::string
midi_get_note_name_with_octave (std::span<const midi_byte_t> short_msg)
{
  midi_byte_t note = midi_get_note_number (short_msg);
  midi_byte_t note_idx = midi_get_chromatic_scale_index (note);
  midi_byte_t octave = midi_get_octave_number (note);
  return fmt::format ("{}{}", midi_get_note_name (note_idx), octave);
}

std::string
midi_get_meta_event_type_name (const midi_byte_t type)
{
  switch (type)
    {
    case 0x00:
      return { "Sequence number" };
      break;
    case 0x01:
      return { "Text" };
      break;
    case 0x02:
      return { "Copyright notice" };
      break;
    case 0x03:
      return { "Track name" };
      break;
    case 0x04:
      return { "Instrument name" };
      break;
    case 0x05:
      return { "Lyrics" };
      break;
    case 0x06:
      return { "Marker" };
      break;
    case 0x07:
      return { "Cue point" };
      break;
    case 0x20:
      return { "Channel prefix" };
      break;
    case 0x2F:
      return { "End of track" };
      break;
    case 0x51:
      return { "Set tempo" };
      break;
    case 0x54:
      return { "SMPTE offset" };
      break;
    case 0x58:
      return { "Time signature" };
      break;
    case 0x59:
      return { "Key signature" };
      break;
    case 0x7F:
      return { "Sequencer specific" };
      break;
    default:
      return fmt::format ("Unknown type {:02x}", type);
      break;
    }
}

std::string
midi_get_hex_str (std::span<const midi_byte_t> msg)
{
  std::string ret{};
  for (const auto &cur_byte : msg)
    {
      ret += fmt::format ("{:02x}", cur_byte);
      if (cur_byte != msg.back ())
        {
          ret += " ";
        }
    }
  return ret;
}

static std::string
midi_print_short_msg_to_str (std::span<const midi_byte_t> short_msg)
{
  midi_byte_t channel = midi_get_channel_1_to_16 (short_msg);

  if (midi_is_note_on (short_msg))
    {
      const auto note = midi_get_note_name_with_octave (short_msg);
      return fmt::format (
        "Note-On {}: Velocity: {}", note, midi_get_velocity (short_msg));
    }
  else if (midi_is_note_off (short_msg))
    {
      const auto note = midi_get_note_name_with_octave (short_msg);
      return fmt::format (
        "Note-Off {}: Velocity: {}", note, midi_get_velocity (short_msg));
    }
  else if (midi_is_aftertouch (short_msg))
    {
      const auto note = midi_get_note_name_with_octave (short_msg);
      return fmt::format (
        "Aftertouch {}: {}", note, midi_get_aftertouch_value (short_msg));
    }
  else if (midi_is_pitch_wheel (short_msg))
    {
      return fmt::format (
        "Pitch Wheel: {} (Ch{})", midi_get_pitchwheel_value (short_msg),
        channel);
    }
  else if (midi_is_channel_pressure (short_msg))
    {
      return fmt::format (
        "Channel Pressure: {} (Ch{})",
        midi_get_channel_pressure_value (short_msg), channel);
    }
  else if (midi_is_controller (short_msg))
    {
      return fmt::format (
        "Controller (Ch{}): {}={}", channel,
        midi_get_controller_name (midi_get_controller_number (short_msg)),
        midi_get_controller_value (short_msg));
    }
  else if (midi_is_program_change (short_msg))
    {
      return fmt::format (
        "Program Change: {} (Ch{})", midi_get_program_change_number (short_msg),
        channel);
    }
  else if (midi_is_all_notes_off (short_msg))
    {
      return fmt::format ("All Notes Off: (Ch{})", channel);
    }
  else if (midi_is_all_sound_off (short_msg))
    {
      return fmt::format ("All Sound Off: (Ch{})", channel);
    }
  else if (midi_is_quarter_frame (short_msg))
    {
      return { "Quarter Frame" };
    }
  else if (midi_is_clock (short_msg))
    {
      return { "Clock" };
    }
  else if (midi_is_start (short_msg))
    {
      return { "Start" };
    }
  else if (midi_is_continue (short_msg))
    {
      return { "Continue" };
    }
  else if (midi_is_stop (short_msg))
    {
      return { "Stop" };
    }
  else if (midi_is_short_msg_meta_event (short_msg))
    {
      return fmt::format (
        "Meta-Event Type: {}", midi_get_meta_event_type (short_msg));
    }
  else if (midi_is_song_position_pointer (short_msg))
    {
      return fmt::format (
        "Song Position: {}", midi_get_song_position_pointer_value (short_msg));
    }
  else
    {
      const auto hex = midi_get_hex_str (short_msg);
      return fmt::format ("Unknown MIDI Message: {}", hex);
    }
}

std::string
midi_print_to_str (std::span<const midi_byte_t> msg)
{
  if (midi_is_short_msg (msg))
    {
      return midi_print_short_msg_to_str (msg);
    }
  else if (midi_is_sysex (msg))
    {
      const auto hex = midi_get_hex_str (msg);
      return fmt::format ("Sysex: {}", hex);
    }
  else if (midi_is_meta_event (msg))
    {
      const midi_byte_t * data{};
      size_t              data_sz =
        midi_get_meta_event_data (&data, msg.data (), msg.size ());
      if (data_sz == 0)
        {
          return { "Invalid meta event" };
        }
      else
        {
          const midi_byte_t meta_event_type = midi_get_meta_event_type (msg);
          const auto        meta_event_name =
            midi_get_meta_event_type_name (meta_event_type);
          const auto hex =
            midi_get_hex_str (std::span<const midi_byte_t>{ data, data_sz });
          return fmt::format (
            "Meta-event: {}\n"
            "  length: {}\n"
            "  data: {}",
            meta_event_name, data_sz, hex);
        }
    }
  else
    {
      return fmt::format (
        "Unknown MIDI event of size {}: {}", msg.size (),
        midi_get_hex_str (msg));
    }
}

void
midi_print (std::span<const midi_byte_t> msg)
{
  z_info ("{}", midi_print_to_str (msg));
}

} // namespace zrythm::utils::midi
