/*
 * audio/mixer.c - The mixer
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
#include "gui/widgets/main_window.h"
#include "gui/widgets/mixer.h"
#include "gui/widgets/track.h"
#include "gui/widgets/tracklist.h"
#include "utils/arrays.h"

#include <gtk/gtk.h>

/**
 * process callback
 */
void
mixer_process (Mixer *   self,
               nframes_t nframes) ///< number of frames to fill in
{
  int loop = 1;

  /* wait for channels to finish processing */
  while (loop)
    {
      loop = 0;
      for (int i = 0; i < self->num_channels; i++)
        {
          if (!self->channels[i]->processed)
            {
              loop = 1;
              break;
            }
        }
    }


  /* process master channel */
  channel_process (self->master,
                   nframes);
}

Mixer *
mixer_new ()
{
  Mixer * self = calloc (1, sizeof (Mixer));

  return self;
}

/**
 * Loads plugins from state files. Used when loading projects.
 */
void
mixer_load_plugins (Mixer * self)
{
  for (int i = 0; i < self->num_channels; i++)
    {
      Channel * channel = self->channels[i];
      for (int j = 0; j < STRIP_SIZE; j++)
        {
          Plugin * plugin = channel->strip[j];
          if (plugin)
            {
              plugin_instantiate (plugin);
            }
        }
    }

  /* do master too  */
  for (int j = 0; j < STRIP_SIZE; j++)
    {
      Plugin * plugin = self->master->strip[j];
      if (plugin)
        {
          plugin_instantiate (plugin);
        }
    }
}

/**
 * Adds channel to mixer.
 *
 * The channel track is created in channel_create but it is
 * setup here.
 */
void
mixer_add_channel (Mixer *   self,
                   Channel * channel)
{
  g_assert (channel);
  g_assert (channel->track);

  if (channel->type == CT_MASTER)
    {
      self->master = channel;
    }
  else
    {
      array_append ((void **) self->channels,
                    &self->num_channels,
                    (void *) channel);
    }

  track_setup (channel->track);
}

/**
 * Returns channel at given position (order)
 *
 * Channel order in the mixer is reflected in the track list
 */
Channel *
mixer_get_channel_at_pos (Mixer * self,
                          int pos)
{
  for (int i = 0; i < self->num_channels; i++)
    {
      Channel * channel = self->channels[i];
      if (channel->id == pos)
        {
          return channel;
        }
    }
  g_warning ("No channel found at pos %d", pos);
  return NULL;
}

/**
 * Removes the given channel.
 */
void
mixer_remove_channel (Mixer * self,
                      Channel * channel)
{
  g_message ("removing channel %s",
             channel->name);
  AUDIO_ENGINE->run = 0;
  channel->enabled = 0;
  channel->stop_thread = 1;
  array_delete ((void **) self->channels,
                &self->num_channels,
                channel);
  channel_free (channel);
}

