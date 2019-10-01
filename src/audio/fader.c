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
#include "audio/control_room.h"
#include "audio/engine.h"
#include "audio/fader.h"
#include "audio/midi.h"
#include "audio/track.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/math.h"
#include "zrythm.h"

#include <glib/gi18n.h>

/**
 * Inits fader after a project is loaded.
 */
void
fader_init_loaded (
  Fader * self)
{
  switch (self->type)
    {
    case FADER_TYPE_AUDIO_CHANNEL:
      port_set_owner_fader (
        self->stereo_in->l, self);
      port_set_owner_fader (
        self->stereo_in->r, self);
      port_set_owner_fader (
        self->stereo_out->l, self);
      port_set_owner_fader (
        self->stereo_out->r, self);
      break;
    case FADER_TYPE_MIDI_CHANNEL:
      self->midi_in->midi_events =
        midi_events_new (
          self->midi_in);
      port_set_owner_fader (
        self->midi_in, self);
      self->midi_out->midi_events =
        midi_events_new (
          self->midi_out);
      port_set_owner_fader (
        self->midi_out, self);
      break;
    default:
      break;
    }

  fader_set_amp ((void *) self, self->amp);
}

/**
 * Inits fader to default values.
 *
 * This assumes that the channel has no plugins.
 *
 * @param self The Fader to init.
 * @param type The FaderType.
 * @param ch Channel, if this is a channel Fader.
 */
void
fader_init (
  Fader * self,
  FaderType type,
  Channel * ch)
{
  self->type = type;
  self->channel = ch;

  /* set volume, phase, pan */
  self->volume = 0.0f;
  self->amp = 1.0f;
  self->fader_val =
    math_get_fader_val_from_amp (self->amp);
  self->phase = 0.0f;
  self->pan = 0.5f;
  self->l_port_db = 0.f;
  self->r_port_db = 0.f;

  if (type == FADER_TYPE_AUDIO_CHANNEL)
    {
      /* stereo in */
      self->stereo_in =
        stereo_ports_new_generic (
        1, _("Fader in"),
        ch ?
          PORT_OWNER_TYPE_FADER :
          PORT_OWNER_TYPE_MONITOR_FADER,
        self);

      /* stereo out */
      self->stereo_out =
        stereo_ports_new_generic (
        0, _("Fader out"),
        /* FIXME just create another type */
        ch ?
          PORT_OWNER_TYPE_FADER :
          PORT_OWNER_TYPE_MONITOR_FADER,
        self);
    }

  if (type == FADER_TYPE_MIDI_CHANNEL)
    {
      /* MIDI in */
      char * pll =
        g_strdup (_("MIDI fader in"));
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
        g_strdup (_("MIDI fader out"));
      self->midi_out =
        port_new_with_type (
          TYPE_EVENT,
          FLOW_OUTPUT,
          pll);
      self->midi_out->midi_events =
        midi_events_new (
          self->midi_out);
      g_free (pll);

      port_set_owner_fader (
        self->midi_in, self);
      port_set_owner_fader (
        self->midi_out, self);
    }
}

/**
 * Sets the amplitude of the fader. (0.0 to 2.0)
 */
void
fader_set_amp (void * _fader, float amp)
{
  Fader * self = (Fader *) _fader;
  self->amp = amp;

  /* calculate volume */
  self->volume = math_amp_to_dbfs (amp);

  self->fader_val =
    math_get_fader_val_from_amp (amp);
}

/**
 * Adds (or subtracts if negative) to the amplitude
 * of the fader (clamped at 0.0 to 2.0).
 */
void
fader_add_amp (
  void *   _self,
  sample_t amp)
{
  Fader * self = (Fader *) _self;

  self->amp =
    CLAMP (self->amp + amp, 0.f, 2.f);

  self->fader_val =
    math_get_fader_val_from_amp (self->amp);

  self->volume =
    math_amp_to_dbfs (self->amp);
}

float
fader_get_amp (void * _self)
{
  Fader * self = (Fader *) _self;
  return self->amp;
}

float
fader_get_fader_val (
  void * self)
{
  return ((Fader *) self)->fader_val;
}

/**
 * Sets the fader levels from a normalized value
 * 0.0-1.0 (such as in widgets).
 */
void
fader_set_fader_val (
  Fader * self,
  float   fader_val)
{
  self->fader_val = fader_val;
  self->amp =
    math_get_amp_val_from_fader (fader_val);
  self->volume = math_amp_to_dbfs (self->amp);

  if (self == MONITOR_FADER)
    {
      g_settings_set_double (
        S_UI, "monitor-out-vol",
        (double) self->amp);
    }
}

/**
 * Clears all buffers.
 */
void
fader_clear_buffers (
  Fader * self)
{
  switch (self->type)
    {
    case FADER_TYPE_AUDIO_CHANNEL:
      port_clear_buffer (self->stereo_in->l);
      port_clear_buffer (self->stereo_in->r);
      port_clear_buffer (self->stereo_out->l);
      port_clear_buffer (self->stereo_out->r);
      break;
    case FADER_TYPE_MIDI_CHANNEL:
      port_clear_buffer (self->midi_in);
      port_clear_buffer (self->midi_out);
      break;
    default:
      break;
    }
}

/**
 * Disconnects all ports connected to the fader.
 */
void
fader_disconnect_all (
  Fader * self)
{
  switch (self->type)
    {
    case FADER_TYPE_AUDIO_CHANNEL:
      port_disconnect_all (self->stereo_in->l);
      port_disconnect_all (self->stereo_in->r);
      port_disconnect_all (self->stereo_out->l);
      port_disconnect_all (self->stereo_out->r);
      break;
    case FADER_TYPE_MIDI_CHANNEL:
      port_disconnect_all (self->midi_in);
      port_disconnect_all (self->midi_out);
      break;
    default:
      break;
    }
}

/**
 * Copy the struct members from source to dest.
 */
void
fader_copy (
  Fader * src,
  Fader * dest)
{
  dest->volume = src->volume;
  dest->amp = src->amp;
  dest->phase = src->phase;
  dest->pan = src->pan;
}

/**
 * Process the Fader.
 *
 * @param start_frame The local offset in this
 *   cycle.
 * @param nframes The number of frames to process.
 */
void
fader_process (
  Fader *         self,
  const nframes_t start_frame,
  const nframes_t nframes)
{
  /*Track * track = self->channel->track;*/

  if (self->type == FADER_TYPE_AUDIO_CHANNEL)
    {
      /* first copy the input to output */
      for (unsigned int i = start_frame;
           i < start_frame + nframes; i++)
        {
          self->stereo_out->l->buf[i] =
            self->stereo_in->l->buf[i];
          self->stereo_out->r->buf[i] =
            self->stereo_in->r->buf[i];
        }

      /* apply fader */
      port_apply_fader (
        self->stereo_out->l, self->amp,
        start_frame, nframes);
      port_apply_fader (
        self->stereo_out->r, self->amp,
        start_frame, nframes);

      /* apply pan */
      port_apply_pan (
        self->stereo_out->l, self->pan,
        AUDIO_ENGINE->pan_law,
        AUDIO_ENGINE->pan_algo,
        start_frame, nframes);
      port_apply_pan (
        self->stereo_out->r, self->pan,
        AUDIO_ENGINE->pan_law,
        AUDIO_ENGINE->pan_algo,
        start_frame, nframes);
    }

  if (self->type == FADER_TYPE_MIDI_CHANNEL)
    {
      midi_events_append (
        self->midi_in->midi_events,
        self->midi_out->midi_events,
        start_frame, nframes, 0);
    }
}
