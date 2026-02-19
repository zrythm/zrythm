// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <filesystem>
#include <string>

#include "utils/utf8_string.h"

#include <nlohmann/json.hpp>

namespace zrythm::structure::project
{

/**
 * @brief Handles loading of Zrythm projects from disk.
 *
 * This class manages the complete project loading pipeline:
 * 1. Reading compressed project file
 * 2. Zstd decompression
 * 3. JSON parsing
 * 4. Schema validation
 * 5. Metadata extraction
 */
class ProjectLoader
{
public:
  /**
   * @brief Result of a project load operation.
   */
  struct LoadResult
  {
    nlohmann::json    json;
    utils::Utf8String title;
    fs::path          project_directory;
  };

  /**
   * @brief Loads and validates a project from the specified directory.
   *
   * @param project_dir The project directory containing project.zpj
   * @return LoadResult containing the parsed JSON and metadata.
   * @throw ZrythmException if loading fails.
   */
  static LoadResult load_from_directory (const fs::path &project_dir);

  /**
   * @brief Reads and decompresses the project file.
   *
   * @param project_dir The project directory.
   * @return The decompressed JSON string.
   * @throw ZrythmException if reading or decompression fails.
   */
  static std::string
  get_uncompressed_project_text (const fs::path &project_dir);

  /**
   * @brief Parses JSON string and validates against schema.
   *
   * @param json_str The raw JSON string.
   * @return Parsed and validated JSON object.
   * @throw ZrythmException if parsing or validation fails.
   */
  static nlohmann::json parse_and_validate (const std::string &json_str);

  /**
   * @brief Extracts the project title from JSON metadata.
   *
   * @param j The validated JSON object.
   * @return The project title, or "Untitled" if not found.
   */
  static utils::Utf8String extract_title (const nlohmann::json &j);
};

}
