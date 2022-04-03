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

#include "utils/objects.h"
#include "utils/yaml.h"

#include <gtk/gtk.h>

cyaml_log_t _cyaml_log_level = CYAML_LOG_WARNING;

void
yaml_set_log_level (cyaml_log_t level)
{
  _cyaml_log_level = level;
}

void
yaml_get_cyaml_config (cyaml_config_t * config)
{
  /** log level: DEBUG, WARNING, INFO... */
  config->log_level = _cyaml_log_level;
  /* use the default logging function */
  //.log_fn = cyaml_log,
  config->log_fn = yaml_cyaml_log_func;
  /* use the default memory allocator */
  config->mem_fn = cyaml_mem;
  config->flags = CYAML_CFG_DOCUMENT_DELIM;
}

/**
 * Custom logging function for libcyaml.
 */
void
yaml_cyaml_log_func (
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

  g_logv ("cyaml", level, format, ap);
}

/**
 * Serializes to XML.
 *
 * MUST be free'd.
 */
char *
yaml_serialize (
  void *                       data,
  const cyaml_schema_value_t * schema)
{
  cyaml_err_t    err;
  cyaml_config_t cyaml_config;
  yaml_get_cyaml_config (&cyaml_config);
  char * output;
  size_t output_len;
  err = cyaml_save_data (
    &output, &output_len, &cyaml_config, schema,
    data, 0);
  if (err != CYAML_OK)
    {
      g_warning ("error %s", cyaml_strerror (err));
      return NULL;
    }
  char * new_str =
    object_new_n (output_len + 1, char);
  memcpy (new_str, output, output_len);
  new_str[output_len] = '\0';
  cyaml_config.mem_fn (
    cyaml_config.mem_ctx, output, 0);

  return new_str;
}

void *
yaml_deserialize (
  const char *                 yaml,
  const cyaml_schema_value_t * schema)
{
  void *         obj;
  cyaml_config_t cyaml_config;
  yaml_get_cyaml_config (&cyaml_config);
  cyaml_err_t err = cyaml_load_data (
    (const unsigned char *) yaml, strlen (yaml),
    &cyaml_config, schema, (cyaml_data_t **) &obj,
    NULL);
  if (err != CYAML_OK)
    {
      g_warning (
        "cyaml error: %s", cyaml_strerror (err));
      return NULL;
    }

  return obj;
}

void
yaml_print (
  void *                       data,
  const cyaml_schema_value_t * schema)
{
  char * yaml = yaml_serialize (data, schema);

  if (yaml)
    {
      g_message ("[YAML]\n%s", yaml);
      g_free (yaml);
    }
  else
    {
      g_warning ("failed to deserialize %p", data);
    }
}
