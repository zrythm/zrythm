// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <QJsonValue>

#include <nlohmann/json.hpp>

namespace zrythm::frontends::mcp
{

/**
 * Doubles hold integers exactly only up to 2^53; narrowing a larger
 * integral double to a 64-bit integer is undefined behavior.
 */
inline constexpr double kMaxExactJsonInteger = 9007199254740992.0;

/** Converts a registry-side JSON value to the Qt JSON used by qtmcp. */
QJsonValue
qjson_from_nlohmann (const nlohmann::json &value);

/** Converts a Qt JSON value from qtmcp to the registry-side JSON. */
nlohmann::json
nlohmann_from_qjson (const QJsonValue &value);

} // namespace zrythm::frontends::mcp
