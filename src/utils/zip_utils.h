// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include <QByteArray>

namespace zrythm::utils::zip_utils
{

/**
 * @brief A single archived regular file.
 *
 * Entry paths are relative ('/'-separated, no leading slash, no ".",
 * ".." or empty components); directories are implied by nested paths.
 */
struct Entry
{
  /** Relative path of the file inside the archive. */
  std::string path_;

  /** File contents. */
  QByteArray data_;
};

/**
 * @brief Serializes @p entries into a compressed archive.
 *
 * @throw ZrythmException when an entry path cannot be represented
 * (empty, absolute, contains a backslash, a NUL byte, or a "."/".."
 * component).
 */
QByteArray
create (const std::vector<Entry> &entries);

/**
 * @brief Parses an archive produced by create().
 *
 * Untrusted input is rejected at the boundary: non-archive data,
 * absolute or traversal paths, directory or symlink entries, truncated
 * entries, and entry counts or total sizes beyond the given caps all
 * throw.
 *
 * @throw ZrythmException on any of the conditions above.
 */
std::vector<Entry>
extract (
  const QByteArray &archive_bytes,
  size_t            max_entries,
  size_t            max_total_size);

} // namespace zrythm::utils::zip_utils
