// SPDX-FileCopyrightText: © 2019, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * A framework from playing back samples independent
 * of the timeline, such as metronomes and samples
 * from the browser.
 */

#ifndef __AUDIO_SAMPLE_PLAYBACK_H__
#define __AUDIO_SAMPLE_PLAYBACK_H__

#include <utility>

#include "utils/types.h"

#include "ext/juce/juce.h"

/**
 * @addtogroup dsp
 *
 * @{
 */

/**
 * A sample playback handle to be used by the engine.
 */
struct SamplePlayback
{
public:
  SamplePlayback () = default;
  SamplePlayback (
    std::shared_ptr<juce::AudioSampleBuffer> buf,
    float                                    volume,
    nframes_t                                start_offset,
    const char *                             file,
    const char *                             func,
    int                                      lineno)
      : buf_ (std::move (buf)), volume_ (volume), start_offset_ (start_offset),
        file_ (file), func_ (func), lineno_ (lineno)
  {
  }
  /** A pointer to the original buffer. */
  std::shared_ptr<juce::AudioSampleBuffer> buf_;

  /** The number of channels. */
  // channels_t channels_ = 0;

  /** The number of frames in the buffer. */
  // unsigned_frame_t buf_size_ = 0;

  /** The current offset in the buffer. */
  unsigned_frame_t offset_ = 0;

  /** The volume to play the sample at (ratio from
   * 0.0 to 2.0, where 1.0 is the normal volume). */
  float volume_ = 1.0f;

  /** Offset relative to the current processing cycle
   * to start playing the sample. */
  nframes_t start_offset_ = 0;

  /** Source file initialized from. */
  const char * file_ = nullptr;

  /** Function initialized from. */
  const char * func_ = nullptr;

  /** Line no initialized from. */
  int lineno_ = 0;
};

/**
 * Initializes a SamplePlayback with a sample to play back.
 */
#define sample_playback_init( \
  self, _buf, _buf_size, _channels, _vol, _start_offset) \
  if (_channels <= 0) \
    { \
      z_error ("channels: %u", _channels); \
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
