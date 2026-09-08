// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <filesystem>
#include <unordered_set>

#include "utils/exceptions.h"
#include "utils/utf8_string.h"
#include "utils/zip_utils.h"

#include <fmt/format.h>
#include <juce_core/juce_core.h>

namespace zrythm::utils::zip_utils
{

namespace
{

// Compression level for created archives: high levels are slow for
// large, mostly incompressible state data on the calling (main)
// thread, while level 1 still compresses the TTL text well
constexpr int kCompressionLevel = 1;

// Lowercased ASCII form of @p path, used to detect paths that collide
// on case-insensitive filesystems
std::string
to_ascii_lower (std::string path)
{
  for (char &c : path)
    {
      if (c >= 'A' && c <= 'Z')
        {
          c = static_cast<char> (c - 'A' + 'a');
        }
    }
  return path;
}

void
validate_path (const std::string &path)
{
  const auto reject = [] (std::string_view reason) {
    throw ZrythmException (
      fmt::format ("Invalid archive entry path: {}", reason));
  };

  if (path.empty ())
    reject ("empty");
  if (path.front () == '/')
    reject ("absolute");
  // A Windows drive prefix (e.g. "C:/evil") replaces the whole path
  // when appended to a directory with operator/, escaping the
  // destination; on POSIX this never triggers
  if (std::filesystem::path (path).has_root_name ())
    reject ("root name");
  if (path.back () == '/')
    reject ("directory entry");
  if (path.find ('\0') != std::string::npos)
    reject ("embedded NUL byte");
  if (path.find ('\\') != std::string::npos)
    reject ("backslash");

  for (size_t start = 0; start <= path.size ();)
    {
      const auto end = path.find ('/', start);
      const auto component = std::string_view (path).substr (
        start, (end == std::string::npos ? path.size () : end) - start);
      if (component.empty ())
        reject ("empty path component");
      if (component == "." || component == "..")
        reject ("dot path component");
      if (end == std::string::npos)
        break;
      start = end + 1;
    }
}

} // namespace

QByteArray
create (const std::vector<Entry> &entries)
{
  juce::ZipFile::Builder builder;
  // Entry paths colliding case-insensitively would overwrite each
  // other when the archive is extracted on such a filesystem, so
  // they are rejected on creation
  std::unordered_set<std::string> seen_paths;
  for (const auto &entry : entries)
    {
      validate_path (entry.path_);
      if (!seen_paths.insert (to_ascii_lower (entry.path_)).second)
        {
          throw ZrythmException (
            fmt::format (
              "Duplicate entry path (case-insensitive): '{}'", entry.path_));
        }

      // The builder reads the stream during writeToStream(), so the
      // stream must borrow memory that outlives it: the caller's entry
      builder.addEntry (
        std::make_unique<juce::MemoryInputStream> (
          entry.data_.constData (), static_cast<size_t> (entry.data_.size ()),
          false),
        kCompressionLevel,
        utils::Utf8String::from_utf8_encoded_string (entry.path_)
          .to_juce_string (),
        juce::Time::getCurrentTime ());
    }

  juce::MemoryOutputStream out;
  if (!builder.writeToStream (out, nullptr))
    {
      throw ZrythmException ("Failed to write archive");
    }

  return {
    static_cast<const char *> (out.getData ()),
    static_cast<qsizetype> (out.getDataSize ())
  };
}

std::vector<Entry>
extract (
  const QByteArray &archive_bytes,
  size_t            max_entries,
  size_t            max_total_size)
{
  juce::MemoryInputStream stream{
    archive_bytes.constData (), static_cast<size_t> (archive_bytes.size ()), false
  };
  juce::ZipFile zip{ stream };

  const auto num_entries = zip.getNumEntries ();
  if (num_entries <= 0)
    {
      throw ZrythmException ("Archive contains no entries");
    }
  if (static_cast<size_t> (num_entries) > max_entries)
    {
      throw ZrythmException (
        fmt::format (
          "Archive holds {} entries; at most {} are allowed", num_entries,
          max_entries));
    }

  std::vector<Entry> entries;
  // Exact duplicate paths are rejected: extraction order would decide
  // which file survives, silently discarding the other's contents.
  // Paths colliding case-insensitively are rejected the same way,
  // since the archive may be extracted on a case-insensitive
  // filesystem
  std::unordered_set<std::string> seen_paths;
  size_t                          total_size = 0;
  for (int index = 0; index < num_entries; ++index)
    {
      const auto * zip_entry = zip.getEntry (index);
      if (zip_entry == nullptr)
        {
          throw ZrythmException ("Failed to read archive entry");
        }

      Entry entry;
      // Checked on the raw name: Utf8String::from_juce_string stops
      // at the first NUL, which would silently shorten the name
      // instead of rejecting it
      if (zip_entry->filename.containsChar (u'\0'))
        {
          throw ZrythmException (
            fmt::format ("Archive entry {} holds a NUL in its name", index));
        }
      entry.path_ =
        utils::Utf8String::from_juce_string (zip_entry->filename).str ();
      validate_path (entry.path_);
      if (!seen_paths.insert (to_ascii_lower (entry.path_)).second)
        {
          throw ZrythmException (
            fmt::format (
              "Archive contains a duplicate entry path (case-insensitive): '{}'",
              entry.path_));
        }
      if (zip_entry->isSymbolicLink)
        {
          throw ZrythmException (
            fmt::format (
              "Archive entry '{}' is a symlink; only regular files are "
              "supported",
              entry.path_));
        }

      const auto size = static_cast<size_t> (zip_entry->uncompressedSize);
      if (size > max_total_size - total_size)
        {
          throw ZrythmException (
            "Archive entries exceed the maximum total size");
        }

      std::unique_ptr<juce::InputStream> entry_stream{
        zip.createStreamForEntry (*zip_entry)
      };
      if (entry_stream == nullptr)
        {
          throw ZrythmException (
            fmt::format ("Failed to open archive entry '{}'", entry.path_));
        }

      juce::MemoryBlock block;
      // One byte more than the declared size is requested so that an
      // entry whose data outgrows its declared size is detected
      // instead of silently truncated
      entry_stream->readIntoMemoryBlock (
        block, static_cast<juce::int64> (size) + 1);
      if (block.getSize () > size)
        {
          throw ZrythmException (
            fmt::format (
              "Archive entry '{}' holds more data than declared", entry.path_));
        }
      if (block.getSize () != size)
        {
          throw ZrythmException (
            fmt::format ("Archive entry '{}' is truncated", entry.path_));
        }

      total_size += size;
      entry.data_ = QByteArray{
        static_cast<const char *> (block.getData ()),
        static_cast<qsizetype> (block.getSize ())
      };
      entries.push_back (std::move (entry));
    }

  return entries;
}

} // namespace zrythm::utils::zip_utils
