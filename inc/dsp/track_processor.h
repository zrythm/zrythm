// SPDX-FileCopyrightText: © 2019-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Track processor.
 */

#ifndef __AUDIO_TRACK_PROCESSOR_H__
#define __AUDIO_TRACK_PROCESSOR_H__

#include "dsp/port.h"
#include "utils/types.h"

class StereoPorts;
class Port;
typedef struct Track                 Track;
typedef struct MidiMappings          MidiMappings;
typedef struct EngineProcessTimeInfo EngineProcessTimeInfo;
TYPEDEF_STRUCT (MPMCQueue);

/**
 * @addtogroup dsp
 *
 * @{
 */

#define TRACK_PROCESSOR_SCHEMA_VERSION 1

#define TRACK_PROCESSOR_MAGIC 81213128
#define IS_TRACK_PROCESSOR(tr) ((tr) && (tr)->magic == TRACK_PROCESSOR_MAGIC)

#define track_processor_is_in_active_project(self) \
  (self->track && track_is_in_active_project (self->track))

/**
 * A TrackProcessor is a processor that is used as
 * the first entry point when processing a track.
 */
typedef struct TrackProcessor
{
  /**
   * L & R audio input ports, if audio.
   */
  StereoPorts * stereo_in;

  /** Mono toggle, if audio. */
  Port * mono;

  /** Input gain, if audio. */
  Port * input_gain;

  /**
   * Output gain, if audio.
   *
   * This is applied after regions are processed to
   * TrackProcessor.streo_out.
   */
  Port * output_gain;

  /**
   * L & R audio output ports, if audio.
   */
  StereoPorts * stereo_out;

  /**
   * MIDI in Port.
   *
   * This port is for receiving MIDI signals from an external MIDI source.
   *
   * This is also where piano roll, midi in and midi manual press will be
   * routed to and this will be the port used to pass midi to the plugins.
   */
  Port * midi_in;

  /**
   * MIDI out port, if MIDI.
   */
  Port * midi_out;

  /**
   * MIDI input for receiving MIDI signals from the piano roll (i.e., MIDI
   * notes inside regions) or other sources.
   *
   * This will not be a separately exposed port during processing. It will
   * be processed by the TrackProcessor internally.
   */
  Port * piano_roll;

  /**
   * Whether to monitor the audio output.
   *
   * This is only used on audio tracks. During recording, if on, the recorded
   * audio will be passed to the output. If off, the recorded audio will not
   * be passed to the output.
   *
   * When not recording, this will only take effect when paused.
   */
  Port * monitor_audio;

  /* --- MIDI controls --- */

  /** Mappings to each CC port. */
  MidiMappings * cc_mappings;

  /** MIDI CC control ports, 16 channels. */
  Port * midi_cc[128 * 16];

  /** Pitch bend. */
  Port * pitch_bend[16];

  /**
   * Polyphonic key pressure (aftertouch).
   *
   * This message is most often sent by pressing down on the key after it
   * "bottoms out".
   *
   * FIXME this is completely wrong. It's supposed to be per-key, so 128 x 16
   * ports.
   */
  Port * poly_key_pressure[16];

  /**
   * Channel pressure (aftertouch).
   *
   * This message is different from polyphonic after-touch - sends the single
   * greatest pressure value (of all the current depressed keys).
   */
  Port * channel_pressure[16];

  /* --- end MIDI controls --- */

  /**
   * Current dBFS after processing each output port.
   *
   * Transient variables only used by the GUI.
   */
  float l_port_db;
  float r_port_db;

  /** Pointer to owner track, if any. */
  Track * track;

  /**
   * To be set to true when a panic (all notes off) message should be sent
   * during processing.
   *
   * Only applies to tracks that receive MIDI input.
   */
  bool pending_midi_panic;

  /**
   * A queue of MIDI CC ports whose values have been recently updated.
   *
   * This is used during processing to avoid checking every single MIDI CC
   * port for changes.
   */
  MPMCQueue * updated_midi_automatable_ports;

  int magic;
} TrackProcessor;

/**
 * Inits a TrackProcessor after a project is loaded.
 */
COLD NONNULL_ARGS (
  1) void track_processor_init_loaded (TrackProcessor * self, Track * track);

#if 0
void
track_processor_set_is_project (
  TrackProcessor * self,
  bool             is_project);
#endif

/**
 * Creates a new track processor for the given
 * track.
 */
COLD WARN_UNUSED_RESULT TrackProcessor *
track_processor_new (Track * track);

/**
 * Copy port values from \ref src to \ref dest.
 */
void
track_processor_copy_values (TrackProcessor * dest, TrackProcessor * src);

/**
 * Clears all buffers.
 */
void
track_processor_clear_buffers (TrackProcessor * self);

/**
 * Disconnects all ports connected to the
 * TrackProcessor.
 */
void
track_processor_disconnect_all (TrackProcessor * self);

static inline Track *
track_processor_get_track (const TrackProcessor * self)
{
#if 0
  g_return_val_if_fail (
    IS_TRACK_PROCESSOR (self) && IS_TRACK (self->track), NULL);
#endif
  g_return_val_if_fail (self->track, NULL);

  return self->track;
}

/**
 * Process the TrackProcessor.
 *
 * This function performs the following:
 * - produce output audio/MIDI into stereo out or midi out, based on any
 *   audio/MIDI regions, if has piano roll or is audio track
 * - produce additional output MIDI events based on any MIDI CC automation
 *   regions, if applicable
 * - change MIDI CC control port values based on any MIDI input, if recording
 *   --- at this point the output is ready ---
 * - handle recording (create events in regions and automation, including
 *   MIDI CC automation, based on the MIDI CC control ports)
 *
 * @param g_start_frames The global start frames.
 * @param local_offset The local start frames.
 * @param nframes The number of frames to process.
 */
void
track_processor_process (
  TrackProcessor *                    self,
  const EngineProcessTimeInfo * const time_nfo);

/**
 * Disconnect the TrackProcessor's stereo out ports
 * from the prefader.
 *
 * Used when there is no plugin in the channel.
 */
void
track_processor_disconnect_from_prefader (TrackProcessor * self);

/**
 * Connects the TrackProcessor's stereo out ports to the Channel's prefader in
 * ports.
 *
 * Used when deleting the only plugin left.
 */
void
track_processor_connect_to_prefader (TrackProcessor * self);

/**
 * Disconnect the TrackProcessor's out ports from the Plugin's input ports.
 */
void
track_processor_disconnect_from_plugin (TrackProcessor * self, Plugin * pl);

/**
 * Connect the TrackProcessor's out ports to the Plugin's input ports.
 */
void
track_processor_connect_to_plugin (TrackProcessor * self, Plugin * pl);

#if 0
void
track_processor_update_track_name_hash (
  TrackProcessor * self);
#endif

void
track_processor_append_ports (TrackProcessor * self, GPtrArray * ports);

/**
 * Frees the TrackProcessor.
 */
void
track_processor_free (TrackProcessor * self);

/**
 * @}
 */

#endif
