// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "controllers/project_json_serializer.h"
#include "controllers/project_loader.h"
#include "controllers/project_saver.h"
#include "structure/project/project_path_provider.h"
#include "structure/project/project_ui_state.h"
#include "undo/undo_stack.h"
#include "utils/io_utils.h"

#include <nlohmann/json.hpp>

namespace zrythm::controllers
{

std::string
ProjectLoader::get_uncompressed_project_text (const fs::path &project_dir)
{
  return ProjectSaver::get_existing_uncompressed_text (project_dir);
}

nlohmann::json
ProjectLoader::parse_and_validate (const std::string &json_str)
{
  nlohmann::json j;
  try
    {
      j = nlohmann::json::parse (json_str);
    }
  catch (const nlohmann::json::parse_error &e)
    {
      throw ZrythmException (
        fmt::format ("Failed to parse project JSON: {}", e.what ()));
    }

  // Validate against schema
  ProjectJsonSerializer::validate_json (j);

  return j;
}

utils::Utf8String
ProjectLoader::extract_title (const nlohmann::json &j)
{
  if (j.contains (ProjectJsonSerializer::kTitle))
    {
      return utils::Utf8String::from_utf8_encoded_string (
        j[ProjectJsonSerializer::kTitle].get<std::string> ());
    }
  return utils::Utf8String::from_utf8_encoded_string ("Untitled");
}

ProjectLoader::LoadResult
ProjectLoader::load_from_directory (const fs::path &project_dir)
{
  z_info ("Loading project from {}", project_dir);

  // 1. Verify directory exists
  if (!fs::is_directory (project_dir))
    {
      throw ZrythmException (
        fmt::format ("Project directory does not exist: {}", project_dir));
    }

  // 2. Read and decompress
  auto json_str = get_uncompressed_project_text (project_dir);

  // 3. Parse and validate
  auto j = parse_and_validate (json_str);

  // 4. Extract metadata
  auto title = extract_title (j);

  return LoadResult{
    .json = std::move (j),
    .title = std::move (title),
    .project_directory = project_dir
  };
}

void
ProjectLoader::deserialize (
  const nlohmann::json               &j,
  structure::project::Project        &project,
  structure::project::ProjectUiState &ui_state,
  undo::UndoStack                    &undo_stack)
{
  ProjectJsonSerializer::deserialize (j, project, ui_state, undo_stack);
}

}
