/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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

/**
 * \file
 *
 * Modulator for each Track/Channel.
 */
#ifndef __AUDIO_MODULATOR_H__
#define __AUDIO_MODULATOR_H__

#include "utils/yaml.h"

typedef struct _ModulatorWidget ModulatorWidget;
typedef struct Track Track;
typedef struct Plugin Plugin;
typedef struct PluginDescriptor PluginDescriptor;

/**
 * A Modulator for modulating/automating parameters
 * of other Plugins.
 *
 * Each Channel/Track contains a list of Modulators
 * that are not in the Channel strip, shown in a
 * separate tab in the bottom panel.
 */
typedef struct Modulator
{
  /**
   * Index in the list of Modulators for the Track.
   */
  int               index;

  /**
   * The plugin that does the processing for this
   * Modulator.
   *
   * This plugin must have at least 1 CV out port that
   * will be shown on the right side of the
   * ModulatorWidget, and any number of Control in
   * ports shown as knobs on the left side of the
   * ModulatorWidget for changing its parameters.
   */
  Plugin *          plugin;

  /** Uniquely identifiable name. */
  char *            name;

  /** Pointer back to the Track. */
  Track *           track;

  /**
   * The widget associated with this Modulator.
   */
  ModulatorWidget * widget;
} Modulator;

static const cyaml_schema_field_t
  modulator_fields_schema[] =
{
	CYAML_FIELD_INT (
    "index", CYAML_FLAG_DEFAULT,
    Modulator, index),

	CYAML_FIELD_END
};

static const cyaml_schema_value_t
modulator_schema = {
  CYAML_VALUE_MAPPING (CYAML_FLAG_POINTER,
    Modulator, modulator_fields_schema),
};

/**
 * Creates a new Modulator.
 *
 * @param descr The descriptor of a plugin to
 *   instantiate.
 * @param track The owner Track.
 */
Modulator *
modulator_new (
  PluginDescriptor * descr,
  Track *       track);

#endif // __AUDIO_MODULATOR_H__
