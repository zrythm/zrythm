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

typedef struct ChannelWidget ChannelWidget;
typedef struct Track Track;
typedef struct TrackWidget TrackWidget;

typedef enum ChannelType
{
  CT_AUDIO,
  CT_MIDI,
  CT_MASTER
} ChannelType;

typedef struct Channel
{
  /* note: the first plugin is special, it is the "main" plugin of the channel
   * where processing starts */
  Plugin            * strip[MAX_PLUGINS]; ///< the channel strip
  //int               num_plugins;
  ChannelType       type;             ///< MIDI / Audio / Master
  sample_t          volume;           ///< value of the volume fader
  int               muted;            ///< muted or not
  int               soloed;           ///< soloed or not
  GdkRGBA           color;          ///< see https://ometer.com/gtk-colors.html
  float             phase;        ///< used by the phase knob (0.0-360.0 value)
  char *            name;        ///< channel name

  /* these are for plugins to connect to if they want
   * processing starts at the first plugin with a clean buffer,
   * and if any ports are connected as that plugin's input,
   * their buffers are added to the first plugin
   */
  StereoPorts       * stereo_in;  ///< l & r input ports
  Port              * midi_in;   ///< MIDI in

  /* connecting to this is also optional
   * plugins are processed slot-by-slot, and if nothing is connected here
   * it will simply remain an empty buffer, i.e., channel will produce no sound */
  StereoPorts       * stereo_out;  ///< l & r output ports

  float             l_port_db;   ///< current db after processing l port
  float             r_port_db;   ///< current db after processing r port
  int               processed;   ///< processed in this cycle or not
  int               recording;  ///< recording mode or not
  //pthread_t         thread;     ///< the channel processing thread.
                          ///< each channel does processing on a separate thread
  jack_native_thread_t    thread;
  int               stop_thread;    ///< flag to stop the thread
  struct Channel *         output;     ///< output channel to route signal to
  Track             * track;   ///< the track associated with this channel
  ChannelWidget     * widget; ///< the channel widget
} Channel;

void
channel_set_phase (void * channel, float phase);

float
channel_get_phase (void * channel);

void
channel_set_volume (void * channel, float volume);

float
channel_get_volume (void * channel);

float
channel_get_current_l_db (void * _channel);

float
channel_get_current_r_db (void * _channel);

void
channel_set_current_l_db (Channel * channel, float val);

void
channel_set_current_r_db (Channel * channel, float val);

/**
 * Creates a channel using the given params.
 */
Channel *
channel_create (int     type, char * label);             ///< the channel type (AUDIO/INS)

/**
 * Creates master channel.
 */
Channel *
channel_create_master ();

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
                 nframes_t   nframes);    ///< sample count

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
                    );

/**
 * Returns the index of the last active slot.
 */
int
channel_get_last_active_slot_index (Channel * channel);

/**
 * Returns the index on the mixer.
 */
int
channel_get_index (Channel * channel);

#endif
