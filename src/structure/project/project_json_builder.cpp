// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/project/project.h"
#include "structure/project/project_json_builder.h"
#include "utils/serialization.h"

namespace zrythm::structure::project
{

nlohmann::json
ProjectJsonBuilder::build_json (
  const Project   &project,
  std::string_view app_version)
{
  nlohmann::json j;

  j[utils::serialization::kDocumentTypeKey] = DOCUMENT_TYPE;
  j[utils::serialization::kFormatMajorKey] = FORMAT_MAJOR_VERSION;
  j[utils::serialization::kFormatMinorKey] = FORMAT_MINOR_VERSION;
  j[kAppVersionKey] = app_version;
  j[kDatetimeKey] = utils::datetime::get_current_as_string ();
  j[kProject] = project;

  return j;
}

}
