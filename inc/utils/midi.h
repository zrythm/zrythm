/*
 * Copyright (C) 2018-2022 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
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
 */

/**
 * \file
 *
 * MIDI utils.
 */

#ifndef __AUDIO_MIDI_H__
#define __AUDIO_MIDI_H__

#include "zrythm-config.h"

#include <stdint.h>
#include <string.h>

#include "utils/types.h"

#include <gtk/gtk.h>

/**
 * @addtogroup audio
 *
 * @{
 */

/* see http://www.onicos.com/staff/iz/formats/midi-event.html */
#define MIDI_CH1_NOTE_ON 0x90
#define MIDI_CH1_NOTE_OFF 0x80
#define MIDI_CH1_POLY_AFTERTOUCH 0xA0
#define MIDI_CH1_CTRL_CHANGE 0xB0
#define MIDI_CH1_PROG_CHANGE 0xC0
#define MIDI_CH1_CHAN_AFTERTOUCH 0xD0
#define MIDI_CH1_PITCH_WHEEL_RANGE 0xE0
#define MIDI_ALL_NOTES_OFF 0x7B
#define MIDI_ALL_SOUND_OFF 0x78
#define MIDI_SYSTEM_MESSAGE 0xF0
#define MIDI_META_EVENT 0xFF

/**
 * Return the name of the given cc (0-127).
 */
CONST
const char *
midi_get_controller_name (const midi_byte_t cc);

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
  midi_byte_t *  msb);

/**
 * Saves a string representation of the given
 * control change event in the given buffer.
 *
 * @return The MIDI channel
 */
int
midi_ctrl_change_get_ch_and_description (
  midi_byte_t * ctrl_change,
  char *        buf);

/**
 * Returns the length of the MIDI message based on
 * the status byte.
 */
int
midi_get_msg_length (const uint8_t status_byte);

/**
 * Returns the frequency in Hz of the given note,
 * using 440Hz base.
 */
static inline float
midi_note_number_to_frequency (const uint8_t note)
{
  return 440.f
         * powf (2.f, ((float) note - 69.f) * (1.f / 12.f));
}

static inline uint8_t
midi_frequency_to_note_number (const float freq)
{
  return (uint8_t) round (
    69.f + (12.f / logf (2.f)) * logf (freq * (1.0f / 440.f)));
}

/**
 * Returns this note's position within an octave,
 * 0-11, where C is 0.
 *
 * TODO maybe move to chord/scale.
 */
static inline uint8_t
midi_get_chromatic_scale_index (const uint8_t note)
{
  return note % 12;
}

/**
 * Returns this note's octave number.
 *
 * TODO maybe move to chord/scale.
 */
static inline uint8_t
midi_get_octave_number (const uint8_t note)
{
  const uint8_t octave_for_middle_c = 3;
  return note / 12 + (octave_for_middle_c - 5);
}

/**
 * Returns whether the given first byte of a MIDI
 * event is of the given type hex.
 */
static inline bool
midi_is_short_message_type (
  const midi_byte_t short_msg[3],
  const midi_byte_t type)
{
  return (short_msg[0] & 0xf0) == type;
}

static inline midi_byte_t
midi_get_note_number (const midi_byte_t short_msg[3])
{
  return short_msg[1];
}

static inline midi_byte_t
midi_get_velocity (const midi_byte_t short_msg[3])
{
  return short_msg[2];
}

/**
 * Returns the note name (eg, "C") for a value
 * between 0 and 127.
 */
CONST
const char *
midi_get_note_name (const midi_byte_t note);

void
midi_get_note_name_with_octave (
  const midi_byte_t short_msg[3],
  char *            buf);

static inline bool
midi_is_note_on (const midi_byte_t short_msg[3])
{
  return midi_is_short_message_type (short_msg, MIDI_CH1_NOTE_ON)
         && midi_get_velocity (short_msg) != 0;
}

static inline bool
midi_is_note_off (const midi_byte_t short_msg[3])
{
  return midi_is_short_message_type (short_msg, MIDI_CH1_NOTE_OFF)
         || (midi_is_short_message_type (short_msg, MIDI_CH1_NOTE_ON) && midi_get_velocity (short_msg) == 0);
}

static inline bool
midi_is_program_change (const midi_byte_t short_msg[3])
{
  return midi_is_short_message_type (
    short_msg, MIDI_CH1_PROG_CHANGE);
}

static inline midi_byte_t
midi_get_program_change_number (const midi_byte_t short_msg[3])
{
  return short_msg[1];
}

static inline bool
midi_is_pitch_wheel (const midi_byte_t short_msg[3])
{
  return midi_is_short_message_type (
    short_msg, MIDI_CH1_PITCH_WHEEL_RANGE);
}

static inline bool
midi_is_aftertouch (const midi_byte_t short_msg[3])
{
  return midi_is_short_message_type (
    short_msg, MIDI_CH1_POLY_AFTERTOUCH);
}

static inline bool
midi_is_channel_pressure (const midi_byte_t short_msg[3])
{
  return midi_is_short_message_type (
    short_msg, MIDI_CH1_CHAN_AFTERTOUCH);
}

static inline bool
midi_is_controller (const midi_byte_t short_msg[3])
{
  return midi_is_short_message_type (
    short_msg, MIDI_CH1_CTRL_CHANGE);
}

/**
 * Used for MIDI controls whose values are split
 * between MSB/LSB.
 *
 * @param second_byte LSB.
 * @param third_byte MSB.
 */
static inline uint32_t
midi_get_14_bit_value (const midi_byte_t short_msg[3])
{
  return short_msg[1] | ((uint32_t) short_msg[2] << 7);
}

static inline midi_byte_t
midi_get_channel_0_to_15 (const midi_byte_t short_msg[3])
{
  return short_msg[0] & 0x0f;
}

static inline midi_byte_t
midi_get_channel_1_to_16 (const midi_byte_t short_msg[3])
{
  return midi_get_channel_0_to_15 (short_msg) + 1u;
}

static inline uint32_t
midi_get_pitchwheel_value (const midi_byte_t short_msg[3])
{
  return midi_get_14_bit_value (short_msg);
}

static inline midi_byte_t
midi_get_aftertouch_value (const midi_byte_t short_msg[3])
{
  return short_msg[2];
}

static inline midi_byte_t
midi_get_channel_pressure_value (
  const midi_byte_t short_msg[3])
{
  return short_msg[1];
}

static inline midi_byte_t
midi_get_controller_number (const midi_byte_t short_msg[3])
{
  return short_msg[1];
}

static inline midi_byte_t
midi_get_controller_value (const midi_byte_t short_msg[3])
{
  return short_msg[2];
}

static inline bool
midi_is_all_notes_off (const midi_byte_t short_msg[3])
{
  return midi_is_controller (short_msg)
         && midi_get_controller_number (short_msg) == 123;
}

static inline bool
midi_is_all_sound_off (const midi_byte_t short_msg[3])
{
  return midi_is_controller (short_msg)
         && midi_get_controller_number (short_msg) == 120;
}

static inline bool
midi_is_quarter_frame (const midi_byte_t short_msg[3])
{
  return short_msg[0] == 0xf1;
}

static inline bool
midi_is_clock (const midi_byte_t short_msg[3])
{
  return short_msg[0] == 0xf8;
}

static inline bool
midi_is_start (const midi_byte_t short_msg[3])
{
  return short_msg[0] == 0xfa;
}

static inline bool
midi_is_continue (const midi_byte_t short_msg[3])
{
  return short_msg[0] == 0xfb;
}

static inline bool
midi_is_stop (const midi_byte_t short_msg[3])
{
  return short_msg[0] == 0xfc;
}

static inline bool
midi_is_active_sense (const midi_byte_t short_msg[3])
{
  return short_msg[0] == 0xfe;
}

static inline bool
midi_is_song_position_pointer (const midi_byte_t short_msg[3])
{
  return short_msg[0] == 0xf2;
}

static inline uint32_t
midi_get_song_position_pointer_value (
  const midi_byte_t short_msg[3])
{
  return midi_get_14_bit_value (short_msg);
}

void
midi_get_hex_str (
  const midi_byte_t * msg,
  const size_t        msg_sz,
  char *              buf);

void
midi_print_to_str (
  const midi_byte_t * msg,
  const size_t        msg_sz,
  char *              buf);

void
midi_print (const midi_byte_t * msg, const size_t msg_sz);

static inline bool
midi_is_short_msg (const midi_byte_t * msg, const size_t msg_sz)
{
  g_return_val_if_fail (msg_sz > 0, false);

  if (msg_sz > 3)
    return false;

  return msg[0] != MIDI_SYSTEM_MESSAGE
         && msg[0] != MIDI_META_EVENT;
}

static inline bool
midi_is_sysex (const midi_byte_t * msg, const size_t msg_sz)
{
  return msg_sz > 1 && msg[0] == MIDI_SYSTEM_MESSAGE;
}

static inline bool
midi_is_meta_event (const midi_byte_t * msg, const size_t msg_sz)
{
  return msg_sz > 2 && msg[0] == MIDI_META_EVENT;
}

static inline bool
midi_is_short_msg_meta_event (const midi_byte_t short_msg[3])
{
  return midi_is_meta_event (short_msg, 3);
}

static inline bool
midi_is_meta_event_of_type (
  const midi_byte_t * msg,
  const size_t        msg_sz,
  const midi_byte_t   type)
{
  return msg_sz > 2 && msg[1] == type
         && msg[0] == MIDI_META_EVENT;
}

static inline midi_byte_t
midi_get_meta_event_type (
  const midi_byte_t * msg,
  const size_t        msg_sz)
{
  g_return_val_if_fail (midi_is_meta_event (msg, msg_sz), 0);

  return msg[1];
}

void
midi_get_meta_event_type_name (
  char *            buf,
  const midi_byte_t type);

/**
 * FIXME NOT TESTED
 *
 * Saves a pointer to the event data in @ref data
 * and returns the size.
 *
 * @note the data is owned by @ref msg.
 *
 * @return The size of the returned data, or 0 if
 *   error.
 */
static inline size_t
midi_get_meta_event_data (
  const midi_byte_t ** data,
  const midi_byte_t *  msg,
  const size_t         msg_sz)
{
  g_return_val_if_fail (midi_is_meta_event (msg, msg_sz), 0);

  /* malformed data */
  if (msg_sz < 4)
    return 0;

  size_t content_len = 0;
  size_t len_bytes = 0;
  for (size_t i = 2; i < msg_sz; i++)
    {
      const midi_byte_t byte = msg[i];
      len_bytes++;
      content_len = (content_len << 7) | (byte & 0x7fu);

      if (byte < 0x80)
        break;

      if (len_bytes == 4 || len_bytes + 2 == msg_sz)
        return 0; /* malformed data */
    }

  size_t content_start = len_bytes + 2;

  if (content_start + content_len > msg_sz)
    return 0; /* malformed data */

  *data = &msg[content_start];
  return content_len;
}

/**
 * @}
 */

#endif
