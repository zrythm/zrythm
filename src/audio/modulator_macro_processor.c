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

#include "audio/modulator_macro_processor.h"
#include "audio/port.h"
#include "utils/dsp.h"
#include "utils/objects.h"

#include <glib/gi18n.h>

Track *
modulator_macro_processor_get_track (
  ModulatorMacroProcessor * self)
{
  return port_get_track (self->cv_in, true);
}

/**
 * Process.
 *
 * @param g_start_frames Global frames.
 * @param start_frame The local offset in this
 *   cycle.
 * @param nframes The number of frames to process.
 */
void
modulator_macro_processor_process (
  ModulatorMacroProcessor * self,
  long                      g_start_frames,
  nframes_t                 start_frame,
  const nframes_t           nframes)
{
  /* if there are inputs, multiply by the knov
   * value */
  if (self->cv_in->num_srcs > 0)
    {
      dsp_mix2 (
        &self->cv_out->buf[start_frame],
        &self->cv_in->buf[start_frame],
        0.f, self->macro->control, nframes);
    }
  /* else if there are no inputs, set the knob value
   * as the output */
  else
    {
      Port * cv_out = self->cv_out;
      dsp_fill (
        &cv_out->buf[start_frame],
        self->macro->control *
          (cv_out->maxf - cv_out->minf) +
          cv_out->minf,
        nframes);
    }
}

ModulatorMacroProcessor *
modulator_macro_processor_new (
  Track * track,
  int     idx)
{
  ModulatorMacroProcessor * self =
    object_new (ModulatorMacroProcessor);

  char str[600];
  sprintf (str, _("Macro %d"), idx + 1);
  self->macro =
    port_new_with_type (
      TYPE_CONTROL, FLOW_INPUT, str);
  Port * port = self->macro;
  port->minf = 0.f;
  port->maxf = 1.f;
  port->deff = 0.f;
  port_set_control_value (
    port, 0.75f, false, false);
  port->id.flags |=
    PORT_FLAG_AUTOMATABLE;
  port->id.flags |=
    PORT_FLAG_MODULATOR_MACRO;
  port->id.port_index = idx;
  port_set_owner_track (port, track);

  sprintf (str, _("Macro CV In %d"), idx + 1);
  self->cv_in =
    port_new_with_type (
      TYPE_CV, FLOW_INPUT, str);
  port = self->cv_in;
  port->id.flags |=
    PORT_FLAG_MODULATOR_MACRO;
  port->id.port_index = idx;
  port_set_owner_track (port, track);

  sprintf (str, _("Macro CV Out %d"), idx + 1);
  self->cv_out =
    port_new_with_type (
      TYPE_CV, FLOW_OUTPUT, str);
  port = self->cv_out;
  port->id.flags |=
    PORT_FLAG_MODULATOR_MACRO;
  port->id.port_index = idx;
  port_set_owner_track (port, track);

  return self;
}

void
modulator_macro_processor_free (
  ModulatorMacroProcessor * self)
{
  object_free_w_func_and_null (
    port_free, self->macro);
  object_free_w_func_and_null (
    port_free, self->cv_in);
  object_free_w_func_and_null (
    port_free, self->cv_out);

  object_zero_and_free (self);
}
