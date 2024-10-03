// SPDX-FileCopyrightText: © 2019-2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

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
 * Ignores a key and doesn't require it to be present either.
 */
#define YAML_FIELD_IGNORE_OPT(key) CYAML_FIELD_IGNORE (key, CYAML_FLAG_OPTIONAL)

/**
 * Mapping embedded inside the struct.
 */
#define YAML_FIELD_MAPPING_EMBEDDED(owner, member, schema) \
  CYAML_FIELD_MAPPING (#member, CYAML_FLAG_DEFAULT, owner, member, schema)

/**
 * Mapping pointer to a struct.
 */
#define YAML_FIELD_MAPPING_PTR(owner, member, schema) \
  CYAML_FIELD_MAPPING_PTR (#member, CYAML_FLAG_POINTER, owner, member, schema)

/**
 * Mapping pointer to a struct.
 */
#define YAML_FIELD_MAPPING_PTR_OPTIONAL(owner, member, schema) \
  CYAML_FIELD_MAPPING_PTR ( \
    #member, CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL, owner, member, schema)

/**
 * Fixed-width array of pointers with variable count.
 *
 * @code@
 * MyStruct * my_structs[MAX_STRUCTS];
 * int        num_my_structs;
 * @endcode@
 */
#define YAML_FIELD_FIXED_SIZE_PTR_ARRAY_VAR_COUNT(owner, member, schema) \
  CYAML_FIELD_SEQUENCE_COUNT ( \
    #member, CYAML_FLAG_DEFAULT, owner, member, num_##member, &schema, 0, \
    CYAML_UNLIMITED)

/**
 * Fixed-width array of pointers with fixed count.
 *
 * @code@
 * MyStruct * my_structs[MAX_STRUCTS_CONST];
 * @endcode@
 */
#define YAML_FIELD_FIXED_SIZE_PTR_ARRAY(owner, member, schema, size) \
  CYAML_FIELD_SEQUENCE_FIXED ( \
    #member, CYAML_FLAG_DEFAULT, owner, member, &schema, size)

/**
 * Dynamic-width (reallocated) array of pointers
 * with variable count.
 *
 * @code@
 * AutomationTrack ** ats;
 * int                num_ats;
 * int                ats_size;
 * @endcode@
 */
#define YAML_FIELD_DYN_PTR_ARRAY_VAR_COUNT(owner, member, schema) \
  CYAML_FIELD_SEQUENCE_COUNT ( \
    #member, CYAML_FLAG_POINTER, owner, member, num_##member, &schema, 0, \
    CYAML_UNLIMITED)

/**
 * Dynamic (allocated) array of pointers
 * with fixed count.
 *
 * @code@
 * AutomationTrack ** ats;
 * @endcode@
 */
#define YAML_FIELD_DYN_FIXED_SIZE_PTR_ARRAY(owner, member, schema, fixed_size) \
  CYAML_FIELD_SEQUENCE_FIXED ( \
    #member, CYAML_FLAG_POINTER, owner, member, &schema, fixed_size)

/**
 * Dynamic-width (reallocated) array of structs
 * with variable count.
 *
 * @code@
 * RegionIdentifier * ids;
 * int                num_ids;
 * int                ids_size;
 * @endcode@
 *
 * @note \ref schema must be declared as
 *   CYAML_VALUE_MAPPING with the flag
 *   CYAML_FLAG_DEFAULT.
 */
#define YAML_FIELD_DYN_ARRAY_VAR_COUNT(owner, member, schema) \
  CYAML_FIELD_SEQUENCE_COUNT ( \
    #member, CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL, owner, member, \
    num_##member, &schema, 0, CYAML_UNLIMITED)

/**
 * Dynamic-width (reallocated) array of pointers
 * with variable count, nullable.
 *
 * @code@
 * AutomationTrack ** ats;
 * int                num_ats;
 * int                ats_size;
 * @endcode@
 */
#define YAML_FIELD_DYN_PTR_ARRAY_VAR_COUNT_OPT(owner, member, schema) \
  YAML_FIELD_DYN_ARRAY_VAR_COUNT (owner, member, schema)

/**
 * Dynamic (allocated) array of pointers
 * with fixed count.
 *
 * @code@
 * AutomationTrack ** ats;
 * @endcode@
 */
#define YAML_FIELD_DYN_FIXED_SIZE_PTR_ARRAY_OPT( \
  owner, member, schema, fixed_size) \
  CYAML_FIELD_SEQUENCE_FIXED ( \
    #member, CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL, owner, member, &schema, \
    fixed_size)

/**
 * Dynamic-width (reallocated) array of primitives
 * with variable count.
 *
 * @code@
 * int * ids;
 * int   num_ids;
 * int   ids_size;
 * @endcode@
 *
 * @note \ref schema must be declared as
 *   CYAML_VALUE_MAPPING with the flag
 *   CYAML_FLAG_DEFAULT.
 */
#define YAML_FIELD_DYN_ARRAY_VAR_COUNT_PRIMITIVES(owner, member, schema) \
  YAML_FIELD_DYN_ARRAY_VAR_COUNT (owner, member, schema)

/**
 * Fixed sequence of pointers.
 */
#define YAML_FIELD_SEQUENCE_FIXED(owner, member, schema, size) \
  CYAML_FIELD_SEQUENCE_FIXED ( \
    #member, CYAML_FLAG_DEFAULT, owner, member, &schema, size)

#define YAML_FIELD_INT(owner, member) \
  CYAML_FIELD_INT (#member, CYAML_FLAG_DEFAULT, owner, member)

#define YAML_FIELD_INT_OPT(owner, member) \
  CYAML_FIELD_INT (#member, CYAML_FLAG_OPTIONAL, owner, member)

#define YAML_FIELD_UINT(owner, member) \
  CYAML_FIELD_UINT (#member, CYAML_FLAG_DEFAULT, owner, member)

#define YAML_FIELD_FLOAT(owner, member) \
  CYAML_FIELD_FLOAT (#member, CYAML_FLAG_DEFAULT, owner, member)

#define YAML_FIELD_STRING_PTR(owner, member) \
  CYAML_FIELD_STRING_PTR ( \
    #member, CYAML_FLAG_POINTER, owner, member, 0, CYAML_UNLIMITED)

#define YAML_FIELD_STRING_PTR_OPTIONAL(owner, member) \
  CYAML_FIELD_STRING_PTR ( \
    #member, CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL, owner, member, 0, \
    CYAML_UNLIMITED)

#define YAML_FIELD_ENUM(owner, member, strings) \
  CYAML_FIELD_ENUM ( \
    #member, CYAML_FLAG_DEFAULT, owner, member, strings, \
    CYAML_ARRAY_LEN (strings))

#define YAML_FIELD_BITFIELD(owner, member, bitvals) \
  CYAML_FIELD_BITFIELD ( \
    #member, CYAML_FLAG_DEFAULT, owner, member, bitvals, \
    CYAML_ARRAY_LEN (bitvals))

/**
 * Schema to be used as a pointer.
 */
#define YAML_VALUE_PTR(cc, fields_schema) \
  CYAML_VALUE_MAPPING (CYAML_FLAG_POINTER, cc, fields_schema)

/**
 * Schema to be used as a pointer that can be
 * NULL.
 */
#define YAML_VALUE_PTR_NULLABLE(cc, fields_schema) \
  CYAML_VALUE_MAPPING (CYAML_FLAG_POINTER_NULL_STR, cc, fields_schema)

/**
 * Schema to be used for arrays of structs directly
 * (not as pointers).
 *
 * For every other case, use the PTR above.
 */
#define YAML_VALUE_DEFAULT(cc, fields_schema) \
  CYAML_VALUE_MAPPING (CYAML_FLAG_DEFAULT, cc, fields_schema)

#define YAML_BITVAL(_name, _offset) \
  { .name = _name, .offset = _offset, .bits = 1 }

/**
 * Serializes to YAML.
 *
 * @return Newly allocated YAML string, or NULL if
 *   error.
 */
NONNULL_ARGS (1, 2) char *
yaml_serialize (void * data, const cyaml_schema_value_t * schema, GError ** error);

NONNULL_ARGS (1, 2) void *
yaml_deserialize (
  const char *                 yaml,
  const cyaml_schema_value_t * schema,
  GError **                    error);

NONNULL void
yaml_print (void * data, const cyaml_schema_value_t * schema);

/**
 * Custom logging function for libcyaml.
 */
void
yaml_cyaml_log_func (
  cyaml_log_t  level,
  void *       ctxt,
  const char * format,
  va_list      args);

void
yaml_set_log_level (cyaml_log_t level);

void
yaml_get_cyaml_config (cyaml_config_t * cyaml_config);

static const cyaml_schema_value_t int_schema = {
  CYAML_VALUE_INT (CYAML_FLAG_DEFAULT, int),
};

static const cyaml_schema_value_t unsigned_int_schema = {
  CYAML_VALUE_UINT (CYAML_FLAG_DEFAULT, unsigned int),
};

static const cyaml_schema_value_t uint8_t_schema = {
  CYAML_VALUE_UINT (CYAML_FLAG_DEFAULT, typeof (uint8_t)),
};

static const cyaml_schema_value_t float_schema = {
  CYAML_VALUE_FLOAT (CYAML_FLAG_DEFAULT, typeof (float)),
};

static const cyaml_schema_field_t gdk_rgba_fields_schema[] = {
  CYAML_FIELD_FLOAT ("red", CYAML_FLAG_DEFAULT, GdkRGBA, red),
  CYAML_FIELD_FLOAT ("green", CYAML_FLAG_DEFAULT, GdkRGBA, green),
  CYAML_FIELD_FLOAT ("blue", CYAML_FLAG_DEFAULT, GdkRGBA, blue),
  CYAML_FIELD_FLOAT ("alpha", CYAML_FLAG_DEFAULT, GdkRGBA, alpha),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t gdk_rgba_schema_default = {
  YAML_VALUE_DEFAULT (GdkRGBA, gdk_rgba_fields_schema),
};

static const cyaml_schema_value_t gdk_rgba_schema = {
  YAML_VALUE_PTR (GdkRGBA, gdk_rgba_fields_schema),
};

typedef enum YamlDummyEnum
{
  YAML_DUMMY_ENUM1,
} YamlDummyEnum;

/**
 * @}
 */

#endif
