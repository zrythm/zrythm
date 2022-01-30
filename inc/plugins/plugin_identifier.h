/*
 * Copyright (C) 2020-2021 Alexandros Theodotou <alex at zrythm dot org>
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
 * Plugin identifier.
 */

#ifndef __PLUGINS_PLUGIN_IDENTIFIER_H__
#define __PLUGINS_PLUGIN_IDENTIFIER_H__

#include "zrythm-config.h"

#include <stdbool.h>

#include "utils/yaml.h"

/**
 * @addtogroup plugins
 *
 * @{
 */

#define PLUGIN_IDENTIFIER_SCHEMA_VERSION 1

typedef enum PluginSlotType
{
  PLUGIN_SLOT_INVALID,
  PLUGIN_SLOT_INSERT,
  PLUGIN_SLOT_MIDI_FX,
  PLUGIN_SLOT_INSTRUMENT,

  /** Plugin is part of a modulator. */
  PLUGIN_SLOT_MODULATOR,
} PluginSlotType;

static const cyaml_strval_t
plugin_slot_type_strings[] =
{
  { "Invalid",    PLUGIN_SLOT_INVALID    },
  { "Insert",     PLUGIN_SLOT_INSERT     },
  { "MIDI FX",    PLUGIN_SLOT_MIDI_FX    },
  { "Instrument", PLUGIN_SLOT_INSTRUMENT },
  { "Modulator",  PLUGIN_SLOT_MODULATOR },
};

static inline const char *
plugin_slot_type_to_string (
  PluginSlotType type)
{
  return plugin_slot_type_strings[type].str;
}

/**
 * Plugin identifier.
 */
typedef struct PluginIdentifier
{
  int              schema_version;

  PluginSlotType   slot_type;

  /** Track name hash. */
  unsigned int     track_name_hash;

  /**
   * The slot this plugin is in the channel, or
   * the index if this is part of a modulator.
   *
   * If PluginIdentifier.slot_type is an instrument,
   * this must be set to -1.
   */
  int              slot;
} PluginIdentifier;

static const cyaml_schema_field_t
plugin_identifier_fields_schema[] =
{
  YAML_FIELD_INT (
    PluginIdentifier, schema_version),
  YAML_FIELD_ENUM (
    PluginIdentifier, slot_type,
    plugin_slot_type_strings),
  YAML_FIELD_UINT (
    PluginIdentifier, track_name_hash),
  YAML_FIELD_INT (
    PluginIdentifier, slot),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t
plugin_identifier_schema = {
  CYAML_VALUE_MAPPING (CYAML_FLAG_POINTER,
    PluginIdentifier,
    plugin_identifier_fields_schema),
};

void
plugin_identifier_init (
  PluginIdentifier * self);

static inline int
plugin_identifier_is_equal (
  const PluginIdentifier * a,
  const PluginIdentifier * b)
{
  return
    a->slot_type == b->slot_type
    && a->track_name_hash == b->track_name_hash
    && a->slot == b->slot;
}

void
plugin_identifier_copy (
  PluginIdentifier * dest,
  const PluginIdentifier * src);

NONNULL
bool
plugin_identifier_validate (
  const PluginIdentifier * self);

/**
 * Verifies that @ref slot_type and @ref slot is
 * a valid combination.
 */
bool
plugin_identifier_validate_slot_type_slot_combo (
  PluginSlotType slot_type,
  int            slot);

static inline void
plugin_identifier_print (
  PluginIdentifier * self,
  char *             str)
{
  sprintf (
    str,
    "slot_type: %d, track_name hash: %u, slot: %d",
    self->slot_type, self->track_name_hash,
    self->slot);
}

/**
 * @}
 */

#endif
