// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "common/utils/exceptions.h"
#include "common/utils/json.h"

CStringRAII
json_get_string (yyjson_val * val)
{
  yyjson_write_err write_err;
  char *           json = yyjson_val_write_opts (
    val, YYJSON_WRITE_PRETTY_TWO_SPACES, nullptr, nullptr, &write_err);

  if (!json)
    {
      throw ZrythmException (
        fmt::format ("Failed to serialize to JSON:\n{}", write_err.msg));
    }

  return CStringRAII (json);
}