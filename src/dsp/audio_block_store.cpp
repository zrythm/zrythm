// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>
#include <array>
#include <bit>
#include <cassert>
#include <fstream>
#include <limits>

#include <fmt/std.h>

#include "utils/exceptions.h"
#include "utils/logger.h"

#include <QCryptographicHash>

#include "audio_block_store.h"

using zrythm::utils::ZrythmException;

namespace zrythm::dsp
{

namespace
{

constexpr std::array<unsigned char, 6> kMagic = { 'Z', 'B', 'L', 'O', 'C', 'K' };
constexpr uint16_t kFormatVersion = 1;
constexpr size_t   kHeaderSize = 18;

void
put_u16 (unsigned char * dest, uint16_t value)
{
  dest[0] = static_cast<unsigned char> (value & 0xff);
  dest[1] = static_cast<unsigned char> ((value >> 8) & 0xff);
}

void
put_u32 (unsigned char * dest, uint32_t value)
{
  for (int byte = 0; byte < 4; ++byte)
    {
      dest[byte] = static_cast<unsigned char> ((value >> (8 * byte)) & 0xff);
    }
}

uint16_t
get_u16 (const unsigned char * src)
{
  return static_cast<uint16_t> (src[0]) | (static_cast<uint16_t> (src[1]) << 8);
}

uint32_t
get_u32 (const unsigned char * src)
{
  uint32_t value = 0;
  for (int byte = 0; byte < 4; ++byte)
    {
      value |= static_cast<uint32_t> (src[byte]) << (8 * byte);
    }
  return value;
}

std::array<unsigned char, kHeaderSize>
make_header (int channels, uint32_t sample_rate, uint32_t frame_count)
{
  std::array<unsigned char, kHeaderSize> header{};
  std::copy (kMagic.begin (), kMagic.end (), header.begin ());
  put_u16 (header.data () + 6, kFormatVersion);
  put_u16 (header.data () + 8, static_cast<uint16_t> (channels));
  put_u32 (header.data () + 10, sample_rate);
  put_u32 (header.data () + 14, frame_count);
  return header;
}

std::vector<unsigned char>
encode_payload (std::span<const float> samples)
{
  std::vector<unsigned char> bytes (samples.size () * sizeof (float));
  for (size_t i = 0; i < samples.size (); ++i)
    {
      const auto bits = std::bit_cast<uint32_t> (samples[i]);
      put_u32 (bytes.data () + i * sizeof (float), bits);
    }
  return bytes;
}

void
decode_payload (std::span<const unsigned char> bytes, std::vector<float> &dest)
{
  dest.resize (bytes.size () / sizeof (float));
  for (size_t i = 0; i < dest.size (); ++i)
    {
      const auto bits = get_u32 (bytes.data () + i * sizeof (float));
      dest[i] = std::bit_cast<float> (bits);
    }
}

} // namespace

bool
BlockId::is_valid () const
{
  if (hex.size () != 64)
    {
      return false;
    }
  return std::all_of (hex.begin (), hex.end (), [] (char character) {
    return (character >= '0' && character <= '9')
           || (character >= 'a' && character <= 'f');
  });
}

AudioBlockStore::AudioBlockStore (std::filesystem::path blocks_directory)
    : blocks_directory_ (std::move (blocks_directory))
{
  // An interrupted write leaves its temporary file behind; the garbage
  // collector only sweeps valid block names, so remove the leftovers here.
  std::error_code ec;
  for (
    auto it = std::filesystem::directory_iterator (blocks_directory_, ec);
    it != std::filesystem::directory_iterator (); ++it)
    {
      if (ec)
        {
          break;
        }
      const auto name = it->path ().filename ().string ();
      if (name.starts_with ('.') && name.ends_with (".tmp"))
        {
          std::filesystem::remove (it->path (), ec);
        }
    }
}

std::filesystem::path
AudioBlockStore::block_path (const BlockId &id) const
{
  return blocks_directory_ / id.hex;
}

void
AudioBlockStore::ensure_blocks_directory () const
{
  std::error_code ec;
  std::filesystem::create_directories (blocks_directory_, ec);
  if (ec)
    {
      throw ZrythmException (
        fmt::format (
          "Failed to create blocks directory '{}': {}", blocks_directory_,
          ec.message ()));
    }
}

BlockId
AudioBlockStore::write_block (
  const utils::audio::AudioBuffer &frames,
  units::sample_rate_t             sample_rate,
  units::sample_t                  start_frame,
  units::sample_t                  num_frames)
{
  const auto start_frame_raw = start_frame.in (units::samples);
  const auto num_frames_raw = num_frames.in (units::samples);

  const auto channels = frames.getNumChannels ();
  if (channels <= 0)
    {
      throw ZrythmException ("Block channel count must be positive");
    }
  if (num_frames_raw <= 0)
    {
      throw ZrythmException ("Block must contain at least one frame");
    }
  if (
    start_frame_raw < 0
    || start_frame_raw + num_frames_raw > frames.getNumSamples ())
    {
      throw ZrythmException ("Block frame range is outside the buffer");
    }
  const auto rate = sample_rate.in (units::sample_rate);
  if (rate <= 0)
    {
      throw ZrythmException (
        fmt::format ("Block sample rate {} is out of range", rate));
    }

  // Gather the frame range into the block's planar payload layout.
  const auto num_samples =
    static_cast<size_t> (num_frames_raw) * static_cast<size_t> (channels);
  std::vector<float> payload_samples (num_samples);
  for (int channel = 0; channel < channels; ++channel)
    {
      const float * src = frames.getReadPointer (channel) + start_frame_raw;
      std::copy (
        src, src + num_frames_raw,
        payload_samples.begin ()
          + static_cast<ptrdiff_t> (channel) * num_frames_raw);
    }

  const auto header = make_header (
    channels, static_cast<uint32_t> (rate),
    static_cast<uint32_t> (num_frames_raw));
  const auto payload = encode_payload (payload_samples);

  QCryptographicHash hash (QCryptographicHash::Sha256);
  hash.addData (
    reinterpret_cast<const char *> (header.data ()),
    static_cast<qsizetype> (header.size ()));
  hash.addData (
    reinterpret_cast<const char *> (payload.data ()),
    static_cast<qsizetype> (payload.size ()));
  const BlockId id{ hash.result ().toHex ().toStdString () };

  const std::lock_guard lock (mutex_);

  const auto      path = block_path (id);
  std::error_code ec;
  if (std::filesystem::exists (path, ec))
    {
      // A file with this name holds this block's bytes: a different size
      // means the file was truncated or corrupted.
      const auto expected_size =
        static_cast<std::uintmax_t> (header.size () + payload.size ());
      std::error_code size_ec;
      const auto      actual_size = std::filesystem::file_size (path, size_ec);
      if (size_ec || actual_size != expected_size)
        {
          throw ZrythmException (
            fmt::format (
              "Existing block file '{}' has an unexpected size (expected {}, "
              "found {})",
              path, expected_size, size_ec ? 0 : actual_size));
        }
      return id;
    }

  ensure_blocks_directory ();
  const auto temp_path =
    blocks_directory_ / fmt::format (".{}.{}.tmp", id.hex, temp_file_counter_++);
  {
    std::ofstream out (temp_path, std::ios::binary | std::ios::trunc);
    if (!out.is_open ())
      {
        throw ZrythmException (
          fmt::format ("Failed to open '{}' for writing", temp_path));
      }
    out.write (
      reinterpret_cast<const char *> (header.data ()),
      static_cast<std::streamsize> (header.size ()));
    out.write (
      reinterpret_cast<const char *> (payload.data ()),
      static_cast<std::streamsize> (payload.size ()));
    out.close ();
    if (out.fail ())
      {
        std::filesystem::remove (temp_path, ec);
        throw ZrythmException (
          fmt::format ("Failed to write block '{}'", temp_path));
      }
  }

  std::filesystem::rename (temp_path, path, ec);
  if (ec)
    {
      std::filesystem::remove (temp_path, ec);
      throw ZrythmException (
        fmt::format (
          "Failed to move block file '{}' to '{}': {}", temp_path, path,
          ec.message ()));
    }

  return id;
}

std::shared_ptr<const utils::audio::AudioBuffer>
AudioBlockStore::read_block (const BlockId &id)
{
  const std::lock_guard lock (mutex_);

  if (const auto it = cache_.find (id); it != cache_.end ())
    {
      lru_order_.erase (it->second.lru_position);
      lru_order_.push_front (id);
      it->second.lru_position = lru_order_.begin ();
      return it->second.data;
    }

  const auto    path = block_path (id);
  std::ifstream in (path, std::ios::binary);
  if (!in.is_open ())
    {
      throw ZrythmException (fmt::format ("Block '{}' not found", path));
    }

  std::array<unsigned char, kHeaderSize> header{};
  in.read (
    reinterpret_cast<char *> (header.data ()),
    static_cast<std::streamsize> (header.size ()));
  if (in.gcount () != static_cast<std::streamsize> (header.size ()))
    {
      throw ZrythmException (fmt::format ("Block '{}' is truncated", path));
    }

  if (!std::equal (kMagic.begin (), kMagic.end (), header.begin ()))
    {
      throw ZrythmException (
        fmt::format ("Block '{}' has an invalid magic value", path));
    }
  if (get_u16 (header.data () + 6) != kFormatVersion)
    {
      throw ZrythmException (
        fmt::format ("Block '{}' has an unsupported version", path));
    }
  const auto channels = get_u16 (header.data () + 8);
  const auto rate = get_u32 (header.data () + 10);
  const auto frame_count = get_u32 (header.data () + 14);
  if (channels == 0 || rate == 0 || frame_count == 0)
    {
      throw ZrythmException (
        fmt::format ("Block '{}' has an invalid header", path));
    }

  // The header comes from the file and its frame count is untrusted: cap
  // the payload by the bytes actually left in the file so a corrupt header
  // is rejected here instead of failing the allocation below.
  in.seekg (0, std::ios::end);
  const auto end_pos = in.tellg ();
  in.seekg (kHeaderSize, std::ios::beg);
  if (end_pos < 0)
    {
      throw ZrythmException (fmt::format ("Block '{}' is truncated", path));
    }
  const auto file_size = static_cast<std::uint64_t> (end_pos);
  const auto payload_size =
    static_cast<std::uint64_t> (frame_count) * channels * sizeof (float);
  if (
    file_size < kHeaderSize || payload_size > file_size - kHeaderSize
    || frame_count > static_cast<uint32_t> (std::numeric_limits<int>::max ()))
    {
      throw ZrythmException (fmt::format ("Block '{}' is truncated", path));
    }

  std::vector<unsigned char> payload (static_cast<size_t> (payload_size));
  in.read (
    reinterpret_cast<char *> (payload.data ()),
    static_cast<std::streamsize> (payload.size ()));
  if (in.gcount () != static_cast<std::streamsize> (payload.size ()))
    {
      throw ZrythmException (fmt::format ("Block '{}' is truncated", path));
    }

  std::vector<float> decoded;
  decode_payload (payload, decoded);
  auto data = std::make_shared<utils::audio::AudioBuffer> (
    channels, static_cast<int> (frame_count));
  for (int channel = 0; channel < channels; ++channel)
    {
      data->copyFrom (
        channel, 0,
        decoded.data () + static_cast<size_t> (channel) * frame_count,
        static_cast<int> (frame_count));
    }

  cache_bytes_ += static_cast<size_t> (payload_size);
  lru_order_.push_front (id);
  cache_[id] = CacheEntry{ data, lru_order_.begin () };

  evict_over_budget ();

  return data;
}

bool
AudioBlockStore::has_block (const BlockId &id) const
{
  std::error_code ec;
  return std::filesystem::exists (block_path (id), ec);
}

bool
AudioBlockStore::delete_block (const BlockId &id)
{
  const std::lock_guard lock (mutex_);

  if (const auto it = cache_.find (id); it != cache_.end ())
    {
      const auto &buffer = *it->second.data;
      cache_bytes_ -=
        static_cast<size_t> (buffer.getNumChannels ())
        * static_cast<size_t> (buffer.getNumSamples ()) * sizeof (float);
      lru_order_.erase (it->second.lru_position);
      cache_.erase (it);
    }

  std::error_code ec;
  const bool      removed = std::filesystem::remove (block_path (id), ec);
  if (ec)
    {
      throw ZrythmException (
        fmt::format (
          "Failed to delete block '{}': {}", block_path (id), ec.message ()));
    }
  return removed;
}

std::vector<BlockId>
AudioBlockStore::get_all_block_ids () const
{
  std::vector<BlockId> ids;
  std::error_code      ec;
  for (
    auto it = std::filesystem::directory_iterator (blocks_directory_, ec);
    it != std::filesystem::directory_iterator (); ++it)
    {
      if (!it->is_regular_file (ec))
        {
          continue;
        }
      BlockId id{ it->path ().filename ().string () };
      if (id.is_valid ())
        {
          ids.push_back (std::move (id));
        }
    }
  return ids;
}

const std::filesystem::path &
AudioBlockStore::get_blocks_directory () const
{
  return blocks_directory_;
}

void
AudioBlockStore::set_cache_budget_bytes (size_t bytes)
{
  const std::lock_guard lock (mutex_);
  cache_budget_bytes_ = bytes;
  evict_over_budget ();
}

size_t
AudioBlockStore::cached_block_count () const
{
  const std::lock_guard lock (mutex_);
  return cache_.size ();
}

size_t
AudioBlockStore::cached_bytes () const
{
  const std::lock_guard lock (mutex_);
  return cache_bytes_;
}

void
AudioBlockStore::evict_over_budget ()
{
  while (cache_bytes_ > cache_budget_bytes_ && !lru_order_.empty ())
    {
      const auto &oldest_id = lru_order_.back ();
      const auto  it = cache_.find (oldest_id);
      assert (it != cache_.end ());
      const auto &buffer = *it->second.data;
      cache_bytes_ -=
        static_cast<size_t> (buffer.getNumChannels ())
        * static_cast<size_t> (buffer.getNumSamples ()) * sizeof (float);
      cache_.erase (it);
      lru_order_.pop_back ();
    }
}

} // namespace zrythm::dsp
