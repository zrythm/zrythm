/*
 * audio/engine.h - Audio engine
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

#ifndef __AUDIO_ENGINE_H__
#define __AUDIO_ENGINE_H__

#define BLOCK_LENGTH 4096 // should be set by backend
#define MIDI_BUF_SIZE 1024 // should be set by backend

#include "zrythm_system.h"

#include <jack/jack.h>
#include <jack/midiport.h>

#define AUDIO_ENGINE zrythm_system->audio_engine

typedef struct Mixer Mixer;

typedef struct MIDI_Controller
{
  jack_midi_event_t    in_event[30];
  int                  num_events;
} MIDI_Controller;

typedef struct Audio_Engine
{
  jack_client_t     * client;     ///< jack client
	uint32_t           block_length;   ///< Audio buffer size (block length)
	size_t             midi_buf_size;  ///< Size of MIDI port buffers
	uint32_t           sample_rate;    ///< Sample rate
	int               buf_size_set;   ///< True iff buffer size callback fired
  Mixer              * mixer;        ///< the mixer
  MIDI_Controller    * midi_controller; ///< the midi input on JACK
  //Port_Manager      * port_manager;  ///< manages all ports created for/by plugins
} Audio_Engine;

void
init_audio_engine();

#endif
