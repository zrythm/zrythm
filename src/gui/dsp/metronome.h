// SPDX-FileCopyrightText: © 2019-2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * Metronome related logic.
 */

#ifndef __AUDIO_METRONOME_H__
#define __AUDIO_METRONOME_H__

#include <filesystem>

#include "dsp/position.h"
#include "utils/types.h"

#include "juce_wrapper.h"

class AudioEngine;

/**
 * @addtogroup dsp
 *
 * @{
 */

#define METRONOME (AUDIO_ENGINE->metronome_)

/**
 * Metronome settings.
 */
class Metronome final
{
public:
  /**
   * The type of the metronome sound.
   */
  enum class Type
  {
    None,
    Emphasis,
    Normal,
  };

public:
  Metronome () = default;
  /**
   * Initializes the Metronome by loading the samples into memory.
   *
   * @throw ZrythmException if loading fails.
   */
  Metronome (AudioEngine &engine);

  using SampleBufferPtr = std::shared_ptr<zrythm::utils::audio::AudioBuffer>;

  void set_volume (float volume);

  /**
   * Queues metronome events (if any) within the current processing cycle.
   *
   * @param loffset Local offset in this cycle.
   */
  void queue_events (
    AudioEngine *   engine,
    const nframes_t loffset,
    const nframes_t nframes);

public:
  /** Absolute path of the "emphasis" sample. */
  fs::path emphasis_path_;

  /** Absolute path of the "normal" sample. */
  fs::path normal_path_;

  /** The emphasis sample. */
  SampleBufferPtr emphasis_;

  /** The normal sample. */
  SampleBufferPtr normal_;

  float volume_;
};

/**
 * @}
 */

#endif
