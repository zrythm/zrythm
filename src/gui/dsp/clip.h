// SPDX-FileCopyrightText: © 2019-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef ZRYTHM_DSP_AUDIO_CLIP_H
#define ZRYTHM_DSP_AUDIO_CLIP_H

#include "utils/audio.h"
#include "utils/audio_file.h"
#include "utils/hash.h"
#include "utils/icloneable.h"
#include "utils/iserializable.h"
#include "utils/monotonic_time_provider.h"
#include "utils/types.h"
#include "utils/uuid_identifiable_object.h"

using namespace zrythm;

/**
 * Audio clips for the pool.
 *
 * These should be loaded in the project's sample rate.
 */
class AudioClip final
    : public zrythm::utils::serialization::ISerializable<AudioClip>,
      public ICloneable<AudioClip>,
      public utils::UuidIdentifiableObject<AudioClip>,
      private utils::QElapsedTimeProvider
{
public:
  using BitDepth = zrythm::utils::audio::BitDepth;
  using AudioFile = zrythm::utils::audio::AudioFile;

public:
  AudioClip () = default;

  /**
   * Creates an audio clip from a file.
   *
   * The basename of the file will be used as the name of the clip.
   *
   * @param current_bpm Current BPM from TempoTrack. @ref bpm_ will be set to
   * this. FIXME: should this be optional? does "current" BPM make sense?
   *
   * @throw ZrythmException on error.
   */
  AudioClip (
    const fs::path &full_path,
    sample_rate_t   project_sample_rate,
    bpm_t           current_bpm);

  /**
   * Creates an audio clip by copying the given buffer.
   *
   * @param buf Buffer to copy.
   * @param name A name for this clip.
   */
  AudioClip (
    const utils::audio::AudioBuffer &buf,
    utils::audio::BitDepth           bit_depth,
    sample_rate_t                    project_sample_rate,
    bpm_t                            current_bpm,
    const utils::Utf8String         &name);

  /**
   * Creates an audio clip by copying the given interleaved float array.
   *
   * @param arr Interleaved array.
   * @param nframes Number of frames per channel.
   * @param channels Number of channels.
   * @param name A name for this clip.
   */
  AudioClip (
    const float *                  arr,
    unsigned_frame_t               nframes,
    channels_t                     channels,
    zrythm::utils::audio::BitDepth bit_depth,
    sample_rate_t                  project_sample_rate,
    bpm_t                          current_bpm,
    const utils::Utf8String       &name)
      : AudioClip (
          *utils::audio::AudioBuffer::from_interleaved (arr, nframes, channels),
          bit_depth,
          project_sample_rate,
          current_bpm,
          name)
  {
  }

  /**
   * Create an audio clip while recording.
   *
   * The frames will keep getting reallocated until the recording is
   * finished.
   *
   * @param nframes Number of frames to allocate. This should be the
   * current cycle's frames when called during recording.
   */
  AudioClip (
    channels_t               channels,
    unsigned_frame_t         nframes,
    sample_rate_t            project_sample_rate,
    bpm_t                    current_bpm,
    const utils::Utf8String &name);

public:
  static bool should_use_flac (zrythm::utils::audio::BitDepth bd)
  {
    return false;
    /* FLAC seems to fail writing sometimes so disable for now */
#if 0
  return bd < BIT_DEPTH_32;
#endif
  }

  void init_after_cloning (const AudioClip &other, ObjectCloneType clone_type)
    override;

  /**
   * Inits after loading a Project.
   *
   * @param full_path Full path to the corresponding audio file in the pool.
   * @throw ZrythmException on error.
   */
  void init_loaded (const fs::path &full_path);

  /**
   * Shows a dialog with info on how to edit a file, with an option to open an
   * app launcher.
   *
   * When the user closes the dialog, the clip is assumed to have been edited.
   *
   * The given audio clip will be free'd.
   *
   * @note This must not be used on pool clips.
   *
   * @return A new instance of AudioClip if successful, nullptr, if not.
   *
   * @throw ZrythmException on error.
   */
  std::unique_ptr<AudioClip> edit_in_ext_program ();

  /**
   * Writes the given audio clip data to a file.
   *
   * @param parts If true, only write new data. @see AudioClip.frames_written.
   *
   * @throw ZrythmException on error.
   */
  void write_to_file (const fs::path &filepath, bool parts);

  auto        get_bit_depth () const { return bit_depth_; }
  auto        get_name () const { return name_; }
  auto        get_file_hash () const { return file_hash_; }
  auto        get_bpm () const { return bpm_; }
  const auto &get_samples () const { return ch_frames_; }
  auto        get_last_write_to_file () const { return last_write_; }
  auto        get_use_flac () const { return use_flac_; }

  void set_name (const utils::Utf8String &name) { name_ = name; }
  void set_file_hash (utils::hash::HashT hash) { file_hash_ = hash; }

  /**
   * @brief Expands (appends to the end) the frames in the clip by the given
   * frames.
   *
   * @param frames Non-interleaved frames.
   */
  void expand_with_frames (const utils::audio::AudioBuffer &frames);

  /**
   * Replaces the clip's frames starting from @p start_frame with @p frames.
   *
   * @warning Not realtime safe.
   *
   * @param src_frames Frames to copy.
   * @param start_frame Frame to start copying to (@p src_frames are always
   * copied from the start).
   */
  void replace_frames (
    const utils::audio::AudioBuffer &src_frames,
    unsigned_frame_t                 start_frame);

  /**
   * Replaces the clip's frames starting from @p start_frame with @p frames.
   *
   * @warning Not realtime safe.
   *
   * @param frames Frames, interleaved.
   * @param start_frame Frame to start copying to (@p src_frames are always
   * copied from the start).
   */
  void replace_frames_from_interleaved (
    const float *    frames,
    unsigned_frame_t start_frame,
    unsigned_frame_t num_frames_per_channel,
    channels_t       channels);

  /**
   * @brief Unloads the clip's frames from memory.
   */
  void clear_frames ()
  {
    ch_frames_.setSize (ch_frames_.getNumChannels (), 0, false, true);
  }

  auto get_num_channels () const { return ch_frames_.getNumChannels (); };
  auto get_num_frames () const { return ch_frames_.getNumSamples (); };

  /**
   * @brief Finalizes buffered write to a file (when `parts` is true in @ref
   * write_to_file()).
   */
  void finalize_buffered_write ();

  /**
   * @brief Used during tests to verify that the recorded file is valid.
   *
   * @param current_bpm Current BPM from TempoTrack, used when creating a
   * temporary clip from the filepath.
   */
  bool verify_recorded_file (
    const fs::path &filepath,
    sample_rate_t   project_sample_rate,
    bpm_t           current_bpm) const;

  /**
   * @brief Returns whether enough time has elapsed since the last write to
   * file. This is used so that writing to file is done in chunks.
   */
  bool enough_time_elapsed_since_last_write () const;

  DECLARE_DEFINE_FIELDS_METHOD ();

private:
  /**
   * @brief Initializes members from an audio file.
   *
   * @param full_path Path to the file.
   * @param bpm_to_set BPM of the clip to set (optional - obtained from current
   * BPM in TempoTrack).
   *
   * @throw ZrythmException on I/O error.
   */
  void init_from_file (
    const fs::path      &full_path,
    sample_rate_t        project_sample_rate,
    std::optional<bpm_t> bpm_to_set);

private:
  /** Name of the clip. */
  utils::Utf8String name_;

  /**
   * Per-channel frames.
   */
  utils::audio::AudioBuffer ch_frames_;

  /**
   * BPM of the clip, or BPM of the project when the clip was first loaded.
   */
  bpm_t bpm_{};

  /**
   * Samplerate of the clip, or samplerate when the clip was imported into the
   * project.
   */
  sample_rate_t samplerate_{};

  /**
   * Bit depth of the clip when the clip was imported into the project.
   */
  utils::audio::BitDepth bit_depth_{};

  /** Whether the clip should use FLAC when being serialized. */
  bool use_flac_{ false };

  /** File hash, used for checking if a clip is already written to the pool. */
  utils::hash::HashT file_hash_{};

  /**
   * Frames already written to the file, per channel.
   *
   * Used when writing in chunks/parts.
   */
  unsigned_frame_t frames_written_{};

  /**
   * Time the last write took place.
   *
   * This is used so that we can write every x ms instead of all the time.
   *
   * @see AudioClip.frames_written.
   */
  utils::MonotonicTime last_write_{};

  /**
   * @brief Audio writer mainly to be used during recording to write data
   * continuously.
   */
  std::unique_ptr<juce::AudioFormatWriter> writer_;
  std::optional<fs::path>                  writer_path_;
};

using AudioClipResolverFunc =
  std::function<AudioClip *(const AudioClip::Uuid &clip_id)>;
using RegisterNewAudioClipFunc =
  std::function<void (std::shared_ptr<AudioClip>)>;

DEFINE_UUID_HASH_SPECIALIZATION (AudioClip::Uuid)

/**
 * @}
 */

#endif
