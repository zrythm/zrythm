// SPDX-FileCopyrightText: © 2021, 2023-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#include "utils/gtest_wrapper.h"
#include "utils/logger.h"
#include "utils/string.h"
#include "utils/yaml.h"

#ifdef HAVE_CYAML

using float_struct = struct float_struct
{
  float fval;
};

static const cyaml_schema_field_t float_struct_fields_schema[] = {
  YAML_FIELD_FLOAT (float_struct, fval),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t float_struct_schema = {
  CYAML_VALUE_MAPPING (CYAML_FLAG_POINTER, float_struct, float_struct_fields_schema),
};

TEST (Yaml, LoadPreciseFloat)
{
  yaml_set_log_level (CYAML_LOG_DEBUG);

  float_struct my_struct;
  my_struct.fval = 12;
  char * ret = yaml_serialize (&my_struct, &float_struct_schema, nullptr);
  ASSERT_NONNULL (ret);
  ASSERT_TRUE (string_is_equal (ret, "---\nfval: 12\n...\n"));
  g_free (ret);

  my_struct.fval = 1.55331e-40f;
  z_info ("my_struct.fval %e %g", my_struct.fval, my_struct.fval);
  ret = yaml_serialize (&my_struct, &float_struct_schema, nullptr);
  ASSERT_NONNULL (ret);
  z_info ("\n{}", ret);
  bool eq = string_is_equal (ret, "---\nfval: 1.55331e-40\n...\n");
  if (!eq)
    {
      eq = string_is_equal (ret, "---\nfval: 0\n...\n");
    }
  ASSERT_TRUE (eq);

  const char *   str1 = "---\nfval: 1.55331e-40\n...\n";
  float_struct * ret2 =
    (float_struct *) yaml_deserialize (str1, &float_struct_schema, nullptr);
  z_info ("loaded val %g", ret2->fval);

#  if 0

  my_struct.fval = 1.55331e-40f;
  ret =
    float_struct_serialize (&my_struct);
  g_assert_cmpstr (
    ret, ==, "---\nfval: 1e-37\n...\n");
  z_info ("\n{}", ret);

  const char * str1 = "---\nfval: 1e-37\n...\n";
  float_struct * ret2 =
    float_struct_deserialize (str1);
  (void) ret2;
#  endif

#  if 0

  z_info ("FLT_ROUNDS {}", FLT_ROUNDS);
  z_info ("FLT_EVAL_METHOD {}", FLT_EVAL_METHOD);
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
  z_info ("new val for {}: %g", fval_str, (double) new_val);
  if (errno == ERANGE)
    {
      z_warning("ERANGE1");
    }

  errno = 0;
  end = NULL;
  new_val = strtof ("1.55331e-40", &end);
  z_info ("new val: {:f}", (double) new_val);
  if (errno == ERANGE)
    {
      z_warning("ERANGE2");
    }

  const char * str1 = "---\nfval: 1.55331e-40\n...\n";
  float_struct * ret2 =
    float_struct_deserialize (str1);
  z_warning("{:f}", (double) ret2->fval);
#  endif
}
#endif