// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <filesystem>
#include <string>

#include "utils/utf8_string.h"

#include <nlohmann/json.hpp>

namespace zrythm::structure::project
{
class Project;
class ProjectUiState;
}

namespace zrythm::undo
{
class UndoStack;
}

namespace zrythm::controllers
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
 * 6. Deserialization of Project, ProjectUiState, and UndoStack
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

  /**
   * @brief Deserializes the project data into the given objects.
   *
   * @param j The JSON to deserialize from.
   * @param project The project instance to populate.
   * @param ui_state The UI state instance to populate.
   * @param undo_stack The undo stack instance to populate.
   * @throw ZrythmException on deserialization error.
   */
  static void deserialize (
    const nlohmann::json               &j,
    structure::project::Project        &project,
    structure::project::ProjectUiState &ui_state,
    undo::UndoStack                    &undo_stack);
};

}
