// SPDX-FileCopyrightText: © 2018-2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Transport API.
 */

#ifndef __AUDIO_TRANSPORT_H__
#define __AUDIO_TRANSPORT_H__

#include <stdint.h>

#include "audio/port.h"
#include "audio/region.h"
#include "utils/types.h"

#include "zix/sem.h"

typedef struct TimelineSelections TimelineSelections;
typedef struct AudioEngine        AudioEngine;

/**
 * @addtogroup audio
 *
 * @{
 */

#define TIME_SIGNATURE_SCHEMA_VERSION 1
#define TRANSPORT_SCHEMA_VERSION 1

#define TRANSPORT (AUDIO_ENGINE->transport)
#define TRANSPORT_DEFAULT_TOTAL_BARS 128

#define PLAYHEAD (&TRANSPORT->playhead_pos)
#define TRANSPORT_IS_ROLLING \
  (TRANSPORT->play_state == PLAYSTATE_ROLLING)
#define TRANSPORT_IS_PAUSED \
  (TRANSPORT->play_state == PLAYSTATE_PAUSED)
#define TRANSPORT_IS_LOOPING (TRANSPORT->loop)
#define TRANSPORT_IS_RECORDING (TRANSPORT->recording)

typedef enum PrerollCountBars
{
  PREROLL_COUNT_BARS_NONE,
  PREROLL_COUNT_BARS_ONE,
  PREROLL_COUNT_BARS_TWO,
  PREROLL_COUNT_BARS_FOUR,
} PrerollCountBars;

static const char * preroll_count_bars_str[] = {
  __ ("None"),
  __ ("1 bar"),
  __ ("2 bars"),
  __ ("4 bars"),
};

static inline const char *
transport_preroll_count_to_str (PrerollCountBars bars)
{
  return preroll_count_bars_str[bars];
}

static inline int
transport_preroll_count_bars_enum_to_int (
  PrerollCountBars bars)
{
  switch (bars)
    {
    case PREROLL_COUNT_BARS_NONE:
      return 0;
    case PREROLL_COUNT_BARS_ONE:
      return 1;
    case PREROLL_COUNT_BARS_TWO:
      return 2;
    case PREROLL_COUNT_BARS_FOUR:
      return 4;
    }
  return -1;
}

typedef enum
{
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

#if 0
typedef struct TimeSignature
{
  int           schema_version;

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
    TimeSignature, schema_version),
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
#endif

/**
 * The transport.
 */
typedef struct Transport
{
  int schema_version;

  /** Total bars in the song. */
  int total_bars;

  /** Playhead position. */
  Position playhead_pos;

  /** Cue point position. */
  Position cue_pos;

  /** Loop start position. */
  Position loop_start_pos;

  /** Loop end position. */
  Position loop_end_pos;

  /** Punch in position. */
  Position punch_in_pos;

  /** Punch out position. */
  Position punch_out_pos;

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
  Position range_1;
  Position range_2;

  /** Whether range should be displayed or not. */
  int has_range;

  //TimeSignature time_sig;

  /* ---------- CACHE -------------- */
  int ticks_per_beat;
  int ticks_per_bar;
  int sixteenths_per_beat;
  int sixteenths_per_bar;

  /* ------------------------------- */

  /** Transport position in frames.
   * FIXME is this used? */
  nframes_t position;

  /** Looping or not. */
  bool loop;

  /** Whether punch in/out mode is enabled. */
  bool punch_mode;

  /** Whether MIDI/audio recording is enabled
   * (recording toggle in transport bar). */
  bool recording;

  /** Metronome enabled or not. */
  bool metronome_enabled;

  /** Recording preroll frames remaining. */
  signed_frame_t preroll_frames_remaining;

  /** Metronome countin frames remaining. */
  signed_frame_t countin_frames_remaining;

  /** Whether to start playback on MIDI input. */
  bool start_playback_on_midi_input;

  TransportRecordingMode recording_mode;

  /**
   * This is set when record is toggled and is used to check
   * if a new region should be created.
   *
   * It should be set to off after the first cycle it is
   * processed in the audio engine post process.
   */
  //int               starting_recording;

  /** Paused signal from process thread. */
  ZixSem paused;

  /**
   * Position of the playhead before pausing.
   *
   * Used by UndoableAction.
   */
  Position playhead_before_pause;

  /**
   * Roll/play MIDI port.
   *
   * Any event received on this port will request
   * a roll.
   */
  Port * roll;

  /**
   * Stop MIDI port.
   *
   * Any event received on this port will request
   * a stop/pause.
   */
  Port * stop;

  /** Backward MIDI port. */
  Port * backward;

  /** Forward MIDI port. */
  Port * forward;

  /** Loop toggle MIDI port. */
  Port * loop_toggle;

  /** Rec toggle MIDI port. */
  Port * rec_toggle;

  /** Play state. */
  Play_State play_state;

  /** Last timestamp the playhead position was
   * changed manually. */
  gint64 last_manual_playhead_change;

  /** Pointer to owner audio engine, if any. */
  AudioEngine * audio_engine;
} Transport;

static const cyaml_schema_field_t transport_fields_schema[] = {
  YAML_FIELD_INT (Transport, schema_version),
  YAML_FIELD_INT (Transport, total_bars),
  YAML_FIELD_MAPPING_EMBEDDED (
    Transport,
    playhead_pos,
    position_fields_schema),
  YAML_FIELD_MAPPING_EMBEDDED (
    Transport,
    cue_pos,
    position_fields_schema),
  YAML_FIELD_MAPPING_EMBEDDED (
    Transport,
    loop_start_pos,
    position_fields_schema),
  YAML_FIELD_MAPPING_EMBEDDED (
    Transport,
    loop_end_pos,
    position_fields_schema),
  YAML_FIELD_MAPPING_EMBEDDED (
    Transport,
    punch_in_pos,
    position_fields_schema),
  YAML_FIELD_MAPPING_EMBEDDED (
    Transport,
    punch_out_pos,
    position_fields_schema),
  YAML_FIELD_MAPPING_EMBEDDED (
    Transport,
    range_1,
    position_fields_schema),
  YAML_FIELD_MAPPING_EMBEDDED (
    Transport,
    range_2,
    position_fields_schema),
  YAML_FIELD_INT (Transport, has_range),
  YAML_FIELD_INT (Transport, position),
  YAML_FIELD_MAPPING_PTR_OPTIONAL (
    Transport,
    roll,
    port_fields_schema),
  YAML_FIELD_MAPPING_PTR_OPTIONAL (
    Transport,
    stop,
    port_fields_schema),
  YAML_FIELD_MAPPING_PTR_OPTIONAL (
    Transport,
    backward,
    port_fields_schema),
  YAML_FIELD_MAPPING_PTR_OPTIONAL (
    Transport,
    forward,
    port_fields_schema),
  YAML_FIELD_MAPPING_PTR_OPTIONAL (
    Transport,
    loop_toggle,
    port_fields_schema),
  YAML_FIELD_MAPPING_PTR_OPTIONAL (
    Transport,
    rec_toggle,
    port_fields_schema),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t transport_schema = {
  YAML_VALUE_PTR (Transport, transport_fields_schema),
};

/**
 * Initialize transport
 */
NONNULL Transport *
transport_new (AudioEngine * engine);

/**
 * Initialize loaded transport.
 *
 * @param engine Owner engine, if any.
 * @param tempo_track Tempo track, used to
 *   initialize the caches. Only needed on the
 *   active project transport.
 */
void
transport_init_loaded (
  Transport *   self,
  AudioEngine * engine,
  Track *       tempo_track);

/**
 * Clones the transport values.
 */
Transport *
transport_clone (const Transport * src);

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
 * Stretches regions.
 *
 * @param selections If NULL, all regions
 *   are used. If non-NULL, only the regions in the
 *   selections are used.
 * @param with_fixed_ratio Stretch all regions with
 *   a fixed ratio. If this is off, the current
 *   region length and \ref ZRegion.before_length
 *   will be used to calculate the ratio.
 * @param force Force stretching, regardless of
 *   musical mode.
 *
 * @return Whether successful.
 */
bool
transport_stretch_regions (
  Transport *          self,
  TimelineSelections * sel,
  bool                 with_fixed_ratio,
  double               time_ratio,
  bool                 force,
  GError **            error);

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
  Transport *            self,
  TransportRecordingMode mode);

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
  Transport *          self,
  const signed_frame_t nframes);

/**
 * Request pause.
 *
 * Must only be called in-between engine processing
 * calls.
 *
 * @param with_wait Wait for lock before requesting.
 */
void
transport_request_pause (Transport * self, bool with_wait);

/**
 * Request playback.
 *
 * Must only be called in-between engine processing
 * calls.
 *
 * @param with_wait Wait for lock before requesting.
 */
void
transport_request_roll (Transport * self, bool with_wait);

/**
 * Setter for playhead Position.
 */
void
transport_set_playhead_pos (Transport * self, Position * pos);

void
transport_set_playhead_to_bar (Transport * self, int bar);

/**
 * Getter for playhead Position.
 */
void
transport_get_playhead_pos (Transport * self, Position * pos);

/**
 * Move to the previous snap point on the timeline.
 */
void
transport_move_backward (Transport * self, bool with_wait);

/**
 * Move to the next snap point on the timeline.
 */
void
transport_move_forward (Transport * self, bool with_wait);

/**
 * Returns whether the user can currently move the playhead
 * (eg, via the UI or via scripts).
 */
bool
transport_can_user_move_playhead (const Transport * self);

/**
 * Moves playhead to given pos.
 *
 * This is only for moves other than while playing
 * and for looping while playing.
 *
 * Should not be used during exporting.
 *
 * @param target Position to set to.
 * @param panic Send MIDI panic or not FIXME unused.
 * @param set_cue_point Also set the cue point at
 *   this position.
 */
void
transport_move_playhead (
  Transport *      self,
  const Position * target,
  bool             panic,
  bool             set_cue_point,
  bool             fire_events);

/**
 * Enables or disables loop.
 */
void
transport_set_loop (
  Transport * self,
  bool        enabled,
  bool        with_wait);

/**
 * Moves the playhead to the start Marker.
 */
void
transport_goto_start_marker (Transport * self);

/**
 * Moves the playhead to the end Marker.
 */
void
transport_goto_end_marker (Transport * self);

/**
 * Moves the playhead to the prev Marker.
 */
void
transport_goto_prev_marker (Transport * self);

/**
 * Moves the playhead to the next Marker.
 */
void
transport_goto_next_marker (Transport * self);

/**
 * Updates the frames in all transport positions
 *
 * @param update_from_ticks Whether to update the
 *   positions based on ticks (true) or frames
 *   (false).
 */
void
transport_update_positions (
  Transport * self,
  bool        update_from_ticks);

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
  const Transport *    self,
  Position *           pos,
  const signed_frame_t frames);

/**
 * Returns the PPQN (Parts/Ticks Per Quarter Note).
 */
double
transport_get_ppqn (Transport * self);

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
transport_set_has_range (Transport * self, bool has_range);

/**
 * Set the range1 or range2 position.
 *
 * @param range1 True to set range1, false to set
 *   range2.
 */
void
transport_set_range (
  Transport *      self,
  bool             range1,
  const Position * start_pos,
  const Position * pos,
  bool             snap);

/**
 * Returns the number of processable frames until
 * and excluding the loop end point as a positive
 * number (>= 1) if the loop point was met between
 * g_start_frames and (g_start_frames + nframes),
 * otherwise returns 0;
 */
HOT NONNULL WARN_UNUSED_RESULT static inline nframes_t
transport_is_loop_point_met (
  const Transport *    self,
  const signed_frame_t g_start_frames,
  const nframes_t      nframes)
{
  if (
    self->loop
    && G_UNLIKELY (
      self->loop_end_pos.frames > g_start_frames
      && self->loop_end_pos.frames
           <= g_start_frames + (long) nframes))
    {
      return (
        nframes_t) (self->loop_end_pos.frames - g_start_frames);
    }
  return 0;
}

bool
transport_position_is_inside_punch_range (
  Transport * self,
  Position *  pos);

/**
 * Recalculates the total bars based on the last
 * object's position.
 *
 * @param sel If given, only these objects will
 *   be checked, otherwise every object in the
 *   project will be checked.
 */
void
transport_recalculate_total_bars (
  Transport *          self,
  ArrangerSelections * sel);

/**
 * Updates the total bars.
 */
void
transport_update_total_bars (
  Transport * self,
  int         total_bars,
  bool        fire_events);

void
transport_update_caches (
  Transport * self,
  int         beats_per_bar,
  int         beat_unit);

/**
 * Sets recording on/off.
 */
void
transport_set_recording (
  Transport * self,
  bool        record,
  bool        with_wait,
  bool        fire_events);

void
transport_free (Transport * self);

/**
 * @}
 */

#endif
