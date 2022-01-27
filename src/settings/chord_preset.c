/*
 * Copyright (C) 2022 Alexandros Theodotou <alex at zrythm dot org>
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

#include "settings/chord_preset.h"
#include "utils/objects.h"

ChordPreset *
chord_preset_new (
  const char * name)
{
  ChordPreset * self = object_new (ChordPreset);

  self->name = g_strdup (name);

  return self;
}

ChordPreset *
chord_preset_clone (
  const ChordPreset * src)
{
  ChordPreset * self = chord_preset_new (src->name);

  for (int i = 0; i < 12; i++)
    {
      self->descr[i] =
        chord_descriptor_clone (src->descr[i]);
    }

  return self;
}

/**
 * Gets informational text.
 *
 * Must be free'd by caller.
 */
char *
chord_preset_get_info_text (
  const ChordPreset * self)
{
  g_return_val_if_reached (NULL);
}

/**
 * Frees the plugin setting.
 */
void
chord_preset_free (
  ChordPreset * self)
{
  object_free_w_func_and_null (
    g_free, self->name);

  for (int i = 0; i < 12; i++)
    {
      object_free_w_func_and_null (
        chord_descriptor_free, self->descr[i]);
    }

  object_zero_and_free (self);
}
