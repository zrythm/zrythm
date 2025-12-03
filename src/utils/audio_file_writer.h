// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "utils/types.h"
#include "utils/units.h"

#include <QPromise>

#include <juce_wrapper.h>

namespace zrythm::utils
{

class AudioFileWriter
{
public:
  struct WriteOptions
  {
    juce::AudioFormatWriterOptions writer_options_{};
    units::sample_t                block_length_ = units::samples (4096);
  };

  /**
   * @brief Executes write() asynchronously and returns a QFuture to control
   * the task.
   *
   * @param options Write options including writer options and block size
   * @param file_path Output file path
   * @param buffer Audio buffer to write (will be moved)
   */
  static QFuture<void> write_async (
    WriteOptions              options,
    const fs::path           &file_path,
    juce::AudioSampleBuffer &&buffer);

private:
  /**
   * @brief Writes the audio buffer to file.
   *
   * @param promise Promise for progress and cancellation
   * @param options Write options
   * @param file_path Output file path
   * @param buffer Audio buffer to write
   */
  static void write (
    QPromise<void>           &promise,
    WriteOptions              options,
    const fs::path           &file_path,
    juce::AudioSampleBuffer &&buffer);
};

} // namespace zrythm::utils
