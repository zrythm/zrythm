/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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
 * Mixer implementation. */

#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#include "audio/audio_region.h"
#include "audio/audio_track.h"
#include "audio/channel.h"
#include "audio/engine.h"
#include "audio/instrument_track.h"
#include "audio/mixer.h"
#include "audio/region.h"
#include "audio/routing.h"
#include "audio/track.h"
#include "audio/tracklist.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/channel.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/mixer.h"
#include "gui/widgets/track.h"
#include "gui/widgets/tracklist.h"
#include "plugins/plugin_manager.h"
#include "project.h"
#include "utils/audio.h"
#include "utils/arrays.h"
#include "utils/dialogs.h"
#include "utils/objects.h"
#include "utils/ui.h"

#include <gtk/gtk.h>

#include <glib/gi18n.h>

/**
 * Recalculates the process acyclic directed graph.
 */
void
mixer_recalc_graph (
  Mixer * mixer)
{
  if (mixer->router.graph2)
    free_later (mixer->router.graph2, graph_destroy);
  else
    router_init (&mixer->router);

  /* create the spare graph. this will be copied to
   * graph1 and used in processing */
  mixer->router.graph2 =
    graph_new (&mixer->router);
}

void
mixer_init_loaded ()
{
  MIXER->master =
    project_get_channel (MIXER->master_id);

  for (int i = 0; i < MIXER->num_channels; i++)
    MIXER->channels[i] =
      project_get_channel (
        MIXER->channel_ids[i]);
}

/**
 * Returns if mixer has soloed channels.
 */
int
mixer_has_soloed_channels ()
{
  for (int i = 0; i < MIXER->num_channels; i++)
    {
      if (MIXER->channels[i]->track->solo)
        return 1;
    }
  return 0;
}

/**
 * Loads plugins from state files. Used when loading projects.
 */
void
mixer_load_plugins ()
{
  for (int i = 0; i < MIXER->num_channels; i++)
    {
      Channel * channel = MIXER->channels[i];
      for (int j = 0; j < STRIP_SIZE; j++)
        {
          Plugin * plugin = channel->plugins[j];
          if (plugin)
            {
              plugin_instantiate (plugin);
            }
        }
    }

  /* do master too  */
  for (int j = 0; j < STRIP_SIZE; j++)
    {
      Plugin * plugin = MIXER->master->plugins[j];
      if (plugin)
        {
          plugin_instantiate (plugin);
        }
    }
}

/**
 * Gets next unique channel ID.
 *
 * Gets the max ID of all channels and increments it.
 */
int
mixer_get_next_channel_id ()
{
  int count = 1;
  for (int i = 0; i < MIXER->num_channels; i++)
    {
      Channel * channel = MIXER->channels[i];
      if (channel->id >= count)
        count = channel->id + 1;
    }
  return count;
}

/**
 * Adds channel to the mixer and connects its ports.
 *
 * This acts like the "connection" function of
 * the channel.
 *
 * Note that this should have nothing to do with
 * the track, but only with the mixer and routing
 * in general.
 *
 * @param recalc_graph Recalculate routing graph.
 */
void
mixer_add_channel (
  Mixer *   self,
  Channel * channel,
  int       recalc_graph)
{
  g_warn_if_fail (channel);

  if (channel->type == CT_MASTER)
    {
      self->master = channel;
      self->master_id = channel->id;
    }
  else
    {
      self->channel_ids[self->num_channels] =
        channel->id;
      array_append (self->channels,
                    self->num_channels,
                    channel);
    }

  channel_connect (channel);

  if (recalc_graph)
    mixer_recalc_graph (self);
}

/**
 * Returns channel at given position (order)
 *
 * Channel order in the mixer is reflected in the track list
 */
Channel *
mixer_get_channel_at_pos (int pos)
{
  if (pos < MIXER->num_channels)
    {
      return MIXER->channels[pos];
    }
  g_warning ("No channel found at pos %d", pos);
  return NULL;
}

/**
 * Removes the given channel from the mixer and
 * disconnects it.
 *
 * @param free Also free the channel (later).
 * @param publish_events Publish GUI events.
 */
void
mixer_remove_channel (
  Mixer *   self,
  Channel * ch,
  int       free,
  int       publish_events)
{
  g_message ("removing channel %s",
             ch->track->name);
  ch->enabled = 0;

  array_double_delete (
    self->channels,
    self->channel_ids,
    self->num_channels,
    ch,
    ch->id);

  channel_disconnect (ch);
  if (free)
    free_later (ch, channel_free);

  if (publish_events)
    EVENTS_PUSH (ET_CHANNEL_REMOVED, NULL);
}

/**
 * Moves the given plugin to the given slot in
 * the given channel.
 *
 * If a plugin already exists, it deletes it and
 * replaces it.
 */
void
mixer_move_plugin (
  Mixer *   self,
  Plugin *  pl,
  Channel * ch,
  int       slot)
{
  /* confirm if another plugin exists */
  if (ch->plugins[slot])
    {
      GtkDialog * dialog =
        dialogs_get_overwrite_plugin_dialog (
          GTK_WINDOW (MAIN_WINDOW));
      int result =
        gtk_dialog_run (dialog);
      gtk_widget_destroy (GTK_WIDGET (dialog));

      /* do nothing if not accepted */
      if (result != GTK_RESPONSE_ACCEPT)
        return;
    }

  /* remove plugin from its channel */
  int prev_slot =
    channel_get_plugin_index (pl->channel, pl);
  Channel * prev_ch = pl->channel;
  channel_remove_plugin (
    pl->channel, prev_slot, 0, 0);

  /* move plugin's automation from src to dest */
  plugin_move_automation (
    pl, prev_ch, ch);

  /* add plugin to its new channel */
  channel_add_plugin (
    ch, slot, pl, 0, 0, 1);

  gtk_widget_queue_draw (
    GTK_WIDGET (prev_ch->widget->slots[prev_slot]));
  gtk_widget_queue_draw (
    GTK_WIDGET (ch->widget->slots[slot]));
}

Channel *
mixer_get_channel_by_name (char *  name)
{
  Channel * chan;
  for (int i = 0; i < MIXER->num_channels; i++)
    {
      chan = MIXER->channels[i];
      if (g_strcmp0 (chan->track->name, name) == 0)
        return chan;
    }
  return NULL;
}
