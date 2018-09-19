/*
 * plugins/plugin.c - a base plugin for plugins (lv2/vst/ladspa/etc.)
 *                         to inherit from
 *
 * Copyright (C) 2018 Alexandros Theodotou
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

/** \file
 * Implementation of Plugin. */

#include <stdlib.h>
#include <string.h>

#include "audio/engine.h"
#include "audio/transport.h"
#include "plugins/plugin.h"
#include "plugins/lv2_plugin.h"
#include "plugins/lv2/control.h"

#include <gtk/gtk.h>

/**
 * Creates an empty plugin.
 *
 * To be filled in by the caller.
 * FIXME should be static?
 */
static Plugin *
_plugin_new ()
{
  Plugin * plugin = calloc (1, sizeof (Plugin));

  plugin->processed = 1;

  return plugin;
}

/**
 * Creates/initializes a plugin and its internal plugin (LV2, etc.)
 * using the given descriptor.
 */
Plugin *
plugin_create_from_descr (Plugin_Descriptor * descr)
{
  Plugin * plugin = _plugin_new ();
  plugin->descr = descr;
  switch (plugin->descr->protocol)
    {
    case PROT_LV2:
      lv2_create_from_uri (plugin, descr->uri);
      break;
    }
  return plugin;
}

/**
 * Instantiates the plugin (e.g. when adding to a channel).
 */
int
plugin_instantiate (Plugin * plugin ///< the plugin
                   )
{
  g_message ("Instantiating %s...", plugin->descr->name);
  /* TODO */
  if (plugin->descr->protocol == PROT_LV2)
    {
      LV2_Plugin *lv2 = (LV2_Plugin *) plugin->original_plugin;
      lv2_instantiate (lv2, NULL);
    }
  plugin->enabled = 1;
}


/**
 * Process plugin
 */
void
plugin_process (Plugin * plugin, nframes_t nframes)
{
  jack_client_t * client = AUDIO_ENGINE->client;
  switch (plugin->descr->protocol)
    {
    case PROT_LV2:
        {
      LV2_Plugin * lv2_plugin = (LV2_Plugin *) plugin->original_plugin;

        /* If transport state is not as expected, then something has changed */
        const bool xport_changed = (
          lv2_plugin->rolling != (TRANSPORT->play_state == PLAYSTATE_ROLLING) ||
          lv2_plugin->pos.frames != PLAYHEAD.frames ||
          lv2_plugin->bpm != TRANSPORT->bpm);

	uint8_t   pos_buf[256];
	LV2_Atom* lv2_pos = (LV2_Atom*)pos_buf;
        if (xport_changed)
          {
            /* Build an LV2 position object to report change to plugin */
            lv2_atom_forge_set_buffer(&lv2_plugin->forge, pos_buf, sizeof(pos_buf));
            LV2_Atom_Forge*      forge = &lv2_plugin->forge;
            LV2_Atom_Forge_Frame frame;
            lv2_atom_forge_object(forge, &frame, 0, lv2_plugin->urids.time_Position);
            lv2_atom_forge_key(forge, lv2_plugin->urids.time_frame);
            lv2_atom_forge_long(forge, PLAYHEAD.frames);
            lv2_atom_forge_key(forge, lv2_plugin->urids.time_speed);
            lv2_atom_forge_float(forge, TRANSPORT->play_state == PLAYSTATE_ROLLING ? 1.0 : 0.0);
            lv2_atom_forge_key(forge, lv2_plugin->urids.time_barBeat);
            lv2_atom_forge_float(
                    forge, (float) PLAYHEAD.beats - 1 + ((float) PLAYHEAD.ticks / (float) TICKS_PER_BEAT));
            lv2_atom_forge_key(forge, lv2_plugin->urids.time_bar);
            lv2_atom_forge_long(forge, PLAYHEAD.bars - 1);
            lv2_atom_forge_key(forge, lv2_plugin->urids.time_beatUnit);
            lv2_atom_forge_int(forge, TRANSPORT->beat_unit);
            lv2_atom_forge_key(forge, lv2_plugin->urids.time_beatsPerBar);
            lv2_atom_forge_float(forge, TRANSPORT->beats_per_bar);
            lv2_atom_forge_key(forge, lv2_plugin->urids.time_beatsPerMinute);
            lv2_atom_forge_float(forge, TRANSPORT->bpm);

            if (1)
              {
                /*char* str = sratom_to_turtle(*/
                        /*lv2_plugin->sratom, &lv2_plugin->unmap, "time:", NULL, NULL,*/
                        /*lv2_pos->type, lv2_pos->size,*/
                        /*LV2_ATOM_BODY(lv2_pos));*/
                /*lv2_ansi_start(stdout, 36);*/
                /*printf("\n## Position ##\n%s\n", str);*/
                /*lv2_ansi_reset(stdout);*/
                /*free(str);*/
              }
          }

        /* Update transport state to expected values for next cycle */
        lv2_plugin->pos.frames =
          TRANSPORT->play_state == PLAYSTATE_ROLLING ?
          PLAYHEAD.frames + nframes :
          lv2_plugin->pos.frames;
        lv2_plugin->bpm      = TRANSPORT->bpm;
        lv2_plugin->rolling  = TRANSPORT->play_state == PLAYSTATE_ROLLING;

        /*switch (TRANSPORT->play_state) {*/
        /*case PLAYSTATE_PAUSE_REQUESTED:*/
                /*[>jalv->play_state = JALV_PAUSED;<]*/
                /*[>zix_sem_post(&jalv->paused);<]*/
                /*break;*/
        /*case JALV_PAUSED:*/
          /*for (uint32_t p = 0; p < lv2_plugin->num_ports; ++p)*/
            /*{*/
              /*jack_port_t* jport = jalv->ports[p].sys_port;*/
              /*if (jport && jalv->ports[p].flow == FLOW_OUTPUT)*/
                /*{*/
                  /*void* buf = jack_port_get_buffer(jport, nframes);*/
                  /*if (jalv->ports[p].type == TYPE_EVENT) {*/
                      /*jack_midi_clear_buffer(buf);*/
                  /*} else {*/
                      /*memset(buf, '\0', nframes * sizeof(float));*/
                  /*}*/
                /*}*/
            /*}*/
          /*return 0;*/
        /*default:*/
                /*break;*/
        /*}*/
      /* Prepare port buffers */
      for (uint32_t p = 0; p < lv2_plugin->num_ports; ++p)
        {
          LV2_Port * lv2_port = &lv2_plugin->ports[p];
          Port * port = lv2_port->port;
          if (port->type == TYPE_AUDIO)
            {
              /* Connect lv2 ports  to plugin port buffers */
              port->nframes = nframes;
              lilv_instance_connect_port(
                      lv2_plugin->instance, p,
                      port->buf);
#ifdef HAVE_JACK_METADATA
          } else if (port->type == TYPE_CV && lv2_port->sys_port) {
                  /* Connect plugin port directly to Jack port buffer */
                  lilv_instance_connect_port(
                          lv2_plugin->instance, p,
                          jack_port_get_buffer(lv2_port->sys_port, nframes));
#endif
          } else if (port->type == TYPE_EVENT && port->flow == FLOW_INPUT) {
                  lv2_evbuf_reset(lv2_port->evbuf, true);

                  /* Write transport change event if applicable */
                  LV2_Evbuf_Iterator iter = lv2_evbuf_begin(lv2_port->evbuf);
                  if (xport_changed)
                    {
                      lv2_evbuf_write(&iter, 0, 0,
                                      lv2_pos->type, lv2_pos->size,
                                      (const uint8_t*)LV2_ATOM_BODY(lv2_pos));
                    }

                  if (lv2_plugin->request_update)
                    {
                      /* Plugin state has changed, request an update */
                      const LV2_Atom_Object get = {
                              { sizeof(LV2_Atom_Object_Body),
                                lv2_plugin->urids.atom_Object },
                              { 0, lv2_plugin->urids.patch_Get } };
                      lv2_evbuf_write(&iter, 0, 0,
                                      get.atom.type, get.atom.size,
                                      (const uint8_t*)LV2_ATOM_BODY(&get));
                    }

                  if (port->midi_events.num_events > 0)
                    {
                      g_message ("Lv2 plugin port midi events %d",
                                 port->midi_events.num_events);
                      /* Write Jack MIDI input */
                      for (uint32_t i = 0; i < port->midi_events.num_events; ++i)
                        {
                          jack_midi_event_t * ev = &port->midi_events.jack_midi_events[i];
                          lv2_evbuf_write(&iter,
                                          ev->time, 0,
                                          lv2_plugin->midi_event_id,
                                          ev->size, ev->buffer);
                        }
                    }
                  midi_events_clear (&port->midi_events);
          } else if (port->type == TYPE_EVENT) {
                  /* Clear event output for plugin to write to */
                  lv2_evbuf_reset(lv2_port->evbuf, false);
          }
        }
      lv2_plugin->request_update = false;

      /* Run plugin for this cycle */
      const bool send_ui_updates = lv2_run (lv2_plugin, nframes);

      /* Deliver MIDI output and UI events */
      for (uint32_t p = 0; p < lv2_plugin->num_ports; ++p)
        {
          LV2_Port* const lv2_port = &lv2_plugin->ports[p];
          Port * port = lv2_port->port;
          if (port->flow == FLOW_OUTPUT && port->type == TYPE_CONTROL &&
                lilv_port_has_property(lv2_plugin->lilv_plugin,
                                       lv2_port->lilv_port,
                                       lv2_plugin->nodes.lv2_reportsLatency))
            {
              if (lv2_plugin->plugin_latency != lv2_port->control)
                {
                  lv2_plugin->plugin_latency = lv2_port->control;
                  jack_recompute_total_latencies(client);
                }
            }
          else if (port->flow == FLOW_OUTPUT && port->type == TYPE_EVENT)
              {
                void* buf = NULL;

                for (LV2_Evbuf_Iterator i = lv2_evbuf_begin(lv2_port->evbuf);
                     lv2_evbuf_is_valid(i);
                     i = lv2_evbuf_next(i)) {
                        // Get event from LV2 buffer
                        uint32_t frames, subframes, type, size;
                        uint8_t* body;
                        lv2_evbuf_get(i, &frames, &subframes, &type, &size, &body);

                        if (buf && type == lv2_plugin->midi_event_id) {
                            /* Write MIDI event to port TODO */
                            /*jack_midi_event_write(buf, frames, body, size);*/
                        }

                        if (lv2_plugin->has_ui && !lv2_port->old_api) {
                          // Forward event to UI
                          lv2_send_to_ui(lv2_plugin, p, type, size, body);
                        }
                }
            } else if (send_ui_updates &&
                       port->flow == FLOW_OUTPUT && port->type == TYPE_CONTROL) {
                    char buf[sizeof(Lv2ControlChange) + sizeof(float)];
                    Lv2ControlChange* ev = (Lv2ControlChange*)buf;
                    ev->index    = p;
                    ev->protocol = 0;
                    ev->size     = sizeof(float);
                    *(float*)ev->body = lv2_port->control;
                    if (zix_ring_write(lv2_plugin->plugin_events, buf, sizeof(buf))
                        < sizeof(buf)) {
                            fprintf(stderr, "Plugin => UI buffer overflow!\n");
                    }
              }
      }


      break;
        }
    }

  plugin->processed = 1;
}

/**
 * Disconnects all connected ports from each port in the given array and
 * frees them.
 */
static void
clean_ports (Port ** array, int * size)
{
  /* go through each port */
  for (int i = 0; i < (* size); i++)
    {
      Port * port = array[i];

      if (port->flow == FLOW_INPUT) /* disconnect incoming ports */
        {
          for (int j = 0; j < port->num_srcs; j++)
            {
              port_disconnect (port->srcs[j], port);
            }
        }
        else if (port->flow == FLOW_OUTPUT) /* disconnect outgoing ports */
          {
            for (int j = 0; j < port->num_dests; j++)
              {
                /* disconnect outgoing ports */
                port_disconnect (port, port->dests[j]);
              }
          }
      port_delete (port);
    }
  (* size) = 0;
}


/**
 * Frees given plugin, breaks all its port connections, and frees its ports
 * and other internal pointers
 */
void
plugin_free (Plugin *plugin)
{
  zix_sem_wait (&AUDIO_ENGINE->port_operation_lock);

  /* disconnect all ports and free them */
  clean_ports (plugin->in_ports, &plugin->num_in_ports);
  clean_ports (plugin->out_ports, &plugin->num_out_ports);

  /* TODO other cleanup work */

  free (plugin);
  zix_sem_post (&AUDIO_ENGINE->port_operation_lock);
}
