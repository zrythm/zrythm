/*
 * audio/channel.c - a channel on the mixer
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

#include <math.h>
#include <unistd.h>
#include <stdlib.h>
#if _POSIX_C_SOURCE >= 199309L
#include <time.h>   // for nanosleep
#else
#include <unistd.h> // for usleep
#endif

#include "audio/audio_track.h"
#include "audio/automation_track.h"
#include "audio/automation_tracklist.h"
#include "audio/channel.h"
#include "audio/instrument_track.h"
#include "audio/mixer.h"
#include "audio/pan.h"
#include "audio/track.h"
#include "audio/transport.h"
#include "plugins/lv2_plugin.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/channel.h"
#include "gui/widgets/connections.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/track.h"
#include "gui/widgets/tracklist.h"
#include "project.h"
#include "utils/math.h"

#include <gtk/gtk.h>
#include <jack/thread.h>

/* microseconds */
#define SLEEPTIME_USEC 60

/**
 * Thread work
 * Pass the L/R to each channel strip and let them handle it
 */
static void *
process_channel_work (void * argument)
{
  Channel * channel = (Channel *) argument;

  while (!channel->stop_thread)
    {
      if (!channel->processed)
        {
          if (AUDIO_ENGINE->nframes > 0)
            {
              /*g_message ("calling channel process %s",*/
                         /*channel->name);*/
              channel_process (channel);
              /*g_message ("done calling channel process %s",*/
                         /*channel->name);*/
              channel->processed = 1;
            }
        }

      g_usleep (SLEEPTIME_USEC);
    }

  return 0;
}


/**
 * For each plugin
 *   -> for each input port it owns
 *     -> check if any output ports point to it (including of the previous slot)
 *       -> 1. wait for their owner to finish
 *       -> 2. sum their signals
 *     -> process plugin
 */
void
channel_process (Channel * channel)
{
  /* clear buffers */
  if (channel->type == CT_MASTER ||
      channel->type == CT_AUDIO ||
      channel->type == CT_BUS)
    {
      port_clear_buffer (channel->stereo_in->l);
      port_clear_buffer (channel->stereo_in->r);
    }
  if (channel->type == CT_MIDI)
    {
      port_clear_buffer (channel->midi_in);
      port_clear_buffer (channel->piano_roll);
    }
  port_clear_buffer (channel->stereo_out->l);
  port_clear_buffer (channel->stereo_out->r);
  for (int j = 0; j < STRIP_SIZE; j++)
    {
      Plugin * plugin = channel->plugins[j];
      if (plugin)
        {
          for (int i = 0; i < plugin->num_in_ports; i++)
            {
              port_clear_buffer (plugin->in_ports[i]);
            }
          for (int i = 0; i < plugin->num_out_ports; i++)
            {
              port_clear_buffer (plugin->out_ports[i]);
            }
          for (int i = 0; i < plugin->num_unknown_ports; i++)
            {
              port_clear_buffer (plugin->unknown_ports[i]);
            }
        }
    }
  /*g_message ("buffers cleared %s",*/
             /*channel->name);*/

  /* sum AUDIO IN coming to the channel */
  if (channel->type == CT_MASTER ||
      channel->type == CT_BUS)
    {
      port_sum_signal_from_inputs (
        channel->stereo_in->l);
      port_sum_signal_from_inputs (
        channel->stereo_in->r);
    }
  /*g_message ("summed signal coming in %s",*/
             /*channel->name);*/

  /* panic MIDI if necessary */
  if (AUDIO_ENGINE->panic)
    {
      midi_panic (&channel->piano_roll->midi_events);
    }
  /* get events from track if playing */
  else if (TRANSPORT->play_state == PLAYSTATE_ROLLING)
    {
      /* fill midi events to pass to ins plugin */
      if (channel->track->type == TRACK_TYPE_INSTRUMENT)
        {
          instrument_track_fill_midi_events (
            (InstrumentTrack *)channel->track,
            &PLAYHEAD,
            AUDIO_ENGINE->block_length,
            &channel->piano_roll->midi_events);
        }
      /* fill stereo in buffers with info from the current
       * clip */
      else if (channel->track->type ==
                 TRACK_TYPE_AUDIO)
        {
          audio_track_fill_stereo_in_buffers (
            (AudioTrack *)channel->track,
            channel->stereo_in);
        }
    }
  midi_events_dequeue (&channel->piano_roll->midi_events);

  /* go through each slot (plugin) on the channel strip */
  for (int i = 0; i < STRIP_SIZE; i++)
    {
      Plugin * plugin = channel->plugins[i];
      if (plugin)
        {

          /* sum incoming signals for each input port */
          for (int j = 0; j < plugin->num_in_ports; j++)
            {
              Port * port = plugin->in_ports[j];

              port_sum_signal_from_inputs (port);
            }

          /*g_message ("summed plugin signals coming in %s",*/
                     /*plugin->descr->name);*/

            /* run plugin processing
             * this should put the appropriate result in the plugin's audio out */
            plugin_process (plugin);

          /*g_message ("processed plugin %s",*/
                     /*plugin->descr->name);*/
        }
    }

  /* same for channel ports */
  /*g_message ("summing stereo out ports %s",*/
             /*channel->name);*/
  port_sum_signal_from_inputs (channel->stereo_out->l);
  port_sum_signal_from_inputs (channel->stereo_out->r);
  /*g_message ("summed stereo out ports %s",*/
             /*channel->name);*/

  /* apply pan */
  port_apply_pan_stereo (channel->stereo_out->l,
                         channel->stereo_out->r,
                         channel->pan,
                         PAN_LAW_MINUS_3DB,
                         PAN_ALGORITHM_SINE_LAW);
  /*g_message ("applied pan %s",*/
             /*channel->name);*/

  /* apply faders */
  port_apply_fader (channel->stereo_out->l, channel->fader_amp);
  port_apply_fader (channel->stereo_out->r, channel->fader_amp);
  /*g_message ("applied fader %s",*/
             /*channel->name);*/

  /* mark as processed */
  channel->processed = 1;
  /*g_message ("marked as processed %s",*/
             /*channel->name);*/
}

static void
setup_thread (Channel * channel)
{
  channel->stop_thread = 0;
  channel->processed = 1;
  jack_client_create_thread (
         AUDIO_ENGINE->client,
         &channel->thread,
         jack_client_real_time_priority (AUDIO_ENGINE->client),
         jack_is_realtime (AUDIO_ENGINE->client),
         process_channel_work,
         channel);

  if (channel->thread == -1)
    {
      g_error ("%lu: Failed creating thread for channel %d",
               channel->thread, channel->id);
    }
}

/**
 * Creates, inits, and returns a new channel with given info.
 */
static Channel *
_create_channel (char * name)
{
  Channel * channel = calloc (1, sizeof (Channel));
  channel->name = g_strdup (name);

  /* create ports */
  char * pll =
    g_strdup_printf ("%s stereo in L",
                     channel->name);
  char * plr =
    g_strdup_printf ("%s stereo in R",
                     channel->name);
  channel->stereo_in =
    stereo_ports_new (
      port_new_with_type (TYPE_AUDIO,
                          FLOW_INPUT,
                          pll),
      port_new_with_type (TYPE_AUDIO,
                          FLOW_INPUT,
                          plr));
  g_free (pll);
  g_free (plr);
  pll = g_strdup_printf ("%s MIDI in", channel->name);
  channel->midi_in =
    port_new_with_type (
      TYPE_EVENT,
      FLOW_INPUT,
      pll);
  g_free (pll);
  pll = g_strdup_printf ("%s Stereo out L", channel->name);
  plr = g_strdup_printf ("%s Stereo out R", channel->name);
  channel->stereo_out =
    stereo_ports_new (
     port_new_with_type (TYPE_AUDIO,
                         FLOW_OUTPUT,
                         pll),
     port_new_with_type (TYPE_AUDIO,
                         FLOW_OUTPUT,
                         plr));
  g_message ("Created stereo out ports");
  g_free (pll);
  g_free (plr);
  channel->stereo_in->l->owner_ch = channel;
  channel->stereo_in->r->owner_ch = channel;
  channel->stereo_out->l->owner_ch = channel;
  channel->stereo_out->r->owner_ch = channel;
  channel->midi_in->owner_ch = channel;

  /* init plugins */
  for (int i = 0; i < STRIP_SIZE; i++)
    {
      channel->plugins[i] = NULL;
    }

  /* set volume, phase, pan */
  channel->volume = 0.0f;
  channel->fader_amp = 1.0f;
  channel->phase = 0.0f;
  channel->pan = 0.5f;
  channel->l_port_db = 0.f;
  channel->r_port_db = 0.f;

  /* connect MIDI in port from engine's jack port */
  port_connect (AUDIO_ENGINE->midi_in, channel->midi_in);

  /* thread related */
  setup_thread (channel);

  /* set up piano roll port */
  char * tmp =g_strdup_printf ("%s Piano Roll", channel->name);
  channel->piano_roll =
    port_new_with_type (
      TYPE_EVENT,
      FLOW_INPUT,
      tmp);
  channel->piano_roll->is_piano_roll = 1;
  channel->piano_roll->owner_jack = 0;
  channel->piano_roll->owner_ch = channel;
  channel->piano_roll->midi_events.queue = calloc (1, sizeof (MidiEvents));

  channel->visible = 1;

  return channel;
}

void
channel_toggle_solo (Channel * channel)
{
  channel->solo = !channel->solo;
  if (channel->widget)
    {
      channel_widget_refresh (channel->widget);
    }
  if (channel->track->widget)
    {
      track_widget_refresh (channel->track->widget);
    }
}

void
channel_toggle_mute (Channel * channel)
{
  channel->mute = !channel->mute;
}

/**
 * Generates automatables for the channel.
 *
 * Should be called as soon as it is created
 */
static void
generate_automatables (Channel * channel)
{
  g_message ("Generating automatables for channel %s",
             channel->name);

  /* generate channel automatables if necessary */
  if (!channel_get_fader_automatable (channel))
    {
      channel->automatables[channel->num_automatables++] =
        automatable_create_fader (channel);
    }
}

/**
 * Used when loading projects.
 */
Channel *
channel_get_or_create_blank (int id)
{
  if (MIXER->channels[id])
    {
      return MIXER->channels[id];
    }

  Channel * channel = calloc (1, sizeof (Channel));

  channel->id = id;

  /* thread related */
  setup_thread (channel);

  MIXER->channels[id] = channel;
  MIXER->num_channels++;

  g_message ("[channel_new] Creating blank channel %d", id);

  return channel;
}

/**
 * Creates a channel of the given type with the given label
 */
Channel *
channel_create (ChannelType type,
                char *      label)
{
  g_assert (label);

  int count = 1;
  char * new_label = label;
  while (mixer_get_channel_by_name (new_label))
    {
      if (new_label != label)
        g_free (new_label);
      new_label =
        g_strdup_printf ("%s %d",
                         label,
                         count++);
    }

  Channel * channel = _create_channel (new_label);

  channel->type = type;

  /* set default output */
  if (type == CT_MASTER)
    {
      channel->output = NULL;
      channel->id = 0;
    }
  else
    {
      channel->id = mixer_get_next_channel_id ();
      channel->output = MIXER->master;
    }

  if (type == CT_BUS ||
      type == CT_AUDIO ||
      type == CT_MASTER)
    {
      /* connect stereo in to stereo out */
      port_connect (channel->stereo_in->l,
                    channel->stereo_out->l);
      port_connect (channel->stereo_in->r,
                    channel->stereo_out->r);
    }

  if (type != CT_MASTER)
    {
      /* connect channel out ports to master */
      port_connect (channel->stereo_out->l,
                    MIXER->master->stereo_in->l);
      port_connect (channel->stereo_out->r,
                    MIXER->master->stereo_in->r);
    }

  channel->track = track_new (channel);
  generate_automatables (channel);

  g_message ("Created channel %s of type %i", label, type);

  return channel;
}

void
channel_set_phase (void * _channel, float phase)
{
  Channel * channel = (Channel *) _channel;
  channel->phase = phase;

  if (channel->widget)
    gtk_label_set_text (channel->widget->phase_reading,
                        g_strdup_printf ("%.1f", phase));
}

float
channel_get_phase (void * _channel)
{
  Channel * channel = (Channel *) _channel;
  return channel->phase;
}

/*void*/
/*channel_set_volume (void * _channel, float volume)*/
/*{*/
  /*Channel * channel = (Channel *) _channel;*/
  /*channel->volume = volume;*/
  /*[> TODO update tooltip <]*/
  /*gtk_label_set_text (channel->widget->phase_reading,*/
                      /*g_strdup_printf ("%.1f", volume));*/
  /*g_idle_add ((GSourceFunc) gtk_widget_queue_draw,*/
              /*GTK_WIDGET (channel->widget->fader));*/
/*}*/

/*float*/
/*channel_get_volume (void * _channel)*/
/*{*/
  /*Channel * channel = (Channel *) _channel;*/
  /*return channel->volume;*/
/*}*/

void
channel_set_fader_amp (void * _channel, float amp)
{
  Channel * channel = (Channel *) _channel;
  channel->fader_amp = amp;

  /* calculate volume */
  channel->volume = math_amp_to_dbfs (amp);

  /* TODO update tooltip */
  gtk_label_set_text (channel->widget->phase_reading,
                      g_strdup_printf ("%.1f", channel->volume));
  g_idle_add ((GSourceFunc) gtk_widget_queue_draw,
              GTK_WIDGET (channel->widget->fader));
}

float
channel_get_fader_amp (void * _channel)
{
  Channel * channel = (Channel *) _channel;
  return channel->fader_amp;
}

void
channel_set_pan (void * _channel, float pan)
{
  Channel * channel = (Channel *) _channel;
  channel->pan = pan;
  g_idle_add ((GSourceFunc) gtk_widget_queue_draw,
              GTK_WIDGET (channel->widget->pan));
}

float
channel_get_pan (void * _channel)
{
  Channel * channel = (Channel *) _channel;
  return channel->pan;
}

float
channel_get_current_l_db (void * _channel)
{
  Channel * channel = (Channel *) _channel;
  return channel->l_port_db;
}

float
channel_get_current_r_db (void * _channel)
{
  Channel * channel = (Channel *) _channel;
  return channel->r_port_db;
}

void
channel_set_current_l_db (Channel * channel, float val)
{
  channel->l_port_db = val;
}

void
channel_set_current_r_db (Channel * channel, float val)
{
  channel->r_port_db = val;
}

void
channel_remove_plugin (Channel * channel, int pos)
{
  Plugin * plugin = channel->plugins[pos];
  if (plugin)
    {
      g_message ("Removing %s from %s:%d", plugin->descr->name,
                 channel->name, pos);
      if (plugin->descr->protocol == PROT_LV2)
        {
          lv2_close_ui ((Lv2Plugin *) plugin->original_plugin);

        }
      plugin_free (channel->plugins[pos]);
    }
  AutomationTracklist * automation_tracklist =
    track_get_automation_tracklist (channel->track);
  automation_tracklist_refresh (automation_tracklist);
}

/**
 * Adds given plugin to given position in the strip.
 *
 * The plugin must be already instantiated at this point.
 */
void
channel_add_plugin (Channel * channel,    ///< the channel
                    int         pos,     ///< the position in the strip
                                        ///< (starting from 0)
                    Plugin      * plugin  ///< the plugin to add
                    )
{
  int prev_enabled = channel->enabled;
  channel->enabled = 0;

  /* free current plugin */
  /*Plugin * old = channel->plugins[pos];*/
  channel_remove_plugin (channel, pos);

  g_message ("Inserting %s at %s:%d", plugin->descr->name,
             channel->name, pos);
  channel->plugins[pos] = plugin;
  plugin->channel = channel;

  Plugin * next_plugin = NULL;
  for (int i = pos + 1; i < STRIP_SIZE; i++)
    {
      next_plugin = channel->plugins[i];
      if (next_plugin)
        break;
    }

  Plugin * prev_plugin = NULL;
  for (int i = pos - 1; i >= 0; i--)
    {
      prev_plugin = channel->plugins[i];
      if (prev_plugin)
        break;
    }

  /* ------------------------------------------------------
   * connect input ports
   * ------------------------------------------------------*/
  /* if main plugin or no other plugin before it */
  if (pos == 0 || !prev_plugin)
    {
      switch (channel->type)
        {
        case CT_AUDIO:
          /* TODO connect L and R audio ports for recording */
          break;
        case CT_MIDI:
          /* Connect MIDI port and piano roll to the plugin */
          for (int i = 0; i < plugin->num_in_ports; i++)
            {
              Port * port = plugin->in_ports[i];
              if (port->type == TYPE_EVENT &&
                  port->flow == FLOW_INPUT)
                {
                  /*if (channel->recording)*/
                    port_connect (AUDIO_ENGINE->midi_in, port);
                    port_connect (channel->piano_roll, port);
                }
            }
          break;
        case CT_BUS:
        case CT_MASTER:
          /* disconnect channel stereo in */
          port_disconnect_all (channel->stereo_in->l);
          port_disconnect_all (channel->stereo_in->r);

          /* connect channel stereo in to plugin */
          int last_index = 0;
          for (int j = 0; j < 2; j++)
            {
              for (;
                   last_index < plugin->num_in_ports;
                   last_index++)
                {
                  if (plugin->in_ports[last_index]->type ==
                      TYPE_AUDIO)
                    {
                      if (j == 0)
                        {
                          port_connect (
                            channel->stereo_in->l,
                            plugin->in_ports[last_index]);
                          break;
                        }
                      else
                        {
                          port_connect (
                            channel->stereo_in->r,
                            plugin->in_ports[last_index]);
                          break;
                        }
                    }
                }
            }
          break;
        }
    }
  else if (prev_plugin)
    {
      /* connect each input port from the previous plugin sequentially */

      /* connect output AUDIO ports of prev plugin to AUDIO input ports of
       * plugin */
      int last_index = 0;
      for (int i = 0; i < plugin->num_in_ports; i++)
        {
          if (plugin->in_ports[i]->type == TYPE_AUDIO)
            {
              for (int j = last_index; j < prev_plugin->num_out_ports; j++)
                {
                  if (prev_plugin->out_ports[j]->type == TYPE_AUDIO)
                    {
                      port_connect (prev_plugin->out_ports[j],
                                    plugin->in_ports[i]);
                    }
                  last_index = j;
                  if (last_index == prev_plugin->num_out_ports - 1)
                    break;
                }
            }
        }
    }


  /* ------------------------------------------------------
   * connect output ports
   * ------------------------------------------------------*/


  /* connect output AUDIO ports of plugin to AUDIO input ports of
   * next plugin */
  if (next_plugin)
    {
      int last_index = 0;
      for (int i = 0; i < plugin->num_out_ports; i++)
        {
          if (plugin->out_ports[i]->type == TYPE_AUDIO)
            {
              for (int j = last_index; j < next_plugin->num_in_ports; j++)
                {
                  if (next_plugin->in_ports[j]->type == TYPE_AUDIO)
                    {
                      port_connect (plugin->out_ports[i],
                                    next_plugin->in_ports[j]);
                    }
                  last_index = j;
                  if (last_index == next_plugin->num_in_ports - 1)
                    break;
                }
            }
        }
    }
  /* if last plugin, connect to channel's AUDIO_OUT */
  else
    {
      int count = 0;
      for (int i = 0; i < plugin->num_out_ports; i++)
        {
          /* prepare destinations */
          Port * dests[2];
          dests[0] = channel->stereo_out->l;
          dests[1] = channel->stereo_out->r;

          /* connect to destinations one by one */
          Port * port = plugin->out_ports[i];
          if (port->type == TYPE_AUDIO)
            {
              port_connect (port, dests[count++]);
            }
          if (count == 2)
            break;
        }
    }
  channel->enabled = prev_enabled;

  plugin_generate_automatables (plugin);
  AutomationTracklist * automation_tracklist =
    track_get_automation_tracklist (channel->track);
  automation_tracklist_refresh (automation_tracklist);
  connections_widget_refresh (MW_CONNECTIONS);
}

/**
 * Returns the index of the last active slot.
 */
int
channel_get_last_active_slot_index (Channel * channel)
{
  int index = -1;
  for (int i = 0; i < STRIP_SIZE; i++)
    {
      if (channel->plugins[i])
        index = i;
    }
  return index;
}

/**
 * Returns the index on the mixer.
 */
int
channel_get_index (Channel * channel)
{
  for (int i = 0; i < MIXER->num_channels; i++)
    {
      if (MIXER->channels[i] == channel)
        return i;
    }
  g_error ("Channel index for %s not found", channel->name);
}

/**
 * Convenience method to get the first active plugin in the channel
 */
Plugin *
channel_get_first_plugin (Channel * channel)
{
  /* find first plugin */
  Plugin * plugin = NULL;
  for (int i = 0; i < STRIP_SIZE; i++)
    {
      if (channel->plugins[i])
        {
          plugin = channel->plugins[i];
          break;
        }
    }
  return plugin;
}

/**
 * Toggles the recording state of the channel.
 *
 * To be called when the record buttin is toggled.
 * TODO actually call this.
 */
void
channel_toggle_recording (Channel * channel)
{
  channel->recording = channel->recording == 0 ? 1 : 0;

  /* find first plugin */
  Plugin * plugin = channel_get_first_plugin (channel);

  if (plugin)
    {
      if (channel->type == CT_AUDIO)
        {
          /* TODO connect L and R audio ports for recording */
        }
      else if (channel->type == CT_MIDI)
        {
          /* Connect/Disconnect MIDI port to the plugin */
          for (int i = 0; i < plugin->num_in_ports; i++)
            {
              Port * port = plugin->in_ports[i];
              if (port->type == TYPE_EVENT &&
                  port->flow == FLOW_INPUT)
                {
                  g_message ("%d MIDI In port: %s", i, port->label);
                  if (channel->recording)
                    port_connect (AUDIO_ENGINE->midi_in, port);
                  else
                    port_disconnect (AUDIO_ENGINE->midi_in, port);
                }
            }
        }
    }
}

/**
 * Connects or disconnects the MIDI editor key press port to the channel's
 * first plugin
 */
void
channel_reattach_midi_editor_manual_press_port (Channel * channel,
                                                int     connect)
{
  /* find first plugin */
  Plugin * plugin = channel_get_first_plugin (channel);

  if (plugin)
    {
      if (channel->type == CT_MIDI)
        {
          /* Connect/Disconnect MIDI editor manual press port to the plugin */
          for (int i = 0; i < plugin->num_in_ports; i++)
            {
              Port * port = plugin->in_ports[i];
              if (port->type == TYPE_EVENT &&
                  port->flow == FLOW_INPUT)
                {
                  if (connect)
                    {
                      port_connect (AUDIO_ENGINE->midi_editor_manual_press, port);
                    }
                  else
                    {
                    port_disconnect (AUDIO_ENGINE->midi_editor_manual_press, port);
                    }
                }
            }
        }
    }
}

/**
 * Returns the plugin's strip index on the channel
 */
int
channel_get_plugin_index (Channel * channel,
                          Plugin *  plugin)
{
  for (int i = 0; i < STRIP_SIZE; i++)
    {
      if (channel->plugins[i] == plugin)
        {
          return i;
        }
    }
  g_warning ("channel_get_plugin_index: plugin not found");
  return -1;
}

/**
 * Convenience function to get the fader automatable of the channel.
 */
Automatable *
channel_get_fader_automatable (Channel * channel)
{
  for (int i = 0; i < channel->num_automatables; i++)
    {
      Automatable * automatable = channel->automatables[i];

      if (IS_AUTOMATABLE_CH_FADER (automatable))
        return automatable;
    }
  return NULL;
}

/**
 * Frees the channel.
 */
void
channel_free (Channel * channel)
{
  g_assert (channel);

  g_free (channel->name);
  FOREACH_STRIP
    {
      Plugin * pl = channel->plugins[i];
      if (pl)
        {
          plugin_free (pl);
        }
    }
  port_free (channel->stereo_in->l);
  port_free (channel->stereo_in->r);
  port_free (channel->midi_in);
  port_free (channel->piano_roll);
  port_free (channel->stereo_out->l);
  port_free (channel->stereo_out->r);

  track_free (channel->track);

  FOREACH_AUTOMATABLE (channel)
    {
      Automatable * a = channel->automatables[i];
      automatable_free (a);
    }
}

