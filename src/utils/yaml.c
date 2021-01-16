/*
 * Copyright (C) 2019-2021 Alexandros Theodotou <alex at zrythm.org>
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

#include <string.h>

#include "utils/yaml.h"

#include <gtk/gtk.h>

void
yaml_get_cyaml_config (
  cyaml_config_t * config)
{
  /** log level: DEBUG, WARNING, INFO... */
  config->log_level = CYAML_LOG_WARNING;
  /* use the default loggin function */
  //.log_fn = cyaml_log,
  config->log_fn = yaml_cyaml_log_func;
  /* use the default memory allocator */
  config->mem_fn = cyaml_mem;
}

/**
 * Custom logging function for libcyaml.
 */
void yaml_cyaml_log_func (
  cyaml_log_t  lvl,
  void *       ctxt,
  const char * fmt,
  va_list      ap)
{
  GLogLevelFlags level = G_LOG_LEVEL_MESSAGE;
  switch (lvl)
    {
    case CYAML_LOG_WARNING:
      level = G_LOG_LEVEL_WARNING;
      break;
    case CYAML_LOG_ERROR:
      level = G_LOG_LEVEL_WARNING;
      break;
    default:
      break;
    }

  char format[900];
  strcpy (format, fmt);
  format[strlen (format) - 1] = '\0';

  g_logv (
    "cyaml", level, format, ap);
}
