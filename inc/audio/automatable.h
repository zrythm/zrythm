/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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

#ifndef __AUDIO_AUTOMATABLE_H__
#define __AUDIO_AUTOMATABLE_H__

#include "config.h"

#include "audio/port.h"
#include "plugins/lv2/control.h"
#include "utils/yaml.h"

#ifdef HAVE_CARLA
#include "CarlaNativePlugin.h"
#endif

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

typedef struct Track Track;
typedef struct Channel Channel;
typedef struct AutomatableTrack AutomatableTrack;
typedef struct Plugin Plugin;
typedef struct AutomationTrack AutomationTrack;

/**
 * @addtogroup audio
 *
 * @{
 */

/**
 * An automatable control.
 *
 * These are not meant to be serialized and are generated
 * at run time.
 */
typedef struct Automatable
{
  /** Index in its parent. */
  int              index;

  /**
   * Pointer to the Port, if plugin port.
   */
  Port *           port;

  /**
   * Port identifier, used when saving/loading so
   * we can fetch the port.
   *
   * It is a pointer so it can be NULL.
   */
  PortIdentifier * port_id;

  /**
   * Pointer to the control, if LV2 plugin.
   *
   * When loading, this can be fetched using the
   * port.
   *
   * FIXME convert to getter function, having
   * this everywhere is confusing.
   */
  Lv2Control *     control;

  /** Associated track. */
  Track *          track;

  /** Used when saving/loading projects. */
  int              track_id;

  /** Slot, if plugin automation. */
  int              slot;
  /** Plugin, for convenience, if plugin
   * automation. */
  Plugin *         plugin;

  /** Human friendly label. */
  char *           label;

  /** Volume/pan/plugin control/etc. */
  AutomatableType  type;

  float            minf;
  float            maxf;
  float            sizef;
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
    "index", CYAML_FLAG_DEFAULT,
    Automatable, index),
  CYAML_FIELD_MAPPING_PTR (
    "identifier",
    CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
    Automatable, port_id,
    port_identifier_fields_schema),
  CYAML_FIELD_INT (
    "slot", CYAML_FLAG_DEFAULT,
    Automatable, slot),
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

/**
 * Inits a loaded automatable.
 */
void
automatable_init_loaded (Automatable * self);

/**
 * Finds the Automatable in the project from the
 * given clone.
 */
Automatable *
automatable_find (
  Automatable * clone);

Automatable *
automatable_create_fader (Channel * channel);

Automatable *
automatable_create_mute (Channel * channel);

Automatable *
automatable_create_pan (Channel * channel);

/**
 * Creates an automatable for an LV2 control.
 */
Automatable *
automatable_create_lv2_control (
  Plugin *       plugin,
  Lv2Control * control);

#ifdef HAVE_CARLA
/**
 * Creates an automatable for a Carla native
 * plugin control.
 */
Automatable *
automatable_create_carla_control (
  Plugin *          plugin,
  const NativeParameter * param);
#endif

Automatable *
automatable_create_plugin_enabled (Plugin * plugin);

int
automatable_is_bool (Automatable * automatable);

int
automatable_is_float (Automatable * automatable);

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
 * Updates the actual value.
 *
 * The given value is always a normalized 0.0-1.0
 * value and must be translated to the actual value
 * before setting it.
 *
 * @param automating 1 if this is from an automation
 *   event. This will set Lv2Port's automating field
 *   to 1 which will cause the plugin to receive
 *   a UI event for this change.
 */
void
automatable_set_val_from_normalized (
  Automatable * self,
  float         val,
  int           automating);

/**
 * Gets automation track for given automatable, if
 * any.
 */
AutomationTrack *
automatable_get_automation_track (
  Automatable * automatable);

/**
 * Frees the automatable.
 */
void
automatable_free (
  Automatable * automatable);

/**
 * @}
 */

#endif /* __AUDIO_AUTOMATABLE_H__ */
