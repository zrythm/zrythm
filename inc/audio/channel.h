/*
 * audio/channel.h - a channel on the mixer
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

#ifndef __AUDIO_CHANNEL_H__
#define __AUDIO_CHANNEL_H__

/**
 * \file
 *
 * API for Channel, representing a channel strip on the mixer.
 *
 * Detailed description.
 */

#include "plugins/plugin.h"

#include <gdk/gdk.h>

#include <jack/jack.h>

#define MAX_PLUGINS 9

typedef jack_default_audio_sample_t   sample_t;
typedef jack_nframes_t                nframes_t;

enum ChannelType
{
  CT_AUDIO,
  CT_MIDI,
  CT_MASTER
};

typedef struct Channel
{
  Plugin    * strip[MAX_PLUGINS]; ///< the channel strip (array of plugins)
  int               type;             ///< MIDI / Audio / Master
  sample_t          volume;           ///< value of the volume fader
  int               muted;            ///< muted or not
  int               soloed;           ///< soloed or not
  GdkRGBA           color;          ///< see https://ometer.com/gtk-colors.html
  float             phase;        ///< used by the phase knob (0.0-360.0 value)
} Channel;

/**
 * Creates a channel using the given params
 */
Channel *
channel_create (int     type);             ///< the channel type (AUDIO/INS)

/**
 * The process function prototype.
 * Channels must implement this.
 * It is used to perform processing of the audio signal at every cycle.
 *
 * Normally, the channel will call the process func on each of its plugins
 * in order.
 */
void
channel_process (Channel * channel, ///< the channel
                 nframes_t   samples,    ///< sample count
                 sample_t *  l_buf,   ///< sample buffer L
                 sample_t *  r_buf);   ///< sample buffer R

/**
 * Adds given plugin to given position in the strip.
 *
 * The plugin must be already initialized (but not instantiated) at this point.
 */
void
channel_add_initialized_plugin (Channel * channel,    ///< the channel
                    int         pos,     ///< the position in the strip
                                        ///< (starting from 0)
                    Plugin      * plugin  ///< the plugin to add
                    );

#endif
