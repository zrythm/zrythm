// SPDX-FileCopyrightText: © 2019-2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <string.h>

#include "utils/objects.h"
#include "utils/yaml.h"

#include <gtk/gtk.h>

#include <locale.h>

typedef enum
{
  Z_YAML_ERROR_FAILED,
} ZYamlError;

#define Z_YAML_ERROR z_yaml_error_quark ()
GQuark
z_yaml_error_quark (void);
G_DEFINE_QUARK (z - yaml - error - quark, z_yaml_error)

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
yaml_cyaml_log_func (cyaml_log_t lvl, void * ctxt, const char * fmt, va_list ap)
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
yaml_serialize (void * data, const cyaml_schema_value_t * schema, GError ** error)
{
  char * result = NULL;

  /* remember current locale and temporarily switch to C */
  char * orig_locale = g_strdup (setlocale (LC_ALL, NULL));
  setlocale (LC_ALL, "C");

  cyaml_config_t cyaml_config = {};
  yaml_get_cyaml_config (&cyaml_config);
  char *      output;
  size_t      output_len;
  cyaml_err_t err =
    cyaml_save_data (&output, &output_len, &cyaml_config, schema, data, 0);
  if (err != CYAML_OK)
    {
      g_set_error (
        error, Z_YAML_ERROR, Z_YAML_ERROR_FAILED, "cyaml error: %s",
        cyaml_strerror (err));
      goto return_serialize_result;
    }
  result = object_new_n (output_len + 1, char);
  memcpy (result, output, output_len);
  result[output_len] = '\0';
  cyaml_config.mem_fn (cyaml_config.mem_ctx, output, 0);

return_serialize_result:

  /* Restore the original locale. */
  setlocale (LC_ALL, orig_locale);
  g_free (orig_locale);

  return result;
}

void *
yaml_deserialize (
  const char *                 yaml,
  const cyaml_schema_value_t * schema,
  GError **                    error)
{
  void * obj = NULL;

  /* remember current locale and temporarily switch to C */
  char * orig_locale = g_strdup (setlocale (LC_ALL, NULL));
  setlocale (LC_ALL, "C");

  cyaml_config_t cyaml_config;
  yaml_get_cyaml_config (&cyaml_config);
  cyaml_err_t err = cyaml_load_data (
    (const unsigned char *) yaml, strlen (yaml), &cyaml_config, schema,
    (cyaml_data_t **) &obj, NULL);
  if (err != CYAML_OK)
    {
      g_set_error (
        error, Z_YAML_ERROR, Z_YAML_ERROR_FAILED, "cyaml error: %s",
        cyaml_strerror (err));
      goto return_deserialize_result;
    }

return_deserialize_result:

  /* Restore the original locale. */
  setlocale (LC_ALL, orig_locale);
  g_free (orig_locale);

  return obj;
}

void
yaml_print (void * data, const cyaml_schema_value_t * schema)
{
  GError * err = NULL;
  char *   yaml = yaml_serialize (data, schema, &err);
  if (yaml)
    {
      g_message ("[YAML]\n%s", yaml);
      g_free (yaml);
    }
  else
    {
      g_warning ("failed to deserialize %p: %s", data, err->message);
      g_error_free (err);
    }
}
