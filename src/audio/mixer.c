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

#include "audio/channel.h"
#include "audio/engine.h"
#include "audio/instrument_track.h"
#include "audio/mixer.h"
#include "audio/track.h"
#include "audio/tracklist.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/mixer.h"
#include "gui/widgets/track.h"
#include "gui/widgets/tracklist.h"
#include "plugins/plugin_manager.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/ui.h"

#include <gtk/gtk.h>

/**
 * process callback
 */
void
mixer_process () ///< number of frames to fill in
{
  /*g_message ("procesing mixer");*/
  int loop = 1;

  /* wait for channels to finish processing */
  while (loop)
    {
      loop = 0;
      for (int i = 0; i < MIXER->num_channels; i++)
        {
          if (!MIXER->channels[i]->processed)
            {
              loop = 1;
              break;
            }
        }
      g_usleep (1000);
    }


  /* process master channel */
  /*g_message ("procesing master");*/
  channel_process (MIXER->master);
  /*g_message ("procesing finished");*/
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
 * Adds channel to mixer.
 *
 * The channel track is created in channel_create but it is
 * setup here.
 */
void
mixer_add_channel (Channel * channel)
{
  g_assert (channel);
  g_assert (channel->track);

  if (channel->type == CT_MASTER)
    {
      MIXER->master = channel;
    }
  else
    {
      MIXER->channel_ids[MIXER->num_channels] =
        channel->id;
      array_append (MIXER->channels,
                    MIXER->num_channels,
                    channel);
    }

  track_setup (channel->track);
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
 * Removes the given channel.
 */
void
mixer_remove_channel (Channel * channel)
{
  g_message ("removing channel %s",
             channel->name);
  AUDIO_ENGINE->run = 0;
  channel->enabled = 0;
  channel->stop_thread = 1;
  array_delete (MIXER->channels,
                MIXER->num_channels,
                channel);
  channel_free (channel);
}

void
mixer_add_channel_from_plugin_descr (
  PluginDescriptor * descr)
{
  Plugin * plugin = plugin_create_from_descr (descr);

  if (plugin_instantiate (plugin) < 0)
    {
      char * message =
        g_strdup_printf (
          "Error instantiating plugin “%s”. Please see log for details.",
          plugin->descr->name);

      if (MAIN_WINDOW)
        ui_show_error_message (
          GTK_WINDOW (MAIN_WINDOW),
          message);
      g_free (message);
      plugin_free (plugin);
      return;
    }

  ChannelType ct;
  if (IS_LV2_PLUGIN_CATEGORY (plugin,
                              LV2_INSTRUMENT_PLUGIN))
    ct = CT_MIDI;
  else
    ct = CT_BUS;
  Channel * new_channel =
    channel_create (ct,
                    descr->name);
  mixer_add_channel (new_channel);
  tracklist_append_track (new_channel->track);
  channel_add_plugin (new_channel,
                      0,
                      plugin);

  EVENTS_PUSH (ET_TRACK_ADDED,
               new_channel->track);
}

Channel *
mixer_get_channel_by_name (char *  name)
{
  for (int i = 0; i < MIXER->num_channels; i++)
    {
      Channel * chan = MIXER->channels[i];
      if (g_strcmp0 (chan->name, name) == 0)
        return chan;
    }
  return NULL;
}
