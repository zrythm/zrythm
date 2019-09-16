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
 * Metronome related logic.
 */

#ifndef __AUDIO_METRONOME_H__
#define __AUDIO_METRONOME_H__

#include "utils/types.h"

/**
 * @addtogroup audio
 *
 * @{
 */

#define METRONOME_SAMPLES_DIR \
  CONFIGURE_DATADIR "/zrythm/samples"
#define METRONOME (&ZRYTHM->metronome)

/**
 * The type of the metronome sound.
 */
typedef enum MetronomeType
{
  METRONOME_TYPE_NONE,
  METRONOME_TYPE_EMPHASIS,
  METRONOME_TYPE_NORMAL,
} MetronomeType;

/**
 * Metronome settings.
 */
typedef struct Metronome
{
  /** Absolute path of the "emphasis" sample. */
  char *     emphasis_path;

  /** Absolute path of the "normal" sample. */
  char *     normal_path;

  /** The emphasis sample. */
  float *    emphasis;

  /** Size per channel. */
  long       emphasis_size;

  channels_t emphasis_channels;

  /** The normal sample. */
  float *    normal;

  /** Size per channel. */
  long       normal_size;

  channels_t normal_channels;
} Metronome;

/**
 * Initializes the Metronome by loading the samples
 * into memory.
 */
void
metronome_init (
  Metronome * self);

/**
 * Fills the given frame buffer with metronome audio
 * based on the current position.
 *
 * @param buf The frame buffer.
 * @param g_start_frame The global start position in
 *   frames.
 * @param nframes Number of frames to fill. These must
 *   not exceed the buffer size.
 */
//void
//metronome_fill_buffer (
  //Metronome * self,
  //float *     buf,
  //const long  g_start_frame,
  //const int   nframes);

/**
 * @}
 */

#endif
