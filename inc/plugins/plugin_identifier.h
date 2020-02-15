/*
 * Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
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

/**
 * @addtogroup plugins
 *
 * @{
 */

/**
 * Plugin identifier.
 */
typedef struct PluginIdentifier
{
  /** The Channel this plugin belongs to. */
  int                  track_pos;

  /** The slot this plugin is in in the channel. */
  int                  slot;
} PluginIdentifier;

static const cyaml_schema_field_t
plugin_identifier_fields_schema[] =
{
  CYAML_FIELD_INT (
    "track_pos", CYAML_FLAG_DEFAULT,
    PluginIdentifier, track_pos),
  CYAML_FIELD_INT (
    "slot", CYAML_FLAG_DEFAULT,
    PluginIdentifier, slot),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t
plugin_identifier_schema = {
  CYAML_VALUE_MAPPING (CYAML_FLAG_POINTER,
    PluginIdentifier,
    plugin_identifier_fields_schema),
};

static inline int
plugin_identifier_is_equal (
  const PluginIdentifier * a,
  const PluginIdentifier * b)
{
  return
    a->track_pos == b->track_pos &&
    a->slot == b->slot;
}

static inline void
plugin_identifier_copy (
  PluginIdentifier * dest,
  const PluginIdentifier * src)
{
  dest->track_pos = src->track_pos;
  dest->slot = src->slot;
}

/**
 * @}
 */

#endif
