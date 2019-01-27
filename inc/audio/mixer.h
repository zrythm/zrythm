/*
 * audio/mixer.h - The mixer
 *
 * Copyright (C) 2019 Alexandros Theodotou
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

#ifndef __AUDIO_MIXER_H__
#define __AUDIO_MIXER_H__

#include <jack/jack.h>

#define MIXER (&AUDIO_ENGINE->mixer)
#define STRIP_SIZE 9 /* mixer strip size (number of plugin slots) */
#define FOREACH_CH for (int i = 0; i < MIXER->num_channels; i++)
#define MAX_CHANNELS 300

typedef struct Channel Channel;
typedef struct PluginDescriptor PluginDescriptor;

/**
 * Mixer is a single global struct defined in the Project
 * struct.
 *
 * It can be easily fetched using the MIXER macro.
 */
typedef struct Mixer
{
  /**
   * Array of channels besides master.
   */
  Channel        * channels[MAX_CHANNELS];
  int            channel_ids[MAX_CHANNELS];
  int            num_channels; ///< # of channels

  Channel        * master; ///< master channel
} Mixer;

/**
 * The mixer begins the audio processing process.
 *
 * For each channel, it calls the process function of each plugin in order,
 * and keeps the final signal for each channel.
 *
 * When every channel has finished processing, the signals are summed
 * and sent to Jack.
 */
void
mixer_process ();

/**
 * Loads plugins from state files. Used when loading projects.
 */
void
mixer_load_plugins ();

/**
 * Returns channel at given position (order)
 *
 * Channel order in the mixer is reflected in the track list
 */
Channel *
mixer_get_channel_at_pos (int pos);

/**
 * Adds channel to mixer and initializes track.
 */
void
mixer_add_channel (Channel * channel);

/**
 * Removes the given channel.
 */
void
mixer_remove_channel (Channel * channel);

void
mixer_add_channel_from_plugin_descr (
  PluginDescriptor * descr);

Channel *
mixer_get_channel_by_name (char *  name);

/**
 * Gets next unique channel ID.
 *
 * Gets the max ID of all channels and increments it.
 */
int
mixer_get_next_channel_id ();

#endif
