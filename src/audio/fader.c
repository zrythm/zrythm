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

#include "audio/balance_control.h"
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
    case FADER_TYPE_MONITOR:
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

  port_set_owner_fader (self->amp, self);
  port_set_owner_fader (self->balance, self);

  fader_set_amp ((void *) self, self->amp->control);
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
  g_return_if_fail (self);

  self->type = type;
  if (type == FADER_TYPE_AUDIO_CHANNEL ||
      type == FADER_TYPE_MIDI_CHANNEL)
    self->track_pos = ch->track_pos;

  /* set volume */
  self->volume = 0.0f;
  float amp = 1.f;
  self->amp =
    port_new_with_type (
      TYPE_CONTROL, FLOW_INPUT, _("Volume"));
  port_set_control_value (self->amp, amp, 0, 0);
  self->fader_val =
    math_get_fader_val_from_amp (amp);
  port_set_owner_fader (self->amp, self);
  self->amp->id.flags |=
    PORT_FLAG_AMPLITUDE;
  if (type == FADER_TYPE_AUDIO_CHANNEL ||
      type == FADER_TYPE_MIDI_CHANNEL)
    {
      self->amp->id.flags |=
        PORT_FLAG_CHANNEL_FADER;
    }

  /* set phase */
  self->phase = 0.0f;

  /* set pan */
  float balance = 0.5f;
  self->balance =
    port_new_with_type (
      TYPE_CONTROL, FLOW_INPUT,
      _("Balance control"));
  port_set_control_value (
    self->balance, balance, 0, 0);
  port_set_owner_fader (self->balance, self);
  self->balance->id.flags |=
    PORT_FLAG_STEREO_BALANCE;

  if (type == FADER_TYPE_AUDIO_CHANNEL ||
      type == FADER_TYPE_MONITOR)
    {
      /* stereo in */
      self->stereo_in =
        stereo_ports_new_generic (
        1, _("Fader in"),
        type == FADER_TYPE_AUDIO_CHANNEL ?
          PORT_OWNER_TYPE_FADER :
          PORT_OWNER_TYPE_MONITOR_FADER,
        self);

      /* stereo out */
      self->stereo_out =
        stereo_ports_new_generic (
        0, _("Fader out"),
        type == FADER_TYPE_AUDIO_CHANNEL ?
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

void
fader_update_volume_and_fader_val (
  Fader * self)
{
  /* calculate volume */
  self->volume =
    math_amp_to_dbfs (self->amp->control);

  self->fader_val =
    math_get_fader_val_from_amp (self->amp->control);
}

/**
 * Sets the amplitude of the fader. (0.0 to 2.0)
 */
void
fader_set_amp (void * _fader, float amp)
{
  Fader * self = (Fader *) _fader;
  port_set_control_value (self->amp, amp, 0, 0);

  fader_update_volume_and_fader_val (
    self);
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

  float fader_amp = fader_get_amp (self);
  fader_amp =
    CLAMP (fader_amp + amp, 0.f, 2.f);
  fader_set_amp (self, fader_amp);

  fader_update_volume_and_fader_val (self);
}

float
fader_get_amp (void * _self)
{
  Fader * self = (Fader *) _self;
  return self->amp->control;
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
  float fader_amp =
    math_get_amp_val_from_fader (fader_val);
  fader_set_amp (self, fader_amp);
  self->volume = math_amp_to_dbfs (fader_amp);

  if (self == MONITOR_FADER)
    {
      g_settings_set_double (
        S_UI, "monitor-out-vol",
        (double) fader_amp);
    }
}

Channel *
fader_get_channel (
  Fader * self)
{
  Track * track = fader_get_track (self);
  g_return_val_if_fail (IS_TRACK (track), NULL);

  Channel * ch =
    track_get_channel (track);
  g_return_val_if_fail (IS_CHANNEL (ch), NULL);

  return ch;
}

Track *
fader_get_track (
  Fader * self)
{
  g_return_val_if_fail (
    self && self->track_pos < TRACKLIST->num_tracks,
    NULL);
  Track * track =
    TRACKLIST->tracks[self->track_pos];
  g_return_val_if_fail (IS_TRACK (track), NULL);

  return track;
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
    case FADER_TYPE_MONITOR:
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
    case FADER_TYPE_MONITOR:
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

  port_disconnect_all (self->amp);
  port_disconnect_all (self->balance);
}

/**
 * Copy the fader values from source to dest.
 *
 * Used when cloning channels.
 */
void
fader_copy_values (
  Fader * src,
  Fader * dest)
{
  dest->volume = src->volume;
  dest->phase = src->phase;
  dest->fader_val = src->fader_val;
  dest->amp->control = src->amp->control;
  dest->balance->control = src->balance->control;
}

/**
 * Updates the track pos of the fader.
 */
void
fader_update_track_pos (
  Fader * self,
  int     pos)
{
  self->track_pos = pos;

  if (self->amp)
    {
      port_update_track_pos (self->amp, pos);
    }
  if (self->balance)
    {
      port_update_track_pos (self->balance, pos);
    }
  if (self->stereo_in)
    {
      port_update_track_pos (
        self->stereo_in->l, pos);
      port_update_track_pos (
        self->stereo_in->r, pos);
    }
  if (self->stereo_out)
    {
      port_update_track_pos (
        self->stereo_out->l, pos);
      port_update_track_pos (
        self->stereo_out->r, pos);
    }
  if (self->midi_in)
    {
      port_update_track_pos (self->midi_in, pos);
    }
  if (self->midi_out)
    {
      port_update_track_pos (self->midi_out, pos);
    }
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
  nframes_t       start_frame,
  const nframes_t nframes)
{
  /*Track * track = self->channel->track;*/

  float pan =
    port_get_control_value (self->balance, 0);
  float amp =
    port_get_control_value (self->amp, 0);

  if (self->type == FADER_TYPE_AUDIO_CHANNEL ||
      self->type == FADER_TYPE_MONITOR)
    {
      nframes_t end = start_frame + nframes;
      float calc_l, calc_r;
      balance_control_get_calc_lr (
        BALANCE_CONTROL_ALGORITHM_LINEAR,
        pan, &calc_l, &calc_r);

      while (start_frame < end)
        {
          /* 1. get input
           * 2. apply fader
           * 3. apply pan */
          self->stereo_out->l->buf[start_frame] =
            self->stereo_in->l->buf[start_frame] *
              amp * calc_l;
          self->stereo_out->r->buf[start_frame] =
            self->stereo_in->r->buf[start_frame] *
              amp * calc_r;
          start_frame++;
        }
    }

  if (self->type == FADER_TYPE_MIDI_CHANNEL)
    {
      midi_events_append (
        self->midi_in->midi_events,
        self->midi_out->midi_events,
        start_frame, nframes, 0);
    }
}
