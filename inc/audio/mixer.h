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

/**
 * adds given channel to mixer and updates count
 **/
#define ADD_CHANNEL(channel) MIXER->channels[ \
                                  MIXER->num_channels++] = channel;


typedef jack_default_audio_sample_t   sample_t;
typedef jack_nframes_t                nframes_t;

typedef struct Channel Channel;
typedef struct Port Port;


typedef struct Mixer
{
  Channel        * channels[100];      ///< array of channel strips
  int            num_channels;           ///< # of channels EXCLUDING master
  Channel        * master;                  ///< master output

} Mixer;

/**
 * Initializes the mixer
 */
void
mixer_init ();

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
mixer_process (nframes_t     _nframes);       ///< number of frames to fill in

/**
 * Loads plugins from state files. Used when loading projects.
 */
void
mixer_load_plugins ();

#endif
