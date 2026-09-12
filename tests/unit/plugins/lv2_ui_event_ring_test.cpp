// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <array>
#include <cstdint>
#include <cstring>
#include <vector>

#include "plugins/lv2_ui_event_ring.h"

#include <gtest/gtest.h>

namespace zrythm::plugins
{

namespace
{

struct TestRing
{
  static constexpr int kCapacity = 48;

  TestRing () : fifo (kCapacity), data (kCapacity) { }

  void advance (int bytes)
  {
    std::vector<std::byte> scratch (bytes);
    ASSERT_TRUE (fifo_write_record (
      fifo, data,
      { reinterpret_cast<const std::byte *> (scratch.data ()),
        static_cast<size_t> (bytes) }));
    scratch.assign (scratch.size (), std::byte{ 0 });
    ASSERT_TRUE (fifo_read_bytes (
      fifo, data, { scratch.data (), static_cast<size_t> (bytes) }));
  }

  juce::AbstractFifo     fifo;
  std::vector<std::byte> data;
};

std::vector<std::byte>
make_record (uint32_t port_index, uint32_t protocol, uint32_t body_size)
{
  const UiEventHeader    header{ port_index, protocol, body_size };
  std::vector<std::byte> bytes (sizeof (header) + body_size);
  std::memcpy (bytes.data (), &header, sizeof (header));
  for (size_t i = sizeof (header); i < bytes.size (); ++i)
    {
      bytes[i] = static_cast<std::byte> ((i * 31 + port_index) % 251);
    }
  return bytes;
}

bool
write_record (TestRing &ring, const std::vector<std::byte> &record)
{
  return fifo_write_record (
    ring.fifo, ring.data, { record.data (), sizeof (UiEventHeader) },
    { record.data () + sizeof (UiEventHeader),
      record.size () - sizeof (UiEventHeader) });
}

std::vector<std::byte>
read_record (TestRing &ring, uint32_t body_size)
{
  std::vector<std::byte> bytes (sizeof (UiEventHeader) + body_size);
  if (
    !fifo_read_bytes (
      ring.fifo, ring.data,
      { bytes.data (), static_cast<size_t> (bytes.size ()) }))
    return {};
  return bytes;
}

} // namespace

// A record written at any ring offset is read back byte-identical,
// including when the first write segment is longer than the record
// header (the body then wraps while the header fits entirely in the
// first segment)
TEST (Lv2UiEventRingTest, RecordsSurviveEveryWrapOffset)
{
  // Record sizes 12..44 bytes: every size fits the 48-byte ring at every
  // write offset, wrapping at different positions
  for (const auto body_size : { 0u, 4u, 16u, 32u })
    {
      for (int offset = 0; offset < TestRing::kCapacity; ++offset)
        {
          TestRing ring;
          SCOPED_TRACE (
            testing::Message () << "body " << body_size << " offset " << offset);
          ring.advance (offset);

          const auto record = make_record (7, 0x1234, body_size);
          ASSERT_TRUE (write_record (ring, record));
          EXPECT_EQ (read_record (ring, body_size), record);

          // The ring is empty again after the read
          std::vector<std::byte> leftover (1);
          EXPECT_FALSE (fifo_read_bytes (
            ring.fifo, ring.data, { leftover.data (), leftover.size () }));
        }
    }
}

// A record that does not fit the free space is rejected without
// touching the ring: the records already in the stream stay intact
// and in order
TEST (Lv2UiEventRingTest, RejectedWriteLeavesStreamUntouched)
{
  TestRing   ring;
  const auto first = make_record (1, 0, 4);
  const auto second = make_record (2, 0, 8);
  ASSERT_TRUE (write_record (ring, first));
  ASSERT_TRUE (write_record (ring, second));

  // 24 of 48 bytes are taken; this record needs 28
  const auto rejected = make_record (3, 0, 16);
  EXPECT_FALSE (write_record (ring, rejected));

  EXPECT_EQ (read_record (ring, 4), first);
  EXPECT_EQ (read_record (ring, 8), second);
  std::vector<std::byte> leftover (1);
  EXPECT_FALSE (fifo_read_bytes (
    ring.fifo, ring.data, { leftover.data (), leftover.size () }));
}

// A stream of records whose sizes do not divide the ring capacity
// keeps its contents and order across many wrap positions
TEST (Lv2UiEventRingTest, AlternatingSizesKeepOrderAcrossWraps)
{
  TestRing                            ring;
  std::vector<std::vector<std::byte>> written;
  for (uint32_t i = 0; i < 64; ++i)
    {
      const auto record = make_record (i, 0, (i % 2 == 0) ? 8u : 20u);
      if (!write_record (ring, record))
        break;
      written.push_back (record);
    }
  // The small ring cannot hold all of them
  ASSERT_LT (written.size (), 64u);

  for (const auto &record : written)
    {
      const auto * header =
        reinterpret_cast<const UiEventHeader *> (record.data ());
      EXPECT_EQ (read_record (ring, header->size), record);
    }
  std::vector<std::byte> leftover (1);
  EXPECT_FALSE (fifo_read_bytes (
    ring.fifo, ring.data, { leftover.data (), leftover.size () }));
}

} // namespace zrythm::plugins
