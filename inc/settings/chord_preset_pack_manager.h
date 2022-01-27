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

/**
 * @file
 *
 * Chord preset pack manager.
 */

#ifndef __SETTINGS_CHORD_PRESET_PACK_MANAGERH__
#define __SETTINGS_CHORD_PRESET_PACK_MANAGERH__

#include "zrythm-config.h"

#include <stdbool.h>

#include "settings/chord_preset_pack.h"

/**
 * @addtogroup settings
 *
 * @{
 */

#define CHORD_PRESET_PACK_MANAGER \
  (ZRYTHM->chord_preset_pack_manager)

/**
 * Chord preset pack manager.
 */
typedef struct ChordPresetPackManager
{
  /**
   * Scanned preset packs.
   */
  GPtrArray *     pset_packs;
} ChordPresetPackManager;

/**
 * Creates a new chord preset pack manager.
 *
 * @param scan_for_packs Whether to scan for preset
 *   packs.
 */
ChordPresetPackManager *
chord_preset_pack_manager_new (
  bool scan_for_packs);

int
chord_preset_pack_manager_get_num_packs (
  const ChordPresetPackManager * self);

ChordPresetPack *
chord_preset_pack_manager_get_pack_at (
  const ChordPresetPackManager * self,
  int                            idx);

void
chord_preset_pack_manager_free (
  const ChordPresetPackManager * self);

/**
 * @}
 */

#endif
