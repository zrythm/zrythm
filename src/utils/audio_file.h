// SPDX-FileCopyrightText: © 2023-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __IO_AUDIO_FILE_H__
#define __IO_AUDIO_FILE_H__

#include "zrythm-config.h"

#include <memory>
#include <utility>

#include "utils/audio.h"

namespace zrythm::utils::audio
{

struct AudioFileMetadata
{
  /* Rule of 0 */
  AudioFileMetadata () = default;

  AudioFileMetadata (const std::string &path);

  int samplerate = 0;
  int channels = 0;

  /** Milliseconds. */
  int64_t length = 0;

  /** Total number of frames (eg. a frame for 16bit stereo is
   * 4 bytes. */
  int64_t num_frames = 0;

  int bit_rate = 0;
  int bit_depth = 0;

  /** BPM if detected, or 0. */
  float bpm = 0.f;

  /** Whether metadata are already filled (valid). */
  bool filled = false;
};

/**
 * RAII class to read and write audio files (or their metadata).
 */
class AudioFile
{
public:
  AudioFile () = default;

  /**
   * @brief Creates a new instance of an AudioFile for the given path.
   *
   * @param filepath Path to the file.
   * @param for_writing Whether to create the file for writing.
   */
  AudioFile (fs::path filepath, bool for_writing = false)
      : filepath_ (std::move (filepath)), for_writing_ (for_writing)
  {
  }

  AudioFile (AudioFile &&other) noexcept = default;
  AudioFile &operator= (AudioFile &&other) noexcept = default;

  /**
   * Reads the metadata for the specified file.
   *
   * @throw ZrythmException on error.
   */
  AudioFileMetadata read_metadata ();

  /**
   * Reads the file into an internal float array (interleaved).
   *
   * @param samples Samples to fill in.
   * @param in_parts Whether to read the file in parts. If true,
   *   @p start_from and @p num_frames_to_read must be specified.
   * @param samples[out] Pre-allocated frame array. Caller must
   *   ensure there is enough space (ie, number of frames *
   *   number of channels).
   *
   * @throw ZrythmException on error.
   */
  void read_samples_interleaved (
    bool    in_parts,
    float * samples,
    size_t  start_from,
    size_t  num_frames_to_read);

  /**
   * Simple blocking API for reading and optionally resampling audio files.
   *
   * Only to be used on small files.
   *
   * @param[out] buffer Buffer to store the result to. Its internal buffers may
   * be re-allocated.
   * @param samplerate If specified, the audio will be resampled to the given
   * samplerate.
   *
   * @throw ZrythmException on error.
   */
  void read_full (
    zrythm::utils::audio::AudioBuffer &buffer,
    std::optional<size_t>              samplerate);

private:
  /**
   * Ensures there is an opened file.
   *
   * @throw ZrythmException on error.
   */
  void ensure_file_is_open ();

public:
  /** Absolute path. */
  fs::path filepath_;

  AudioFileMetadata metadata_;

  std::unique_ptr<juce::AudioFormatReader> reader_;
  std::unique_ptr<juce::AudioFormatWriter> writer_;

  bool for_writing_ = false;
};

}; // namespace zrythm::utils::audio

#endif
