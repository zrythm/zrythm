/*
 * audio/transport.h - The transport struct containing all objects in
 *  the transport and their metadata
 *
 * Copyright (C) 2018 Alexandros Theodotou
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

#ifndef __AUDIO_TRANSPORT_H__
#define __AUDIO_TRANSPORT_H__

#include <stdint.h>

#include "audio/engine.h"
#include "audio/region.h"
#include "utils/sem.h"

#include <gtk/gtk.h>

#define TRANSPORT AUDIO_ENGINE->transport
#define DEFAULT_TOTAL_BARS 128
#define MAX_BPM 420.f
#define MIN_BPM 40.f
#define DEFAULT_BPM 140.f
#define DEFAULT_BEATS_PER_BAR 4
#define DEFAULT_BEAT_UNIT BEAT_UNIT_4
#define DEFAULT_ZOOM_LEVEL 1.0f

#define PLAYHEAD TRANSPORT->playhead_pos
#define IS_TRANSPORT_ROLLING TRANSPORT->play_state == PLAYSTATE_ROLLING

struct Project;

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

typedef struct Transport
{
  int           total_bars;             ///< total bars in the song
  Position      playhead_pos;           ///< playhead position
  Position      cue_pos;            ///< position to use when stopped
  Position      loop_start_pos;         ///< loop marker start position
  Position      loop_end_pos;           ///< loop marker end position
  Position      start_marker_pos;       ///< start marker position
  Position      end_marker_pos;         ///< end marker position

  /**
   * The top part (beats_per_par) is the number of beat units
   * (the bottom part) there will be per bar.
   *
   * Example: 4/4 = 4 (top) 1/4th (bot) notes per bar.
   * 2/8 = 2 (top) 1/8th (bot) notes per bar.
   */
  int                beats_per_bar; ///< top part of time signature
  BeatUnit           beat_unit;   ///< bottom part of time signature, power of 2
  float         zoom_level;             ///< zoom level used in ruler/transport widget calculations FIXME move to gui
  uint32_t           position;       ///< Transport position in frames
	float              bpm;            ///< Transport tempo in beats per minute
	int               rolling;        ///< Transport speed (0=stop, 1=play)
  int               loop;        ///< loop or not
	ZixSem             paused;         ///< Paused signal from process thread
  Play_State         play_state;     ///< play state
} Transport;

/**
 * Initialize transport
 */
Transport *
transport_new ();

void
transport_setup (Transport * self,
                 AudioEngine * engine);

/**
 * Sets BPM and does any necessary processing (like notifying interested
 * parties).
 */
void
transport_set_bpm (Transport * self,
                   AudioEngine * engine,
                   float bpm);

/**
 * Moves the playhead by the time corresponding to given samples.
 */
void
transport_add_to_playhead (Transport * self,
                           int nframes);

void
transport_request_pause (Transport * self);

void
transport_request_roll (Transport * self);

/**
 * Moves playhead to given pos
 */
void
transport_move_playhead (Transport * self,
                         Position * target, ///< position to set to
                         int      panic); ///< send MIDI panic or not

/**
 * Updates the frames in all transport positions
 */
void
transport_update_position_frames (Transport * self);

/**
 * Gets beat unit as int.
 */
int
transport_get_beat_unit (Transport * self);

#endif
