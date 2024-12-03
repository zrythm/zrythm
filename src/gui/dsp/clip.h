// SPDX-FileCopyrightText: © 2019-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * Audio clip.
 */

#ifndef __AUDIO_CLIP_H__
#define __AUDIO_CLIP_H__

#include "juce_wrapper.h"
#include "utils/audio.h"
#include "utils/audio_file.h"
#include "utils/hash.h"
#include "utils/icloneable.h"
#include "utils/iserializable.h"
#include "utils/types.h"

using namespace zrythm;

/**
 * Audio clips for the pool.
 *
 * These should be loaded in the project's sample rate.
 */
class AudioClip final
    : public zrythm::utils::serialization::ISerializable<AudioClip>,
      public ICloneable<AudioClip>
{
public:
  using BitDepth = zrythm::utils::audio::BitDepth;
  using AudioFile = zrythm::utils::audio::AudioFile;
  using PoolId = int;

public:
  /* Rule of 0 */
  AudioClip () = default;

  /**
   * Creates an audio clip from a file.
   *
   * The name used is the basename of the file.
   *
   * @throw ZrythmException on error.
   */
  AudioClip (const std::string &full_path);

  /**
   * Creates an audio clip by copying the given buffer.
   *
   * @param buf Buffer to copy.
   * @param name A name for this clip.
   */
  AudioClip (
    const utils::audio::AudioBuffer &buf,
    utils::audio::BitDepth           bit_depth,
    const std::string               &name);

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
    const std::string             &name)
      : AudioClip (
          *utils::audio::AudioBuffer::from_interleaved (arr, nframes, channels),
          bit_depth,
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
    channels_t         channels,
    unsigned_frame_t   nframes,
    const std::string &name);

public:
  static bool should_use_flac (zrythm::utils::audio::BitDepth bd)
  {
    return false;
    /* FLAC seems to fail writing sometimes so disable for now */
#if 0
  return bd < BIT_DEPTH_32;
#endif
  }

  void init_after_cloning (const AudioClip &other) override;

  /**
   * Inits after loading a Project.
   *
   * @throw ZrythmException on error.
   */
  void init_loaded ();

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
  void write_to_file (const std::string &filepath, bool parts);

  auto        get_pool_id () const { return pool_id_; }
  auto        get_bit_depth () const { return bit_depth_; }
  auto        get_name () const { return name_; }
  auto        get_file_hash () const { return file_hash_; }
  auto        get_bpm () const { return bpm_; }
  const auto &get_samples () const { return ch_frames_; }
  auto        get_last_write_to_file () const { return last_write_; }
  auto        get_use_flac () const { return use_flac_; }

  void set_name (const std::string &name) { name_ = name; }
  void set_pool_id (PoolId id) { pool_id_ = id; }

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

  /**
   * Writes the clip to the pool as a wav file.
   *
   * @param parts If true, only write new data. @see AudioClip.frames_written.
   * @param is_backup Whether writing to a backup project.
   *
   * @throw ZrythmException on error.
   */
  void write_to_pool (bool parts, bool is_backup);

  /**
   * Gets the path of a clip matching @ref name from the pool.
   *
   * @param use_flac Whether to look for a FLAC file instead of a wav file.
   * @param is_backup Whether writing to a backup project.
   */
  static std::string get_path_in_pool_from_name (
    const std::string &name,
    bool               use_flac,
    bool               is_backup);

  /**
   * Gets the path of the given clip from the pool.
   *
   * @param is_backup Whether writing to a backup project.
   */
  std::string get_path_in_pool (bool is_backup) const;

  /**
   * Returns whether the clip is used inside the project.
   *
   * @param check_undo_stack If true, this checks both project regions and the
   * undo stack. If false, this only checks actual project regions only.
   */
  bool is_in_use (bool check_undo_stack) const;

  /**
   * To be called by audio_pool_remove_clip().
   *
   * Removes the file associated with the clip.
   *
   * @param backup Whether to remove from backup directory.
   *
   * @throw ZrythmException If the file cannot be removed.
   */
  void remove (bool backup);

  auto get_num_channels () const { return ch_frames_.getNumChannels (); };
  auto get_num_frames () const { return ch_frames_.getNumSamples (); };

  /**
   * @brief Finalizes buffered write to a file (when `parts` is true in @ref
   * write_to_file()).
   */
  void finalize_buffered_write ();

  /**
   * @brief Used during tests to verify that the recorded file is valid.
   */
  bool verify_recorded_file (const fs::path &filepath) const;

  DECLARE_DEFINE_FIELDS_METHOD ();

private:
  /**
   * @brief Initializes members from an audio file.
   *
   * @param full_path Path to the file.
   * @param set_bpm Whether to set the BPM of the clip.
   *
   * @throw ZrythmException on I/O error.
   */
  void init_from_file (const std::string &full_path, bool set_bpm);

private:
  /** Name of the clip. */
  std::string name_;

  /**
   * Per-channel frames.
   */
  utils::audio::AudioBuffer ch_frames_;

  /**
   * BPM of the clip, or BPM of the project when the clip was first loaded.
   */
  bpm_t bpm_ = 0.f;

  /**
   * Samplerate of the clip, or samplerate when the clip was imported into the
   * project.
   */
  int samplerate_ = 0;

  /**
   * Bit depth of the clip when the clip was imported into the project.
   */
  utils::audio::BitDepth bit_depth_{};

  /** Whether the clip should use FLAC when being serialized. */
  bool use_flac_ = false;

  /** ID in the audio pool. */
  PoolId pool_id_{};

  /** File hash, used for checking if a clip is already written to the pool. */
  utils::hash::HashT file_hash_{};

  /**
   * Frames already written to the file, per channel.
   *
   * Used when writing in chunks/parts.
   */
  unsigned_frame_t frames_written_ = 0;

  /**
   * Time the last write took place.
   *
   * This is used so that we can write every x ms instead of all the time.
   *
   * @see AudioClip.frames_written.
   */
  std::uint64_t last_write_ = 0;

  /**
   * @brief Audio writer mainly to be used during recording to write data
   * continuously.
   */
  std::unique_ptr<juce::AudioFormatWriter> writer_;
  std::optional<fs::path>                  writer_path_;
};

/**
 * @}
 */

#endif
