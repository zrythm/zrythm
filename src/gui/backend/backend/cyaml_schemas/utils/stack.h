// SPDX-FileCopyrightText: © 2019-2021 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Stack schema.
 */

#ifndef __SCHEMAS_UTILS_STACK_H__
#define __SCHEMAS_UTILS_STACK_H__

#include <cstdlib>

#include "utils/yaml.h"

typedef struct Stack_v1
{
  int     schema_version;
  void ** elements;
  int     max_length;
  gint    top;
} Stack_v1_v1;

static const cyaml_schema_field_t stack_fields_schema_v1[] = {
  YAML_FIELD_INT (Stack_v1, schema_version),
  YAML_FIELD_INT (Stack_v1, max_length),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t stack_schema = {
  YAML_VALUE_PTR (Stack_v1, stack_fields_schema_v1),
};

#endif
