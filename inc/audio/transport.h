/*
 * Copyright (C) 2018-2020 Alexandros Theodotou <alex at zrythm dot org>
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
 */

/**
 * \file
 *
 * Transport API.
 */

#ifndef __AUDIO_TRANSPORT_H__
#define __AUDIO_TRANSPORT_H__

#include <stdint.h>

#include "audio/region.h"
#include "utils/types.h"

#include "zix/sem.h"

typedef struct TimelineSelections TimelineSelections;
typedef struct AudioEngine AudioEngine;

/**
 * @addtogroup audio
 *
 * @{
 */

#define TRANSPORT (AUDIO_ENGINE->transport)
#define TRANSPORT_DEFAULT_TOTAL_BARS 128
#define TRANSPORT_MAX_BPM 420.f
#define TRANSPORT_MIN_BPM 40.f
#define TRANSPORT_DEFAULT_BPM 140.f
#define TRANSPORT_DEFAULT_BEATS_PER_BAR 4
#define TRANSPORT_DEFAULT_BEAT_UNIT BEAT_UNIT_4

#define PLAYHEAD (&TRANSPORT->playhead_pos)
#define TRANSPORT_IS_ROLLING \
  (TRANSPORT->play_state == PLAYSTATE_ROLLING)
#define TRANSPORT_IS_PAUSED \
  (TRANSPORT->play_state == PLAYSTATE_PAUSED)
#define TRANSPORT_IS_LOOPING \
  (TRANSPORT->loop)
#define TRANSPORT_BEATS_PER_BAR \
  (TRANSPORT->time_sig.beats_per_bar)
#define TRANSPORT_BEAT_UNIT_INT \
  (TRANSPORT->time_sig.beat_unit)

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
 * Corrseponts to "transport-display" in the
 * gsettings.
 */
typedef enum TransportDisplay
{
  TRANSPORT_DISPLAY_BBT,
  TRANSPORT_DISPLAY_TIME,
} TransportDisplay;

/**
 * Recording mode for MIDI and audio.
 *
 * In all cases, only objects created during the
 * current recording cycle can be changed. Previous
 * objects shall not be touched.
 */
typedef enum TransportRecordingMode
{
  /**
   * Overwrite events in first recorded region.
   *
   * In the case of MIDI, this will remove existing
   * MIDI notes during recording.
   *
   * In the case of audio, this will act exactly
   * the same as \ref RECORDING_MODE_TAKES_MUTED.
   */
  RECORDING_MODE_OVERWRITE_EVENTS,

  /**
   * Merge events in existing region.
   *
   * In the case of MIDI, this will append MIDI
   * notes.
   *
   * In the case of audio, this will act exactly
   * the same as \ref RECORDING_MODE_TAKES.
   */
  RECORDING_MODE_MERGE_EVENTS,

  /**
   * Events get put in new lanes each time recording
   * starts/resumes (eg, when looping or
   * entering/leaving the punch range).
   */
  RECORDING_MODE_TAKES,

  /**
   * Same as \ref RECORDING_MODE_TAKES, except
   * previous takes (since recording started) are
   * muted.
   */
  RECORDING_MODE_TAKES_MUTED,
} TransportRecordingMode;

typedef struct TimeSignature
{
  /**
   * The top part (beats_per_par) is the number of
   * beat units
   * (the bottom part) there will be per bar.
   *
   * Example: 4/4 = 4 (top) 1/4th (bot) notes per bar.
   * 2/8 = 2 (top) 1/8th (bot) notes per bar.
   */
  int           beats_per_bar;

  /**
   * Bottom part of the time signature.
   *
   * Power of 2.
   */
  int           beat_unit;
} TimeSignature;

static const cyaml_schema_field_t
time_signature_fields_schema[] =
{
  YAML_FIELD_INT (
    TimeSignature, beats_per_bar),
  YAML_FIELD_INT (
    TimeSignature, beat_unit),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t
time_signature_schema =
{
  YAML_VALUE_PTR (
    TimeSignature, time_signature_fields_schema),
};

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

  /** Punch in position. */
  Position      punch_in_pos;

  /** Punch out position. */
  Position      punch_out_pos;

  /**
   * Selected range.
   *
   * This is 2 points instead of start/end because
   * the 2nd point can be dragged past the 1st
   * point so the order gets swapped.
   *
   * Should be compared each time to see which one
   * is first.
   */
  Position      range_1;
  Position      range_2;

  /** Whether range should be displayed or not. */
  int           has_range;

  TimeSignature time_sig;

  /* ---------- CACHE -------------- */
  double        ticks_per_beat;
  double        ticks_per_bar;
  int           sixteenths_per_beat;
  int           sixteenths_per_bar;

  /* ------------------------------- */

  /** Transport position in frames.
   * FIXME is this used? */
  nframes_t     position;

  /** Transport speed (0=stop, 1=play). */
  int           rolling;

  /** Looping or not. */
  bool          loop;

  /** Whether punch in/out mode is enabled. */
  bool          punch_mode;

  /** Recording or not. */
  bool          recording;

  /** Metronome enabled or not. */
  bool          metronome_enabled;

  /** Whether to start playback on MIDI input. */
  bool          start_playback_on_midi_input;

  TransportRecordingMode recording_mode;

  /**
   * This is set when record is toggled and is used to check
   * if a new region should be created.
   *
   * It should be set to off after the first cycle it is
   * processed in in the audio engine post process.
   */
  //int               starting_recording;

  /** Paused signal from process thread. */
  ZixSem        paused;

  /**
   * Position of the playhead before pausing.
   *
   * Used by UndoableAction.
   */
  Position      playhead_before_pause;

  /** Play state. */
  Play_State    play_state;
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
  YAML_FIELD_INT (
    Transport, total_bars),
  YAML_FIELD_MAPPING_EMBEDDED (
    Transport, playhead_pos,
    position_fields_schema),
  YAML_FIELD_MAPPING_EMBEDDED (
    Transport, cue_pos,
    position_fields_schema),
  YAML_FIELD_MAPPING_EMBEDDED (
    Transport, loop_start_pos,
    position_fields_schema),
  YAML_FIELD_MAPPING_EMBEDDED (
    Transport, loop_end_pos,
    position_fields_schema),
  YAML_FIELD_MAPPING_EMBEDDED (
    Transport, punch_in_pos,
    position_fields_schema),
  YAML_FIELD_MAPPING_EMBEDDED (
    Transport, punch_out_pos,
    position_fields_schema),
  YAML_FIELD_MAPPING_EMBEDDED (
    Transport, range_1, position_fields_schema),
  YAML_FIELD_MAPPING_EMBEDDED (
    Transport, range_2, position_fields_schema),
  YAML_FIELD_INT (
    Transport, has_range),
  YAML_FIELD_MAPPING_EMBEDDED (
    Transport, time_sig,
    time_signature_fields_schema),
  YAML_FIELD_INT (
    Transport, position),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t
transport_schema =
{
  YAML_VALUE_PTR (
    Transport, transport_fields_schema),
};

/**
 * Initialize transport
 */
Transport *
transport_new (
  AudioEngine * engine);

void
transport_init_loaded (
  Transport * self);

/**
 * Clones the transport values.
 */
Transport *
transport_clone (
  Transport * self);

/**
 * Prepares audio regions for stretching (sets the
 * \ref ZRegion.before_length).
 *
 * @param selections If NULL, all audio regions
 *   are used. If non-NULL, only the regions in the
 *   selections are used.
 */
void
transport_prepare_audio_regions_for_stretch (
  Transport *          self,
  TimelineSelections * sel);

/**
 * Stretches audio regions.
 *
 * @param selections If NULL, all audio regions
 *   are used. If non-NULL, only the regions in the
 *   selections are used.
 * @param with_fixed_ratio Stretch all regions with
 *   a fixed ratio. If this is off, the current
 *   region length and \ref ZRegion.before_length
 *   will be used to calculate the ratio.
 */
void
transport_stretch_audio_regions (
  Transport *          self,
  TimelineSelections * sel,
  bool                 with_fixed_ratio,
  double               time_ratio);

void
transport_set_punch_mode_enabled (
  Transport * self,
  bool        enabled);

void
transport_set_start_playback_on_midi_input (
  Transport * self,
  bool        enabled);

void
transport_set_recording_mode (
  Transport * self,
  TransportRecordingMode mode);

void
time_signature_set_beat_unit_from_enum (
  TimeSignature * self,
  BeatUnit        ebeat_unit);

BeatUnit
time_signature_get_beat_unit_enum (
  int beat_unit);

void
time_signature_set_beat_unit (
  TimeSignature * self,
  int             beat_unit);

/**
 * Updates beat unit and anything depending on it.
 */
void
transport_set_time_sig (
  Transport *     self,
  TimeSignature * time_sig);

/**
 * Sets whether metronome is enabled or not.
 */
void
transport_set_metronome_enabled (
  Transport * self,
  const int   enabled);

/**
 * Moves the playhead by the time corresponding to
 * given samples, taking into account the loop
 * end point.
 */
void
transport_add_to_playhead (
  Transport *     self,
  const nframes_t nframes);

void
transport_request_pause (
  Transport * self);

void
transport_request_roll (
  Transport * self);

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
 * @param set_cue_point Also set the cue point at
 *   this position.
 */
void
transport_move_playhead (
  Transport * self,
  Position *  target,
  bool        panic,
  bool        set_cue_point);

/**
 * Enables or disables loop.
 */
void
transport_set_loop (
  Transport * self,
  bool        enabled);

/**
 * Moves the playhead to the prev Marker.
 */
void
transport_goto_prev_marker (
  Transport * self);

/**
 * Moves the playhead to the next Marker.
 */
void
transport_goto_next_marker (
  Transport * self);

/**
 * Updates the frames in all transport positions
 */
void
transport_update_position_frames (
  Transport * self);

#if 0
/**
 * Adds frames to the given global frames, while
 * adjusting the new frames to loop back if the
 * loop point was crossed.
 *
 * @return The new frames adjusted.
 */
long
transport_frames_add_frames (
  const Transport * self,
  const long        gframes,
  const nframes_t   frames);
#endif

/**
 * Adds frames to the given position similar to
 * position_add_frames, except that it adjusts
 * the new Position if the loop end point was
 * crossed.
 */
void
transport_position_add_frames (
  const Transport * self,
  Position *        pos,
  const nframes_t   frames);

/**
 * Returns the PPQN (Parts/Ticks Per Quarter Note).
 */
double
transport_get_ppqn (
  Transport * self);

/**
 * Stores the position of the range in \ref pos.
 */
void
transport_get_range_pos (
  Transport * self,
  bool        first,
  Position *  pos);

/**
 * Sets if the project has range and updates UI.
 */
void
transport_set_has_range (
  Transport * self,
  bool        has_range);

void
transport_set_range1 (
  Transport * self,
  Position *  pos,
  bool        snap);

void
transport_set_range2 (
  Transport * self,
  Position *  pos,
  bool        snap);

/**
 * Returns the number of processable frames until
 * and excluding the loop end point as a positive
 * number (>= 1) if the loop point was met between
 * g_start_frames and (g_start_frames + nframes),
 * otherwise returns 0;
 */
nframes_t
transport_is_loop_point_met (
  const Transport * self,
  const long        g_start_frames,
  const nframes_t   nframes);

bool
transport_position_is_inside_punch_range (
  Transport * self,
  Position *  pos);

/**
 * Recalculates the total bars based on the last
 * object's position.
 */
void
transport_recalculate_total_bars (
  Transport * self);

/**
 * Updates the total bars.
 */
void
transport_update_total_bars (
  Transport * self,
  int         total_bars,
  bool        fire_events);

/**
 * Sets recording on/off.
 */
void
transport_set_recording (
  Transport * self,
  bool        record,
  bool        fire_events);

void
transport_free (
  Transport * self);

/**
 * @}
 */

#endif
