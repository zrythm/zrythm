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

#ifndef __AUDIO_AUTOMATABLE_H__
#define __AUDIO_AUTOMATABLE_H__

#include "plugins/lv2/control.h"
#include "utils/yaml.h"

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

/**
 * An automatable control.
 *
 * These are not meant to be serialized and are generated
 * at run time.
 */
typedef struct Automatable
{
  int                  id;

  /**
   * Port, if plugin port.
   */
  Port *               port; ///< cache

  /**
   * This should be serialized instead of port.
   */
  int                  port_id;

  /**
   * Control, if LV2 plugin.
   *
   * When loading, this can be fetched using the
   * port.
   */
  Lv2ControlID *       control; ///< cache

  /** Associated track. */
  int                  track_id;
  Track *              track; ///< cache

  int                  slot_index; ///< slot index in channel, if plugin automation
                                    ///< eg. automating enabled/disabled
  char *               label; ///< human friendly label
  AutomatableType      type; ///< volume/pan/plugin control/etc.
} Automatable;

static const cyaml_strval_t
automatable_type_strings[] =
{
	{ "Plugin Control",
    AUTOMATABLE_TYPE_PLUGIN_CONTROL },
	{ "Plugin Enabled",
    AUTOMATABLE_TYPE_PLUGIN_ENABLED },
	{ "Channel Fader",
    AUTOMATABLE_TYPE_CHANNEL_FADER },
	{ "Channel Mute",
    AUTOMATABLE_TYPE_CHANNEL_MUTE },
	{ "Channel Pan",
    AUTOMATABLE_TYPE_CHANNEL_PAN },
};

static const cyaml_schema_field_t
automatable_fields_schema[] =
{
  CYAML_FIELD_INT (
    "id", CYAML_FLAG_DEFAULT,
    Automatable, id),
  CYAML_FIELD_INT (
    "port_id", CYAML_FLAG_DEFAULT,
    Automatable, port_id),
  CYAML_FIELD_INT (
    "track_id", CYAML_FLAG_DEFAULT,
    Automatable, track_id),
  CYAML_FIELD_INT (
    "slot_index", CYAML_FLAG_DEFAULT,
    Automatable, slot_index),
  CYAML_FIELD_STRING_PTR (
    "label", CYAML_FLAG_POINTER,
    Automatable, label,
   	0, CYAML_UNLIMITED),
  CYAML_FIELD_ENUM (
    "type", CYAML_FLAG_DEFAULT,
    Automatable, type, automatable_type_strings,
    CYAML_ARRAY_LEN (automatable_type_strings)),

	CYAML_FIELD_END
};

static const cyaml_schema_value_t
automatable_schema = {
	CYAML_VALUE_MAPPING (
    CYAML_FLAG_POINTER,
		Automatable, automatable_fields_schema),
};

void
automatable_init_loaded (Automatable * self);

Automatable *
automatable_create_fader (Channel * channel);

Automatable *
automatable_create_mute (Channel * channel);

Automatable *
automatable_create_pan (Channel * channel);

Automatable *
automatable_create_lv2_control (Plugin *       plugin,
                                Lv2ControlID * control);

Automatable *
automatable_create_plugin_enabled (Plugin * plugin);

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
 * Returns the type of its value (float, bool, etc.)
 * as a string.
 *
 * Must be free'd.
 */
char *
automatable_stringize_value_type (Automatable * a);

/**
 * Converts real value (eg. -10.0 to 100.0) to
 * normalized value (0.0 to 1.0).
 */
float
automatable_real_val_to_normalized (
  Automatable * self,
  float         real_val);

/**
 * Converts normalized value (0.0 to 1.0) to
 * real value (eg. -10.0 to 100.0).
 */
float
automatable_normalized_val_to_real (
  Automatable * self,
  float         normalized_val);

/**
 * Gets the current value of the parameter the
 * automatable is for.
 *
 * This does not consider the automation track, it
 * only looks in the actual parameter for its
 * current value.
 */
float
automatable_get_val (Automatable * self);

/**
 * Updates the value.
 */
void
automatable_set_val_from_normalized (
  Automatable * self,
  float         val);

/**
 * Gets automation track for given automatable, if any.
 */
AutomationTrack *
automatable_get_automation_track (Automatable * automatable);

void
automatable_free (Automatable * automatable);

#endif /* __AUDIO_AUTOMATABLE_H__ */
