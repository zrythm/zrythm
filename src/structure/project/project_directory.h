// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <filesystem>

namespace zrythm::structure::project
{

/**
 * @brief What a candidate project directory contains.
 */
enum class DirectoryContent
{
  /** The path does not exist. */
  NonExistent,
  /** The directory exists and holds no entries. */
  Empty,
  /** The directory holds a project file. */
  ContainsProjectFile,
  /**
   * The path cannot be adopted as a project directory: it holds
   * unrelated files, or it is not a directory.
   */
  OtherContent,
  /** The path could not be inspected, for example because of a missing
   * permission. */
  Unknown,
};

/**
 * @brief Reports what the given directory contains.
 *
 * @param dir Candidate project directory.
 * @return The directory's content class. A filesystem error during the
 * check maps to Unknown.
 */
[[nodiscard]] DirectoryContent
classify_directory (const std::filesystem::path &dir);

} // namespace zrythm::structure::project
