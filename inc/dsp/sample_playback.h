// SPDX-FileCopyrightText: © 2019 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

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
 * @addtogroup dsp
 *
 * @{
 */

/**
 * A sample playback handle to be used by the engine.
 */
typedef struct SamplePlayback
{
  /** A pointer to the original buffer. */
  sample_t ** buf;

  /** The number of channels. */
  channels_t channels;

  /** The number of frames in the buffer. */
  unsigned_frame_t buf_size;

  /** The current offset in the buffer. */
  unsigned_frame_t offset;

  /** The volume to play the sample at (ratio from
   * 0.0 to 2.0, where 1.0 is the normal volume). */
  float volume;

  /** Offset relative to the current processing cycle
   * to start playing the sample. */
  nframes_t start_offset;

  /** Source file initialized from. */
  const char * file;

  /** Function initialized from. */
  const char * func;

  /** Line no initialized from. */
  int lineno;
} SamplePlayback;

/**
 * Initializes a SamplePlayback with a sample to
 * play back.
 */
#define sample_playback_init( \
  self, _buf, _buf_size, _channels, _vol, _start_offset) \
  if (_channels <= 0) \
    { \
      g_critical ("channels: %u", _channels); \
    } \
  (self)->buf = _buf; \
  (self)->buf_size = _buf_size; \
  (self)->volume = _vol; \
  (self)->offset = 0; \
  (self)->channels = _channels; \
  (self)->start_offset = _start_offset; \
  (self)->file = __FILE__; \
  (self)->func = __func__; \
  (self)->lineno = __LINE__

/**
 * @}
 */

#endif
