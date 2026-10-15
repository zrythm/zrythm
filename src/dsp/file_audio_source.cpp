// SPDX-FileCopyrightText: © 2019-2022, 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <cassert>
#include <cstring>
#include <exception>
#include <limits>
#include <mutex>
#include <utility>

#include <fmt/std.h>

#include "dsp/file_audio_source.h"
#include "dsp/loop_tempo_estimator.h"
#include "dsp/panning.h"
#include "utils/audio.h"
#include "utils/audio_file.h"
#include "utils/debug.h"
#include "utils/exceptions.h"
#include "utils/float_ranges.h"
#include "utils/io_utils.h"
#include "utils/logger.h"
#include "utils/serialization.h"

#include <fmt/format.h>

using zrythm::utils::ZrythmException;

namespace zrythm::dsp
{

namespace
{

void
convert_mono_to_stereo (utils::audio::AudioBuffer &buf)
{
  if (buf.getNumChannels () != 1)
    {
      return;
    }

  const auto [left_gain, _] =
    calculate_panning (PanLaw::Minus3dB, PanAlgorithm::SquareRoot, 0.5f);
  const auto num_samples = buf.getNumSamples ();
  assert (num_samples >= 0);
  const auto samples = static_cast<size_t> (num_samples);

  buf.setSize (2, num_samples, true);

  auto * left = buf.getWritePointer (0);
  auto * right = buf.getWritePointer (1);

  utils::float_ranges::mul_k2 ({ left, samples }, left_gain);
  utils::float_ranges::copy ({ right, samples }, { left, samples });
}

} // namespace

FileAudioSource::FileAudioSource (QObject * parent)
    : utils::UuidIdentifiableObject<FileAudioSource> (parent)
{
}

FileAudioSource::FileAudioSource (
  const std::filesystem::path &full_path,
  units::sample_rate_t         project_sample_rate,
  QObject *                    parent)
    : utils::UuidIdentifiableObject<FileAudioSource> (parent)
{
  samplerate_ = project_sample_rate;
  assert (samplerate_ > units::sample_rate (0));

  /* read metadata */
  AudioFile                       file (full_path);
  utils::audio::AudioFileMetadata md;
  try
    {
      md = file.read_metadata ();
    }
  catch (const ZrythmException &)
    {
      std::throw_with_nested (ZrythmException (
        fmt::format ("Failed to read metadata from file '{}'", full_path)));
    }
  bit_depth_ = utils::audio::bit_depth_int_to_enum (md.bit_depth);
  bpm_ = units::bpm (md.bpm);

  utils::audio::AudioBuffer frames;
  try
    {
      /* read frames into project's samplerate */
      file.read_full (frames, samplerate_.in (units::sample_rate));
      convert_mono_to_stereo (frames);
    }
  catch (const ZrythmException &)
    {
      std::throw_with_nested (ZrythmException (
        fmt::format ("Failed to read frames from file '{}'", full_path)));
    }

  name_ = utils::Utf8String::from_path (
    utils::io::path_get_basename_without_ext (full_path));
  if (bpm_ <= units::bpm (0.0))
    {
      /* no BPM in metadata: attempt to estimate the tempo */
      if (const auto estimated_bpm = estimate_loop_bpm (frames, samplerate_))
        {
          z_debug (
            "estimated BPM of '{}' as {}", full_path,
            estimated_bpm->in (units::bpm));
          bpm_ = *estimated_bpm;
        }
    }

  channels_ = frames.getNumChannels ();
  tail_ = frames;

  Q_EMIT samplesChanged ();
}

FileAudioSource::FileAudioSource (
  const utils::audio::AudioBuffer &buf,
  BitDepth                         bit_depth,
  units::sample_rate_t             project_sample_rate,
  units::bpm_t                     source_bpm,
  const utils::Utf8String         &name,
  QObject *                        parent)
    : utils::UuidIdentifiableObject<FileAudioSource> (parent)
{
  if (buf.getNumSamples () == 0)
    {
      throw std::runtime_error ("Buffer is empty");
    }

  samplerate_ = project_sample_rate;
  assert (samplerate_ > units::sample_rate (0));
  name_ = name;
  bit_depth_ = bit_depth;

  utils::audio::AudioBuffer converted = buf;
  convert_mono_to_stereo (converted);
  channels_ = converted.getNumChannels ();
  tail_ = converted;
  bpm_ = source_bpm;
}

void
init_from (
  FileAudioSource       &obj,
  const FileAudioSource &other,
  utils::ObjectCloneType clone_type)
{
  init_from (
    static_cast<FileAudioSource::UuidIdentifiableObject &> (obj),
    static_cast<const FileAudioSource::UuidIdentifiableObject &> (other),
    clone_type);
  obj.name_ = other.name_;
  obj.bpm_ = other.bpm_;
  obj.samplerate_ = other.samplerate_;
  obj.bit_depth_ = other.bit_depth_;
  obj.channels_ = other.channels_;
  obj.spans_ = other.spans_;
  obj.tail_ = other.tail_;
  obj.store_ = other.store_;
}

// ========================================================================
// Manifest
// ========================================================================

void
FileAudioSource::set_store (std::shared_ptr<AudioBlockStore> store)
{
  const std::unique_lock lock (mutex_);
  store_ = std::move (store);
}

// The following frame-count readers require the caller to hold @ref
// mutex_ (shared suffices).

units::sample_t
FileAudioSource::tail_frames () const
{
  return units::samples (tail_.getNumSamples ());
}

units::sample_t
FileAudioSource::frames_in_spans () const
{
  auto total = units::samples (0);
  for (const auto &span : spans_)
    {
      total += span.num_frames;
    }
  return total;
}

units::sample_t
FileAudioSource::total_frames () const
{
  return frames_in_spans () + tail_frames ();
}

bool
FileAudioSource::has_pending_tail () const
{
  const std::shared_lock lock (mutex_);
  return tail_frames () > units::samples (0);
}

units::sample_t
FileAudioSource::get_num_frames () const
{
  const std::shared_lock lock (mutex_);
  return total_frames ();
}

std::vector<FileAudioSource::BlockSpan>
FileAudioSource::get_spans () const
{
  const std::shared_lock lock (mutex_);
  return spans_;
}

// The following writers all assume @ref mutex_ is held uniquely.

units::sample_t
FileAudioSource::block_frames () const
{
  z_return_val_if_fail (channels_ > 0, units::samples (0));
  return units::samples (
    static_cast<int64_t> (
      AudioBlockStore::kTargetBlockPayloadBytes
      / (static_cast<size_t> (channels_) * sizeof (float))));
}

void
FileAudioSource::append_block_span (
  const BlockId   &id,
  units::sample_t  num_frames,
  units::sample_t &dest_start_frame)
{
  spans_.push_back (
    BlockSpan{
      .block = id,
      .src_start_frame = units::samples (0),
      .num_frames = num_frames,
      .dest_start_frame = dest_start_frame });
  dest_start_frame += num_frames;
}

void
FileAudioSource::flush_full_blocks ()
{
  z_return_if_fail (store_ != nullptr);

  // Full blocks are written straight out of the tail at their offsets;
  // the unconsumed remainder is moved to the front once, after the loop.
  const auto block = block_frames ();
  z_return_if_fail (block > units::samples (0));
  const auto tail_total = tail_frames ();
  const auto num_blocks = static_cast<int64_t> (tail_total / block);
  if (num_blocks == 0)
    {
      return;
    }

  // All blocks are written before any span is appended: a failed write
  // leaves the manifest and the tail untouched (blocks already written
  // stay unreferenced on disk and are swept by the garbage collector).
  const auto           block_raw = block.in (units::samples);
  const auto           consumed = units::samples (num_blocks * block_raw);
  std::vector<BlockId> written (static_cast<size_t> (num_blocks));
  for (int64_t i = 0; i < num_blocks; ++i)
    {
      written[static_cast<size_t> (i)] = store_->write_block (
        tail_, samplerate_, units::samples (i * block_raw), block);
    }

  auto dest_start = frames_in_spans ();
  for (const auto &id : written)
    {
      append_block_span (id, block, dest_start);
    }

  const auto remaining = tail_total - consumed;
  const auto consumed_raw = consumed.in (units::samples);
  for (int channel = 0; channel < channels_; ++channel)
    {
      auto * channel_data = tail_.getWritePointer (channel);
      std::memmove (
        channel_data, channel_data + consumed_raw,
        static_cast<size_t> (remaining.in (units::samples)) * sizeof (float));
    }
  tail_.setSize (
    channels_, static_cast<int> (remaining.in (units::samples)), true, true,
    true);
}

void
FileAudioSource::append_frames (const utils::audio::AudioBuffer &frames)
{
  z_return_if_fail (frames.getNumChannels () == channels_);
  z_return_if_fail (frames.getNumSamples () > 0);

  {
    const std::unique_lock lock (mutex_);
    z_return_if_fail (store_ != nullptr);

    const auto num_frames = units::samples (frames.getNumSamples ());
    const auto old_frames = tail_frames ();
    tail_.setSize (
      channels_,
      static_cast<int> ((old_frames + num_frames).in (units::samples)), true,
      true, true);
    for (int channel = 0; channel < channels_; ++channel)
      {
        tail_.copyFrom (
          channel, static_cast<int> (old_frames.in (units::samples)),
          frames.getReadPointer (channel),
          static_cast<int> (num_frames.in (units::samples)));
      }

    flush_full_blocks ();
  }

  Q_EMIT samplesChanged ();
}

void
FileAudioSource::flush_tail ()
{
  {
    const std::unique_lock lock (mutex_);
    z_return_if_fail (store_ != nullptr);

    flush_full_blocks ();
    if (tail_frames () > units::samples (0))
      {
        auto       dest_start = frames_in_spans ();
        const auto num_frames = tail_frames ();
        const auto id = store_->write_block (
          tail_, samplerate_, units::samples (0), num_frames);
        append_block_span (id, num_frames, dest_start);
        tail_.setSize (channels_, 0, true, true, true);
      }
  }

  Q_EMIT samplesChanged ();
}

void
FileAudioSource::set_content (
  const utils::audio::AudioBuffer &frames,
  units::sample_rate_t             sample_rate)
{
  z_return_if_fail (frames.getNumChannels () == channels_);

  {
    const std::unique_lock lock (mutex_);
    z_return_if_fail (store_ != nullptr);

    // The replacement manifest is built first: a failed write leaves this
    // source's previous content untouched.
    const auto block = block_frames ();
    z_return_if_fail (block > units::samples (0));
    const auto block_raw = block.in (units::samples);
    const auto total = units::samples (frames.getNumSamples ());
    const auto num_blocks = static_cast<int64_t> (total / block);

    std::vector<BlockSpan> new_spans;
    units::sample_t        dest_start{};
    const auto             add_span =
      [&new_spans, &dest_start] (const BlockId &id, units::sample_t num_frames) {
        new_spans.push_back (
          BlockSpan{
            .block = id,
            .src_start_frame = units::samples (0),
            .num_frames = num_frames,
            .dest_start_frame = dest_start });
        dest_start += num_frames;
      };
    for (int64_t i = 0; i < num_blocks; ++i)
      {
        const auto id = store_->write_block (
          frames, sample_rate, units::samples (i * block_raw), block);
        add_span (id, block);
      }

    const auto remaining = total - units::samples (num_blocks * block_raw);
    if (remaining > units::samples (0))
      {
        const auto offset = units::samples (num_blocks * block_raw);
        const auto id =
          store_->write_block (frames, sample_rate, offset, remaining);
        add_span (id, remaining);
      }

    samplerate_ = sample_rate;
    spans_ = std::move (new_spans);
    tail_.setSize (channels_, 0, true, true, true);
  }

  Q_EMIT samplesChanged ();
}

void
FileAudioSource::replace_span (
  units::sample_t                  start_frame,
  units::sample_t                  old_num_frames,
  const utils::audio::AudioBuffer &frames)
{
  z_return_if_fail (frames.getNumChannels () == channels_);

  {
    const std::unique_lock lock (mutex_);
    z_return_if_fail (store_ != nullptr);
    z_return_if_fail (start_frame >= units::samples (0));
    z_return_if_fail (old_num_frames >= units::samples (0));
    z_return_if_fail (start_frame + old_num_frames <= frames_in_spans ());

    std::vector<BlockSpan> new_spans;
    units::sample_t        dest_start{};
    const auto             push_span =
      [&new_spans, &dest_start] (
        const BlockId &block, units::sample_t src_start,
        units::sample_t num_frames) {
        new_spans.push_back (
          BlockSpan{
            .block = block,
            .src_start_frame = src_start,
            .num_frames = num_frames,
            .dest_start_frame = dest_start });
        dest_start += num_frames;
      };

    const auto replaced_end = start_frame + old_num_frames;
    // The new content is written at block granularity once, between the
    // span preceding the range and the span following it. The replacement
    // content is re-blocked from the range's boundaries, so it dedupes
    // with existing blocks only when those boundaries are block-aligned.
    const auto insert_new = [&] {
      const auto block = block_frames ();
      const auto block_raw = block.in (units::samples);
      const auto total = units::samples (frames.getNumSamples ());
      const auto num_blocks = static_cast<int64_t> (total / block);
      for (int64_t i = 0; i < num_blocks; ++i)
        {
          const auto id = store_->write_block (
            frames, samplerate_, units::samples (i * block_raw), block);
          push_span (id, units::samples (0), block);
        }
      const auto remaining = total - units::samples (num_blocks * block_raw);
      if (remaining > units::samples (0))
        {
          const auto offset = units::samples (num_blocks * block_raw);
          const auto id =
            store_->write_block (frames, samplerate_, offset, remaining);
          push_span (id, units::samples (0), remaining);
        }
    };

    bool inserted = false;
    for (const auto &span : spans_)
      {
        const auto span_start = span.dest_start_frame;
        const auto span_end = span_start + span.num_frames;
        if (span_end <= start_frame)
          {
            push_span (span.block, span.src_start_frame, span.num_frames);
            continue;
          }
        if (span_start >= replaced_end)
          {
            if (!inserted)
              {
                insert_new ();
                inserted = true;
              }
            push_span (span.block, span.src_start_frame, span.num_frames);
            continue;
          }

        // The span overlaps the replaced range.
        if (span_start < start_frame)
          {
            push_span (
              span.block, span.src_start_frame, start_frame - span_start);
          }
        if (!inserted)
          {
            insert_new ();
            inserted = true;
          }
        if (span_end > replaced_end)
          {
            const auto skip = replaced_end - span_start;
            push_span (
              span.block, span.src_start_frame + skip, span_end - replaced_end);
          }
      }
    if (!inserted)
      {
        insert_new ();
      }

    spans_ = std::move (new_spans);
  }

  Q_EMIT samplesChanged ();
}

void
FileAudioSource::read_span (
  units::sample_t            start_frame,
  units::sample_t            num_frames,
  utils::audio::AudioBuffer &out) const
{
  const std::shared_lock lock (mutex_);

  z_return_if_fail (channels_ > 0);
  z_return_if_fail (
    start_frame >= units::samples (0) && num_frames >= units::samples (0));
  z_return_if_fail (start_frame + num_frames <= total_frames ());
  z_return_if_fail (out.getNumChannels () == channels_);
  z_return_if_fail (units::samples (out.getNumSamples ()) == num_frames);

  // block spans
  for (const auto &span : spans_)
    {
      const auto span_dest_end = span.dest_start_frame + span.num_frames;
      const auto overlap_start = max (start_frame, span.dest_start_frame);
      const auto overlap_end = min (start_frame + num_frames, span_dest_end);
      if (overlap_end <= overlap_start)
        {
          continue;
        }

      z_return_if_fail (store_ != nullptr);
      const auto block = store_->read_block (span.block);
      z_return_if_fail (block->getNumChannels () == channels_);
      const auto src_start =
        span.src_start_frame + (overlap_start - span.dest_start_frame);
      const auto len = overlap_end - overlap_start;
      z_return_if_fail (
        src_start + len
        <= units::samples (static_cast<int64_t> (block->getNumSamples ())));
      for (int channel = 0; channel < channels_; ++channel)
        {
          out.copyFrom (
            channel,
            static_cast<int> ((overlap_start - start_frame).in (units::samples)),
            *block, channel, static_cast<int> (src_start.in (units::samples)),
            static_cast<int> (len.in (units::samples)));
        }
    }

  // pending tail
  const auto tail_start = frames_in_spans ();
  const auto tail_read_start = max (start_frame, tail_start);
  const auto tail_read_end =
    min (start_frame + num_frames, tail_start + tail_frames ());
  if (tail_read_end > tail_read_start)
    {
      const auto len = tail_read_end - tail_read_start;
      for (int channel = 0; channel < channels_; ++channel)
        {
          out.copyFrom (
            channel,
            static_cast<int> (
              (tail_read_start - start_frame).in (units::samples)),
            tail_, channel,
            static_cast<int> ((tail_read_start - tail_start).in (units::samples)),
            static_cast<int> (len.in (units::samples)));
        }
    }
}

static constexpr auto kNameKey = "name"sv;
/**
 * Upper bound on the channel count a serialized source may declare.
 * Keep in sync with the manifest schema (data/schemas/common.schema.json).
 */
static constexpr int  kMaxChannels = 64;
static constexpr auto kBpmKey = "bpm"sv;
static constexpr auto kManifestKey = "manifest"sv;
static constexpr auto kChannelsKey = "channels"sv;
static constexpr auto kSampleRateKey = "sampleRate"sv;
static constexpr auto kBitDepthKey = "bitDepth"sv;
static constexpr auto kBlocksKey = "blocks"sv;
static constexpr auto kIdKey = "id"sv;
static constexpr auto kSrcStartKey = "srcStart"sv;
static constexpr auto kNumFramesKey = "numFrames"sv;
static constexpr auto kDestStartKey = "destStart"sv;

void
to_json (nlohmann::json &j, const FileAudioSource &clip)
{
  // The GUI thread can be mutating the manifest while a save task
  // serializes it.
  const std::shared_lock lock (clip.mutex_);

  if (clip.tail_frames () > units::samples (0))
    {
      z_warning (
        "serializing source '{}' with pending tail frames - they are not "
        "part of the serialized manifest",
        clip.name_.str ());
    }

  to_json (
    j, static_cast<const FileAudioSource::UuidIdentifiableObject &> (clip));
  j[kNameKey] = clip.name_;
  j[kBpmKey] = clip.bpm_;

  auto blocks = nlohmann::json::array ();
  for (const auto &span : clip.spans_)
    {
      blocks.push_back (
        nlohmann::json{
          { kIdKey,        span.block.hex                            },
          { kSrcStartKey,  span.src_start_frame.in (units::samples)  },
          { kNumFramesKey, span.num_frames.in (units::samples)       },
          { kDestStartKey, span.dest_start_frame.in (units::samples) }
      });
    }
  j[kManifestKey] = nlohmann::json{
    { kChannelsKey,   clip.channels_                                        },
    { kSampleRateKey, clip.samplerate_.in (units::sample_rate)              },
    { kBitDepthKey,   utils::audio::bit_depth_enum_to_int (clip.bit_depth_) },
    { kBlocksKey,     std::move (blocks)                                    }
  };
}

void
from_json (const nlohmann::json &j, FileAudioSource &clip)
{
  from_json (j, static_cast<FileAudioSource::UuidIdentifiableObject &> (clip));
  j.at (kNameKey).get_to (clip.name_);
  j.at (kBpmKey).get_to (clip.bpm_);

  const auto &manifest = j.at (kManifestKey);
  clip.channels_ = manifest.at (kChannelsKey).get<int> ();
  if (clip.channels_ < 1 || clip.channels_ > kMaxChannels)
    {
      throw ZrythmException (
        fmt::format (
          "Source '{}' declares an invalid channel count {}", clip.name_,
          clip.channels_));
    }
  clip.samplerate_ =
    units::sample_rate (manifest.at (kSampleRateKey).get<int> ());
  if (clip.samplerate_ < units::sample_rate (1))
    {
      throw ZrythmException (
        fmt::format (
          "Source '{}' declares an invalid sample rate {}", clip.name_,
          clip.samplerate_.in (units::sample_rate)));
    }
  clip.bit_depth_ = utils::audio::bit_depth_int_to_enum (
    manifest.at (kBitDepthKey).get<int> ());

  clip.spans_.clear ();
  units::sample_t expected_dest_start = units::samples (0);
  const auto     &blocks = manifest.at (kBlocksKey);
  for (const auto &block : blocks)
    {
      FileAudioSource::BlockSpan span;
      span.block = BlockId{ block.at (kIdKey).get<std::string> () };
      span.src_start_frame =
        units::samples (block.at (kSrcStartKey).get<int64_t> ());
      span.num_frames =
        units::samples (block.at (kNumFramesKey).get<int64_t> ());
      span.dest_start_frame =
        units::samples (block.at (kDestStartKey).get<int64_t> ());
      if (!span.block.is_valid ())
        {
          throw ZrythmException (
            fmt::format (
              "Source '{}' references block '{}' with an invalid id",
              clip.name_, span.block.hex));
        }
      if (span.src_start_frame < units::samples (0))
        {
          throw ZrythmException (
            fmt::format (
              "Source '{}' has a block span starting at a negative frame "
              "inside the block",
              clip.name_));
        }
      if (span.num_frames < units::samples (1))
        {
          throw ZrythmException (
            fmt::format (
              "Source '{}' has a block span with no frames starting at {}",
              clip.name_, span.dest_start_frame.in (units::samples)));
        }
      if (span.dest_start_frame != expected_dest_start)
        {
          throw ZrythmException (
            fmt::format (
              "Source '{}' has non-contiguous block spans (expected destination "
              "start {}, found {})",
              clip.name_, expected_dest_start.in (units::samples),
              span.dest_start_frame.in (units::samples)));
        }
      expected_dest_start += span.num_frames;
      clip.spans_.push_back (std::move (span));
    }
}

} // namespace zrythm::dsp
