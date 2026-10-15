// SPDX-FileCopyrightText: © 2019-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <memory>
#include <shared_mutex>
#include <vector>

#include "dsp/audio_block_store.h"
#include "utils/audio.h"
#include "utils/audio_file.h"
#include "utils/icloneable.h"
#include "utils/typed_uuid_reference.h"
#include "utils/units.h"
#include "utils/utf8_string.h"
#include "utils/uuid_identifiable_object.h"

namespace zrythm::dsp
{

/**
 * Pooled audio media referenced by audio clips.
 *
 * A source is a manifest: an ordered list of block spans referencing
 * immutable, content-addressed blocks in an AudioBlockStore, plus a small
 * in-memory tail of frames not yet written to a block. Frames are read back
 * through read_span() and written to blocks as audio arrives (append_frames,
 * flush_tail).
 *
 * Sources are kept in the project's sample rate and stereo.
 */
class FileAudioSource final
    : public utils::UuidIdentifiableObject<FileAudioSource>
{
  Q_OBJECT

public:
  using BitDepth = zrythm::utils::audio::BitDepth;
  using AudioFile = zrythm::utils::audio::AudioFile;
  using channels_t = uint_fast8_t;

  /**
   * Maps a frame range of a block into this source's frame space.
   *
   * Spans are ordered by dest_start_frame and tile the source's frame space
   * contiguously from frame 0.
   */
  struct BlockSpan
  {
    BlockId         block;
    units::sample_t src_start_frame{};
    units::sample_t num_frames{};
    units::sample_t dest_start_frame{};
  };

public:
  /** Creates an empty source. Used by deserialization. */
  FileAudioSource (QObject * parent = nullptr);

  /**
   * Creates a source by decoding the given audio file.
   *
   * The audio is resampled to the project sample rate and mono files are
   * converted to stereo. The clip's source BPM (@ref bpm_) is initialized
   * from the file. The basename of the file is used as the clip name. The
   * frames are held in the in-memory tail until written to blocks.
   *
   * @throw ZrythmException on error.
   */
  FileAudioSource (
    const std::filesystem::path &full_path,
    units::sample_rate_t         project_sample_rate,
    QObject *                    parent = nullptr);

  /**
   * Creates a source by copying the given buffer.
   *
   * Mono buffers are converted to stereo. The frames are held in the
   * in-memory tail until written to blocks.
   *
   * @param source_bpm The clip's permanent source BPM. For material captured
   *                   in-project (e.g. recording) this is the project tempo at
   *                   capture time; for duplicated clips, the original's BPM.
   * @param name A name for this clip.
   */
  FileAudioSource (
    const utils::audio::AudioBuffer &buf,
    utils::audio::BitDepth           bit_depth,
    units::sample_rate_t             project_sample_rate,
    units::bpm_t                     source_bpm,
    const utils::Utf8String         &name,
    QObject *                        parent = nullptr);

  // ========================================================================
  // QML Interface
  // ========================================================================

  /**
   * @brief Emitted when the source samples change.
   */
  Q_SIGNAL void samplesChanged ();

  // ========================================================================

  auto get_bit_depth () const { return bit_depth_; }
  auto get_name () const
  {
    const std::shared_lock lock (mutex_);
    return name_;
  }
  /**
   * @brief The clip's permanent source BPM (its intrinsic musical tempo).
   *
   * Used to compute musical-mode stretch ratios. A value of 0 means
   * "unknown" — musical mode cannot be applied.
   *
   * @see bpm_ for how this is initialized.
   */
  units::sample_rate_t get_samplerate () const
  {
    const std::shared_lock lock (mutex_);
    return samplerate_;
  }
  units::bpm_t source_bpm () const { return bpm_; }

  void set_name (const utils::Utf8String &name)
  {
    const std::unique_lock lock (mutex_);
    name_ = name;
  }

  // ========================================================================
  // Manifest
  // ========================================================================

  /**
   * @brief Sets the store used for all block I/O.
   *
   * Required before appending frames, writing pending data or reading
   * spans. The store is shared with the pool: sources keep it alive even
   * when the pool rebuilds its own instance.
   */
  void set_store (std::shared_ptr<AudioBlockStore> store);

  /**
   * @brief Appends frames to the end of the source.
   *
   * Full blocks are written as they accumulate; the remaining partial
   * block stays in memory until flush_tail().
   */
  void append_frames (const utils::audio::AudioBuffer &frames);

  /**
   * @brief Writes all pending tail frames (including the partial block) to
   * the store.
   */
  void flush_tail ();

  /** Returns whether frames not yet written to a block exist. */
  bool has_pending_tail () const;

  /**
   * @brief Reads the frame range [start_frame, start_frame + num_frames)
   * into @p out.
   *
   * @param out Buffer to read into; must already be sized to num_frames
   * with the source's channel count.
   */
  void read_span (
    units::sample_t            start_frame,
    units::sample_t            num_frames,
    utils::audio::AudioBuffer &out) const;

  /**
   * @brief Replaces the whole content with @p frames at @p sample_rate,
   * written to blocks immediately.
   *
   * Empty @p frames clear the source to an empty one. Any blocks the
   * previous content referenced are left on disk for the garbage
   * collector.
   */
  void set_content (
    const utils::audio::AudioBuffer &frames,
    units::sample_rate_t             sample_rate);

  /**
   * The source's block spans, in frame order.
   */
  std::vector<BlockSpan> get_spans () const;

  int get_num_channels () const
  {
    const std::shared_lock lock (mutex_);
    return channels_;
  }
  [[nodiscard]] units::sample_t get_num_frames () const;

private:
  friend void init_from (
    FileAudioSource       &obj,
    const FileAudioSource &other,
    utils::ObjectCloneType clone_type);

  friend void to_json (nlohmann::json &j, const FileAudioSource &clip);
  friend void from_json (const nlohmann::json &j, FileAudioSource &clip);

  /** Writes whole blocks from the tail while enough frames are pending. */
  void flush_full_blocks ();

  /** Frame count of a full block at the source's channel count. */
  [[nodiscard]] units::sample_t block_frames () const;

  /** Appends a span covering @p num_frames of @p id's block and advances
   * @p dest_start_frame past it. */
  void append_block_span (
    const BlockId   &id,
    units::sample_t  num_frames,
    units::sample_t &dest_start_frame);

  [[nodiscard]] units::sample_t tail_frames () const;

  /** Total frames covered by the block spans. */
  [[nodiscard]] units::sample_t frames_in_spans () const;

  /** Total frames covered by spans plus pending tail frames. */
  [[nodiscard]] units::sample_t total_frames () const;

private:
  /** Name of the clip. */
  utils::Utf8String name_;

  /**
   * The clip's permanent source BPM — its intrinsic musical tempo.
   *
   * For file-loaded clips this comes from the file's metadata, or from tempo
   * estimation when the metadata carries none (0 if estimation fails). For
   * buffer-backed clips (recording, duplication) it is supplied by the
   * caller. Used to compute musical-mode stretch ratios; 0 means "unknown".
   *
   * @see source_bpm()
   */
  units::bpm_t bpm_{};

  /**
   * Samplerate of the clip, or samplerate when the clip was imported into the
   * project.
   */
  units::sample_rate_t samplerate_{};

  /**
   * Bit depth of the clip when the clip was imported into the project.
   * Round-trip metadata only: blocks always store float32 samples, so this
   * does not affect storage.
   */
  utils::audio::BitDepth bit_depth_{};

  /** Channel count of the source's audio. */
  int channels_ = 0;

  /** Block spans tiling the source's frame space from frame 0. */
  std::vector<BlockSpan> spans_;

  /** Frames not yet written to a block. */
  utils::audio::AudioBuffer tail_;

  /** Store used for all block I/O; shared with the owning pool. */
  std::shared_ptr<AudioBlockStore> store_;

  /**
   * Guards @ref spans_, @ref tail_, @ref samplerate_, @ref channels_ and
   * @ref store_: reads take it shared, appends and flushes take it unique.
   */
  mutable std::shared_mutex mutex_;
};

// ========================================================================

using FileAudioSourcePtrVariant = std::variant<FileAudioSource *>;
using FileAudioSourceUuidReference = utils::TypedUuidReference<FileAudioSource>;

} // namespace zrythm::dsp

DEFINE_UUID_HASH_SPECIALIZATION (zrythm::dsp::FileAudioSource::Uuid)
