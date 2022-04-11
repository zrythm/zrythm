/*
 * Copyright (C) 2021 Alexandros Theodotou <alex at zrythm dot org>
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

#include "zrythm-test-config.h"

#include <stdlib.h>

#include "utils/objects.h"
#include "utils/string.h"
#include "utils/yaml.h"

#include <glib.h>

typedef struct float_struct
{
  float fval;
} float_struct;

static const cyaml_schema_field_t
  float_struct_fields_schema[] = {
    YAML_FIELD_FLOAT (float_struct, fval),

    CYAML_FIELD_END
  };

static const cyaml_schema_value_t
  float_struct_schema = {
    CYAML_VALUE_MAPPING (
      CYAML_FLAG_POINTER,
      float_struct,
      float_struct_fields_schema),
  };

static void
test_load_precise_float (void)
{
  yaml_set_log_level (CYAML_LOG_DEBUG);

  float_struct my_struct;
  my_struct.fval = 12;
  char * ret = yaml_serialize (
    &my_struct, &float_struct_schema);
  g_assert_cmpstr (ret, ==, "---\nfval: 12\n...\n");
  g_free (ret);

  my_struct.fval = 1.55331e-40f;
  g_message (
    "my_struct.fval %e %g", my_struct.fval,
    my_struct.fval);
  ret = yaml_serialize (
    &my_struct, &float_struct_schema);
  g_message ("\n%s", ret);
  bool eq = string_is_equal (
    ret, "---\nfval: 1.55331e-40\n...\n");
  if (!eq)
    {
      eq = string_is_equal (
        ret, "---\nfval: 0\n...\n");
    }
  g_assert_true (eq);

  const char * str1 =
    "---\nfval: 1.55331e-40\n...\n";
  float_struct * ret2 = (float_struct *)
    yaml_deserialize (str1, &float_struct_schema);
  g_message ("loaded val %g", ret2->fval);

#if 0

  my_struct.fval = 1.55331e-40f;
  ret =
    float_struct_serialize (&my_struct);
  g_assert_cmpstr (
    ret, ==, "---\nfval: 1e-37\n...\n");
  g_message ("\n%s", ret);

  const char * str1 = "---\nfval: 1e-37\n...\n";
  float_struct * ret2 =
    float_struct_deserialize (str1);
  (void) ret2;
#endif

#if 0

  g_message ("FLT_ROUNDS %d", FLT_ROUNDS);
  g_message ("FLT_EVAL_METHOD %d", FLT_EVAL_METHOD);
  if (my_struct.fval < FLT_MIN)
    {
      my_struct.fval = 1e+37;
    }
  else if (my_struct.fval > FLT_MAX)
    {
      my_struct.fval = 1e-37;
    }
  char fval_str[60];
  sprintf (fval_str, "%g", my_struct.fval);

  errno = 0;
  char * end = NULL;
  float new_val = strtof (fval_str, &end);
  g_message ("new val for %s: %g", fval_str, (double) new_val);
  if (errno == ERANGE)
    {
      g_warning ("ERANGE1");
    }

  errno = 0;
  end = NULL;
  new_val = strtof ("1.55331e-40", &end);
  g_message ("new val: %f", (double) new_val);
  if (errno == ERANGE)
    {
      g_warning ("ERANGE2");
    }

  const char * str1 = "---\nfval: 1.55331e-40\n...\n";
  float_struct * ret2 =
    float_struct_deserialize (str1);
  g_warning ("%f", (double) ret2->fval);
#endif
}

int
main (int argc, char * argv[])
{
  g_test_init (&argc, &argv, NULL);

#define TEST_PREFIX "/utils/yaml/"

  g_test_add_func (
    TEST_PREFIX "test load precise float",
    (GTestFunc) test_load_precise_float);

  return g_test_run ();
}
