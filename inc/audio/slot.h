/*
 * audio/slot.h - a slot on a channel, being empty or containing a plugin
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

//#ifndef __AUDIO_SLOT_H__
//#define __AUDIO_SLOT_H__

//#include "audio/plugin.h"

//#include <jack/jack.h>

//typedef struct _Slot Slot;

//typedef jack_default_audio_sample_t   sample_t;
//typedef jack_nframes_t                nframes_t;

/**
 * The process function prototype.
 * Channels must implement this.
 * It is used to perform processing of the audio signal at every cycle.
 *
 * Normally, the channel will call the process func on each of its plugins
 * in order.
 */
//void process_slot (Slot       *slot,        ///< the Slot
                   //nframes_t   samples)     ///< sample count

/**
 * A slot on the channel. They are all instantiated at program start.
 */
//typedef struct _Slot
//{
  //Plugin          * plugin;    ///< the plugin
//} Slot;

/*
 * Initializes a slot with default values
 */
//void
//init_slot (Slot * slot);

//#endif
