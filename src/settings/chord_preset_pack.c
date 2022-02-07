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

#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "project.h"
#include "settings/chord_preset_pack.h"
#include "utils/arrays.h"
#include "utils/mem.h"
#include "utils/objects.h"
#include "utils/string.h"
#include "zrythm_app.h"

ChordPresetPack *
chord_preset_pack_new (
  const char * name,
  bool         is_standard)
{
  ChordPresetPack * self =
    object_new (ChordPresetPack);

  self->schema_version =
    CHORD_PRESET_PACK_SCHEMA_VERSION;
  self->name = g_strdup (name);
  self->presets_size = 12;
  self->presets =
    object_new_n (self->presets_size, ChordPreset *);
  self->is_standard = is_standard;

  return self;
}

/**
 * @note The given preset is cloned so the caller is
 *   still responsible for @ref pset.
 */
void
chord_preset_pack_add_preset (
  ChordPresetPack *   self,
  const ChordPreset * pset)
{
  array_double_size_if_full (
    self->presets, self->num_presets,
    self->presets_size, ChordPreset *);

  ChordPreset * clone = chord_preset_clone (pset);
  clone->pack = self;

  array_append (
    self->presets, self->num_presets, clone);
}

bool
chord_preset_pack_contains_name (
  const ChordPresetPack * self,
  const char *            name)
{
  for (int i = 0; i < self->num_presets; i++)
    {
      ChordPreset * pset = self->presets[i];
      if (string_is_equal_ignore_case (
            pset->name, name))
        return true;
    }

  return false;
}

bool
chord_preset_pack_contains_preset (
  const ChordPresetPack * self,
  const ChordPreset *     pset)
{
  for (int i = 0; i < self->num_presets; i++)
    {
      ChordPreset * cur_pset = self->presets[i];
      if (pset == cur_pset)
        return true;
    }

  return false;
}

void
chord_preset_pack_delete_preset (
  ChordPresetPack * self,
  ChordPreset *     pset)
{
  array_delete (
    self->presets, self->num_presets, pset);

  chord_preset_free (pset);

  EVENTS_PUSH (ET_CHORD_PRESET_REMOVED, NULL);
}

const char *
chord_preset_pack_get_name (
  const ChordPresetPack * self)
{
  return self->name;
}

void
chord_preset_pack_set_name (
  ChordPresetPack * self,
  const char *      name)
{
  object_free_w_func_and_null (g_free, self->name);

  self->name = g_strdup (name);

  EVENTS_PUSH (ET_CHORD_PRESET_PACK_EDITED, NULL);
}

ChordPresetPack *
chord_preset_pack_clone (
  const ChordPresetPack * src)
{
  ChordPresetPack * self =
    chord_preset_pack_new (
      src->name, src->is_standard);

  self->presets =
    object_realloc_n (
      self->presets, self->presets_size,
      MAX (1, (size_t) src->num_presets),
      ChordPreset *);
  for (int i = 0; i < src->num_presets; i++)
    {
      self->presets[i] =
        chord_preset_clone (src->presets[i]);
      self->presets[i]->pack = self;
    }

  return self;
}

void
chord_preset_pack_free (
  ChordPresetPack * self)
{
  object_free_w_func_and_null (g_free, self->name);

  for (int i = 0; i < self->num_presets; i++)
    {
      object_free_w_func_and_null (
        chord_preset_free, self->presets[i]);
    }
  free (self->presets);

  object_zero_and_free (self);
}

void
chord_preset_pack_destroy_cb (
  void * self)
{
  chord_preset_pack_free (
    (ChordPresetPack *) self);
}
