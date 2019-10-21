/*
 * Copyright (C) 2019 Alexandros Theodotou
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

/**
 * \file
 *
 * YAML utils.
 */

#ifndef __UTILS_YAML_H__
#define __UTILS_YAML_H__

#include <gtk/gtk.h>

#include <cyaml/cyaml.h>

/**
 * @addtogroup utils
 *
 * @{
 */

/**
 * Serializes to XML.
 *
 * MUST be free'd.
 */
#define SERIALIZE_INC(camelcase, lowercase) \
  char * \
  lowercase##_serialize ( \
    camelcase * x);

#define SERIALIZE_SRC(camelcase, lowercase) \
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
        return NULL; \
      } \
 \
    char * new_str = \
      calloc (output_len + 1, sizeof (char));\
    memcpy (new_str, output, output_len);\
    new_str[output_len] = '\0'; \
    config.mem_fn(config.mem_ctx, output, 0); \
 \
    return new_str; \
  }

#define DESERIALIZE_INC(camelcase, lowercase) \
  camelcase * \
  lowercase##_deserialize (const char * e);

#define DESERIALIZE_SRC(camelcase, lowercase) \
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
        g_warning ( \
          "cyaml error: %s", \
          cyaml_strerror (err)); \
        return NULL; \
      } \
 \
    return self; \
  }

#define PRINT_YAML_INC(camelcase, lowercase) \
  void \
  lowercase##_print_yaml (camelcase * x);

#define PRINT_YAML_SRC(camelcase, lowercase) \
  void \
  lowercase##_print_yaml (camelcase * x) \
  { \
    char * serialized = \
      lowercase##_serialize (x); \
    g_message ("[YAML print]\n"#lowercase" :\n %s", \
               serialized); \
  }

static const cyaml_config_t config = {
  /** log level: DEBUG, WARNING, INFO... */
	.log_level = CYAML_LOG_WARNING,
  /* use the default loggin function */
	.log_fn = cyaml_log,
  /* use the default memory allocator */
	.mem_fn = cyaml_mem,
};

static const cyaml_schema_value_t
int_schema = {
	CYAML_VALUE_INT (
    CYAML_FLAG_DEFAULT,
    typeof (int)),
};

static const cyaml_schema_value_t
float_schema = {
	CYAML_VALUE_FLOAT (
    CYAML_FLAG_DEFAULT,
    typeof (float)),
};

static const cyaml_schema_field_t
gdk_rgba_fields_schema[] =
{
	CYAML_FIELD_FLOAT (
    "red", CYAML_FLAG_DEFAULT,
	  GdkRGBA, red),
	CYAML_FIELD_FLOAT (
    "green", CYAML_FLAG_DEFAULT,
	  GdkRGBA, green),
	CYAML_FIELD_FLOAT (
    "blue", CYAML_FLAG_DEFAULT,
	  GdkRGBA, blue),
	CYAML_FIELD_FLOAT (
    "alpha", CYAML_FLAG_DEFAULT,
	  GdkRGBA, alpha),

	CYAML_FIELD_END
};

/**
 * @}
 */

#endif
