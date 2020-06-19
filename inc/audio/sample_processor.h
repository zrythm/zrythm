/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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

#include "audio/sample_playback.h"
#include "audio/port.h"
#include "utils/types.h"

typedef struct StereoPorts StereoPorts;
typedef enum MetronomeType MetronomeType;

/**
 * @addtogroup audio
 *
 * @{
 */

#define SAMPLE_PROCESSOR \
  (AUDIO_ENGINE->sample_processor)

/**
 * A processor to be used in the routing graph for
 * playing samples independent of the timeline.
 */
typedef struct SampleProcessor
{
  /** An array of samples currently being played. */
  SamplePlayback    current_samples[256];
  int               num_current_samples;

  /** The stereo out ports to be connected to the
   * main output. */
  StereoPorts *     stereo_out;
} SampleProcessor;

static const cyaml_schema_field_t
sample_processor_fields_schema[] =
{
  YAML_FIELD_MAPPING_PTR (
    SampleProcessor, stereo_out,
    stereo_ports_fields_schema),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t
sample_processor_schema =
{
  CYAML_VALUE_MAPPING (
    CYAML_FLAG_POINTER,
    SampleProcessor,
    sample_processor_fields_schema),
};

/**
 * Initializes a SamplePlayback with a sample to
 * play back.
 */
SampleProcessor *
sample_processor_new (void);

void
sample_processor_init_loaded (
  SampleProcessor * self);

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
 * Queues a metronomem tick at the given local offset.
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

void
sample_processor_disconnect (
  SampleProcessor * self);

void
sample_processor_free (
  SampleProcessor * self);

/**
 * @}
 */

#endif
