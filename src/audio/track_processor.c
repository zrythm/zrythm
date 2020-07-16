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

#include "zrythm-config.h"

#include "audio/audio_region.h"
#include "audio/audio_track.h"
#include "audio/channel.h"
#include "audio/clip.h"
#include "audio/control_room.h"
#include "audio/engine.h"
#include "audio/fader.h"
#include "audio/midi.h"
#include "audio/midi_track.h"
#include "audio/recording_manager.h"
#include "audio/track.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/arrays.h"
#include "utils/flags.h"
#include "utils/math.h"
#include "utils/objects.h"
#include "zrythm.h"

#include <glib/gi18n.h>

/**
 * Inits fader after a project is loaded.
 */
void
track_processor_init_loaded (
  TrackProcessor * self,
  bool             is_project)
{
  int max_size = 20;
  Port ** ports =
    calloc ((size_t) max_size, sizeof (Port *));
  int num_ports = 0;
  track_processor_append_ports (
    self, &ports, &num_ports, true, &max_size);
  for (int i = 0; i < num_ports; i++)
    {
      Port * port = ports[i];
      port->magic = PORT_MAGIC;
      port->is_project = is_project;
      port_set_owner_track_processor (port, self);
    }
  free (ports);

  track_processor_set_is_project (self, is_project);
}

/**
 * Inits the MIDI In port of the Channel while
 * exposing it to JACK.
 *
 * This assumes the caller already checked that
 * this channel should have the given MIDI port
 * enabled.
 *
 * @param in 1 for input, 0 for output.
 * @param loading 1 if loading a channel, 0 if
 *   new.
 */
static void
init_midi_port (
  TrackProcessor * self,
  int              in)
{
  if (in)
    {
      self->midi_in =
        port_new_with_type (
          TYPE_EVENT, FLOW_INPUT, "TP MIDI in");
      g_warn_if_fail (IS_PORT (self->midi_in));
      port_set_owner_track_processor (
        self->midi_in, self);
      self->midi_in->id.flags |= PORT_FLAG_SEND_RECEIVABLE;
    }
  else
    {
      self->midi_out =
        port_new_with_type (
          TYPE_EVENT, FLOW_OUTPUT, "TP MIDI out");
      g_warn_if_fail (IS_PORT (self->midi_out));
      port_set_owner_track_processor (
        self->midi_out, self);
    }
}

static void
init_midi_cc_ports (
  TrackProcessor * self,
  int              loading)
{
  for (int i = 0; i < NUM_MIDI_AUTOMATABLES * 16;
       i++)
    {
      char name[400];
      TrackProcessorMidiAutomatable type =
        i % NUM_MIDI_AUTOMATABLES;
      /* starting from 1 */
      int channel = i / NUM_MIDI_AUTOMATABLES + 1;
      Port * cc = NULL;
      switch (type)
        {
        case MIDI_AUTOMATABLE_MOD_WHEEL:
          sprintf (
            name, "Ch%d Mod wheel",
            channel);
          cc =
            port_new_with_type (
              TYPE_CONTROL, FLOW_INPUT, name);
          break;
        case MIDI_AUTOMATABLE_PITCH_BEND:
          sprintf (
            name, "Ch%d Pitch bend",
            channel);
          cc =
            port_new_with_type (
              TYPE_CONTROL, FLOW_INPUT, name);
          cc->maxf = 8191.f;
          cc->minf = -8192.f;
          cc->deff = 0.f;
          cc->zerof = 0.f;
          break;
        default:
          break;
        }
      cc->id.flags |= PORT_FLAG_MIDI_AUTOMATABLE;
      cc->id.flags |= PORT_FLAG_AUTOMATABLE;
      cc->id.port_index = i;

      port_set_owner_track_processor (
        cc, self);
      self->midi_automatables[i] = cc;
    }
}

/**
 * Inits the stereo ports of the Channel while
 * exposing them to the backend.
 *
 * This assumes the caller already checked that
 * this channel should have the given ports
 * enabled.
 *
 * @param in 1 for input, 0 for output.
 * @param loading 1 if loading a channel, 0 if
 *   new.
 */
static void
init_stereo_out_ports (
  TrackProcessor * self,
  bool             in)
{
  Port * l, * r;
  StereoPorts ** sp =
    in ? &self->stereo_in : &self->stereo_out;
  PortFlow flow = in ? FLOW_INPUT : FLOW_OUTPUT;

  l =
    port_new_with_type (
      TYPE_AUDIO, flow,
      in ?
        "TP Stereo in L" :
        "TP Stereo out L");

  r =
    port_new_with_type (
      TYPE_AUDIO, flow,
      in ?
        "TP Stereo in R" :
        "TP Stereo out R");

  port_set_owner_track_processor (l, self);
  port_set_owner_track_processor (r, self);

  *sp = stereo_ports_new_from_existing (l, r);

  if (in)
    {
      l->id.flags |= PORT_FLAG_SEND_RECEIVABLE;
      r->id.flags |= PORT_FLAG_SEND_RECEIVABLE;
    }
}

/**
 * Creates a new track processor for the given
 * track.
 */
TrackProcessor *
track_processor_new (
  Track *          tr)
{
  TrackProcessor * self =
    object_new (TrackProcessor);

  self->track_pos = tr->pos;
  self->track = tr;
  self->l_port_db = 0.f;
  self->r_port_db = 0.f;

  switch  (tr->in_signal_type)
    {
    case TYPE_EVENT:
      init_midi_port (self, 0);
      init_midi_port (self, 1);

      /* set up piano roll port */
      if (track_has_piano_roll (tr) ||
          tr->type == TRACK_TYPE_CHORD)
        {
          self->piano_roll =
            port_new_with_type (
              TYPE_EVENT, FLOW_INPUT,
              "TP Piano Roll");
          self->piano_roll->id.flags =
            PORT_FLAG_PIANO_ROLL;
          port_set_owner_track_processor (
            self->piano_roll, self);
          if (tr->type != TRACK_TYPE_CHORD)
            {
              init_midi_cc_ports (self, false);
            }
        }
      break;
    case TYPE_AUDIO:
      init_stereo_out_ports (self, false);
      init_stereo_out_ports (self, true);
      break;
    default:
      break;
    }

  return self;
}

void
track_processor_append_ports (
  TrackProcessor * self,
  Port ***         ports,
  int *            size,
  bool             is_dynamic,
  int *            max_size)
{
#define _ADD(port) \
  if (is_dynamic) \
    { \
      array_double_size_if_full ( \
        *ports, (*size), (*max_size), Port *); \
    } \
  else if (*size == *max_size) \
    { \
      g_return_if_reached (); \
    } \
  g_warn_if_fail (port); \
  array_append ( \
    *ports, (*size), port)

  if (self->stereo_in)
    {
      _ADD (self->stereo_in->l);
      _ADD (self->stereo_in->r);
    }
  if (self->stereo_out)
    {
      _ADD (self->stereo_out->l);
      _ADD (self->stereo_out->r);
    }
  if (self->midi_in)
    {
      _ADD (self->midi_in);
    }
  if (self->midi_out)
    {
      _ADD (self->midi_out);
    }
  if (self->piano_roll)
    {
      _ADD (self->piano_roll);
    }
  for (int i = 0; i < NUM_MIDI_AUTOMATABLES * 16;
       i++)
    {
      if (self->midi_automatables[i])
        {
          _ADD (self->midi_automatables[i]);
        }
    }
}

void
track_processor_set_is_project (
  TrackProcessor * self,
  bool             is_project)
{
  self->is_project = is_project;

  int max_size = 20;
  Port ** ports =
    calloc ((size_t) max_size, sizeof (Port *));
  int num_ports = 0;
  track_processor_append_ports (
    self, &ports, &num_ports, true, &max_size);
  for (int i = 0; i < num_ports; i++)
    {
      Port * port = ports[i];
      port_set_is_project (port, is_project);
    }
  free (ports);
}

/**
 * Clears all buffers.
 */
void
track_processor_clear_buffers (
  TrackProcessor * self)
{
  /*Track * track =*/
    /*track_processor_get_track (self);*/

  if (self->stereo_in)
    {
      port_clear_buffer (self->stereo_in->l);
      port_clear_buffer (self->stereo_in->r);
    }
  if (self->stereo_out)
    {
      port_clear_buffer (self->stereo_out->l);
      port_clear_buffer (self->stereo_out->r);
    }
  if (self->midi_in)
    {
      port_clear_buffer (self->midi_in);
    }
  if (self->midi_out)
    {
      port_clear_buffer (self->midi_out);
    }
  if (self->piano_roll)
    {
      port_clear_buffer (self->piano_roll);
    }
}

/**
 * Disconnects all ports connected to the
 * TrackProcessor.
 */
void
track_processor_disconnect_all (
  TrackProcessor * self)
{
  Track * track =
    track_processor_get_track (self);
  switch (track->in_signal_type)
    {
    case TYPE_AUDIO:
      port_disconnect_all (self->stereo_in->l);
      port_disconnect_all (self->stereo_in->r);
      port_disconnect_all (self->stereo_out->l);
      port_disconnect_all (self->stereo_out->r);
      break;
    case TYPE_EVENT:
      port_disconnect_all (self->midi_in);
      port_disconnect_all (self->midi_out);
      if (track_has_piano_roll (track))
        port_disconnect_all (self->piano_roll);
      break;
    default:
      break;
    }
}

Track *
track_processor_get_track (
  TrackProcessor * self)
{
  return self->track;

#if 0
  g_return_val_if_fail (
    self &&
    self->track_pos < TRACKLIST->num_tracks, NULL);
  Track * track =
    TRACKLIST->tracks[self->track_pos];
  g_return_val_if_fail (track, NULL);

  return track;
#endif
}

/**
 * Process the TrackProcessor.
 *
 * @param g_start_frames The global start frames.
 * @param local_offset The local start frames.
 * @param nframes The number of frames to process.
 */
void
track_processor_process (
  TrackProcessor * self,
  const long       g_start_frames,
  const nframes_t  local_offset,
  const nframes_t  nframes)
{
  Track * tr = track_processor_get_track (self);
  g_return_if_fail (tr);

  /*g_message ("%s: %s", __func__, tr->name);*/

  /* set the audio clip contents to stereo out */
  if (tr->type == TRACK_TYPE_AUDIO)
    {
      audio_track_fill_stereo_ports_from_clip (
        tr, self->stereo_out,
        g_start_frames, local_offset,
        nframes);
    }

  /* set the piano roll contents to midi out */
  if (track_has_piano_roll (tr) ||
      tr->type == TRACK_TYPE_CHORD)
    {
      Port * pr = self->piano_roll;

      /* panic MIDI if necessary */
      if (g_atomic_int_get (
            &AUDIO_ENGINE->panic))
        {
          midi_events_panic (
            pr->midi_events, 1);
        }
      /* get events from track if playing */
      else if (TRANSPORT->play_state ==
               PLAYSTATE_ROLLING)
        {
          /* fill midi events from piano roll
           * data */
          midi_track_fill_midi_events (
            tr, g_start_frames,
            local_offset, nframes,
            pr->midi_events);
        }
      midi_events_dequeue (
        pr->midi_events);
      if (pr->midi_events->num_events > 0)
        g_message (
          "%s piano roll has %d events",
          tr->name,
          pr->midi_events->num_events);

      /* append midi events from modwheel and
       * pitchbend to MIDI out */
      if (tr->type != TRACK_TYPE_CHORD)
        {
          for (int i = 0;
               i < NUM_MIDI_AUTOMATABLES * 16; i++)
            {
              Port * cc = self->midi_automatables[i];
              if (math_floats_equal (
                    self->last_automatable_vals[i],
                    cc->control))
                continue;

              TrackProcessorMidiAutomatable type =
                i % NUM_MIDI_AUTOMATABLES;
              /* starting from 1 */
              int channel =
                i / NUM_MIDI_AUTOMATABLES + 1;
              switch (type)
                {
                case MIDI_AUTOMATABLE_PITCH_BEND:
                  midi_events_add_pitchbend (
                    self->midi_out->midi_events,
                    channel,
                    math_round_float_to_int (
                      cc->control),
                    local_offset, false);
                  break;
                case MIDI_AUTOMATABLE_MOD_WHEEL:
                  midi_events_add_control_change (
                    self->midi_out->midi_events,
                    channel,
                    0x01,
                    math_round_float_to_type (
                      cc->control * 127.f, midi_byte_t),
                    local_offset, false);
                  break;
                default:
                  break;
                }
              self->last_automatable_vals[i] =
                cc->control;
            }
        }
      if (self->midi_out->midi_events->num_events > 0)
        g_message (
          "%s midi processor out has %d events",
          tr->name,
          self->midi_out->midi_events->num_events);

      /* append the midi events from piano roll to
       * MIDI out */
      midi_events_append (
        pr->midi_events,
        self->midi_out->midi_events, local_offset,
        nframes, false);
    }

  if (track_type_can_record (tr->type))
    {
      /* handle recording. this will only create
       * events in regions. it will not copy the
       * input content to the output ports */
      recording_manager_handle_recording (
        RECORDING_MANAGER, self, g_start_frames,
        local_offset, nframes);
    }

  /* add inputs to outputs */
  switch (tr->in_signal_type)
    {
    case TYPE_AUDIO:
      for (nframes_t l = local_offset;
           l < nframes; l++)
        {
          g_return_if_fail (
            l < AUDIO_ENGINE->block_length);

          self->stereo_out->l->buf[l] +=
            self->stereo_in->l->buf[l];
          self->stereo_out->r->buf[l] +=
            self->stereo_in->r->buf[l];
        }
      break;
    case TYPE_EVENT:
      /* change the MIDI channel on the midi input
       * to the channel set on the track */
      if (!tr->passthrough_midi_input)
        {
          midi_events_set_channel (
            self->midi_in->midi_events, 0,
            tr->midi_ch);
        }
      midi_events_append (
        self->midi_in->midi_events,
        self->midi_out->midi_events, local_offset,
        nframes, 0);
      break;
    default:
      break;
    }
}

/**
 * Disconnect stereo in ports from the fader.
 *
 * Used when there is no plugin in the channel.
 */
void
track_processor_disconnect_from_prefader (
  TrackProcessor * self)
{
  Track * tr = track_processor_get_track (self);
  Fader * prefader = tr->channel->prefader;
  switch (tr->in_signal_type)
    {
    case TYPE_AUDIO:
      if (tr->out_signal_type == TYPE_AUDIO)
        {
          port_disconnect (
            self->stereo_out->l,
            prefader->stereo_in->l);
          port_disconnect (
            self->stereo_out->r,
            prefader->stereo_in->r);
        }
      break;
    case TYPE_EVENT:
      if (tr->out_signal_type == TYPE_EVENT)
        {
          port_disconnect (
            self->midi_out, prefader->midi_in);
        }
      break;
    default:
      break;
    }
}

/**
 * Connects the TrackProcessor's stereo out ports to
 * the Channel's prefader in ports.
 *
 * Used when deleting the only plugin left.
 */
void
track_processor_connect_to_prefader (
  TrackProcessor * self)
{
  Track * tr = track_processor_get_track (self);
  Fader * prefader = tr->channel->prefader;

  /* connect only if signals match */
  if (tr->in_signal_type == TYPE_AUDIO &&
      tr->out_signal_type == TYPE_AUDIO)
    {
      port_connect (
        self->stereo_out->l,
        prefader->stereo_in->l, 1);
      port_connect (
        self->stereo_out->r,
        prefader->stereo_in->r, 1);
    }
  if (tr->in_signal_type == TYPE_EVENT &&
      tr->out_signal_type == TYPE_EVENT)
    {
      port_connect (
        self->midi_out,
        prefader->midi_in, 1);
    }
}

/**
 * Disconnect the TrackProcessor's out ports
 * from the Plugin's input ports.
 */
void
track_processor_disconnect_from_plugin (
  TrackProcessor * self,
  Plugin         * pl)
{
  Track * tr = track_processor_get_track (self);

  int i;
  Port * in_port;
  PortType type = tr->in_signal_type;

  for (i = 0; i < pl->num_in_ports; i++)
    {
      in_port = pl->in_ports[i];

      if (type == TYPE_AUDIO)
        {
          if (in_port->id.type !=
                TYPE_AUDIO)
            continue;

          if (ports_connected (
                self->stereo_out->l,
                in_port))
            port_disconnect (
              self->stereo_out->l,
              in_port);
          if (ports_connected (
                self->stereo_out->r,
                in_port))
            port_disconnect (
              self->stereo_out->r,
              in_port);
        }
      else if (type == TYPE_EVENT)
        {
          if (in_port->id.type != TYPE_EVENT)
            continue;

          if (ports_connected (
                self->midi_out, in_port))
            port_disconnect (
              self->midi_out, in_port);
        }
    }
}

/**
 * Connect the TrackProcessor's out ports to the
 * Plugin's input ports.
 */
void
track_processor_connect_to_plugin (
  TrackProcessor * self,
  Plugin         * pl)
{
  Track * tr = track_processor_get_track (self);
  int last_index, num_ports_to_connect, i;
  Port * in_port;

  if (tr->in_signal_type == TYPE_EVENT)
    {
      /* Connect MIDI port to the plugin */
      for (i = 0; i < pl->num_in_ports; i++)
        {
          in_port = pl->in_ports[i];
          if (in_port->id.type == TYPE_EVENT &&
              in_port->id.flow == FLOW_INPUT)
            {
              port_connect (
                self->midi_out, in_port, 1);
            }
        }
    }
  else if (tr->in_signal_type == TYPE_AUDIO)
    {
      num_ports_to_connect = 0;
      if (pl->descr->num_audio_ins == 1)
        {
          num_ports_to_connect = 1;
        }
      else if (pl->descr->num_audio_ins > 1)
        {
          num_ports_to_connect = 2;
        }

      last_index = 0;
      for (i = 0; i < num_ports_to_connect; i++)
        {
          for (;
               last_index < pl->num_in_ports;
               last_index++)
            {
              in_port =
                pl->in_ports[
                  last_index];
              if (in_port->id.type == TYPE_AUDIO)
                {
                  if (i == 0)
                    {
                      port_connect (
                        self->stereo_out->l,
                        in_port, 1);
                      last_index++;
                      break;
                    }
                  else if (i == 1)
                    {
                      port_connect (
                        self->stereo_out->r,
                        in_port, 1);
                      last_index++;
                      break;
                    }
                }
            }
        }
    }
}

void
track_processor_set_track_pos (
  TrackProcessor * self,
  int              pos)
{
  self->track_pos = pos;

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
  if (self->piano_roll)
    {
      port_update_track_pos (
        self->piano_roll, NULL, pos);
    }
}

/**
 * Frees the members of the TrackProcessor.
 */
void
track_processor_free (
  TrackProcessor * self)
{
  if (self->stereo_in)
    {
      stereo_ports_disconnect (self->stereo_in);
      object_free_w_func_and_null (
        stereo_ports_free, self->stereo_in);
    }
  if (IS_PORT (self->midi_in))
    {
      port_disconnect_all (self->midi_in);
      object_free_w_func_and_null (
        port_free, self->midi_in);
    }
  if (IS_PORT (self->piano_roll))
    {
      port_disconnect_all (self->piano_roll);
      object_free_w_func_and_null (
        port_free, self->piano_roll);
    }
  for (int i = 0; i < NUM_MIDI_AUTOMATABLES * 16;
       i++)
    {
      Port * port = self->midi_automatables[i];
      if (IS_PORT (port))
        {
          port_disconnect_all (port);
          object_free_w_func_and_null (
            port_free, port);
        }
    }

  if (self->stereo_out)
    {
      stereo_ports_disconnect (self->stereo_out);
      object_free_w_func_and_null (
        stereo_ports_free, self->stereo_out);
    }
  if (IS_PORT (self->midi_out))
    {
      port_disconnect_all (self->midi_out);
      object_free_w_func_and_null (
        port_free, self->midi_out);
    }

  object_zero_and_free (self);
}
