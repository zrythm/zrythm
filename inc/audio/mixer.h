/*
 * Copyright (C) 2018-2020 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __AUDIO_MIXER_H__
#define __AUDIO_MIXER_H__

#include "audio/channel.h"
#include "audio/routing.h"
#include "utils/audio.h"

#define MIXER (&AUDIO_ENGINE->mixer)

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
  /** Router. */
  Router         router;

  int            master_id;
} Mixer;

static const cyaml_schema_field_t
mixer_fields_schema[] =
{
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
mixer_init_loaded (void);

/**
 * Recalculates the process acyclic directed graph.
 */
void
mixer_recalc_graph (
  Mixer * mixer);

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
mixer_process (
  Mixer * mixer);

/**
 * Moves the given plugin to the given slot in
 * the given channel.
 *
 * If a plugin already exists, it deletes it and
 * replaces it.
 */
void
mixer_move_plugin (
  Mixer *        self,
  Plugin *       pl,
  Channel *      ch,
  PluginSlotType slot_type,
  int            slot);

#endif
