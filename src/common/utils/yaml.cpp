// SPDX-FileCopyrightText: © 2019-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#include "common/utils/logger.h"

#if HAVE_CYAML

#  include "common/utils/objects.h"
#  include "common/utils/yaml.h"

#  include <locale.h>
#  include <string.h>

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
  QString formatted = QString::vasprintf (fmt, ap);

  switch (lvl)
    {
    case CYAML_LOG_WARNING:
      z_warning (formatted);
      break;
    case CYAML_LOG_ERROR:
      z_error (formatted);
      break;
    default:
      break;
    }
}

CStringRAII
yaml_serialize (void * data, const cyaml_schema_value_t * schema)
{
  char * result = NULL;

  /* remember current locale and temporarily switch to C */
  char * orig_locale = strdup (setlocale (LC_ALL, nullptr));
  setlocale (LC_ALL, "C");

  cyaml_config_t cyaml_config = {};
  yaml_get_cyaml_config (&cyaml_config);
  char *      output;
  size_t      output_len;
  cyaml_err_t err =
    cyaml_save_data (&output, &output_len, &cyaml_config, schema, data, 0);
  if (err != CYAML_OK)
    {
      /* Restore the original locale. */
      setlocale (LC_ALL, orig_locale);
      free (orig_locale);

      throw ZrythmException (
        fmt::format ("Failed to serialize object: {}", cyaml_strerror (err)));
    }
  result = object_new_n (output_len + 1, char);
  memcpy (result, output, output_len);
  result[output_len] = '\0';
  cyaml_config.mem_fn (cyaml_config.mem_ctx, output, 0);

  /* Restore the original locale. */
  setlocale (LC_ALL, orig_locale);
  free (orig_locale);

  return { result };
}

void *
yaml_deserialize (const char * yaml, const cyaml_schema_value_t * schema)
{
  void * obj = NULL;

  /* remember current locale and temporarily switch to C */
  char * orig_locale = strdup (setlocale (LC_ALL, nullptr));
  setlocale (LC_ALL, "C");

  cyaml_config_t cyaml_config;
  yaml_get_cyaml_config (&cyaml_config);
  cyaml_err_t err = cyaml_load_data (
    (const unsigned char *) yaml, strlen (yaml), &cyaml_config, schema,
    (cyaml_data_t **) &obj, nullptr);
  if (err != CYAML_OK)
    {
      /* Restore the original locale. */
      setlocale (LC_ALL, orig_locale);
      free (orig_locale);

      throw ZrythmException (
        fmt::format ("Failed to deserialize object: {}", cyaml_strerror (err)));
    }

  /* Restore the original locale. */
  setlocale (LC_ALL, orig_locale);
  free (orig_locale);

  return obj;
}

void
yaml_print (void * data, const cyaml_schema_value_t * schema)
{
  auto yaml = yaml_serialize (data, schema);
  z_info ("[YAML]\n{}", yaml.c_str ());
}

#endif /* HAVE_CYAML */
