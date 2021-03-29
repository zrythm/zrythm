/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version approved by
 * Alexandros Theodotou in a public statement of acceptance.
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
 * A framework from playing back samples independent
 * of the timeline, such as metronomes and samples
 * from the browser.
 */

#ifndef __AUDIO_SAMPLE_PLAYBACK_H__
#define __AUDIO_SAMPLE_PLAYBACK_H__

#include "utils/types.h"

/**
 * @addtogroup audio
 *
 * @{
 */

/**
 * A sample playback handle to be used by the engine.
 */
typedef struct SamplePlayback
{
  /** A pointer to the original buffer. */
  sample_t **    buf;

  /** The number of channels. */
  channels_t     channels;

  /** The number of frames in the buffer. */
  long           buf_size;

  /** The current offset in the buffer. */
  long           offset;

  /** The volume to play the sample at (ratio from
   * 0.0 to 2.0, where 1.0 is the normal volume). */
  float          volume;

  /** Offset relative to the current processing cycle
   * to start playing the sample. */
  nframes_t      start_offset;
} SamplePlayback;

/**
 * Initializes a SamplePlayback with a sample to
 * play back.
 */
void
sample_playback_init (
  SamplePlayback * self,
  sample_t **      buf,
  long             buf_size,
  channels_t       channels,
  float            vol,
  nframes_t        start_offset);

/**
 * @}
 */

#endif
