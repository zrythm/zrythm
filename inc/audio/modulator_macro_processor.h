/*
 * Copyright (C) 2021-2022 Alexandros Theodotou <alex at zrythm dot org>
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
 * Modulator macro button processor.
 */

#ifndef __AUDIO_MODULATOR_MACRO_PROCESSOR_H__
#define __AUDIO_MODULATOR_MACRO_PROCESSOR_H__

#include "audio/port.h"

#include "utils/yaml.h"

typedef struct Track Track;

/**
 * @addtogroup audio
 *
 * @{
 */

#define MODULATOR_MACRO_PROCESSOR_SCHEMA_VERSION 1

#define modulator_macro_processor_is_in_active_project(self) \
  (self->track \
   && track_is_in_active_project (self->track))

/**
 * Modulator macro button processor.
 *
 * Has 1 control input, many CV inputs and 1 CV
 * output.
 *
 * Can only belong to modulator track.
 */
typedef struct ModulatorMacroProcessor
{
  int               schema_version;

  /**
   * Name to be shown in the modulators tab.
   *
   * @note This is only cosmetic and should not be
   * used anywhere during processing.
   */
  char *            name;

  /** CV input port for connecting CV signals to. */
  Port  *           cv_in;

  /**
   * CV output after macro is applied.
   *
   * This can be routed to other parameters to apply
   * the macro.
   */
  Port *            cv_out;

  /** Control port controlling the amount. */
  Port *            macro;

  /** Pointer to owner track, if any. */
  Track *           track;

} ModulatorMacroProcessor;

static const cyaml_schema_field_t
modulator_macro_processor_fields_schema[] =
{
  YAML_FIELD_INT (
    ModulatorMacroProcessor, schema_version),
  YAML_FIELD_STRING_PTR (
    ModulatorMacroProcessor, name),
  YAML_FIELD_MAPPING_PTR (
    ModulatorMacroProcessor, cv_in,
    port_fields_schema),
  YAML_FIELD_MAPPING_PTR (
    ModulatorMacroProcessor, cv_out,
    port_fields_schema),
  YAML_FIELD_MAPPING_PTR (
    ModulatorMacroProcessor, macro,
    port_fields_schema),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t
modulator_macro_processor_schema = {
  YAML_VALUE_PTR (
    ModulatorMacroProcessor,
    modulator_macro_processor_fields_schema),
};

static inline const char *
modulator_macro_processor_get_name (
  ModulatorMacroProcessor * self)
{
  return self->name;
}

COLD
void
modulator_macro_processor_init_loaded (
  ModulatorMacroProcessor * self,
  Track *                   track);

void
modulator_macro_processor_set_name (
  ModulatorMacroProcessor * self,
  const char *              name);

Track *
modulator_macro_processor_get_track (
  ModulatorMacroProcessor * self);

/**
 * Process.
 */
void
modulator_macro_processor_process (
  ModulatorMacroProcessor *           self,
  const EngineProcessTimeInfo * const time_nfo);

ModulatorMacroProcessor *
modulator_macro_processor_new (
  Track * track,
  int     idx);

void
modulator_macro_processor_free (
  ModulatorMacroProcessor * self);

/**
 * @}
 */

#endif
