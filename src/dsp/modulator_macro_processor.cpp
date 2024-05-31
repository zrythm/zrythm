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

#include <fmt/format.h>
#include <fmt/printf.h>

void
modulator_macro_processor_init_loaded (
  ModulatorMacroProcessor * self,
  Track *                   track)
{
  self->track = track;

  self->macro->init_loaded (self);
  self->cv_in->init_loaded (self);
  self->cv_out->init_loaded (self);
}

Track *
modulator_macro_processor_get_track (ModulatorMacroProcessor * self)
{
  return self->cv_in->get_track (true);
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
    time_nfo->local_offset + time_nfo->nframes, <=, self->cv_out->last_buf_sz_);

  /* if there are inputs, multiply by the knov
   * value */
  if (self->cv_in->srcs_.size () > 0)
    {
      dsp_mix2 (
        &self->cv_out->buf_[time_nfo->local_offset],
        &self->cv_in->buf_[time_nfo->local_offset], 0.f, self->macro->control_,
        time_nfo->nframes);
    }
  /* else if there are no inputs, set the knob value
   * as the output */
  else
    {
      Port * cv_out = self->cv_out;
      g_return_if_fail (IS_PORT (cv_out));
      dsp_fill (
        &cv_out->buf_[time_nfo->local_offset],
        self->macro->control_ * (cv_out->maxf_ - cv_out->minf_) + cv_out->minf_,
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
  self->macro = new Port (
    PortType::Control, PortFlow::Input, str,
    PortIdentifier::OwnerType::MODULATOR_MACRO_PROCESSOR, self);
  self->macro->id_.sym_ = fmt::sprintf ("macro_%d", idx + 1);
  Port * port = self->macro;
  port->minf_ = 0.f;
  port->maxf_ = 1.f;
  port->deff_ = 0.f;
  port->set_control_value (0.75f, false, false);
  port->id_.flags_ |= PortIdentifier::Flags::AUTOMATABLE;
  port->id_.flags_ |= PortIdentifier::Flags::MODULATOR_MACRO;
  port->id_.port_index_ = idx;

  sprintf (str, _ ("Macro CV In %d"), idx + 1);
  self->cv_in = new Port (
    PortType::CV, PortFlow::Input, str,
    PortIdentifier::OwnerType::MODULATOR_MACRO_PROCESSOR, self);
  self->cv_in->id_.sym_ = fmt::sprintf ("macro_cv_in_%d", idx + 1);
  port = self->cv_in;
  port->id_.flags_ |= PortIdentifier::Flags::MODULATOR_MACRO;
  port->id_.port_index_ = idx;

  sprintf (str, _ ("Macro CV Out %d"), idx + 1);
  self->cv_out = new Port (
    PortType::CV, PortFlow::Output, str,
    PortIdentifier::OwnerType::MODULATOR_MACRO_PROCESSOR, self);
  self->cv_out->id_.sym_ = fmt::sprintf ("macro_cv_out_%d", idx + 1);
  port = self->cv_out;
  port->id_.flags_ |= PortIdentifier::Flags::MODULATOR_MACRO;
  port->id_.port_index_ = idx;

  return self;
}

void
modulator_macro_processor_free (ModulatorMacroProcessor * self)
{
  object_delete_and_null (self->macro);
  object_delete_and_null (self->cv_in);
  object_delete_and_null (self->cv_out);

  object_zero_and_free (self);
}
