// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <span>

#include <juce_core/juce_core.h>

namespace zrythm::plugins
{

/**
 * @brief Header of a relay record in a UI event ring.
 *
 * @see fifo_write_record
 */
struct UiEventHeader
{
  uint32_t port_index;
  /** 0 for the float protocol, else a URID such as atom:eventTransfer. */
  uint32_t protocol;
  uint32_t size;
};
static_assert (sizeof (UiEventHeader) == 12);

/**
 * @brief Appends the two byte spans @p a and @p b as one contiguous record
 * to the byte stream of @p fifo (whose buffer holds @p data).
 *
 * @return False when the ring holds fewer free bytes than the record needs
 * (the ring is left untouched, so a partially written record can never
 * appear).
 */
inline bool
fifo_write_record (
  juce::AbstractFifo        &fifo,
  std::span<std::byte>       data,
  std::span<const std::byte> a,
  std::span<const std::byte> b = {}) noexcept
{
  const auto total_size = static_cast<int> (a.size () + b.size ());
  int        start1 = 0;
  int        length1 = 0;
  int        start2 = 0;
  int        length2 = 0;
  fifo.prepareToWrite (total_size, start1, length1, start2, length2);
  if (length1 + length2 < total_size)
    return false;

  const auto copy_segment =
    [&] (int fifo_start, int fifo_length, size_t source_offset) {
      // The first segment may have consumed the whole of a already
      const auto remaining_a =
        source_offset < a.size () ? a.size () - source_offset : 0uz;
      const auto first_from_a =
        std::min (static_cast<size_t> (fifo_length), remaining_a);
      if (first_from_a > 0)
        {
          std::memcpy (
            data.data () + fifo_start, a.data () + source_offset, first_from_a);
        }
      if (static_cast<size_t> (fifo_length) > first_from_a)
        {
          std::memcpy (
            data.data () + fifo_start + first_from_a,
            b.data () + (source_offset + first_from_a - a.size ()),
            static_cast<size_t> (fifo_length) - first_from_a);
        }
    };
  copy_segment (start1, length1, 0);
  if (length2 > 0)
    copy_segment (start2, length2, length1);
  fifo.finishedWrite (total_size);
  return true;
}

/**
 * @brief Reads @p bytes.size() bytes from the byte stream of @p fifo (whose
 * buffer holds @p data).
 *
 * @return False when the ring holds fewer bytes (the ring is left
 * untouched).
 */
inline bool
fifo_read_bytes (
  juce::AbstractFifo  &fifo,
  std::span<std::byte> data,
  std::span<std::byte> bytes) noexcept
{
  const auto total_size = static_cast<int> (bytes.size ());
  int        start1 = 0;
  int        length1 = 0;
  int        start2 = 0;
  int        length2 = 0;
  fifo.prepareToRead (total_size, start1, length1, start2, length2);
  if (length1 + length2 < total_size)
    return false;

  if (length1 > 0)
    {
      std::memcpy (
        bytes.data (), data.data () + start1, static_cast<size_t> (length1));
    }
  if (length2 > 0)
    {
      std::memcpy (
        bytes.data () + length1, data.data () + start2,
        static_cast<size_t> (length2));
    }
  fifo.finishedRead (total_size);
  return true;
}

} // namespace zrythm::plugins
