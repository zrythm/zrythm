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
#include "gui/widgets/main_window.h"
#include "gui/widgets/mixer.h"
#include "gui/widgets/track.h"
#include "gui/widgets/tracklist.h"
#include "plugins/plugin_manager.h"
#include "project.h"
#include "utils/audio.h"
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
  int i;

  /* wait for channels to finish processing */
  /*while (loop)*/
    /*{*/
      /*loop = 0;*/
      /*for (int i = 0; i < MIXER->num_channels; i++)*/
        /*{*/
          /*if (!MIXER->channels[i]->processed)*/
            /*{*/
              /*loop = 1;*/
              /*break;*/
            /*}*/
        /*}*/
      /*g_usleep (1000);*/
    /*}*/
  /*for (i = 0; i < MIXER->num_channels; i++)*/
    /*{*/
      /*zix_sem_wait (&MIXER->channel_process_sem);*/
    /*}*/

  /* process master channel */
  /*g_message ("procesing master");*/
  /*g_message ("posting for master");*/
  /*zix_sem_post (&MIXER->master->*/
                  /*start_processing_sem);*/
  /*g_message ("procesing finished");*/
}

/**
 * Recalculates the process acyclic directed graph.
 */
void
mixer_recalculate_graph (
  Mixer * mixer,
  int     force)
{
  if (!g_atomic_int_get (
        &AUDIO_ENGINE->run) &&
      !force)
    return;

  int prev_run =
    g_atomic_int_get (
      &AUDIO_ENGINE->run);
  g_atomic_int_set (
    &AUDIO_ENGINE->run, 0);
  g_usleep (1000);

  Router * router;
  if (router = mixer->graph)
    {
      mixer->graph = NULL;
      router_destroy (router);
    }
  mixer->graph =
    router_new ();

  g_atomic_int_set (
    &AUDIO_ENGINE->run, prev_run);
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
 * Adds channel to mixer.
 *
 * The channel track is created in channel_create but it is
 * setup here.
 */
void
mixer_add_channel (Channel * channel)
{
  g_warn_if_fail (channel);
  g_warn_if_fail (channel->track);

  /* stop engine and give it some time to stop
   * running */
  int prev =
    g_atomic_int_get (&AUDIO_ENGINE->run);
  g_atomic_int_set (&AUDIO_ENGINE->run, 0);
  g_usleep (1000);

  if (channel->type == CT_MASTER)
    {
      MIXER->master = channel;
      MIXER->master_id = channel->id;
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

  mixer_recalculate_graph (MIXER, 0);

  g_atomic_int_set (&AUDIO_ENGINE->run, prev);
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
             channel->track->name);
  channel->enabled = 0;

  g_message ("mixer num channels before %d",
             MIXER->num_channels);
  array_delete (MIXER->channels,
                MIXER->num_channels,
                channel);
  int size = MIXER->num_channels + 1;
  array_delete (MIXER->channel_ids,
                size,
                channel->id);
  project_remove_channel (channel);
  channel_free (channel);
  g_message ("mixer num channels after %d",
             MIXER->num_channels);

  EVENTS_PUSH (ET_CHANNEL_REMOVED,
               NULL);
}

void
mixer_add_channel_from_file_descr (
  FileDescriptor * descr)
{
  /* create a channel/track */
  Channel * chan =
    channel_create (CT_AUDIO, "Audio Track");
  mixer_add_channel (chan);
  tracklist_append_track (chan->track);

  /* create an audio region & add to track */
  Position start_pos;
  position_set_to_pos (&start_pos,
                       &PLAYHEAD);
  AudioRegion * ar =
    audio_region_new (chan->track,
                      descr->absolute_path,
                      &start_pos);
  track_add_region (
    chan->track, ar);

  EVENTS_PUSH (ET_TRACK_ADDED,
               chan->track);
}

void
mixer_add_channel_from_plugin_descr (
  PluginDescriptor * descr)
{
  Plugin * plugin = plugin_create_from_descr (descr);

  /* stop engine */
  int prev_run =
    g_atomic_int_get (&AUDIO_ENGINE->run);
  g_atomic_int_set (&AUDIO_ENGINE->run, 0);
  g_usleep (4000);

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

  g_atomic_int_set (&AUDIO_ENGINE->run, prev_run);

  if (g_settings_get_int (
        S_PREFERENCES,
        "open-plugin-uis-on-instantiate"))
    {
      plugin->visible = 1;
      EVENTS_PUSH (ET_PLUGIN_VISIBILITY_CHANGED,
                   plugin);
    }
  EVENTS_PUSH (ET_TRACK_ADDED,
               new_channel->track);
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
