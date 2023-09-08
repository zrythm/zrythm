// SPDX-FileCopyrightText: © 2019, 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "plugins/lv2/lv2_log.h"
#include "plugins/lv2_plugin.h"
#include "plugins/plugin.h"
#include "plugins/plugin_manager.h"
#include "zrythm.h"
#include "zrythm_app.h"

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

/**
 * Needed because we can't set them directly when
 * gi18n.h is included.
 */
void
lv2_log_set_printf_funcs (LV2_Log_Log * log)
{
#ifdef printf
#undef printf
#endif
#ifdef vprintf
#undef vprintf
#endif
  log->printf = lv2_log_printf;
  log->vprintf = lv2_log_vprintf;
}
