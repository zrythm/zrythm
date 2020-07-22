/*
 * Copyright (C) 2019-2020 Alexandros Theodotou <alex at zrythm dot org>
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
#include "audio/control_port.h"
#include "audio/control_room.h"
#include "audio/engine.h"
#include "audio/fader.h"
#include "audio/master_track.h"
#include "audio/midi.h"
#include "audio/track.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/math.h"
#include "utils/objects.h"
#include "zrythm.h"

#include <glib/gi18n.h>

/**
 * Inits fader after a project is loaded.
 */
void
fader_init_loaded (
  Fader * self,
  bool    is_project)
{
  self->magic = FADER_MAGIC;

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
  port_set_owner_fader (self->mute, self);

  fader_set_amp ((void *) self, self->amp->control);

  fader_set_is_project (self, is_project);
}

/**
 * Creates a new fader.
 *
 * This assumes that the channel has no plugins.
 *
 * @param type The FaderType.
 * @param ch Channel, if this is a channel Fader.
 */
Fader *
fader_new (
  FaderType type,
  Channel * ch,
  bool      passthrough)
{
  Fader * self = object_new (Fader);

  self->magic = FADER_MAGIC;

  self->passthrough = passthrough;

  self->type = type;
  if (type == FADER_TYPE_AUDIO_CHANNEL ||
      type == FADER_TYPE_MIDI_CHANNEL)
    {
      self->track_pos = ch->track_pos;
    }

  /* set volume */
  self->volume = 0.0f;
  float amp = 1.f;
  self->amp =
    port_new_with_type (
      TYPE_CONTROL, FLOW_INPUT, _("Volume"));
  self->amp->maxf = 2.f;
  port_set_control_value (self->amp, amp, 0, 0);
  self->fader_val =
    math_get_fader_val_from_amp (amp);
  port_set_owner_fader (self->amp, self);
  self->amp->id.flags |=
    PORT_FLAG_AMPLITUDE;
  if ((type == FADER_TYPE_AUDIO_CHANNEL ||
      type == FADER_TYPE_MIDI_CHANNEL) &&
      !passthrough)
    {
      self->amp->id.flags |=
        PORT_FLAG_AUTOMATABLE;
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
      _("Balance"));
  port_set_control_value (
    self->balance, balance, 0, 0);
  port_set_owner_fader (self->balance, self);
  self->balance->id.flags |=
    PORT_FLAG_STEREO_BALANCE;
  if ((type == FADER_TYPE_AUDIO_CHANNEL ||
      type == FADER_TYPE_MIDI_CHANNEL) &&
      !passthrough)
    {
      self->balance->id.flags |=
        PORT_FLAG_AUTOMATABLE;
    }

  /* set mute */
  self->mute =
    port_new_with_type (
      TYPE_CONTROL, FLOW_INPUT, _("Mute"));
  port_set_control_value (
    self->mute, 0.f, 0, 0);
  self->mute->id.flags |=
    PORT_FLAG_CHANNEL_MUTE;
  self->mute->id.flags |=
    PORT_FLAG_TOGGLE;
  if ((type == FADER_TYPE_AUDIO_CHANNEL ||
      type == FADER_TYPE_MIDI_CHANNEL) &&
      !passthrough)
    {
      self->mute->id.flags |=
        PORT_FLAG_AUTOMATABLE;
    }
  port_set_owner_fader (self->mute, self);

  if (type == FADER_TYPE_AUDIO_CHANNEL ||
      type == FADER_TYPE_MONITOR)
    {
      const char * name = NULL;
      if (type == FADER_TYPE_AUDIO_CHANNEL)
        {
          if (passthrough)
            {
              name = _("Ch Pre-Fader in");
            }
          else
            {
              name = _("Ch Fader in");
            }
        }
      else
        {
          name =  _("Monitor Fader in");
        }

      /* stereo in */
      self->stereo_in =
        stereo_ports_new_generic (
        1, name,
        type == FADER_TYPE_AUDIO_CHANNEL ?
          PORT_OWNER_TYPE_FADER :
          PORT_OWNER_TYPE_MONITOR_FADER,
        self);

      if (type == FADER_TYPE_AUDIO_CHANNEL)
        {
          if (passthrough)
            {
              name = _("Ch Pre-Fader out");
            }
          else
            {
              name = _("Ch Fader out");
            }
        }
      else
        {
          name =  _("Monitor Fader out");
        }

      /* stereo out */
      self->stereo_out =
        stereo_ports_new_generic (
        0, name,
        type == FADER_TYPE_AUDIO_CHANNEL ?
          PORT_OWNER_TYPE_FADER :
          PORT_OWNER_TYPE_MONITOR_FADER,
        self);
    }

  if (type == FADER_TYPE_MIDI_CHANNEL)
    {
      /* MIDI in */
      const char * name = NULL;
      if (passthrough)
        {
          name = _("Ch MIDI Pre-Fader in");
        }
      else
        {
          name = _("Ch MIDI Fader in");
        }
      self->midi_in =
        port_new_with_type (
          TYPE_EVENT, FLOW_INPUT, name);
      self->midi_in->midi_events =
        midi_events_new (self->midi_in);

      /* MIDI out */
      if (passthrough)
        {
          name = _("Ch MIDI Pre-Fader out");
        }
      else
        {
          name = _("Ch MIDI Fader out");
        }
      self->midi_out =
        port_new_with_type (
          TYPE_EVENT, FLOW_OUTPUT, name);
      self->midi_out->midi_events =
        midi_events_new (
          self->midi_out);

      port_set_owner_fader (
        self->midi_in, self);
      port_set_owner_fader (
        self->midi_out, self);
    }

  return self;
}

/**
 * Sets track muted and optionally adds the action
 * to the undo stack.
 */
void
fader_set_muted (
  Fader * self,
  bool    mute,
  bool    trigger_undo,
  bool    fire_events)
{
  Track * track = fader_get_track (self);
  g_return_if_fail (track);

  if (trigger_undo)
    {
      /* this is only supported if the fader track is
       * the only track selected */
      g_return_if_fail (
        TRACKLIST_SELECTIONS->num_tracks == 1 &&
        TRACKLIST_SELECTIONS->tracks[0] == track);
      UndoableAction * action =
        edit_tracks_action_new_mute (
          TRACKLIST_SELECTIONS, mute);
      undo_manager_perform (
        UNDO_MANAGER, action);
    }
  else
    {
      port_set_control_value (
        self->mute, mute ? 1.f : 0.f,
        false, fire_events);

      if (fire_events)
        {
          EVENTS_PUSH (
            ET_TRACK_STATE_CHANGED, track);
        }
    }
}

/**
 * Returns if the fader is muted.
 */
bool
fader_get_muted (
  Fader * self)
{
  return control_port_is_toggled (self->mute);
}

/**
 * Returns if the track is soloed.
 */
bool
fader_get_soloed (
  Fader * self)
{
  return self->solo;
}

/**
 * Sets track soloed and optionally adds the action
 * to the undo stack.
 */
void
fader_set_soloed (
  Fader * self,
  bool    solo,
  bool    trigger_undo,
  bool    fire_events)
{
  Track * track = fader_get_track (self);
  g_return_if_fail (track);

  if (trigger_undo)
    {
      /* this is only supported if the fader track is
       * the only track selected */
      g_return_if_fail (
        TRACKLIST_SELECTIONS->num_tracks == 1 &&
        TRACKLIST_SELECTIONS->tracks[0] == track);
      UndoableAction * action =
        edit_tracks_action_new_solo (
          TRACKLIST_SELECTIONS, solo);
      undo_manager_perform (
        UNDO_MANAGER, action);
    }
  else
    {
      self->solo = solo;

      if (fire_events)
        {
          EVENTS_PUSH (
            ET_TRACK_STATE_CHANGED, track);
        }
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
  g_return_if_fail (IS_FADER (self));

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

void
fader_set_is_project (
  Fader * self,
  bool    is_project)
{
  self->is_project = is_project;

  /* set volume */
  self->amp->is_project = is_project;
  self->balance->is_project = is_project;
  self->mute->is_project = is_project;
  if (self->stereo_in)
    {
      self->stereo_in->l->is_project = is_project;
      self->stereo_in->r->is_project = is_project;
    }
  if (self->stereo_out)
    {
      self->stereo_out->l->is_project = is_project;
      self->stereo_out->r->is_project = is_project;
    }
  if (self->midi_in)
    {
      self->midi_in->is_project = is_project;
    }
  if (self->midi_out)
    {
      self->midi_out->is_project = is_project;
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
  dest->mute->control = src->mute->control;
  dest->solo = src->solo;
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
      port_update_track_pos (self->amp, NULL, pos);
    }
  if (self->balance)
    {
      port_update_track_pos (
        self->balance, NULL, pos);
    }
  if (self->mute)
    {
      port_update_track_pos (self->mute, NULL, pos);
    }
  if (self->stereo_in)
    {
      port_update_track_pos (
        self->stereo_in->l, NULL, pos);
      port_update_track_pos (
        self->stereo_in->r, NULL, pos);
    }
  if (self->stereo_out)
    {
      port_update_track_pos (
        self->stereo_out->l, NULL, pos);
      port_update_track_pos (
        self->stereo_out->r, NULL, pos);
    }
  if (self->midi_in)
    {
      port_update_track_pos (
        self->midi_in, NULL, pos);
    }
  if (self->midi_out)
    {
      port_update_track_pos (
        self->midi_out, NULL, pos);
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
  float pan =
    port_get_control_value (self->balance, 0);
  float amp =
    port_get_control_value (self->amp, 0);

  if (self->type == FADER_TYPE_AUDIO_CHANNEL ||
      self->type == FADER_TYPE_MONITOR)
    {
      if (self->passthrough)
        {
          /* copy the input to output */
          memcpy (
            &self->stereo_out->l->buf[start_frame],
            &self->stereo_in->l->buf[start_frame],
            nframes * sizeof (float));
          memcpy (
            &self->stereo_out->r->buf[start_frame],
            &self->stereo_in->r->buf[start_frame],
            nframes * sizeof (float));
        }
      else
        {

          nframes_t end = start_frame + nframes;
          float calc_l, calc_r;
          balance_control_get_calc_lr (
            BALANCE_CONTROL_ALGORITHM_LINEAR,
            pan, &calc_l, &calc_r);

          Track * track = NULL;
          if (self->type == FADER_TYPE_AUDIO_CHANNEL)
            {
              track = fader_get_track (self);
              g_return_if_fail (track);
            }

          /* clear it if any of the following is
           * true:
           * 1. muted
           * 2. other track(s) is soloed and this
           *   isn't
           * 3. bounce mode and the track is set
           *   to BOUNCE_OFF */
          if (fader_get_muted (self) ||
              (self->type == FADER_TYPE_AUDIO_CHANNEL &&
                tracklist_has_soloed (TRACKLIST) &&
                !fader_get_soloed (self) &&
                track != P_MASTER_TRACK) ||
              (AUDIO_ENGINE->bounce_mode == BOUNCE_ON &&
               self->type == FADER_TYPE_AUDIO_CHANNEL &&
               track->out_signal_type == TYPE_AUDIO &&
               track->type != TRACK_TYPE_MASTER &&
               !track->bounce))
            {
            }
          else
            {
              while (start_frame < end)
                {
                  /* 1. get input
                   * 2. apply fader
                   * 3. apply pan */
                  self->stereo_out->l->buf[
                    start_frame] =
                      self->stereo_in->l->buf[
                        start_frame] * amp * calc_l;
                  self->stereo_out->r->buf[
                    start_frame] =
                      self->stereo_in->r->buf[
                        start_frame] * amp * calc_r;
                  start_frame++;
                }
            }
        }
    }
  else if (self->type == FADER_TYPE_MIDI_CHANNEL)
    {
      /* TODO if not passthrough, apply volume
       * changes */
      midi_events_append (
        self->midi_in->midi_events,
        self->midi_out->midi_events,
        start_frame, nframes, 0);
    }
}

/**
 * Frees the fader members.
 */
void
fader_free (
  Fader * self)
{
#define DISCONNECT_AND_FREE(x) \
  if (x) \
    port_disconnect_all (x); \
  object_free_w_func_and_null ( \
    port_free, x)

  DISCONNECT_AND_FREE (self->amp);
  DISCONNECT_AND_FREE (self->balance);
  DISCONNECT_AND_FREE (self->mute);

#define DISCONNECT_AND_FREE_STEREO(x) \
  if (x) \
    stereo_ports_disconnect (x); \
  object_free_w_func_and_null ( \
    stereo_ports_free, x)

  DISCONNECT_AND_FREE_STEREO (self->stereo_in);
  DISCONNECT_AND_FREE_STEREO (self->stereo_out);

  DISCONNECT_AND_FREE (self->midi_in);
  DISCONNECT_AND_FREE (self->midi_out);

#undef DISCONNECT_AND_FREE
#undef DISCONNECT_AND_FREE_STEREO

  object_zero_and_free (self);
}
