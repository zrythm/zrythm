// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <string_view>

namespace zrythm::structure::project
{

/**
 * @brief Compile-time embedded project JSON schema as hex byte array.
 *
 * This is the JSON schema for project files, embedded at compile time
 * from data/schemas/project.schema.json.
 */
inline constexpr char kProjectSchemaJsonBytes[]
{
  // clang-format off
  @SCHEMA_JSON_HEX_BYTES@
};

/**
 * @brief String view of the embedded JSON schema.
 *
 * Provides a convenient string_view over the hex byte array.
 */
inline constexpr std::string_view kProjectSchemaJsonStr{
  static_cast<const char *> (kProjectSchemaJsonBytes),
  sizeof (kProjectSchemaJsonBytes)
};
}
