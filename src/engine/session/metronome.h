// SPDX-FileCopyrightText: © 2019-2021, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <filesystem>

#include "utils/types.h"

#include "juce_wrapper.h"

namespace zrythm::engine::device_io
{
class AudioEngine;
}

#define METRONOME (AUDIO_ENGINE->metronome_)

namespace zrythm::engine::session
{
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
  Metronome (device_io::AudioEngine &engine);

  using SampleBufferPtr = std::shared_ptr<zrythm::utils::audio::AudioBuffer>;

  void set_volume (float volume);

  /**
   * Queues metronome events (if any) within the current processing cycle.
   *
   * @param loffset Local offset in this cycle.
   */
  void queue_events (
    device_io::AudioEngine * engine,
    nframes_t                loffset,
    nframes_t                nframes);

public:
  /** Absolute path of the "emphasis" sample. */
  fs::path emphasis_path_;

  /** Absolute path of the "normal" sample. */
  fs::path normal_path_;

  /** The emphasis sample. */
  SampleBufferPtr emphasis_;

  /** The normal sample. */
  SampleBufferPtr normal_;

  float volume_{};
};

}