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
  const Project   &project,
  std::string_view app_version,
  std::string_view title)
{
  nlohmann::json j;

  j[utils::serialization::kDocumentTypeKey] = DOCUMENT_TYPE;
  j[utils::serialization::kFormatMajorKey] = FORMAT_MAJOR_VERSION;
  j[utils::serialization::kFormatMinorKey] = FORMAT_MINOR_VERSION;
  j[kAppVersionKey] = app_version;
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

  // The actual deserialization is done by Project's friend from_json function
  from_json (j, project);
}

}
