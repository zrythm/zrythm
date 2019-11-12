/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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

/** \file
 * Implementation of Port API.
 * Ports are passed when processing audio signals. They carry buffers
 * with data */

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "project.h"
#include "audio/channel.h"
#ifdef HAVE_JACK
#include "audio/engine_jack.h"
#endif
#include "audio/midi.h"
#include "audio/mixer.h"
#include "audio/modulator.h"
#include "audio/pan.h"
#include "audio/passthrough_processor.h"
#include "audio/port.h"
#include "plugins/plugin.h"
#include "plugins/lv2/lv2_ui.h"
#include "utils/arrays.h"
#include "utils/math.h"
#include "utils/objects.h"
#include "utils/string.h"

#include <gtk/gtk.h>

#define SLEEPTIME_USEC 60

/**
 * This function finds the Ports corresponding to
 * the PortIdentifiers for srcs and dests.
 *
 * Should be called after the ports are loaded from
 * yml.
 */
void
port_init_loaded (Port * this)
{
#define SET_FIELDS_FROM_ID(id,port) \
  switch (id->owner_type) \
    { \
    case PORT_OWNER_TYPE_PLUGIN: \
      port->track = \
        TRACKLIST->tracks[id->track_pos]; \
      g_warn_if_fail (port->track); \
      port->plugin = \
        port->track->channel->plugins[ \
          id->plugin_slot]; \
      g_warn_if_fail (port->plugin); \
      break; \
    case PORT_OWNER_TYPE_TRACK: \
    case PORT_OWNER_TYPE_TRACK_PROCESSOR: \
      port->track = \
        TRACKLIST->tracks[id->track_pos]; \
      g_warn_if_fail (port->track); \
      break; \
    default: \
      break; \
    }

  /* find plugin and track */
  SET_FIELDS_FROM_ID ((&this->identifier),this);

  PortIdentifier * id;
  for (int i = 0; i < this->num_srcs; i++)
    {
      id = &this->src_ids[i];
      this->srcs[i] =
        port_find_from_identifier (id);
      g_warn_if_fail (this->srcs[i]);
    }
  for (int i = 0; i < this->num_dests; i++)
    {
      id = &this->dest_ids[i];
      this->dests[i] =
        port_find_from_identifier (id);
      g_warn_if_fail (this->dests[i]);
    }

  /* connect to backend if flag set */
  if (port_is_exposed_to_backend (this))
    {
      port_set_expose_to_backend (this, 1);
    }

  this->buf =
    calloc (80000, sizeof (float));
}

/**
 * Finds the Port corresponding to the identifier.
 *
 * @param id The PortIdentifier to use for
 *   searching.
 */
Port *
port_find_from_identifier (
  PortIdentifier * id)
{
  Track * tr;
  Channel * ch;
  Plugin * pl;
  switch (id->owner_type)
    {
    case PORT_OWNER_TYPE_BACKEND:
      switch (id->type)
        {
        case TYPE_EVENT:
          if (id->flow == FLOW_OUTPUT)
            { /* TODO */ }
          else if (id->flow == FLOW_INPUT)
            {
              if (id->flags & PORT_FLAG_MANUAL_PRESS)
                return
                  AUDIO_ENGINE->
                    midi_editor_manual_press;
            }
          break;
        case TYPE_AUDIO:
          if (id->flow == FLOW_OUTPUT)
            {
              if (id->flags & PORT_FLAG_STEREO_L)
                return
                  AUDIO_ENGINE->monitor_out->l;
              else if (id->flags &
                         PORT_FLAG_STEREO_R)
                return
                  AUDIO_ENGINE->monitor_out->r;
            }
          else if (id->flow == FLOW_INPUT)
            {
              /* none */
            }
          break;
        default:
          break;
        }
      break;
    case PORT_OWNER_TYPE_PLUGIN:
      tr =
        TRACKLIST->tracks[id->track_pos];
      g_warn_if_fail (tr);
      pl =
        tr->channel->plugins[id->plugin_slot];
      g_warn_if_fail (pl);
      switch (id->flow)
        {
        case FLOW_INPUT:
          return
            pl->in_ports[id->port_index];
          break;
        case FLOW_OUTPUT:
          return
            pl->out_ports[id->port_index];
          break;
        case FLOW_UNKNOWN:
          return
            pl->unknown_ports[id->port_index];
          break;
        }
      break;
    case PORT_OWNER_TYPE_TRACK_PROCESSOR:
      tr = TRACKLIST->tracks[id->track_pos];
      g_warn_if_fail (tr);
      switch (id->type)
        {
        case TYPE_EVENT:
          if (id->flow == FLOW_OUTPUT)
            {
              return tr->processor.midi_out;
            }
          else if (id->flow == FLOW_INPUT)
            {
              if (id->flags & PORT_FLAG_PIANO_ROLL)
                return tr->processor.piano_roll;
              else
                return tr->processor.midi_in;
            }
          break;
        case TYPE_AUDIO:
          if (id->flow == FLOW_OUTPUT)
            {
              if (id->flags & PORT_FLAG_STEREO_L)
                return tr->processor.stereo_out->l;
              else if (id->flags &
                         PORT_FLAG_STEREO_R)
                return tr->processor.stereo_out->r;
            }
          else if (id->flow == FLOW_INPUT)
            {
              if (id->flags & PORT_FLAG_STEREO_L)
                return tr->processor.stereo_in->l;
              else if (id->flags &
                         PORT_FLAG_STEREO_R)
                return tr->processor.stereo_in->r;
            }
          break;
        default:
          break;
        }
      break;
    case PORT_OWNER_TYPE_TRACK:
      tr = TRACKLIST->tracks[id->track_pos];
      g_warn_if_fail (tr);
      ch = tr->channel;
      g_warn_if_fail (ch);
      switch (id->type)
        {
        case TYPE_EVENT:
          if (id->flow == FLOW_OUTPUT)
            {
              return ch->midi_out;
            }
          break;
        case TYPE_AUDIO:
          if (id->flow == FLOW_OUTPUT)
            {
              if (id->flags & PORT_FLAG_STEREO_L)
                return ch->stereo_out->l;
              else if (id->flags &
                         PORT_FLAG_STEREO_R)
                return ch->stereo_out->r;
            }
          break;
        default:
          break;
        }
      break;
    case PORT_OWNER_TYPE_FADER:
      tr = TRACKLIST->tracks[id->track_pos];
      g_warn_if_fail (tr);
      ch = tr->channel;
      g_warn_if_fail (ch);
      switch (id->type)
        {
        case TYPE_EVENT:
          switch (id->flow)
            {
            case FLOW_INPUT:
              return ch->fader.midi_in;
              break;
            case FLOW_OUTPUT:
              return ch->fader.midi_out;
              break;
            default:
              break;
            }
          break;
        case TYPE_AUDIO:
          if (id->flow == FLOW_OUTPUT)
            {
              if (id->flags & PORT_FLAG_STEREO_L)
                return ch->fader.stereo_out->l;
              else if (id->flags &
                         PORT_FLAG_STEREO_R)
                return ch->fader.stereo_out->r;
            }
          else if (id->flow == FLOW_INPUT)
            {
              if (id->flags & PORT_FLAG_STEREO_L)
                return ch->fader.stereo_in->l;
              else if (id->flags &
                         PORT_FLAG_STEREO_R)
                return ch->fader.stereo_in->r;
            }
          break;
        default:
          break;
        }
      break;
    case PORT_OWNER_TYPE_PREFADER:
      tr = TRACKLIST->tracks[id->track_pos];
      g_warn_if_fail (tr);
      ch = tr->channel;
      g_warn_if_fail (ch);
      switch (id->type)
        {
        case TYPE_EVENT:
          switch (id->flow)
            {
            case FLOW_INPUT:
              return ch->prefader.midi_in;
              break;
            case FLOW_OUTPUT:
              return ch->prefader.midi_out;
              break;
            default:
              break;
            }
          break;
        case TYPE_AUDIO:
          if (id->flow == FLOW_OUTPUT)
            {
              if (id->flags & PORT_FLAG_STEREO_L)
                return ch->prefader.stereo_out->l;
              else if (id->flags &
                         PORT_FLAG_STEREO_R)
                return ch->prefader.stereo_out->r;
            }
          else if (id->flow == FLOW_INPUT)
            {
              if (id->flags & PORT_FLAG_STEREO_L)
                return ch->prefader.stereo_in->l;
              else if (id->flags &
                         PORT_FLAG_STEREO_R)
                return ch->prefader.stereo_in->r;
            }
          break;
        default:
          break;
        }
      break;
    case PORT_OWNER_TYPE_SAMPLE_PROCESSOR:
      if (id->flags & PORT_FLAG_STEREO_L)
        return SAMPLE_PROCESSOR->stereo_out->l;
      else if (id->flags &
                 PORT_FLAG_STEREO_R)
        return SAMPLE_PROCESSOR->stereo_out->r;
      else
        g_return_val_if_reached (NULL);
    case PORT_OWNER_TYPE_MONITOR_FADER:
      if (id->flow == FLOW_OUTPUT)
        {
          if (id->flags & PORT_FLAG_STEREO_L)
            return
              MONITOR_FADER->
                stereo_out->l;
          else if (id->flags &
                     PORT_FLAG_STEREO_R)
            return
              MONITOR_FADER->
                stereo_out->r;
        }
      else if (id->flow == FLOW_INPUT)
        {
          if (id->flags & PORT_FLAG_STEREO_L)
            return
              MONITOR_FADER->
                stereo_in->l;
          else if (id->flags &
                     PORT_FLAG_STEREO_R)
            return
              MONITOR_FADER->
                stereo_in->r;
        }
    }

  g_return_val_if_reached (NULL);
}

void
stereo_ports_init_loaded (StereoPorts * sp)
{
  port_init_loaded (sp->l);
  port_init_loaded (sp->r);
}

/**
 * Returns if the 2 PortIdentifier's are equal.
 */
int
port_identifier_is_equal (
  PortIdentifier * src,
  PortIdentifier * dest)
{
  return
    string_is_equal (
      dest->label, src->label, 0) &&
    dest->owner_type == src->owner_type &&
    dest->type == src->type &&
    dest->flow == src->flow &&
    dest->flags == src->flags &&
    dest->plugin_slot == src->plugin_slot &&
    dest->track_pos == src->track_pos &&
    dest->port_index == src->port_index;
}

/**
 * Creates port.
 *
 * Sets id and updates appropriate counters.
 */
Port *
port_new (
  const char * label)
{
  Port * port = calloc (1, sizeof (Port));

  port->identifier.plugin_slot = -1;
  port->identifier.track_pos = -1;

  port->num_dests = 0;
  g_warn_if_fail (
    AUDIO_ENGINE->block_length > 0);
  port->buf =
    calloc (AUDIO_ENGINE->block_length,
            sizeof (float));
  port->identifier.flow = FLOW_UNKNOWN;
  port->identifier.label = g_strdup (label);

  g_message ("[port_new] Creating port %s",
             port->identifier.label);
  return port;
}

/**
 * Creates port.
 */
Port *
port_new_with_type (
  PortType     type,
  PortFlow     flow,
  const char * label)
{
  Port * self = port_new (label);

  self->identifier.type = type;
  if (self->identifier.type == TYPE_EVENT)
    self->midi_events = midi_events_new (self);
  self->identifier.flow = flow;

  if (type == TYPE_EVENT)
    {
      self->midi_events =
        midi_events_new (self);
    }

  return self;
}

/**
 * Creates stereo ports.
 */
StereoPorts *
stereo_ports_new_from_existing (
  Port * l, Port * r)
{
  StereoPorts * sp =
    calloc (1, sizeof (StereoPorts));
  sp->l = l;
  l->identifier.flags |= PORT_FLAG_STEREO_L;
  r->identifier.flags |= PORT_FLAG_STEREO_R;
  sp->r = r;

  return sp;
}

/**
 * Connects the internal ports using
 * port_connect().
 *
 * @param locked Lock the connection.
 */
void
stereo_ports_connect (
  StereoPorts * src,
  StereoPorts * dest,
  int           locked)
{
  port_connect (
    src->l,
    dest->l, locked);
  port_connect (
    src->r,
    dest->r, locked);
}

/**
 * Gathers all ports in the project and puts them
 * in the given array and size.
 */
void
port_get_all (
  Port ** ports,
  int *   size)
{
  *size = 0;

#define _ADD(port) \
  array_append ( \
    ports, (*size), \
    port)

  /* add fader ports */
  _ADD (MONITOR_FADER->stereo_in->l);
  _ADD (MONITOR_FADER->stereo_in->r);
  _ADD (MONITOR_FADER->stereo_out->l);
  _ADD (MONITOR_FADER->stereo_out->r);

  _ADD (AUDIO_ENGINE->monitor_out->l);
  _ADD (AUDIO_ENGINE->monitor_out->r);
  _ADD (AUDIO_ENGINE->midi_editor_manual_press);
  _ADD (SAMPLE_PROCESSOR->stereo_out->l);
  _ADD (SAMPLE_PROCESSOR->stereo_out->r);

  Channel * ch;
  Track * tr;
  int i;
  for (i = 0; i < TRACKLIST->num_tracks; i++)
    {
      tr = TRACKLIST->tracks[i];
      if (!tr->channel)
        continue;
      ch = tr->channel;

      channel_append_all_ports (
        ch, ports, size, 1);
    }
}

/**
 * Creates port and adds given data to it.
 */
Port *
port_new_with_data (
  PortInternalType internal_type,
  PortType     type,
  PortFlow     flow,
  const char * label,
  void *       data)
{
  Port * port =
    port_new_with_type (type, flow, label);

  /** TODO semaphore **/
  port->data = data;
  port->internal_type = internal_type;

  /** TODO end semaphore */

  return port;
}

/**
 * Sets the owner plugin & its slot.
 */
void
port_set_owner_plugin (
  Port *   port,
  Plugin * pl)
{
  g_warn_if_fail (port && pl);

  port->plugin = pl;
  port->track = pl->track;
  port->identifier.track_pos = pl->track->pos;
  port->identifier.plugin_slot = pl->slot;
  port->identifier.owner_type =
    PORT_OWNER_TYPE_PLUGIN;
}

/**
 * Sets the owner sample processor.
 */
void
port_set_owner_sample_processor (
  Port *   port,
  SampleProcessor * sample_processor)
{
  port->sample_processor = sample_processor;
  port->identifier.owner_type =
    PORT_OWNER_TYPE_SAMPLE_PROCESSOR;
}

/**
 * Sets the owner track & its ID.
 */
void
port_set_owner_track (
  Port *    port,
  Track *   track)
{
  g_warn_if_fail (port && track);

  port->track = track;
  port->identifier.track_pos = track->pos;
  port->identifier.owner_type =
    PORT_OWNER_TYPE_TRACK;
}

/**
 * Sets the owner track & its ID.
 */
void
port_set_owner_track_processor (
  Port *    port,
  Track *   track)
{
  port_set_owner_track (port, track);
  port->identifier.owner_type =
    PORT_OWNER_TYPE_TRACK_PROCESSOR;
}

/**
 * Sets the owner fader & its ID.
 */
void
port_set_owner_fader (
  Port *    port,
  Fader *   fader)
{
  g_warn_if_fail (port && fader);

  port->track = fader->channel->track;
  port->identifier.track_pos =
    fader->channel->track->pos;
  port->identifier.owner_type =
    PORT_OWNER_TYPE_FADER;
}

/**
 * Sets the owner fader & its ID.
 */
void
port_set_owner_prefader (
  Port *                 port,
  PassthroughProcessor * fader)
{
  g_warn_if_fail (port && fader);

  port->track = fader->channel->track;
  port->identifier.track_pos =
    fader->channel->track->pos;
  port->identifier.owner_type =
    PORT_OWNER_TYPE_PREFADER;
}

/**
 * Returns whether the Port's can be connected (if
 * the connection will be valid and won't break the
 * acyclicity of the graph).
 */
int
ports_can_be_connected (
  const Port * src,
  const Port *dest)
{
  return
    graph_validate (&MIXER->router, src, dest);
}

/**
 * Connets src to dest.
 *
 * @param locked Lock the connection or not.
 */
int
port_connect (
  Port * src,
  Port * dest,
  const int locked)
{
  g_warn_if_fail (src != NULL);
  g_warn_if_fail (dest != NULL);
  port_disconnect (src, dest);
  if ((src->identifier.type !=
       dest->identifier.type) &&
      !(src->identifier.type == TYPE_CV &&
        dest->identifier.type == TYPE_CONTROL))
    {
      g_warning ("Cannot connect ports, incompatible types");
      return -1;
    }
  src->dests[src->num_dests] = dest;
  port_identifier_copy (
    &dest->identifier,
    &src->dest_ids[src->num_dests]);
  src->multipliers[src->num_dests] = 1.f;
  src->dest_locked[src->num_dests] = locked;
  src->dest_enabled[src->num_dests] = 1;
  src->num_dests++;
  dest->srcs[dest->num_srcs] = src;
  port_identifier_copy (
    &src->identifier,
    &dest->src_ids[dest->num_srcs]);
  dest->num_srcs++;

  /* set base value if cv -> control */
  if (src->identifier.type == TYPE_CV &&
      dest->identifier.type == TYPE_CONTROL)
    {
      if (dest->internal_type == INTERNAL_LV2_PORT)
        {
          dest->base_value =
            dest->lv2_port->control;
        }
      /*dest->has_modulators = 1;*/
    }

  char * sd =
    port_get_full_designation (src);
  char * dd =
    port_get_full_designation (dest);
  g_message (
    "Connected port \"%s\" to \"%s\"",
    sd, dd);
  g_free (sd);
  g_free (dd);
  return 0;
}

/**
 * Disconnects src from dest.
 */
int
port_disconnect (Port * src, Port * dest)
{
  g_warn_if_fail (src && dest);

  int pos = -1;
  /* disconnect dest from src */
  array_delete_return_pos (
    src->dests,
    src->num_dests,
    dest,
    pos);
  if (pos >= 0)
    {
      for (int i = pos;
           i < src->num_dests;
           i++)
        {
          port_identifier_copy (
            &src->dest_ids[i + 1],
            &src->dest_ids[i]);
        }
    }

  /* disconnect src from dest */
  pos = -1;
  array_delete_return_pos (
    dest->srcs,
    dest->num_srcs,
    src,
    pos);
  if (pos >= 0)
    {
      for (int i = pos;
           i < dest->num_srcs;
           i++)
        {
          port_identifier_copy (
            &dest->src_ids[i + 1],
            &dest->src_ids[i]);
        }
    }

  char * sd =
    port_get_full_designation (src);
  char * dd =
    port_get_full_designation (dest);
  g_message (
    "Disconnected port \"%s\" from \"%s\"",
    sd, dd);
  g_free (sd);
  g_free (dd);
  return 0;
}

/**
 * Returns if the two ports are connected or not.
 */
int
ports_connected (Port * src, Port * dest)
{
  if (array_contains ((void **)src->dests,
                      src->num_dests,
                      dest))
    return 1;
  return 0;
}

/**
 * Disconnects all srcs and dests from port.
 */
int
port_disconnect_all (Port * port)
{
  g_warn_if_fail (port);

  FOREACH_SRCS (port)
    {
      Port * src = port->srcs[i];
      port_disconnect (src, port);
    }

  FOREACH_DESTS (port)
    {
      Port * dest = port->dests[i];
      port_disconnect (port, dest);
    }

  if (port->internal_type == INTERNAL_JACK_PORT)
    {
      port_set_expose_to_jack (
        port, 0);
    }

  return 0;
}

/**
 * Sets the given control value to the
 * corresponding underlying structure in the Port.
 *
 * @param is_normalized Whether the given value is
 *   normalized between 0 and 1.
 * @param forward_event Whether to forward a port
 *   control change event to the plugin UI. Only
 *   applicable for plugin control ports.
 */
void
port_set_control_value (
  Port *      self,
  const float val,
  const int   is_normalized,
  const int   forward_event)
{
  g_return_if_fail (
    self->identifier.type == TYPE_CONTROL);

  if (self->lv2_port)
    {
      Lv2Port * lv2_port = self->lv2_port;
      g_return_if_fail (
        self->plugin && self->plugin->lv2);
      Lv2Plugin * lv2_plugin =
        self->plugin->lv2;
      if (is_normalized)
        {
          float minf = port_get_minf (self);
          float maxf = port_get_maxf (self);
          self->base_value =
            minf + val * (maxf - minf);
        }
      else
        {
          self->base_value = val;
        }
      self->lv2_port->control =
        self->base_value;

      if (forward_event)
        {
          lv2_ui_send_control_val_event_from_plugin_to_ui (
            lv2_plugin, lv2_port);
        }
    }
  else
    g_warn_if_reached ();
}

/**
 * Gets the given control value from the
 * corresponding underlying structure in the Port.
 *
 * @param normalized Whether to get the value
 *   normalized or not.
 */
float
port_get_control_value (
  Port *      self,
  const int   normalize)
{
  g_return_val_if_fail (
    self->identifier.type == TYPE_CONTROL, 0.f);

  if (self->lv2_port)
    {
      g_return_val_if_fail (
        self->plugin && self->plugin->lv2, 0.f);
      if (normalize)
        {
          float minf = port_get_minf (self);
          float maxf = port_get_maxf (self);
          return
            (self->lv2_port->control - minf) /
            (maxf - minf);
        }
      else
        {
          return self->lv2_port->control;
        }
    }
  else
    g_return_val_if_reached (0.f);
}

/**
 * Returns the minimum possible value for this
 * port.
 *
 * Note that for Audio we should consider the
 * amp (0.0 and 2.0).
 */
float
port_get_minf (
  Port * port)
{
  switch (port->identifier.type)
    {
    case TYPE_AUDIO:
      return 0.f;
    case TYPE_CV:
      return -1.f;
    case TYPE_EVENT:
      return  0.f;
    case TYPE_CONTROL:
      /* FIXME this wont work with zrythm controls
       * like volume and pan */
      g_return_val_if_fail (
        port->lv2_port &&
        port->lv2_port->lv2_control, 0.f);
      return port->lv2_port->lv2_control->minf;
    default:
      break;
    }
  g_return_val_if_reached (0.f);
}

/**
 * Returns the maximum possible value for this
 * port.
 *
 * Note that for Audio we should consider the
 * amp (0.0 and 2.0).
 */
float
port_get_maxf (
  Port * port)
{
  switch (port->identifier.type)
    {
    case TYPE_AUDIO:
      return 2.f;
    case TYPE_CV:
      return 1.f;
    case TYPE_EVENT:
      return  1.f;
    case TYPE_CONTROL:
      /* FIXME this wont work with zrythm controls
       * like volume and pan */
      g_return_val_if_fail (
        port->lv2_port &&
        port->lv2_port->lv2_control, 0.f);
      return port->lv2_port->lv2_control->maxf;
    default:
      break;
    }
  g_return_val_if_reached (0.f);
}

/**
 * Returns the zero value for the given port.
 *
 * Note that for Audio we should consider the
 * amp (0.0 and 2.0).
 */
float
port_get_zerof (
  Port * port)
{
  switch (port->identifier.type)
    {
    case TYPE_AUDIO:
      return 0.f;
    case TYPE_CV:
      return 0.f;
    case TYPE_EVENT:
      return 0.f;
    case TYPE_CONTROL:
      /* FIXME this wont work with zrythm controls
       * like volume and pan */
      g_return_val_if_fail (
        port->lv2_port &&
        port->lv2_port->lv2_control, 0.f);
      return port->lv2_port->lv2_control->minf;
    default:
      break;
    }
  g_return_val_if_reached (0.f);
}

/**
 * Removes all the given ports from the project,
 * optionally freeing them.
 */
int
ports_remove (
  Port ** ports,
  int *   num_ports)
{
  int i;
  Port * port;

  /* go through each port */
  for (i = 0; i < *num_ports; i++)
    {
      port = ports[i];

      /* assert no connections */
      g_warn_if_fail (port->num_srcs == 0);
      g_warn_if_fail (port->num_dests == 0);

      free_later (port, port_free);
    }
  * num_ports = 0;

  return i;
}

/**
 * Creates stereo ports for generic use.
 *
 * @param in 1 for in, 0 for out.
 * @param owner Pointer to the owner. The type is
 *   determined by owner_type.
 */
StereoPorts *
stereo_ports_new_generic (
  int           in,
  const char *  name,
  PortOwnerType owner_type,
  void *        owner)
{
  char * pll =
    g_strdup_printf (
      "%s L", name);
  char * plr =
    g_strdup_printf (
      "%s R", name);

  StereoPorts * ports =
    stereo_ports_new_from_existing (
      port_new_with_type (
        TYPE_AUDIO,
        in ? FLOW_INPUT : FLOW_OUTPUT,
        pll),
      port_new_with_type (
        TYPE_AUDIO,
        in ? FLOW_INPUT : FLOW_OUTPUT,
        plr));
  ports->l->identifier.flags |=
    PORT_FLAG_STEREO_L;
  ports->r->identifier.flags |=
    PORT_FLAG_STEREO_R;

  switch (owner_type)
    {
    case PORT_OWNER_TYPE_FADER:
      port_set_owner_fader (
        ports->l, (Fader *) owner);
      port_set_owner_fader (
        ports->r, (Fader *) owner);
      break;
    case PORT_OWNER_TYPE_PREFADER:
      port_set_owner_prefader (
        ports->l, (PassthroughProcessor *) owner);
      port_set_owner_prefader (
        ports->r, (PassthroughProcessor *) owner);
      break;
    case PORT_OWNER_TYPE_TRACK:
      port_set_owner_track (
        ports->l, (Track *) owner);
      port_set_owner_track (
        ports->r, (Track *) owner);
      break;
    case PORT_OWNER_TYPE_TRACK_PROCESSOR:
      port_set_owner_track_processor (
        ports->l, (Track *) owner);
      port_set_owner_track_processor (
        ports->r, (Track *) owner);
      break;
    case PORT_OWNER_TYPE_SAMPLE_PROCESSOR:
      port_set_owner_sample_processor (
        ports->l, (SampleProcessor *) owner);
      port_set_owner_sample_processor (
        ports->r, (SampleProcessor *) owner);
      break;
    case PORT_OWNER_TYPE_MONITOR_FADER:
      ports->l->identifier.owner_type =
        PORT_OWNER_TYPE_MONITOR_FADER;
      ports->r->identifier.owner_type =
        PORT_OWNER_TYPE_MONITOR_FADER;
    default:
      break;
    }

  g_free (pll);
  g_free (plr);

  return ports;
}

/**
 * First sets port buf to 0, then sums the given
 * port signal from its inputs.
 *
 * @param start_frame The start frame offset from
 *   0 in this cycle.
 * @param nframes The number of frames to process.
 * @param noroll Clear the port buffer in this
 *   range.
 */
void
port_sum_signal_from_inputs (
  Port *    port,
  const nframes_t start_frame,
  const nframes_t nframes,
  const int noroll)
{
  Port * src_port;
  int k;
  unsigned int l;

  g_warn_if_fail (
    start_frame + nframes <=
    AUDIO_ENGINE->nframes);

  switch (port->identifier.type)
    {
    case TYPE_EVENT:
      if (noroll)
        break;

      /* only consider incoming external data if
       * armed for recording (if the port is owner
       * by a track), otherwise always consider
       * incoming external data */
      if (port->identifier.owner_type !=
            PORT_OWNER_TYPE_TRACK_PROCESSOR ||
          (port->identifier.owner_type ==
             PORT_OWNER_TYPE_TRACK_PROCESSOR &&
           port->track->recording))
        {
          port_sum_data_from_jack (
            port, start_frame, nframes);
        }

      for (k = 0; k < port->num_srcs; k++)
        {
          src_port = port->srcs[k];
          g_warn_if_fail (
            src_port->identifier.type ==
              TYPE_EVENT);
          midi_events_append (
            src_port->midi_events,
            port->midi_events, start_frame,
            nframes, 0);
        }

      port_send_data_to_jack (
        port, start_frame, nframes);

      /* send UI notification */
      if (port->midi_events->num_events > 0)
        {
          g_message ("port %s has %d events",
                     port->identifier.label,
                     port->midi_events->num_events);
          /*if (port == AUDIO_ENGINE->midi_in)*/
            /*{*/
              /*AUDIO_ENGINE->trigger_midi_activity = 1;*/
            /*}*/
          if (port->identifier.owner_type ==
                PORT_OWNER_TYPE_TRACK_PROCESSOR)
            {
              port->track->trigger_midi_activity = 1;
            }
        }
      break;
    case TYPE_AUDIO:
      if (noroll)
        {
          memset (
            &port->buf[start_frame], 0,
            nframes * sizeof (float));
          break;
        }

      port_sum_data_from_jack (
        port, start_frame, nframes);

      for (k = 0; k < port->num_srcs; k++)
        {
          src_port = port->srcs[k];

          /* sum the signals */
          for (l = start_frame; l < nframes; l++)
            {
              port->buf[l] += src_port->buf[l];
            }
        }

      port_send_data_to_jack (
        port, start_frame, nframes);
      break;
    case TYPE_CONTROL:
      {
        float maxf, minf, depth_range, val_to_use;
        /* whether this is the first CV processed
         * on this control port */
        int first_cv = 1;
        for (k = 0; k < port->num_srcs; k++)
          {
            src_port = port->srcs[k];

            if (src_port->identifier.type ==
                  TYPE_CV)
              {
                maxf =
                  port->lv2_port->lv2_control->maxf;
                minf =
                  port->lv2_port->lv2_control->minf;
                /*float deff =*/
                  /*port->lv2_port->lv2_control->deff;*/
                /*port->lv2_port->control =*/
                  /*deff + (maxf - deff) **/
                    /*src_port->buf[0];*/
                depth_range =
                  (maxf - minf) / 2.f;

                /* figure out whether to use base
                 * value or the current value */
                if (first_cv)
                  {
                    val_to_use = port->base_value;
                    first_cv = 0;
                  }
                else
                  val_to_use =
                    port->lv2_port->control;

                port->lv2_port->control =
                  CLAMP (
                    val_to_use +
                      depth_range *
                        src_port->buf[0] *
                        src_port->multipliers[
                          port_get_dest_index (
                            src_port, port)],
                    minf, maxf);
              }
          }
      }
      break;
    default:
      break;
    }
}

/**
 * Sets whether to expose the port to the backend
 * and exposes it or removes it.
 *
 * It checks what the backend is using the engine's
 * audio backend or midi backend settings.
 */
void
port_set_expose_to_backend (
  Port * self,
  int    expose)
{
  if (self->identifier.type == TYPE_AUDIO)
    {
      switch (AUDIO_ENGINE->audio_backend)
        {
        case AUDIO_BACKEND_JACK:
#ifdef HAVE_JACK
          port_set_expose_to_jack (self, expose);
#endif
          break;
        default:
          break;
        }
    }
  else if (self->identifier.type == TYPE_EVENT)
    {
      switch (AUDIO_ENGINE->midi_backend)
        {
        case MIDI_BACKEND_JACK:
#ifdef HAVE_JACK
          port_set_expose_to_jack (self, expose);
#endif
          break;
        case MIDI_BACKEND_ALSA:
#ifdef HAVE_ALSA
          port_set_expose_to_alsa (self, expose);
#endif
          break;
        default:
          break;
        }
    }
  else
    g_return_if_reached ();
}

/**
 * Returns if the port is exposed to the backend.
 */
int
port_is_exposed_to_backend (
  const Port * self)
{
  return
    self->internal_type == INTERNAL_JACK_PORT ||
    self->internal_type ==
      INTERNAL_ALSA_SEQ_PORT ||
    self->identifier.owner_type ==
      PORT_OWNER_TYPE_BACKEND ||
    self->exposed_to_backend;
}

/**
 * Renames the port on the backend side.
 */
void
port_rename_backend (
  const Port * self)
{
  if ((!port_is_exposed_to_backend (self)))
    return;

  char * str;
  switch (self->internal_type)
    {
    case INTERNAL_JACK_PORT:
#ifdef HAVE_JACK
      str = port_get_full_designation (self);
      jack_port_rename (
        AUDIO_ENGINE->client,
        (jack_port_t *) self->data,
        str);
      g_free (str);
#endif
      break;
    case INTERNAL_ALSA_SEQ_PORT:
      break;
    default:
      break;
    }
}

#ifdef HAVE_JACK

void
port_receive_midi_events_from_jack (
  Port *      self,
  const nframes_t         start_frame,
  const nframes_t   nframes)
{
  if (self->internal_type !=
        INTERNAL_JACK_PORT ||
      self->identifier.type !=
        TYPE_EVENT)
    return;

  void * port_buf =
    jack_port_get_buffer (
      JACK_PORT_T (self->data), nframes);
  uint32_t num_events =
    jack_midi_get_event_count (port_buf);

  jack_midi_event_t jack_ev;
  for(unsigned i = 0; i < num_events; i++)
    {
      jack_midi_event_get (
        &jack_ev, port_buf, i);

      if (jack_ev.time >= start_frame &&
          jack_ev.time < start_frame + nframes)
        {
          midi_byte_t channel =
            jack_ev.buffer[0] & 0xf;
          if (self->identifier.owner_type ==
                PORT_OWNER_TYPE_TRACK_PROCESSOR &&
              (self->track->type ==
                 TRACK_TYPE_MIDI ||
               self->track->type ==
                 TRACK_TYPE_INSTRUMENT) &&
              !self->track->channel->
                all_midi_channels &&
              !self->track->channel->
                midi_channels[channel])
            {
              /* different channel */
            }
          else
            {
              /*g_message (*/
                /*"JACK MIDI (%s): adding events "*/
                /*"from buffer:\n"*/
                /*"[%u] %hhx %hhx %hhx",*/
                /*port_get_full_designation (self),*/
                /*jack_ev.time,*/
                /*jack_ev.buffer[0],*/
                /*jack_ev.buffer[1],*/
                /*jack_ev.buffer[2]);*/
              midi_events_add_event_from_buf (
                self->midi_events,
                jack_ev.time, jack_ev.buffer,
                (int) jack_ev.size);
            }
        }
    }

  if (self->midi_events->num_events > 0)
    {
      MidiEvent * ev =
        &self->midi_events->events[0];
      g_message (
        "JACK MIDI (%s): have %d events\n"
        "first event is: [%u] %hhx %hhx %hhx",
        port_get_full_designation (self),
        num_events,
        ev->time, ev->raw_buffer[0],
        ev->raw_buffer[1], ev->raw_buffer[2]);
    }
}

void
port_receive_audio_data_from_jack (
  Port *      port,
  const nframes_t         start_frames,
  const nframes_t   nframes)
{
  if (port->internal_type !=
        INTERNAL_JACK_PORT ||
      port->identifier.type !=
        TYPE_AUDIO)
    return;

  float * in;
  in =
    (float *)
    jack_port_get_buffer (
      JACK_PORT_T (port->data),
      AUDIO_ENGINE->nframes);

  for (unsigned int i = start_frames;
       i < start_frames + nframes; i++)
    {
      port->buf[i] += in[i];
    }
}

void
port_send_midi_events_to_jack (
  Port *      port,
  const nframes_t         start_frames,
  const nframes_t   nframes)
{
  if (port->internal_type !=
        INTERNAL_JACK_PORT ||
      port->identifier.type !=
        TYPE_EVENT)
    return;

  midi_events_copy_to_jack (
    port->midi_events,
    jack_port_get_buffer (
      JACK_PORT_T (port->data),
      AUDIO_ENGINE->nframes));
}

void
port_send_audio_data_to_jack (
  Port *      port,
  const nframes_t         start_frames,
  const nframes_t         nframes)
{
  if (port->internal_type !=
        INTERNAL_JACK_PORT ||
      port->identifier.type !=
        TYPE_AUDIO)
    return;

  float * out;
  out =
    (float *)
    jack_port_get_buffer (
      JACK_PORT_T (port->data),
      AUDIO_ENGINE->nframes);

  for (unsigned int i = start_frames;
       i < start_frames + nframes; i++)
    {
      out[i] = port->buf[i];
    }
}

/**
 * Sums the inputs coming in from JACK, before the
 * port is processed.
 */
void
port_sum_data_from_jack (
  Port * self,
  const nframes_t start_frame,
  const nframes_t nframes)
{
  if (self->identifier.owner_type ==
        PORT_OWNER_TYPE_BACKEND ||
      self->internal_type !=
        INTERNAL_JACK_PORT ||
      self->identifier.flow !=
        FLOW_INPUT ||
      AUDIO_ENGINE->audio_backend !=
        AUDIO_BACKEND_JACK ||
      AUDIO_ENGINE->midi_backend !=
        MIDI_BACKEND_JACK)
    return;

  /* append events from JACK if any */
  port_receive_midi_events_from_jack (
    self, start_frame, nframes);

  /* audio */
  port_receive_audio_data_from_jack (
    self, start_frame, nframes);
}

/**
 * Sends the port data to JACK, after the port
 * is processed.
 */
void
port_send_data_to_jack (
  Port * self,
  const nframes_t start_frame,
  const nframes_t nframes)
{
  if (self->internal_type !=
        INTERNAL_JACK_PORT ||
      self->identifier.flow !=
        FLOW_OUTPUT ||
      AUDIO_ENGINE->audio_backend !=
        AUDIO_BACKEND_JACK ||
      AUDIO_ENGINE->midi_backend !=
        MIDI_BACKEND_JACK)
    return;

  /* send midi events */
  port_send_midi_events_to_jack (
    self, start_frame, nframes);

  /* send audio data */
  port_send_audio_data_to_jack (
    self, start_frame, nframes);
}

/**
 * Sets whether to expose the port to JACk and
 * exposes it or removes it from JACK.
 */
void
port_set_expose_to_jack (
  Port * self,
  int    expose)
{
  enum JackPortFlags flags;
  if (self->identifier.flow == FLOW_INPUT)
    flags = JackPortIsInput;
  else if (self->identifier.flow == FLOW_OUTPUT)
    flags = JackPortIsOutput;
  else
    g_return_if_reached ();

  const char * type =
    engine_jack_get_jack_type (
      self->identifier.type);;
  if (!type)
    g_return_if_reached ();

  char * label =
    port_get_full_designation (self);
  if (expose)
    {
      g_message (
        "exposing port %s to JACK",
        label);
      self->data =
        (void *) jack_port_register (
          AUDIO_ENGINE->client,
          label,
          type, flags, 0);
      g_warn_if_fail (self->data);
      g_free (label);
      self->internal_type = INTERNAL_JACK_PORT;
    }
  else
    {
      g_message (
        "unexposing port %s from JACK",
        label);
      int ret =
        jack_port_unregister (
          AUDIO_ENGINE->client,
          JACK_PORT_T (self->data));
      if (ret)
        {
          g_warning (
            "JACK port unregister error: %s",
            engine_jack_get_error_message (
              (jack_status_t) ret));
        }
      self->internal_type = INTERNAL_NONE;
      self->data = NULL;
    }

  self->exposed_to_backend = expose;
}

/**
 * Returns the JACK port attached to this port,
 * if any.
 */
jack_port_t *
port_get_internal_jack_port (
  Port * port)
{
  if (port->internal_type ==
        INTERNAL_JACK_PORT &&
      port->data)
    return (jack_port_t *) port->data;
  else
    return NULL;
}

#endif

#ifdef __linux__

/**
 * Returns the Port matching the given ALSA
 * sequencer port ID.
 */
Port *
port_find_by_alsa_seq_id (
  const int id)
{
  Port * ports[10000];
  int num_ports;
  Port * port;
  port_get_all (ports, &num_ports);
  for (int i = 0; i < num_ports; i++)
    {
      port = ports[i];

      if (port->internal_type ==
            INTERNAL_ALSA_SEQ_PORT &&
          snd_seq_port_info_get_port (
            (snd_seq_port_info_t *) port->data) ==
            id)
        return port;
    }
  g_return_val_if_reached (NULL);
}

void
port_set_expose_to_alsa (
  Port * self,
  int    expose)
{
  if (self->identifier.type == TYPE_EVENT)
    {
      unsigned int flags = 0;
      if (self->identifier.flow == FLOW_INPUT)
        flags =
          SND_SEQ_PORT_CAP_WRITE |
          SND_SEQ_PORT_CAP_SUBS_WRITE;
      else if (self->identifier.flow == FLOW_OUTPUT)
        flags =
          SND_SEQ_PORT_CAP_READ |
          SND_SEQ_PORT_CAP_SUBS_READ;
      else
        g_return_if_reached ();

      if (expose)
        {
          char * lbl =
            port_get_full_designation (self);

          int id =
            snd_seq_create_simple_port (
              AUDIO_ENGINE->seq_handle,
              lbl, flags,
              SND_SEQ_PORT_TYPE_APPLICATION);
          g_return_if_fail (id >= 0);
          snd_seq_port_info_t * info = NULL;
          snd_seq_get_port_info (
            AUDIO_ENGINE->seq_handle,
            id, info);
          self->data =
            (void *) info;
          g_free (lbl);
          self->internal_type =
            INTERNAL_ALSA_SEQ_PORT;
        }
      else
        {
          snd_seq_delete_port (
            AUDIO_ENGINE->seq_handle,
            snd_seq_port_info_get_port (
              (snd_seq_port_info_t *)
                self->data));
          self->internal_type = INTERNAL_NONE;
          self->data = NULL;
        }
    }
  self->exposed_to_backend = expose;
}

#endif

/**
 * Returns a full designation of the port in the
 * format "Track/Port" or "Track/Plugin/Port".
 *
 * Must be free'd.
 */
char *
port_get_full_designation (
  const Port * self)
{
  const PortIdentifier * id = &self->identifier;

  switch (id->owner_type)
    {
    case PORT_OWNER_TYPE_BACKEND:
    case PORT_OWNER_TYPE_SAMPLE_PROCESSOR:
      return g_strdup (id->label);
      break;
    case PORT_OWNER_TYPE_PLUGIN:
      return
        g_strdup_printf (
          "%s/%s/%s",
          self->plugin->track->name,
          self->plugin->descr->name,
          id->label);
      break;
    case PORT_OWNER_TYPE_TRACK:
    case PORT_OWNER_TYPE_TRACK_PROCESSOR:
    case PORT_OWNER_TYPE_PREFADER:
    case PORT_OWNER_TYPE_FADER:
      return
        g_strdup_printf (
          "%s/%s",
          self->track->name,
          id->label);
      break;
    case PORT_OWNER_TYPE_MONITOR_FADER:
      return
        g_strdup_printf (
          "Engine/%s",
          id->label);
    default:
      g_return_val_if_reached (NULL);
    }
}

/**
 * Prints all connections.
 */
void
port_print_connections_all ()
{
  /*Port * src, * dest;*/
  /*int i, j;*/

  /*for (i = 0; i < PROJECT->num_ports; i++)*/
    /*{*/
      /*src = project_get_port (i);*/
      /*if (!src->owner_pl &&*/
          /*!src->owner_tr &&*/
          /*!src->owner_backend)*/
        /*{*/
          /*g_warning ("Port %s has no owner",*/
                     /*src->identifier.label);*/
        /*}*/
      /*for (j = 0; j < src->num_dests; j++)*/
        /*{*/
          /*dest = src->dests[j];*/
          /*g_warn_if_fail (dest);*/
          /*g_message ("%s connected to %s", src->identifier.label, dest->identifier.label);*/
        /*}*/
    /*}*/
}

/**
 * Clears the port buffer.
 */
void
port_clear_buffer (Port * port)
{
  if (port->identifier.type == TYPE_AUDIO &&
        port->buf)
    {
      memset (
        port->buf, 0,
        AUDIO_ENGINE->block_length *
          sizeof (float));
      return;
    }
  if (port->identifier.type == TYPE_EVENT &&
      port->midi_events)
    {
      port->midi_events->num_events = 0;
    }
}

/**
 * Applies the pan to the given L/R ports.
 */
void
port_apply_pan_stereo (
  Port *       l,
  Port *       r,
  float        pan,
  PanLaw       pan_law,
  PanAlgorithm pan_algo)
{
  /* FIXME not used */
  g_warn_if_reached ();
  /*int nframes = AUDIO_ENGINE->block_length;*/
  /*int i;*/
  /*switch (pan_algo)*/
    /*{*/
    /*case PAN_ALGORITHM_SINE_LAW:*/
      /*for (i = 0; i < nframes; i++)*/
        /*{*/
          /*if (r->buf[i] != 0.f)*/
            /*r->buf[i] *= sinf (pan * (M_PIF / 2.f));*/
          /*if (l->buf[i] != 0.f)*/
            /*l->buf[i] *= sinf ((1.f - pan) * (M_PIF / 2.f));*/
        /*}*/
      /*break;*/
    /*case PAN_ALGORITHM_SQUARE_ROOT:*/
      /*break;*/
    /*case PAN_ALGORITHM_LINEAR:*/
      /*for (i = 0; i < nframes; i++)*/
        /*{*/
          /*if (r->buf[i] != 0.f)*/
            /*r->buf[i] *= pan;*/
          /*if (l->buf[i] != 0.f)*/
            /*l->buf[i] *= (1.f - pan);*/
        /*}*/
      /*break;*/
    /*}*/
}

/**
 * Applies the pan to the given port.
 *
 * @param start_frame The start frame offset from
 *   0 in this cycle.
 * @param nframes The number of frames to process.
 */
void
port_apply_pan (
  Port *       port,
  float        pan,
  PanLaw       pan_law,
  PanAlgorithm pan_algo,
  nframes_t    start_frame,
  const nframes_t    nframes)
{
  float calc_r, calc_l;
  pan_get_calc_lr (
    pan_law, pan_algo, pan, &calc_l, &calc_r);

  nframes_t end = start_frame + nframes;
  int is_stereo_r =
    port->identifier.flags & PORT_FLAG_STEREO_R;
  while (start_frame < end)
    {
      if (is_stereo_r)
        {
          port->buf[start_frame++] *= calc_r;
          continue;
        }

      port->buf[start_frame++] *= calc_l;
    }
}

/**
 * Deletes port, doing required cleanup and updating counters.
 */
void
port_free (Port * port)
{
  /* assert no connections */
  g_warn_if_fail (port->num_srcs == 0);
  g_warn_if_fail (port->num_dests == 0);

  if (port->identifier.label)
    g_free (port->identifier.label);
  if (port->buf)
    free (port->buf);

  free (port);
}
