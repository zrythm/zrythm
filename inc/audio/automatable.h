/*
 * audio/automatable.h - An automatable
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

#ifndef __AUDIO_AUTOMATABLE_H__
#define __AUDIO_AUTOMATABLE_H__

#include "plugins/lv2/control.h"

#define IS_AUTOMATABLE_LV2_CONTROL(x) x->type == AUTOMATABLE_TYPE_PLUGIN_CONTROL && \
                                   x->control
#define IS_AUTOMATABLE_CH_FADER(x) x->type == AUTOMATABLE_TYPE_CHANNEL_FADER

typedef enum AutomatableType
{
  AUTOMATABLE_TYPE_PLUGIN_CONTROL,
  AUTOMATABLE_TYPE_PLUGIN_ENABLED,
  AUTOMATABLE_TYPE_CHANNEL_FADER,
  AUTOMATABLE_TYPE_CHANNEL_MUTE,
  AUTOMATABLE_TYPE_CHANNEL_PAN
} AutomatableType;

typedef struct Port Port;
typedef struct Track Track;
typedef struct Channel Channel;
typedef struct AutomatableTrack AutomatableTrack;
typedef struct Plugin Plugin;
typedef struct AutomationTrack AutomationTrack;

typedef struct Automatable
{
  Port *               port; ///< port, if plugin port
  Lv2ControlID *       control; ///< control, if LV2 plugin
  Track *              track; ///< associated track
  //AutomatableTrack *   at; ///< automatable track, if any
  int                  slot_index; ///< slot index in channel, if plugin automation
                                    ///< eg. automating enabled/disabled
  char *               label; ///< human friendly label
  AutomatableType      type; ///< volume/pan/plugin control/etc.
} Automatable;

Automatable *
automatable_create_fader (Channel * channel);

Automatable *
automatable_create_mute (Track * track);

Automatable *
automatable_create_pan (Track * track);

Automatable *
automatable_create_lv2_control (Plugin *       plugin,
                                Lv2ControlID * control);

int
automatable_is_bool (Automatable * automatable);

int
automatable_is_float (Automatable * automatable);

const float
automatable_get_minf (Automatable * automatable);

const float
automatable_get_maxf (Automatable * automatable);

/**
 * Returns max - min for the float automatable
 */
const float
automatable_get_sizef (Automatable * automatable);

/**
 * Gets automation track for given automatable, if any.
 */
AutomationTrack *
automatable_get_automation_track (Automatable * automatable);

void
automatable_free (Automatable * automatable);

#endif /* __AUDIO_AUTOMATABLE_H__ */
