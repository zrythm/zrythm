// SPDX-FileCopyrightText: © 2019-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * Audio clip.
 */

#ifndef __AUDIO_CLIP_H__
#define __AUDIO_CLIP_H__

#include "io/serialization/iserializable.h"
#include "utils/audio.h"
#include "utils/icloneable.h"
#include "utils/types.h"

#include "ext/juce/juce.h"

/**
 * @addtogroup dsp
 *
 * @{
 */

/**
 * Audio clips for the pool.
 *
 * These should be loaded in the project's sample rate.
 */
class AudioClip final
    : public ISerializable<AudioClip>,
      public ICloneable<AudioClip>
{
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
   * Creates an audio clip by copying the given float array.
   *
   * @param arr Interleaved array.
   * @param name A name for this clip.
   */
  AudioClip (
    const float *          arr,
    const unsigned_frame_t nframes,
    const channels_t       channels,
    BitDepth               bit_depth,
    const std::string     &name);

  /**
   * Create an audio clip while recording.
   *
   * The frames will keep getting reallocated until the recording is finished.
   *
   * @param nframes Number of frames to allocate. This should be the current
   * cycle's frames when called during recording.
   */
  AudioClip (
    const channels_t       channels,
    const unsigned_frame_t nframes,
    const std::string     &name);

public:
  static bool use_flac (BitDepth bd)
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
   * Updates the channel caches.
   *
   * See @ref AudioClip.ch_frames.
   *
   * @param start_from Frames to start from (per channel). The previous frames
   * will be kept.
   */
  void update_channel_caches (size_t start_from);

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

  auto get_num_channels () const { return channels_; };
  auto get_num_frames () const { return num_frames_; };

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

public:
  /** Name of the clip. */
  std::string name_;

  /** The audio frames, interleaved. */
  juce::AudioBuffer<sample_t> frames_;

  /**
   * Per-channel frames for convenience.
   */
  juce::AudioBuffer<sample_t> ch_frames_;

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
  BitDepth bit_depth_;

  /** Whether the clip should use FLAC when being serialized. */
  bool use_flac_ = false;

  /** ID in the audio pool. */
  int pool_id_ = 0;

  /** File hash, used for checking if a clip is already written to the pool. */
  std::string file_hash_;

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
  gint64 last_write_ = 0;

  /** Number of frames per channel. FIXME this might not be needed since we have
   * ch_frames_ */
  unsigned_frame_t num_frames_ = 0;

  /** Number of channels. FIXME this might not be needed since we have
   * ch_frames_ */
  channels_t channels_ = 0;

private:
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
