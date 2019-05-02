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

#ifndef __AUDIO_MIXER_H__
#define __AUDIO_MIXER_H__

#include "audio/channel.h"
#include "audio/routing.h"
#include "utils/audio.h"

#define MIXER (&AUDIO_ENGINE->mixer)
#define FOREACH_CH for (int i = 0; i < MIXER->num_channels; i++)
#define MAX_CHANNELS 3000

typedef struct Channel Channel;
typedef struct PluginDescriptor PluginDescriptor;
typedef struct FileDescriptor FileDescriptor;

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

  /** Router. */
  Router         router;

  int            master_id;
} Mixer;

static const cyaml_schema_field_t
mixer_fields_schema[] =
{
  CYAML_FIELD_SEQUENCE_COUNT (
    "channel_ids", CYAML_FLAG_DEFAULT,
    Mixer, channel_ids, num_channels,
    &int_schema, 0, CYAML_UNLIMITED),
	CYAML_FIELD_INT (
    "master_id", CYAML_FLAG_DEFAULT,
    Mixer, master_id),

	CYAML_FIELD_END
};

static const cyaml_schema_value_t
mixer_schema =
{
	CYAML_VALUE_MAPPING (
    CYAML_FLAG_POINTER,
	  Mixer, mixer_fields_schema),
};

void
mixer_init_loaded ();

/**
 * Recalculates the process acyclic directed graph.
 */
void
mixer_recalc_graph (
  Mixer * mixer);

/**
 * Returns if mixer has soloed channels.
 */
int
mixer_has_soloed_channels ();

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
  int       recalc_graph);

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
  Channel * channel,
  int       free,
  int       publish_events);

Channel *
mixer_get_channel_by_name (char *  name);

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
  int       slot);

/**
 * Gets next unique channel ID.
 *
 * Gets the max ID of all channels and increments it.
 */
int
mixer_get_next_channel_id ();

#endif
