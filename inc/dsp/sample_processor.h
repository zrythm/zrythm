// SPDX-FileCopyrightText: © 2019-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Sample processor.
 */

#ifndef __AUDIO_SAMPLE_PROCESSOR_H__
#define __AUDIO_SAMPLE_PROCESSOR_H__

#include "dsp/fader.h"
#include "dsp/port.h"
#include "dsp/position.h"
#include "dsp/sample_playback.h"
#include "utils/types.h"

#include <zix/sem.h>

enum class MetronomeType;
typedef struct SupportedFile SupportedFile;
typedef struct Tracklist     Tracklist;
typedef struct PluginSetting PluginSetting;
typedef struct MidiEvents    MidiEvents;
typedef struct ChordPreset   ChordPreset;
TYPEDEF_STRUCT (Graph);

/**
 * @addtogroup dsp
 *
 * @{
 */

#define SAMPLE_PROCESSOR (AUDIO_ENGINE->sample_processor)

#define sample_processor_is_in_active_project(self) \
  (self->audio_engine && engine_is_in_active_project (self->audio_engine))

/**
 * A processor to be used in the routing graph for
 * playing samples independent of the timeline.
 *
 * Also used for auditioning files.
 */
typedef struct SampleProcessor
{
  /** An array of samples currently being played. */
  SamplePlayback current_samples[256];
  int            num_current_samples;

  /** Tracklist for file auditioning. */
  Tracklist * tracklist;

  /** Instrument for MIDI auditioning. */
  PluginSetting * instrument_setting;

  MidiEvents * midi_events;

  /** Fader connected to the main output. */
  Fader * fader;

  /** Playhead for the tracklist (used when auditioning files). */
  Position playhead;

  /**
   * Position the file ends at.
   *
   * Once this position is reached,
   * SampleProcessor.roll will be set to false.
   */
  Position file_end_pos;

  /** Whether to roll or not. */
  bool roll;

  /** Pointer to owner audio engin, if any. */
  AudioEngine * audio_engine;

  /** Temp processing graph. */
  Graph * graph;

  /** Semaphore to be locked while rebuilding the sample processor tracklist and
   * graph. */
  ZixSem rebuilding_sem;
} SampleProcessor;

/**
 * Initializes a SamplePlayback with a sample to
 * play back.
 */
COLD WARN_UNUSED_RESULT SampleProcessor *
sample_processor_new (AudioEngine * engine);

COLD void
sample_processor_init_loaded (SampleProcessor * self, AudioEngine * engine);

/**
 * Clears the buffers.
 */
void
sample_processor_prepare_process (
  SampleProcessor * self,
  const nframes_t   nframes);

/**
 * Process the samples for the given number of frames.
 *
 * @param offset The local offset in the processing cycle.
 * @param nframes The number of frames to process in this call.
 */
void
sample_processor_process (
  SampleProcessor * self,
  const nframes_t   offset,
  const nframes_t   nframes);

/**
 * Removes a SamplePlayback from the array.
 */
void
sample_processor_remove_sample_playback (
  SampleProcessor * self,
  SamplePlayback *  sp);

/**
 * Queues a metronomem tick at the given offset.
 *
 * Used for countin.
 */
void
sample_processor_queue_metronome_countin (SampleProcessor * self);

/**
 * Queues a metronomem tick at the given local
 * offset.
 *
 * Realtime function.
 */
void
sample_processor_queue_metronome (
  SampleProcessor * self,
  MetronomeType     type,
  nframes_t         offset);

/**
 * Adds a sample to play to the queue from a file
 * path.
 */
void
sample_processor_queue_sample_from_file (
  SampleProcessor * self,
  const char *      path);

/**
 * Adds a file (audio or MIDI) to the queue.
 */
void
sample_processor_queue_file (SampleProcessor * self, const SupportedFile * file);

/**
 * Adds a chord preset to the queue.
 */
void
sample_processor_queue_chord_preset (
  SampleProcessor *   self,
  const ChordPreset * chord_pset);

/**
 * Stops playback of files (auditioning).
 */
void
sample_processor_stop_file_playback (SampleProcessor * self);

void
sample_processor_disconnect (SampleProcessor * self);

/**
 * To be used for serialization.
 */
SampleProcessor *
sample_processor_clone (const SampleProcessor * src);

void
sample_processor_free (SampleProcessor * self);

/**
 * @}
 */

#endif
