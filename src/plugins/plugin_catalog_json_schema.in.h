// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <string_view>

namespace zrythm::plugins::discovery
{

/**
 * @brief Compile-time embedded known-plugins JSON schema.
 */
inline constexpr char kPluginCatalogSchemaJsonBytes[]
{
  // clang-format off
  @PLUGIN_CATALOG_SCHEMA_JSON_HEX_BYTES@
};

inline constexpr std::string_view kPluginCatalogSchemaJsonStr{
  static_cast<const char *> (kPluginCatalogSchemaJsonBytes),
  sizeof (kPluginCatalogSchemaJsonBytes)
};
}
