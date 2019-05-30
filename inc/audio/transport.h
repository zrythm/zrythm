/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * The transport struct containing all objects in
 *  the transport and their metadata
 */

#ifndef __AUDIO_TRANSPORT_H__
#define __AUDIO_TRANSPORT_H__

#include <stdint.h>

#include "audio/region.h"
#include "zix/sem.h"

/**
 * @addtogroup audio
 *
 * @{
 */

#define TRANSPORT (&AUDIO_ENGINE->transport)
#define DEFAULT_TOTAL_BARS 128
#define MAX_BPM 420.f
#define MIN_BPM 40.f
#define DEFAULT_BPM 140.f
#define DEFAULT_BEATS_PER_BAR 4
#define DEFAULT_BEAT_UNIT BEAT_UNIT_4

#define PLAYHEAD TRANSPORT->playhead_pos
#define IS_TRANSPORT_ROLLING TRANSPORT->play_state == PLAYSTATE_ROLLING

typedef enum BeatUnit
{
  BEAT_UNIT_2,
  BEAT_UNIT_4,
  BEAT_UNIT_8,
  BEAT_UNIT_16
} BeatUnit;

typedef enum {
  PLAYSTATE_ROLL_REQUESTED,
	PLAYSTATE_ROLLING,
	PLAYSTATE_PAUSE_REQUESTED,
	PLAYSTATE_PAUSED
} Play_State;

/**
 * The transport.
 */
typedef struct Transport
{
  /** Total bars in the song. */
  int           total_bars;

  /** Playhead position. */
  Position      playhead_pos;

  /** Cue point position. */
  Position      cue_pos;

  /** Loop start position. */
  Position      loop_start_pos;

  /** Loop end position. */
  Position      loop_end_pos;

  /**
   * Start marker position.
   *
   * This is where the song starts. Used when
   * exporting, etc.
   */
  Position      start_marker_pos;

  /**
   * End marker position.
   *
   * This is where the song ends. Used when
   * exporting, etc.
   */
  Position      end_marker_pos;

  /**
   * The top part (beats_per_par) is the number of beat units
   * (the bottom part) there will be per bar.
   *
   * Example: 4/4 = 4 (top) 1/4th (bot) notes per bar.
   * 2/8 = 2 (top) 1/8th (bot) notes per bar.
   */
  int                beats_per_bar; ///< top part of time signature
  int                beat_unit;   ///< bottom part of time signature, power of 2
  BeatUnit           ebeat_unit;

  /* ---------- CACHE -------------- */
  long               lticks_per_beat;
  long               lticks_per_bar;
  int                ticks_per_beat;
  int                ticks_per_bar;
  int                sixteenths_per_beat;

  /* ------------------------------- */

  uint32_t           position;       ///< Transport position in frames
	float              bpm;            ///< Transport tempo in beats per minute
	int               rolling;        ///< Transport speed (0=stop, 1=play)

  /** Looping or not. */
  int               loop;

  /** Recording or not. */
  int               recording;

  /**
   * This is set when record is toggled and is used to check
   * if a new region should be created.
   *
   * It should be set to off after the first cycle it is
   * processed in in the audio engine post process.
   */
  //int               starting_recording;

	ZixSem             paused;         ///< Paused signal from process thread
  Play_State         play_state;     ///< play state
} Transport;

static const cyaml_strval_t
beat_unit_strings[] =
{
	{ "2",      BEAT_UNIT_2    },
	{ "4",      BEAT_UNIT_4    },
	{ "8",      BEAT_UNIT_8    },
	{ "16",     BEAT_UNIT_16   },
};

static const cyaml_schema_field_t
transport_fields_schema[] =
{
  CYAML_FIELD_INT (
    "total_bars", CYAML_FLAG_DEFAULT,
    Transport, total_bars),
  CYAML_FIELD_MAPPING (
    "playhead_pos", CYAML_FLAG_DEFAULT,
    Transport, playhead_pos,
    position_fields_schema),
  CYAML_FIELD_MAPPING (
    "cue_pos", CYAML_FLAG_DEFAULT,
    Transport, cue_pos,
    position_fields_schema),
  CYAML_FIELD_MAPPING (
    "loop_start_pos", CYAML_FLAG_DEFAULT,
    Transport, loop_start_pos,
    position_fields_schema),
  CYAML_FIELD_MAPPING (
    "loop_end_pos", CYAML_FLAG_DEFAULT,
    Transport, loop_end_pos,
    position_fields_schema),
  CYAML_FIELD_MAPPING (
    "start_marker_pos", CYAML_FLAG_DEFAULT,
    Transport, start_marker_pos,
    position_fields_schema),
  CYAML_FIELD_MAPPING (
    "end_marker_pos", CYAML_FLAG_DEFAULT,
    Transport, end_marker_pos,
    position_fields_schema),
  CYAML_FIELD_INT (
    "beats_per_bar", CYAML_FLAG_DEFAULT,
    Transport, beats_per_bar),
  CYAML_FIELD_ENUM (
    "ebeat_unit", CYAML_FLAG_DEFAULT,
    Transport, ebeat_unit, beat_unit_strings,
    CYAML_ARRAY_LEN (beat_unit_strings)),
  CYAML_FIELD_INT (
    "position", CYAML_FLAG_DEFAULT,
    Transport, position),
  CYAML_FIELD_FLOAT (
    "bpm", CYAML_FLAG_DEFAULT,
    Transport, bpm),
  CYAML_FIELD_INT (
    "loop", CYAML_FLAG_DEFAULT,
    Transport, loop),
  CYAML_FIELD_INT (
    "recording", CYAML_FLAG_DEFAULT,
    Transport, recording),

	CYAML_FIELD_END
};

static const cyaml_schema_value_t
transport_schema =
{
	CYAML_VALUE_MAPPING (
    CYAML_FLAG_POINTER,
	  Transport, transport_fields_schema),
};

/**
 * Initialize transport
 */
void
transport_init (Transport * self,
                int         loading);

/**
 * Sets BPM and does any necessary processing (like notifying interested
 * parties).
 */
void
transport_set_bpm (float bpm);

/**
 * Updates beat unit and anything depending on it.
 */
void
transport_set_beat_unit (
  Transport * self,
  int beat_unit);

/**
 * Moves the playhead by the time corresponding to given samples.
 */
void
transport_add_to_playhead (int nframes);

void
transport_request_pause ();

void
transport_request_roll ();

/**
 * Setter for playhead Position.
 */
void
transport_set_playhead_pos (
  Transport * self,
  Position *  pos);

/**
 * Getter for playhead Position.
 */
void
transport_get_playhead_pos (
  Transport * self,
  Position *  pos);

/**
 * Moves playhead to given pos.
 *
 * This is only for moves other than while playing
 * and for looping while playing.
 *
 * @param target Position to set to.
 * @param panic Send MIDI panic or not.
 */
void
transport_move_playhead (
  Position * target,
  int        panic);

/**
 * Updates the frames in all transport positions
 */
void
transport_update_position_frames ();

void
transport_set_ebeat_unit (
  Transport * self,
  BeatUnit bu);

#endif
