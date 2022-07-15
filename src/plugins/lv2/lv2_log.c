/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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

#include "plugins/lv2/lv2_log.h"
#include "plugins/lv2_plugin.h"
#include "plugins/plugin.h"
#include "plugins/plugin_manager.h"
#include "zrythm.h"
#include "zrythm_app.h"

/**
 * Needed because we can't set them directly when
 * gi18n.h is included.
 */
void
lv2_log_set_printf_funcs (LV2_Log_Log * log)
{
  log->printf = lv2_log_printf;
  log->vprintf = lv2_log_vprintf;
}

int
lv2_log_vprintf (
  LV2_Log_Handle handle,
  LV2_URID       type,
  const char *   _fmt,
  va_list        ap)
{
  Lv2Plugin *    plugin = (Lv2Plugin *) handle;
  GLogLevelFlags level;
  if (type == PM_URIDS.log_Trace)
    level = G_LOG_LEVEL_DEBUG;
  else if (
    type == PM_URIDS.log_Error || type == PM_URIDS.log_Warning)
    level = G_LOG_LEVEL_WARNING;
  else
    level = G_LOG_LEVEL_MESSAGE;

  /* remove trailing new line - glib adds its own */
  char fmt[strlen (_fmt) + 1];
  strcpy (fmt, _fmt);
  if (fmt[strlen (fmt) - 1] == '\n')
    fmt[strlen (fmt) - 1] = '\0';

  g_logv (
    plugin->plugin->setting->descr->name, level, fmt, ap);

  return 0;
}

int
lv2_log_printf (
  LV2_Log_Handle handle,
  LV2_URID       type,
  const char *   fmt,
  ...)
{
  va_list args;
  va_start (args, fmt);
  const int ret = lv2_log_vprintf (handle, type, fmt, args);
  va_end (args);

  return ret;
}
