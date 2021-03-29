/*
 * Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "utils/err_codes.h"

#include <glib/gi18n.h>

const char *
error_code_get_message (
  ErrorCode err_code)
{
  switch (err_code)
    {
    case ERR_PLUGIN_INSTANTIATION_FAILED:
      return
        _("Plugin instantiation failed. "
          "See the logs for details");
    case ERR_OBJECT_IS_NULL:
      return
        _("Object is null. "
          "See the logs for details");
    default:
      break;
    }

  g_return_val_if_reached (_("Unknown error"));
}
