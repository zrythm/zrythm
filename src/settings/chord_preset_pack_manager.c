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
#include "settings/chord_preset_pack_manager.h"
#include "utils/objects.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

/**
 * Creates a new chord preset pack manager.
 *
 * @param scan_for_packs Whether to scan for preset
 *   packs.
 */
ChordPresetPackManager *
chord_preset_pack_manager_new (
  bool scan_for_packs)
{
  ChordPresetPackManager * self =
    object_new (ChordPresetPackManager);

  self->pset_packs =
    g_ptr_array_new_with_free_func (
      chord_preset_pack_destroy_cb);

#define ADD_SIMPLE_CHORD(i,root,chord_type) \
  pset->descr[i] = \
    chord_descriptor_new ( \
      root, false, root, chord_type, \
      CHORD_ACC_NONE, 0);

  /* add standard preset packs */
  ChordPresetPack * pack =
    chord_preset_pack_new (_("Standard"), true);
  ChordPreset * pset = chord_preset_new (_("Pop"));
  ADD_SIMPLE_CHORD (0, NOTE_C, CHORD_TYPE_MAJ);
  ADD_SIMPLE_CHORD (1, NOTE_C, CHORD_TYPE_MAJ);
  ADD_SIMPLE_CHORD (2, NOTE_C, CHORD_TYPE_MAJ);
  ADD_SIMPLE_CHORD (3, NOTE_C, CHORD_TYPE_MAJ);
  ADD_SIMPLE_CHORD (4, NOTE_C, CHORD_TYPE_MAJ);
  ADD_SIMPLE_CHORD (5, NOTE_C, CHORD_TYPE_MAJ);
  ADD_SIMPLE_CHORD (6, NOTE_C, CHORD_TYPE_MAJ);
  ADD_SIMPLE_CHORD (7, NOTE_C, CHORD_TYPE_MAJ);
  ADD_SIMPLE_CHORD (8, NOTE_C, CHORD_TYPE_MAJ);
  ADD_SIMPLE_CHORD (9, NOTE_C, CHORD_TYPE_MAJ);
  ADD_SIMPLE_CHORD (10, NOTE_C, CHORD_TYPE_MAJ);
  ADD_SIMPLE_CHORD (11, NOTE_C, CHORD_TYPE_MAJ);
  chord_preset_pack_add_preset (pack, pset);
  chord_preset_free (pset);
  g_ptr_array_add (self->pset_packs, pack);

  return self;
}

int
chord_preset_pack_manager_get_num_packs (
  const ChordPresetPackManager * self)
{
  return (int) self->pset_packs->len;
}

ChordPresetPack *
chord_preset_pack_manager_get_pack_at (
  const ChordPresetPackManager * self,
  int                            idx)
{
  return
    (ChordPresetPack *)
    g_ptr_array_index (self->pset_packs, idx);
}

/**
 * Add a copy of the given pack.
 */
void
chord_preset_pack_manager_add_pack (
  ChordPresetPackManager * self,
  const ChordPresetPack *  pack)
{
  ChordPresetPack * new_pack =
    chord_preset_pack_clone (pack);
  g_ptr_array_add (self->pset_packs, new_pack);

  EVENTS_PUSH (ET_CHORD_PRESET_PACK_ADDED, NULL);
}

void
chord_preset_pack_manager_delete_pack (
  ChordPresetPackManager * self,
  ChordPresetPack *        pack)
{
  g_ptr_array_remove (self->pset_packs, pack);

  EVENTS_PUSH (ET_CHORD_PRESET_PACK_REMOVED, NULL);
}

ChordPresetPack *
chord_preset_pack_manager_get_pack_for_preset (
  ChordPresetPackManager * self,
  ChordPreset *            pset)
{
  for (size_t i = 0; i < self->pset_packs->len; i++)
    {
      ChordPresetPack * pack =
        (ChordPresetPack *)
        g_ptr_array_index (self->pset_packs, i);

      if (chord_preset_pack_contains_preset (
            pack, pset))
        {
          return pack;
        }
    }

  g_return_val_if_reached (NULL);
}

void
chord_preset_pack_manager_delete_preset (
  ChordPresetPackManager * self,
  ChordPreset *            pset)
{
  ChordPresetPack * pack =
    chord_preset_pack_manager_get_pack_for_preset (
      self, pset);
  if (!pack)
    return;

  chord_preset_pack_delete_preset (
    pack, pset);
}

void
chord_preset_pack_manager_free (
  const ChordPresetPackManager * self)
{
  g_ptr_array_unref (self->pset_packs);

  object_zero_and_free (self);
}
