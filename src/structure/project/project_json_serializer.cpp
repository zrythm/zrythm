// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/project/project.h"
#include "structure/project/project_json_schema.h"
#include "structure/project/project_json_serializer.h"
#include "utils/serialization.h"

#include <nlohmann/json-schema.hpp>

namespace zrythm::structure::project
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
          nlohmann::json::parse (kProjectSchemaJsonStr),
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
  const Project        &project,
  const utils::Version &app_version,
  std::string_view      title)
{
  nlohmann::json j;

  j[utils::serialization::kDocumentTypeKey] = DOCUMENT_TYPE;
  j[utils::serialization::kSchemaVersionKey] = SCHEMA_VERSION;
  j[utils::serialization::kAppVersionKey] = app_version;
  j[kDatetimeKey] = utils::datetime::get_current_as_iso8601_string ();
  j[kTitle] = title;
  j[kProjectData] = project;

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
ProjectJsonSerializer::deserialize (const nlohmann::json &j, Project &project)
{
  validate_json (j);

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

  // The actual deserialization is done by Project's friend from_json function
  // Data is nested under projectData key
  from_json (j.at (kProjectData), project);
}

}
