// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __UTILS_JSON_H__
#define __UTILS_JSON_H__

#include "utils/string.h"

#include <yyjson.h>

/**
 * @addtogroup utils
 *
 * @{
 */

CStringRAII
json_get_string (yyjson_val * val);

/**
 * @}
 */

#endif // ___UTILS_JSON_H__