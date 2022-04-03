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

/**
 * \file
 *
 * Modulator macro button processor schema.
 */

#ifndef __SCHEMAS_AUDIO_MODULATOR_MACRO_PROCESSOR_H__
#define __SCHEMAS_AUDIO_MODULATOR_MACRO_PROCESSOR_H__

#include "utils/yaml.h"

#include "schemas/audio/port.h"

typedef struct ModulatorMacroProcessor_v1
{
  int       schema_version;
  char *    name;
  Port_v1 * cv_in;
  Port_v1 * cv_out;
  Port_v1 * macro;
} ModulatorMacroProcessor_v1;

static const cyaml_schema_field_t
  modulator_macro_processor_fields_schema_v1[] = {
    YAML_FIELD_INT (
      ModulatorMacroProcessor_v1,
      schema_version),
    YAML_FIELD_STRING_PTR (
      ModulatorMacroProcessor_v1,
      name),
    YAML_FIELD_MAPPING_PTR (
      ModulatorMacroProcessor_v1,
      cv_in,
      port_fields_schema_v1),
    YAML_FIELD_MAPPING_PTR (
      ModulatorMacroProcessor_v1,
      cv_out,
      port_fields_schema_v1),
    YAML_FIELD_MAPPING_PTR (
      ModulatorMacroProcessor_v1,
      macro,
      port_fields_schema_v1),

    CYAML_FIELD_END
  };

static const cyaml_schema_value_t
  modulator_macro_processor_schema_v1 = {
    YAML_VALUE_PTR (
      ModulatorMacroProcessor_v1,
      modulator_macro_processor_fields_schema_v1),
  };

#endif
