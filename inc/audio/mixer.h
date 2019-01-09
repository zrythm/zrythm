/*
 * audio/mixer.h - the mixer mixing all the channels and sending the data
 *                 to jack
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

#ifndef __AUDIO_MIXER_H__
#define __AUDIO_MIXER_H__

#include "audio/engine.h"

#include <jack/jack.h>

#define MIXER AUDIO_ENGINE->mixer
#define STRIP_SIZE 9 /* mixer strip size (number of plugin slots) */
#define FOREACH_CH for (int i = 0; i < MIXER->num_channels; i++)

typedef jack_default_audio_sample_t   sample_t;
typedef jack_nframes_t                nframes_t;

typedef struct Channel Channel;
typedef struct Plugin_Descriptor Plugin_Descriptor;

typedef struct Mixer
{
  Channel        * channels[100];      ///< array of channel strips
  int            num_channels;           ///< # of channels EXCLUDING master
  Channel        * master;                  ///< master output
} Mixer;

Mixer *
mixer_new ();

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
mixer_process (Mixer * mixer,
               nframes_t     _nframes);       ///< number of frames to fill in

/**
 * Loads plugins from state files. Used when loading projects.
 */
void
mixer_load_plugins (Mixer * self);

/**
 * Returns channel at given position (order)
 *
 * Channel order in the mixer is reflected in the track list
 */
Channel *
mixer_get_channel_at_pos (Mixer * self,
                          int pos);

/**
 * Adds channel to mixer and initializes track.
 */
void
mixer_add_channel (Mixer *   mixer,
                   Channel * channel);

/**
 * Removes the given channel.
 */
void
mixer_remove_channel (Mixer *   mixer,
                      Channel * channel);

void
mixer_add_channel_from_plugin_descr (
  Mixer * mixer,
  Plugin_Descriptor * descr);

Channel *
mixer_get_channel_by_name (Mixer * self,
                           char *  name);

#endif
