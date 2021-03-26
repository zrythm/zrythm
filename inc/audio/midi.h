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
#include "zix/sem.h"

#ifdef HAVE_JACK
#include "weak_libjack.h"
#endif

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

/**
 * Return the name of the given cc (0-127).
 */
const char *
midi_get_cc_name (
  uint8_t cc);

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
  midi_byte_t msb);

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
  midi_byte_t * msb);

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
 * Queues MIDI note off to event queues.
 *
 * @param queued Send the event to queues instead
 *   of main events.
 */
void
midi_panic_all (
  int queued);

/**
 * Returns the length of the MIDI message based on
 * the status byte.
 */
int
midi_get_msg_length (
  uint8_t status_byte);

/**
 * @}
 */

#endif
