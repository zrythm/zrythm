// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "controllers/project_json_serializer.h"
#include "structure/project/project.h"
#include "structure/project/project_json_schema.h"
#include "structure/project/project_ui_state.h"
#include "undo/undo_stack.h"
#include "utils/serialization.h"

#include <nlohmann/json-schema.hpp>

namespace zrythm::controllers
{

namespace
{
/**
 * @brief Lazy-loaded schema validator singleton.
 */
class SchemaValidator
{
public:
  static SchemaValidator &get_instance ()
  {
    static SchemaValidator instance;
    return instance;
  }

  void validate (const nlohmann::json &json) { validator_.validate (json); }

private:
  SchemaValidator ()
      : validator_ (
          nlohmann::json::parse (structure::project::kProjectSchemaJsonStr),
          nullptr,
          nlohmann::json_schema::default_string_format_check,
          nullptr)
  {
    z_debug ("Project JSON schema validator initialized");
  }

  nlohmann::json_schema::json_validator validator_;
};
}

nlohmann::json
ProjectJsonSerializer::serialize (
  const structure::project::Project        &project,
  const structure::project::ProjectUiState &ui_state,
  const undo::UndoStack                    &undo_stack,
  const utils::Version                     &app_version,
  std::string_view                          title)
{
  nlohmann::json j;

  j[utils::serialization::kDocumentTypeKey] = DOCUMENT_TYPE;
  j[utils::serialization::kSchemaVersionKey] = SCHEMA_VERSION;
  j[utils::serialization::kAppVersionKey] = app_version;
  j[kDatetimeKey] = utils::datetime::get_current_as_iso8601_string ();
  j[kTitle] = title;
  j[kProjectData] = project;
  j[kUiState] = ui_state;
  j[kUndoHistory] = undo_stack;

  return j;
}

void
ProjectJsonSerializer::validate_json (const nlohmann::json &j)
{
  try
    {
      SchemaValidator::get_instance ().validate (j);
    }
  catch (const std::exception &e)
    {
      throw ZrythmException (
        fmt::format ("Project JSON validation failed: {}", e.what ()));
    }
}

void
ProjectJsonSerializer::deserialize (
  const nlohmann::json               &j,
  structure::project::Project        &project,
  structure::project::ProjectUiState &ui_state,
  undo::UndoStack                    &undo_stack)
{
  validate_json (j);

  try
    {
      // Check schema version compatibility
      utils::Version schema_version;
      from_json (j.at (utils::serialization::kSchemaVersionKey), schema_version);

      // Reject files from future major versions
      if (schema_version.major > SCHEMA_VERSION.major)
        {
          throw ZrythmException (
            fmt::format (
              "Project file is from a newer version (format {}.x) - please upgrade Zrythm",
              schema_version.major));
        }

      // Warn about older major versions (may need migration in the future)
      if (schema_version.major < SCHEMA_VERSION.major)
        {
          z_warning (
            "Project file is from an older format version ({}.{}) - migration may be needed",
            schema_version.major, schema_version.minor);
          // In the future, add migration logic here:
          // auto migrated_json = migrate_to_current_version(j);
          // from_json (migrated_json.at (kProjectData), project);
          // return;
        }

      // Deserialize core project data
      from_json (j.at (kProjectData), project);

      // Deserialize UI state (if present)
      if (j.contains (kUiState))
        {
          from_json (j.at (kUiState), ui_state);
        }

      // Deserialize undo history (if present)
      if (j.contains (kUndoHistory))
        {
          from_json (j.at (kUndoHistory), undo_stack);
        }
    }
  catch (const std::exception &e)
    {
      throw ZrythmException (
        fmt::format ("Project JSON deserialization failed: {}", e.what ()));
    }
}

}
