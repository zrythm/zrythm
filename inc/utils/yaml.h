/*
 * utils/yaml.h - YAML utils
 *
 * Copyright (C) 2019 Alexandros Theodotou
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

#ifndef __UTILS_YAML_H__
#define __UTILS_YAML_H__

#include <cyaml/cyaml.h>

/**
 * Serializes to XML.
 *
 * MUST be free'd.
 */
#define X_SERIALIZE_INC(camelcase, lowercase) \
  char * \
  lowercase##_serialize ( \
    camelcase * x);

#define X_SERIALIZE_SRC(camelcase, lowercase) \
  char * \
  lowercase##_serialize ( \
    camelcase * x) \
  { \
    cyaml_err_t err; \
 \
    char * output; \
    size_t output_len; \
    err = \
      cyaml_save_data ( \
        &output, \
        &output_len, \
        &config, \
        &lowercase##_schema, \
        x, \
        0); \
    if (err != CYAML_OK) \
      { \
        g_warning ("error %s", \
                   cyaml_strerror (err)); \
      } \
 \
    output[output_len] = '\0'; \
 \
    return output; \
  }

#define X_DESERIALIZE_INC(camelcase, lowercase) \
  camelcase * \
  lowercase##_deserialize (const char * e);

#define X_DESERIALIZE_SRC(camelcase, lowercase) \
  camelcase * \
  lowercase##_deserialize (const char * e) \
  { \
    camelcase * self; \
 \
    cyaml_err_t err = \
      cyaml_load_data ((const unsigned char *) e, \
                       strlen (e), \
                       &config, \
                       &lowercase##_schema, \
                       (cyaml_data_t **) &self, \
                       NULL); \
    if (err != CYAML_OK) \
      { \
        g_error ("error %s", \
                 cyaml_strerror (err)); \
      } \
 \
    return self; \
  }

static const cyaml_config_t config = {
	.log_level = CYAML_LOG_DEBUG, /* Logging errors and warnings only. */
	.log_fn = cyaml_log,            /* Use the default logging function. */
	.mem_fn = cyaml_mem,            /* Use the default memory allocator. */
};

static const cyaml_schema_value_t
int_schema = {
	CYAML_VALUE_INT (CYAML_FLAG_DEFAULT,
                   typeof (int)),
};

/**
 * Removes lines starting with control chars from the
 * YAML string.
 */
void
yaml_sanitize (char * e, size_t output_len);

#endif
