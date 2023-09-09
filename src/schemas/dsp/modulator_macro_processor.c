// SPDX-FileCopyrightText: © 2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/modulator_macro_processor.h"
#include "utils/objects.h"

#include "schemas/dsp/modulator_macro_processor.h"

ModulatorMacroProcessor *
modulator_macro_processor_upgrade_from_v1 (ModulatorMacroProcessor_v1 * old)
{
  if (!old)
    return NULL;

  ModulatorMacroProcessor * self = object_new (ModulatorMacroProcessor);

  self->schema_version = MODULATOR_MACRO_PROCESSOR_SCHEMA_VERSION;
  self->name = old->name;
  self->cv_in = port_upgrade_from_v1 (old->cv_in);
  self->cv_out = port_upgrade_from_v1 (old->cv_out);
  self->macro = port_upgrade_from_v1 (old->macro);

  return self;
}
