/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "plugins/lv2_plugin.h"
#include "plugins/lv2/log.h"
#include "plugins/plugin.h"

int
lv2_vprintf(LV2_Log_Handle handle,
             LV2_URID       type,
             const char*    fmt,
             va_list        ap)
{
  Lv2Plugin* plugin  = (Lv2Plugin*) handle;
  GLogLevelFlags level;
  if (type == plugin->urids.log_Trace)
    level = G_LOG_LEVEL_DEBUG;
  else if (type == plugin->urids.log_Error)
    level = G_LOG_LEVEL_ERROR;
  else if (type == plugin->urids.log_Warning)
    level = G_LOG_LEVEL_WARNING;
  else
    level = G_LOG_LEVEL_MESSAGE;

  g_logv (plugin->plugin->descr->name,
          level,
          fmt,
          ap);

  return 0;
}

int
lv2_printf(LV2_Log_Handle handle,
            LV2_URID       type,
            const char*    fmt, ...)
{
  Lv2Plugin* plugin  = (Lv2Plugin*) handle;
  GLogLevelFlags level;
  if (type == plugin->urids.log_Trace)
    level = G_LOG_LEVEL_DEBUG;
  else if (type == plugin->urids.log_Error)
    level = G_LOG_LEVEL_ERROR;
  else if (type == plugin->urids.log_Warning)
    level = G_LOG_LEVEL_WARNING;
  else
    level = G_LOG_LEVEL_MESSAGE;

	va_list args;
	va_start(args, fmt);
	const int ret = lv2_vprintf(handle, type, fmt, args);
	va_end(args);

  return ret;
}
