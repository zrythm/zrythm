// SPDX-FileCopyrightText: © 2019-2021 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Metronome related logic.
 */

#ifndef __AUDIO_METRONOME_H__
#define __AUDIO_METRONOME_H__

#include <stddef.h>

#include "utils/types.h"

typedef struct AudioEngine AudioEngine;

/**
 * @addtogroup dsp
 *
 * @{
 */

#define METRONOME (AUDIO_ENGINE->metronome)

/**
 * The type of the metronome sound.
 */
enum class MetronomeType
{
  METRONOME_TYPE_NONE,
  METRONOME_TYPE_EMPHASIS,
  METRONOME_TYPE_NORMAL,
};

/**
 * Metronome settings.
 */
typedef struct Metronome
{
  /** Absolute path of the "emphasis" sample. */
  char * emphasis_path;

  /** Absolute path of the "normal" sample. */
  char * normal_path;

  /** The emphasis sample. */
  float * emphasis;

  /** Size per channel. */
  size_t emphasis_size;

  channels_t emphasis_channels;

  /** The normal sample. */
  float * normal;

  /** Size per channel. */
  size_t normal_size;

  channels_t normal_channels;

  float volume;
} Metronome;

/**
 * Initializes the Metronome by loading the samples
 * into memory.
 */
Metronome *
metronome_new (void);

NONNULL void
metronome_set_volume (Metronome * self, float volume);

/**
 * Queues metronome events (if any) within the
 * current processing cycle.
 *
 * @param loffset Local offset in this cycle.
 */
NONNULL void
metronome_queue_events (
  AudioEngine *   self,
  const nframes_t loffset,
  const nframes_t nframes);

NONNULL void
metronome_free (Metronome * self);

/**
 * @}
 */

#endif
