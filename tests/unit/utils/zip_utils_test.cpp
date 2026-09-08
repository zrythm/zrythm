// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <cstdint>
#include <cstring>
#include <filesystem>
#include <initializer_list>

#include "utils/exceptions.h"
#include "utils/zip_utils.h"

#include <gtest/gtest.h>
#include <juce_core/juce_core.h>

namespace zrythm::utils::zip_utils
{

namespace
{

Entry
make_entry (std::string path, QByteArray data = {})
{
  return Entry{ std::move (path), std::move (data) };
}

} // namespace

TEST (ZipUtilsTest, RoundTripsNestedEntriesWithBinaryData)
{
  const char raw[] = {
    '\x00', '\x01', '\x02', '\x7f', '\xff', 'h', 'e', 'l',   'l',
    'o',    '\x00', 'w',    'o',    'r',    'l', 'd', '\x7f'
  };
  const QByteArray binary{ raw, static_cast<qsizetype> (sizeof (raw)) };
  const auto       archive = create (
    {
      make_entry ("state.ttl", QByteArray ("<plugin>\n")),
      make_entry ("files/data.bin", binary),
      make_entry ("files/deep/nested/blob", binary),
    });

  const auto entries = extract (archive, 100, 1 << 20);
  ASSERT_EQ (entries.size (), 3u);
  EXPECT_EQ (entries[0].path_, "state.ttl");
  EXPECT_EQ (entries[0].data_, QByteArray ("<plugin>\n"));
  EXPECT_EQ (entries[1].path_, "files/data.bin");
  EXPECT_EQ (entries[1].data_, binary);
  EXPECT_EQ (entries[2].path_, "files/deep/nested/blob");
  EXPECT_EQ (entries[2].data_, binary);
}

TEST (ZipUtilsTest, CreateRejectsUnsafePaths)
{
  const auto expect_rejected = [] (const std::string &path) {
    EXPECT_THROW (
      { create ({ make_entry (path) }); }, ZrythmException)
      << "path: " << path;
  };

  expect_rejected ("");
  expect_rejected ("/absolute/path");
  // "C:/evil" only carries a root name on platforms with drive
  // prefixes; there it would replace the extraction destination
  if (std::filesystem::path ("C:/evil").has_root_name ())
    {
      expect_rejected ("C:/evil");
    }
  expect_rejected ("directory/");
  expect_rejected ("../traversal");
  expect_rejected ("nested/../traversal");
  expect_rejected ("./dot");
  expect_rejected ("double//slash");
  expect_rejected ("back\\slash");
}

TEST (ZipUtilsTest, ExtractRejectsNonArchiveInput)
{
  const QByteArray garbage{ "this is not an archive" };
  EXPECT_THROW ({ extract (garbage, 100, 1 << 20); }, ZrythmException);
}

namespace
{

// Builds a raw archive with JUCE directly, bypassing create()'s
// validation, so that extract()'s boundary checks can be exercised
QByteArray
make_unvalidated_archive (std::initializer_list<Entry> entries)
{
  juce::ZipFile::Builder builder;
  for (const auto &entry : entries)
    {
      builder.addEntry (
        std::make_unique<juce::MemoryInputStream> (
          entry.data_.constData (), static_cast<size_t> (entry.data_.size ()),
          false),
        0, juce::String::fromUTF8 (entry.path_.c_str ()),
        juce::Time::getCurrentTime ());
    }
  juce::MemoryOutputStream out;
  builder.writeToStream (out, nullptr);
  return {
    static_cast<const char *> (out.getData ()),
    static_cast<qsizetype> (out.getDataSize ())
  };
}

} // namespace

TEST (ZipUtilsTest, ExtractRejectsUnsafeEntryPaths)
{
  const auto expect_rejected = [] (const std::string &path) {
    const auto archive = make_unvalidated_archive ({ make_entry (path) });
    EXPECT_THROW (
      { extract (archive, 100, 1 << 20); }, ZrythmException)
      << "path: " << path;
  };

  expect_rejected ("");
  expect_rejected ("/absolute/path");
  if (std::filesystem::path ("C:/evil").has_root_name ())
    {
      expect_rejected ("C:/evil");
    }
  expect_rejected ("../traversal");
  expect_rejected ("nested/../traversal");
  expect_rejected ("./relative");
  expect_rejected ("nested//separator");
  expect_rejected ("directory/");
  expect_rejected ("back\\slash");
}

TEST (ZipUtilsTest, ExtractRejectsDuplicateEntryPaths)
{
  const auto archive = make_unvalidated_archive (
    { make_entry ("state.ttl", QByteArray ("a")),
      make_entry ("state.ttl", QByteArray ("b")) });
  EXPECT_THROW ({ extract (archive, 100, 1 << 20); }, ZrythmException);
}

// Paths differing only in ASCII case collide on case-insensitive
// filesystems, so both creation and extraction must reject them
TEST (ZipUtilsTest, RejectsCaseInsensitiveDuplicatePaths)
{
  EXPECT_THROW (
    {
      create (
        { make_entry ("Foo.bin", QByteArray ("a")),
          make_entry ("foo.bin", QByteArray ("b")) });
    },
    ZrythmException);

  const auto archive = make_unvalidated_archive (
    { make_entry ("Foo.bin", QByteArray ("a")),
      make_entry ("foo.bin", QByteArray ("b")) });
  EXPECT_THROW ({ extract (archive, 100, 1 << 20); }, ZrythmException);
}

// An entry whose data outgrows its declared uncompressed size must be
// rejected instead of silently truncated. The mismatch is produced by
// patching the central directory record of a stored (uncompressed)
// archive built with JUCE directly.
TEST (ZipUtilsTest, ExtractRejectsEntryWithUndersizedDeclaration)
{
  QByteArray archive =
    make_unvalidated_archive ({ make_entry ("a", QByteArray (4, 'x')) });

  // Central directory record: signature at 0x02014b50, uncompressed
  // size 24 bytes into the record
  const auto signature = QByteArray::fromHex ("504b0102");
  const auto record_offset = archive.indexOf (signature);
  ASSERT_GE (record_offset, 0);
  const auto     size_offset = record_offset + 24;
  const uint32_t patched_size = 3;
  memcpy (archive.data () + size_offset, &patched_size, sizeof (patched_size));

  EXPECT_THROW ({ extract (archive, 100, 1 << 20); }, ZrythmException);
}

TEST (ZipUtilsTest, ExtractEnforcesEntryCountCap)
{
  const auto archive = make_unvalidated_archive (
    { make_entry ("a", QByteArray ("x")), make_entry ("b", QByteArray ("x")) });
  EXPECT_NO_THROW ({ extract (archive, 2, 1 << 20); });
  EXPECT_THROW ({ extract (archive, 1, 1 << 20); }, ZrythmException);
}

TEST (ZipUtilsTest, ExtractEnforcesTotalSizeCap)
{
  const auto archive =
    make_unvalidated_archive ({ make_entry ("a", QByteArray (100, 'x')) });
  EXPECT_NO_THROW ({ extract (archive, 10, 100); });
  EXPECT_THROW ({ extract (archive, 10, 99); }, ZrythmException);
}

} // namespace zrythm::utils::zip_utils
