// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <string_view>

namespace zrythm::structure::project
{

/**
 * @brief Compile-time embedded project JSON schema string.
 *
 * This is the JSON schema for project files, embedded at compile time
 * from data/schemas/project.schema.json.
 */
inline constexpr std::string_view kProjectSchemaJsonStr =
  R"schema(@SCHEMA_JSON@)schema";

}
