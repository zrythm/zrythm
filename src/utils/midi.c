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

#include <inttypes.h>
#include <math.h>
#include <signal.h>
#include <stdlib.h>

#include "dsp/channel.h"
#include "dsp/engine.h"
#include "dsp/midi_event.h"
#include "dsp/router.h"
#include "dsp/transport.h"
#include "project.h"
#include "utils/midi.h"
#include "utils/objects.h"

/* https://www.midi.org/specifications-old/item/table-3-control-change-messages-data-bytes-2 */
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

/**
 * Return the name of the given cc (0-127).
 */
const char *
midi_get_controller_name (uint8_t cc)
{
  return midi_cc_names[cc];
}

/**
 * Saves a string representation of the given
 * control change event in the given buffer.
 *
 * @param buf The string buffer to fill, or NULL
 *   to only get the channel.
 *
 * @return The MIDI channel, or -1 if not ctrl
 *   change.
 */
int
midi_ctrl_change_get_ch_and_description (
  midi_byte_t * ctrl_change,
  char *        buf)
{
  /* FIXME is there a reason this is not using
   * get_controller_name() ? */

  /* assert the given event is a ctrl change event */
  if (ctrl_change[0] < 0xB0 || ctrl_change[0] > 0xBF)
    {
      return -1;
    }

  if (buf)
    {
      if (ctrl_change[1] >= 0x08 && ctrl_change[1] <= 0x1F)
        {
          sprintf (
            buf, "Continuous controller #%u", ctrl_change[1]);
        }
      else if (ctrl_change[1] >= 0x28 && ctrl_change[1] <= 0x3F)
        {
          sprintf (
            buf, "Continuous controller #%u",
            ctrl_change[1] - 0x28);
        }
      else
        {
          switch (ctrl_change[1])
            {
            case 0x00:
            case 0x20:
              strcpy (buf, "Continuous controller #0");
              break;
            case 0x01:
            case 0x21:
              strcpy (buf, "Modulation wheel");
              break;
            case 0x02:
            case 0x22:
              strcpy (buf, "Breath control");
              break;
            case 0x03:
            case 0x23:
              strcpy (buf, "Continuous controller #3");
              break;
            case 0x04:
            case 0x24:
              strcpy (buf, "Foot controller");
              break;
            case 0x05:
            case 0x25:
              strcpy (buf, "Portamento time");
              break;
            case 0x06:
            case 0x26:
              strcpy (buf, "Data Entry");
              break;
            case 0x07:
            case 0x27:
              strcpy (buf, "Main Volume");
              break;
            default:
              strcpy (buf, "Unknown");
              break;
            }
        }
    }
  return (ctrl_change[0] - 0xB0) + 1;
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
  /* https://arduino.stackexchange.com/questions/18955/how-to-send-a-pitch-bend-midi-message-using-arcore */
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
  g_return_val_if_reached (-1);
}

/**
 * Returns the note name (eg, "C") for a value
 * between 0 and 127.
 */
const char *
midi_get_note_name (const midi_byte_t note)
{
  return note_labels[note % 12];
}

void
midi_get_note_name_with_octave (
  const midi_byte_t short_msg[3],
  char *            buf)
{
  midi_byte_t note = midi_get_note_number (short_msg);
  midi_byte_t note_idx = midi_get_chromatic_scale_index (note);
  midi_byte_t octave = midi_get_octave_number (note);
  sprintf (buf, "%s%u", midi_get_note_name (note_idx), octave);
}

void
midi_get_meta_event_type_name (
  char *            buf,
  const midi_byte_t type)
{
  switch (type)
    {
    case 0x00:
      strcpy (buf, "Sequence number");
      break;
    case 0x01:
      strcpy (buf, "Text");
      break;
    case 0x02:
      strcpy (buf, "Copyright notice");
      break;
    case 0x03:
      strcpy (buf, "Track name");
      break;
    case 0x04:
      strcpy (buf, "Instrument name");
      break;
    case 0x05:
      strcpy (buf, "Lyrics");
      break;
    case 0x06:
      strcpy (buf, "Marker");
      break;
    case 0x07:
      strcpy (buf, "Cue point");
      break;
    case 0x20:
      strcpy (buf, "Channel prefix");
      break;
    case 0x2F:
      strcpy (buf, "End of track");
      break;
    case 0x51:
      strcpy (buf, "Set tempo");
      break;
    case 0x54:
      strcpy (buf, "SMPTE offset");
      break;
    case 0x58:
      strcpy (buf, "Time signature");
      break;
    case 0x59:
      strcpy (buf, "Key signature");
      break;
    case 0x7F:
      strcpy (buf, "Sequencer specific");
      break;
    default:
      sprintf (buf, "Unknown type %hhx", type);
      break;
    }
}

void
midi_get_hex_str (
  const midi_byte_t * msg,
  const size_t        msg_sz,
  char *              buf)
{
  buf[0] = '\0';
  for (size_t i = 0; i < msg_sz; i++)
    {
      char tmp[6];
      sprintf (tmp, "%hhx", msg[i]);
      strcat (buf, tmp);
      if (i + 1 < msg_sz)
        strcat (buf, " ");
    }
}

static void
midi_print_short_msg_to_str (
  const midi_byte_t short_msg[3],
  char *            buf)
{
  midi_byte_t channel = midi_get_channel_1_to_16 (short_msg);

  if (midi_is_note_on (short_msg))
    {
      char note[40];
      midi_get_note_name_with_octave (short_msg, note);
      sprintf (
        buf, "Note-On %s: Velocity: %u", note,
        midi_get_velocity (short_msg));
    }
  else if (midi_is_note_off (short_msg))
    {
      char note[40];
      midi_get_note_name_with_octave (short_msg, note);
      sprintf (
        buf, "Note-Off %s: Velocity: %u", note,
        midi_get_velocity (short_msg));
    }
  else if (midi_is_aftertouch (short_msg))
    {
      char note[40];
      midi_get_note_name_with_octave (short_msg, note);
      sprintf (
        buf, "Aftertouch %s: %u", note,
        midi_get_aftertouch_value (short_msg));
    }
  else if (midi_is_pitch_wheel (short_msg))
    {
      sprintf (
        buf, "Pitch Wheel: %u (Ch%u)",
        midi_get_pitchwheel_value (short_msg), channel);
    }
  else if (midi_is_channel_pressure (short_msg))
    {
      sprintf (
        buf, "Channel Pressure: %u (Ch%u)",
        midi_get_channel_pressure_value (short_msg), channel);
    }
  else if (midi_is_controller (short_msg))
    {
      sprintf (
        buf, "Controller (Ch%u): %s=%u", channel,
        midi_get_controller_name (
          midi_get_controller_number (short_msg)),
        midi_get_controller_value (short_msg));
    }
  else if (midi_is_program_change (short_msg))
    {
      sprintf (
        buf, "Program Change: %u (Ch%u)",
        midi_get_program_change_number (short_msg), channel);
    }
  else if (midi_is_all_notes_off (short_msg))
    {
      sprintf (buf, "All Notes Off: (Ch%u)", channel);
    }
  else if (midi_is_all_sound_off (short_msg))
    {
      sprintf (buf, "All Sound Off: (Ch%u)", channel);
    }
  else if (midi_is_quarter_frame (short_msg))
    {
      strcpy (buf, "Quarter Frame");
    }
  else if (midi_is_clock (short_msg))
    {
      strcpy (buf, "Clock");
    }
  else if (midi_is_start (short_msg))
    {
      strcpy (buf, "Start");
    }
  else if (midi_is_continue (short_msg))
    {
      strcpy (buf, "Continue");
    }
  else if (midi_is_stop (short_msg))
    {
      strcpy (buf, "Stop");
    }
  else if (midi_is_short_msg_meta_event (short_msg))
    {
      sprintf (
        buf, "Meta-Event Type: %u",
        midi_get_meta_event_type (short_msg, 3));
    }
  else if (midi_is_song_position_pointer (short_msg))
    {
      sprintf (
        buf, "Song Position: %u",
        midi_get_song_position_pointer_value (short_msg));
    }
  else
    {
      char hex[3 * 4];
      midi_get_hex_str (short_msg, 3, hex);
      sprintf (buf, "Unknown MIDI Message: %s", hex);
    }
}

void
midi_print_to_str (
  const midi_byte_t * msg,
  const size_t        msg_sz,
  char *              buf)
{
  if (midi_is_short_msg (msg, msg_sz))
    {
      midi_print_short_msg_to_str (msg, buf);
    }
  else if (midi_is_sysex (msg, msg_sz))
    {
      char hex[msg_sz * 4];
      midi_get_hex_str (msg, msg_sz, hex);
      sprintf (buf, "Sysex: %s", hex);
    }
  else if (midi_is_meta_event (msg, msg_sz))
    {
      const midi_byte_t * data = NULL;
      size_t              data_sz =
        midi_get_meta_event_data (&data, msg, msg_sz);
      if (data_sz == 0)
        {
          strcpy (buf, "Invalid meta event");
        }
      else
        {
          midi_byte_t meta_event_type =
            midi_get_meta_event_type (msg, msg_sz);
          char meta_event_name[600];
          midi_get_meta_event_type_name (
            meta_event_name, meta_event_type);
          char hex[data_sz * 4];
          midi_get_hex_str (data, data_sz, hex);
          sprintf (
            buf,
            "Meta-event: %s\n"
            "  length: %zu\n"
            "  data: %s",
            meta_event_name, data_sz, hex);
        }
    }
  else
    {
      char hex[msg_sz * 4];
      midi_get_hex_str (msg, msg_sz, hex);
      sprintf (
        buf, "Unknown MIDI event of size %zu: %s", msg_sz,
        hex);
    }
}

void
midi_print (const midi_byte_t * msg, const size_t msg_sz)
{
  char str[600];
  midi_print_to_str (msg, msg_sz, str);
  g_message ("%s", str);
}
