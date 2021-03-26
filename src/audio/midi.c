/*
 * Copyright (C) 2018-2020 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <inttypes.h>
#include <math.h>
#include <stdlib.h>
#include <signal.h>

#include "audio/channel.h"
#include "audio/engine.h"
#include "audio/router.h"
#include "audio/midi.h"
#include "audio/midi_event.h"
#include "audio/transport.h"
#include "project.h"
#include "utils/objects.h"

/* https://www.midi.org/specifications-old/item/table-3-control-change-messages-data-bytes-2 */
static const char * midi_cc_names[128] = {
  "Bank Select",
  "Modulation Wheel",
  "Breath Controller",
  "Undefined 3",
  "Foot Controller",
  "Portamento Time",
  "Data Entry MSB",
  "Channel Volume",
  "Balance",
  "Undefined 9",
  "Pan",
  "Expression Controller",
  "Effect Control 1",
  "Effect Control 2",
  "Undefined 14",
  "Undefined 15",
  "General Purpose Controller 1",
  "General Purpose Controller 2",
  "General Purpose Controller 3",
  "General Purpose Controller 4",
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
  "Effect 2 Amount (Tremelo)",
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

/**
 * Return the name of the given cc (0-127).
 */
const char *
midi_get_cc_name (
  uint8_t cc)
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
  /* assert the given event is a ctrl change event */
  if (ctrl_change[0] < 0xB0 ||
      ctrl_change[0] > 0xBF)
    {
      return -1;
    }

  if (buf)
    {
      if (ctrl_change[1] >= 0x08 &&
          ctrl_change[1] <= 0x1F)
        {
          sprintf (
            buf, "Continuous controller #%u",
            ctrl_change[1]);
        }
      else if (ctrl_change[1] >= 0x28 &&
               ctrl_change[1] <= 0x3F)
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
              strcpy (
                buf, "Continuous controller #0");
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
              strcpy (
                buf, "Continuous controller #3");
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
int
midi_combine_bytes_to_int (
  midi_byte_t lsb,
  midi_byte_t msb)
{
  /* http://midi.teragonaudio.com/tech/midispec/wheel.htm */
  return (int) ((msb << 7) | lsb);
}

/**
 * Used for MIDI controls whose values are split
 * between MSB/LSB.
 *
 * @param lsb First byte (pos 1).
 * @param msb Second byte (pos 2).
 */
void
midi_get_bytes_from_int (
  int           val,
  midi_byte_t * lsb,
  midi_byte_t * msb)
{
  /* https://arduino.stackexchange.com/questions/18955/how-to-send-a-pitch-bend-midi-message-using-arcore */
  *lsb = val & 0x7F;
  *msb = val >> 7;
}

/**
 * Queues MIDI note off to event queues.
 *
 * @param queued Send the event to queues instead
 *   of main events.
 */
void
midi_panic_all (
  int queued)
{
  midi_events_panic (
    AUDIO_ENGINE->midi_editor_manual_press->
      midi_events, queued);

  Track * track;
  for (int i = 0; i < TRACKLIST->num_tracks; i++)
    {
      track = TRACKLIST->tracks[i];

      if (track_has_piano_roll (track) ||
          track->type == TRACK_TYPE_CHORD)
        {
          midi_events_panic (
            track->processor->piano_roll->
              midi_events,
            queued);
        }
    }
}

/**
 * Returns the length of the MIDI message based on
 * the status byte.
 *
 * TODO move this and other functions to utils/midi,
 * split separate files for MidiEvents and MidiEvent.
 */
int
midi_get_msg_length (
  uint8_t status_byte)
{
  // define these with meaningful names
  switch (status_byte & 0xf0) {
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
    switch (status_byte) {
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
