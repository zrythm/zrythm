// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <string_view>

namespace zrythm::structure::project
{

/**
 * @brief Compile-time embedded clipboard JSON schema as hex byte array.
 *
 * This is the JSON schema for clipboard payloads, embedded at compile time
 * from data/schemas/clipboard.schema.json.
 */
inline constexpr char kClipboardSchemaJsonBytes[]
{
  // clang-format off
  @CLIPBOARD_SCHEMA_JSON_HEX_BYTES@
};

/**
 * @brief String view of the embedded JSON schema.
 *
 * Provides a convenient string_view over the hex byte array.
 */
inline constexpr std::string_view kClipboardSchemaJsonStr{
  static_cast<const char *> (kClipboardSchemaJsonBytes),
  sizeof (kClipboardSchemaJsonBytes)
};
}
