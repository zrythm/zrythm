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
plugin_identifier_init (
  PluginIdentifier * self)
{
  self->schema_version =
    PLUGIN_IDENTIFIER_SCHEMA_VERSION;
  self->slot = -1;
}

bool
plugin_identifier_validate (
  PluginIdentifier * self)
{
  g_return_val_if_fail (
    self->schema_version ==
      PLUGIN_IDENTIFIER_SCHEMA_VERSION,
    false);
  return true;
}
