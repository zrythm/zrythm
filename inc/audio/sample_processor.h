/*
 * Copyright (C) 2019-2022 Alexandros Theodotou <alex at zrythm dot org>
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
 * Sample processor.
 */

#ifndef __AUDIO_SAMPLE_PROCESSOR_H__
#define __AUDIO_SAMPLE_PROCESSOR_H__

#include "audio/fader.h"
#include "audio/port.h"
#include "audio/sample_playback.h"
#include "utils/types.h"

typedef enum MetronomeType MetronomeType;
typedef struct SupportedFile SupportedFile;
typedef struct Tracklist Tracklist;
typedef struct PluginSetting PluginSetting;
typedef struct MidiEvents MidiEvents;
typedef struct ChordPreset ChordPreset;

/**
 * @addtogroup audio
 *
 * @{
 */

#define SAMPLE_PROCESSOR_SCHEMA_VERSION 1

#define SAMPLE_PROCESSOR \
  (AUDIO_ENGINE->sample_processor)

#define sample_processor_is_in_active_project(self) \
  (self->audio_engine \
   && \
   engine_is_in_active_project (self->audio_engine))

/**
 * A processor to be used in the routing graph for
 * playing samples independent of the timeline.
 *
 * Also used for auditioning files.
 */
typedef struct SampleProcessor
{
  int               schema_version;

  /** An array of samples currently being played. */
  SamplePlayback    current_samples[256];
  int               num_current_samples;

  /** Tracklist for file auditioning. */
  Tracklist *       tracklist;

  /** Instrument for MIDI auditioning. */
  PluginSetting *   instrument_setting;

  MidiEvents *      midi_events;

  /** Fader connected to the main output. */
  Fader *           fader;

  /** Playhead for the tracklist (used when
   * auditioning files). */
  Position          playhead;

  /**
   * Position the file ends at.
   *
   * Once this position is reached,
   * SampleProcessor.roll will be set to false.
   */
  Position          file_end_pos;

  /** Whether to roll or not. */
  bool              roll;

  /** Pointer to owner audio engin, if any. */
  AudioEngine *     audio_engine;
} SampleProcessor;

static const cyaml_schema_field_t
sample_processor_fields_schema[] =
{
  YAML_FIELD_INT (
    SampleProcessor, schema_version),
  YAML_FIELD_MAPPING_PTR (
    SampleProcessor, fader,
    fader_fields_schema),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t
sample_processor_schema =
{
  YAML_VALUE_PTR (
    SampleProcessor,
    sample_processor_fields_schema),
};

/**
 * Initializes a SamplePlayback with a sample to
 * play back.
 */
COLD
WARN_UNUSED_RESULT
SampleProcessor *
sample_processor_new (
  AudioEngine * engine);

COLD
void
sample_processor_init_loaded (
  SampleProcessor * self,
  AudioEngine *     engine);

/**
 * Clears the buffers.
 */
void
sample_processor_prepare_process (
  SampleProcessor * self,
  const nframes_t   nframes);

/**
 * Process the samples for the given number of
 * frames.
 *
 * @param offset The local offset in the processing
 *   cycle.
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
sample_processor_queue_metronome_countin (
  SampleProcessor * self);

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
sample_processor_queue_file (
  SampleProcessor *     self,
  const SupportedFile * file);

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
sample_processor_stop_file_playback (
  SampleProcessor *     self);

void
sample_processor_disconnect (
  SampleProcessor * self);

/**
 * To be used for serialization.
 */
SampleProcessor *
sample_processor_clone (
  const SampleProcessor * src);

void
sample_processor_free (
  SampleProcessor * self);

/**
 * @}
 */

#endif
