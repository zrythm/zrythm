/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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

#include "audio/channel.h"
#include "audio/engine.h"
#include "audio/midi.h"
#include "audio/passthrough_processor.h"
#include "audio/track.h"
#include "project.h"
#include "utils/math.h"

#include <glib/gi18n.h>

/**
 * Inits a PassthroughProcessor after loading a
 * project.
 */
void
passthrough_processor_init_loaded (
  PassthroughProcessor * self)
{
  switch (self->type)
    {
    case PP_TYPE_AUDIO_CHANNEL:
      port_set_owner_prefader (
        self->stereo_in->l, self);
      port_set_owner_prefader (
        self->stereo_in->r, self);
      port_set_owner_prefader (
        self->stereo_out->l, self);
      port_set_owner_prefader (
        self->stereo_out->r, self);
      break;
    case PP_TYPE_MIDI_CHANNEL:
      self->midi_in->midi_events =
        midi_events_new (
          self->midi_in);
      port_set_owner_prefader (
        self->midi_in, self);
      self->midi_out->midi_events =
        midi_events_new (
          self->midi_out);
      port_set_owner_prefader (
        self->midi_out, self);
      break;
    default:
      break;
    }
}

/**
 * Inits passthrough_processor to default values.
 *
 * This assumes that the channel has no plugins.
 *
 * @param self The PassthroughProcessor to init.
 * @param ch Channel.
 */
void
passthrough_processor_init (
  PassthroughProcessor * self,
  PassthroughProcessorType type,
  Channel * ch)
{
  self->channel = ch;
  self->type = type;

  self->l_port_db = 0.f;
  self->r_port_db = 0.f;

  if (type == PP_TYPE_AUDIO_CHANNEL)
    {
      /* stereo in */
      self->stereo_in =
        stereo_ports_new_generic (
          1, _("Pre-Fader in"),
          PORT_OWNER_TYPE_PREFADER, self);

      /* stereo out */
      self->stereo_out =
        stereo_ports_new_generic (
          0, _("Pre-Fader out"),
          PORT_OWNER_TYPE_PREFADER, self);
    }

  if (type == PP_TYPE_MIDI_CHANNEL)
    {
      /* MIDI in */
      char * pll =
        g_strdup (_("MIDI pre-fader in"));
      self->midi_in =
        port_new_with_type (
          TYPE_EVENT,
          FLOW_INPUT,
          pll);
      self->midi_in->midi_events =
        midi_events_new (
          self->midi_in);
      g_free (pll);

      /* MIDI out */
      pll =
        g_strdup (_("MIDI pre-fader out"));
      self->midi_out =
        port_new_with_type (
          TYPE_EVENT,
          FLOW_OUTPUT,
          pll);
      self->midi_out->midi_events =
        midi_events_new (
          self->midi_out);
      g_free (pll);

      port_set_owner_prefader (
        self->midi_in, self);
      port_set_owner_prefader (
        self->midi_out, self);
    }
}

/**
 * Clears all buffers.
 */
void
passthrough_processor_clear_buffers (
  PassthroughProcessor * self)
{
  switch (self->type)
    {
    case PP_TYPE_AUDIO_CHANNEL:
      port_clear_buffer (self->stereo_in->l);
      port_clear_buffer (self->stereo_in->r);
      port_clear_buffer (self->stereo_out->l);
      port_clear_buffer (self->stereo_out->r);
      break;
    case PP_TYPE_MIDI_CHANNEL:
      port_clear_buffer (self->midi_in);
      port_clear_buffer (self->midi_out);
      break;
    default:
      break;
    }
}

/**
 * Disconnects all ports connected to the processor.
 */
void
passthrough_processor_disconnect_all (
  PassthroughProcessor * self)
{
  switch (self->type)
    {
    case PP_TYPE_AUDIO_CHANNEL:
      port_disconnect_all (self->stereo_in->l);
      port_disconnect_all (self->stereo_in->r);
      port_disconnect_all (self->stereo_out->l);
      port_disconnect_all (self->stereo_out->r);
      break;
    case PP_TYPE_MIDI_CHANNEL:
      port_disconnect_all (self->midi_in);
      port_disconnect_all (self->midi_out);
      break;
    default:
      break;
    }
}

/**
 * Process the PassthroughProcessor.
 *
 * @param start_frame The local offset in this
 *   cycle.
 * @param nframes The number of frames to process.
 */
void
passthrough_processor_process (
  PassthroughProcessor * self,
  long    start_frame,
  int     nframes)
{
  if (self->type == PP_TYPE_AUDIO_CHANNEL)
    {
      /* copy the input to output */
      for (int i = start_frame;
           i < start_frame + nframes; i++)
        {
          self->stereo_out->l->buf[i] =
            self->stereo_in->l->buf[i];
          self->stereo_out->r->buf[i] =
            self->stereo_in->r->buf[i];
        }
    }

  if (self->type == PP_TYPE_MIDI_CHANNEL)
    {
      midi_events_append (
        self->midi_in->midi_events,
        self->midi_out->midi_events,
        start_frame, nframes, 0);
    }
}
