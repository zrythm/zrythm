// SPDX-FileCopyrightText: © 2021-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Modulator macro button processor schema.
 */

#ifndef __SCHEMAS_AUDIO_MODULATOR_MACRO_PROCESSOR_H__
#define __SCHEMAS_AUDIO_MODULATOR_MACRO_PROCESSOR_H__

#include "schemas/dsp/port.h"
#include "utils/yaml.h"

typedef struct ModulatorMacroProcessor_v1
{
  int       schema_version;
  char *    name;
  Port_v1 * cv_in;
  Port_v1 * cv_out;
  Port_v1 * macro;
} ModulatorMacroProcessor_v1;

static const cyaml_schema_field_t modulator_macro_processor_fields_schema_v1[] = {
  YAML_FIELD_INT (ModulatorMacroProcessor_v1, schema_version),
  YAML_FIELD_STRING_PTR (ModulatorMacroProcessor_v1, name),
  YAML_FIELD_MAPPING_PTR (ModulatorMacroProcessor_v1, cv_in, port_fields_schema_v1),
  YAML_FIELD_MAPPING_PTR (ModulatorMacroProcessor_v1, cv_out, port_fields_schema_v1),
  YAML_FIELD_MAPPING_PTR (ModulatorMacroProcessor_v1, macro, port_fields_schema_v1),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t modulator_macro_processor_schema_v1 = {
  YAML_VALUE_PTR (
    ModulatorMacroProcessor_v1,
    modulator_macro_processor_fields_schema_v1),
};

#endif
