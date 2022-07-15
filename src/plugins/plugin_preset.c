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

#include "plugins/plugin_preset.h"
#include "utils/objects.h"

void
plugin_preset_identifier_init (PluginPresetIdentifier * id)
{
  id->schema_version = PLUGIN_PRESET_IDENTIFIER_SCHEMA_VERSION;
}

PluginBank *
plugin_bank_new (void)
{
  PluginBank * self = object_new (PluginBank);

  self->schema_version = PLUGIN_BANK_SCHEMA_VERSION;

  plugin_preset_identifier_init (&self->id);

  return self;
}

PluginPreset *
plugin_preset_new (void)
{
  PluginPreset * self = object_new (PluginPreset);

  plugin_preset_identifier_init (&self->id);

  return self;
}
