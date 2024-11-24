// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __UTILS_JSON_H__
#define __UTILS_JSON_H__

#include "utils/string.h"
#include <yyjson.h>

namespace zrythm::utils::json
{

/**
 * @brief Renders and returns the given JSON node to a string.
 */
string::CStringRAII
get_string (yyjson_val * val);

};

#endif // ___UTILS_JSON_H__
