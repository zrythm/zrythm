// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "utils/string.h"

#include <yyjson.h>

CStringRAII
json_get_string (yyjson_val * val);