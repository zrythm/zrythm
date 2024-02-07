// SPDX-License-Identifier: LicenseRef-ZrythmLicense
/*
 * SPDX-FileCopyrightText: © 2021-2022 Alexandros Theodotou <alex@zrythm.org>
 */

#include "dsp/engine.h"
#include "dsp/modulator_macro_processor.h"
#include "dsp/port.h"
#include "utils/debug.h"
#include "utils/dsp.h"
#include "utils/objects.h"

#include <glib/gi18n.h>

void
modulator_macro_processor_init_loaded (
  ModulatorMacroProcessor * self,
  Track *                   track)
{
  self->track = track;

  port_init_loaded (self->macro, self);
  port_init_loaded (self->cv_in, self);
  port_init_loaded (self->cv_out, self);
}

Track *
modulator_macro_processor_get_track (ModulatorMacroProcessor * self)
{
  return port_get_track (self->cv_in, true);
}

void
modulator_macro_processor_set_name (
  ModulatorMacroProcessor * self,
  const char *              name)
{
  self->name = g_strdup (name);
}

/**
 * Process.
 */
void
modulator_macro_processor_process (
  ModulatorMacroProcessor *           self,
  const EngineProcessTimeInfo * const time_nfo)
{
  z_return_if_fail_cmp (
    time_nfo->local_offset + time_nfo->nframes, <=, self->cv_out->last_buf_sz);

  /* if there are inputs, multiply by the knov
   * value */
  if (self->cv_in->num_srcs > 0)
    {
      dsp_mix2 (
        &self->cv_out->buf[time_nfo->local_offset],
        &self->cv_in->buf[time_nfo->local_offset], 0.f, self->macro->control,
        time_nfo->nframes);
    }
  /* else if there are no inputs, set the knob value
   * as the output */
  else
    {
      Port * cv_out = self->cv_out;
      g_return_if_fail (IS_PORT (cv_out));
      dsp_fill (
        &cv_out->buf[time_nfo->local_offset],
        self->macro->control * (cv_out->maxf - cv_out->minf) + cv_out->minf,
        time_nfo->nframes);
    }
}

ModulatorMacroProcessor *
modulator_macro_processor_new (Track * track, int idx)
{
  ModulatorMacroProcessor * self = object_new (ModulatorMacroProcessor);
  self->schema_version = MODULATOR_MACRO_PROCESSOR_SCHEMA_VERSION;
  self->track = track;

  char str[600];
  sprintf (str, _ ("Macro %d"), idx + 1);
  self->name = g_strdup (str);
  self->macro = port_new_with_type_and_owner (
    TYPE_CONTROL, FLOW_INPUT, str, PORT_OWNER_TYPE_MODULATOR_MACRO_PROCESSOR,
    self);
  self->macro->id.sym = g_strdup_printf ("macro_%d", idx + 1);
  Port * port = self->macro;
  port->minf = 0.f;
  port->maxf = 1.f;
  port->deff = 0.f;
  port_set_control_value (port, 0.75f, false, false);
  port->id.flags |= PORT_FLAG_AUTOMATABLE;
  port->id.flags |= PORT_FLAG_MODULATOR_MACRO;
  port->id.port_index = idx;

  sprintf (str, _ ("Macro CV In %d"), idx + 1);
  self->cv_in = port_new_with_type_and_owner (
    TYPE_CV, FLOW_INPUT, str, PORT_OWNER_TYPE_MODULATOR_MACRO_PROCESSOR, self);
  self->cv_in->id.sym = g_strdup_printf ("macro_cv_in_%d", idx + 1);
  port = self->cv_in;
  port->id.flags |= PORT_FLAG_MODULATOR_MACRO;
  port->id.port_index = idx;

  sprintf (str, _ ("Macro CV Out %d"), idx + 1);
  self->cv_out = port_new_with_type_and_owner (
    TYPE_CV, FLOW_OUTPUT, str, PORT_OWNER_TYPE_MODULATOR_MACRO_PROCESSOR, self);
  self->cv_out->id.sym = g_strdup_printf ("macro_cv_out_%d", idx + 1);
  port = self->cv_out;
  port->id.flags |= PORT_FLAG_MODULATOR_MACRO;
  port->id.port_index = idx;

  return self;
}

void
modulator_macro_processor_free (ModulatorMacroProcessor * self)
{
  object_free_w_func_and_null (port_free, self->macro);
  object_free_w_func_and_null (port_free, self->cv_in);
  object_free_w_func_and_null (port_free, self->cv_out);

  object_zero_and_free (self);
}
