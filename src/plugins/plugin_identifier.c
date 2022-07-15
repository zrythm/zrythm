/*
 * Copyright (C) 2021 Alexandros Theodotou <alex at zrythm dot org>
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

#include "plugins/plugin_identifier.h"

void
plugin_identifier_init (PluginIdentifier * self)
{
  memset (self, 0, sizeof (PluginIdentifier));
  self->schema_version = PLUGIN_IDENTIFIER_SCHEMA_VERSION;
  self->slot = -1;
}

bool
plugin_identifier_validate (const PluginIdentifier * self)
{
  g_return_val_if_fail (
    self->schema_version == PLUGIN_IDENTIFIER_SCHEMA_VERSION,
    false);
  g_return_val_if_fail (
    plugin_identifier_validate_slot_type_slot_combo (
      self->slot_type, self->slot),
    false);
  return true;
}

/**
 * Verifies that @ref slot_type and @ref slot is
 * a valid combination.
 */
bool
plugin_identifier_validate_slot_type_slot_combo (
  PluginSlotType slot_type,
  int            slot)
{
  return (slot_type == PLUGIN_SLOT_INSTRUMENT && slot == -1)
         || (slot_type == PLUGIN_SLOT_INVALID && slot == -1)
         || (slot_type != PLUGIN_SLOT_INSTRUMENT && slot >= 0);
}

void
plugin_identifier_copy (
  PluginIdentifier *       dest,
  const PluginIdentifier * src)
{
  g_return_if_fail (plugin_identifier_validate (src));

  dest->schema_version = src->schema_version;
  dest->slot_type = src->slot_type;
  dest->track_name_hash = src->track_name_hash;
  dest->slot = src->slot;
}
