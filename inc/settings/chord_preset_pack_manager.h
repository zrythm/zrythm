// SPDX-FileCopyrightText: © 2022-2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

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
  GPtrArray * pset_packs;
} ChordPresetPackManager;

/**
 * Creates a new chord preset pack manager.
 *
 * @param scan_for_packs Whether to scan for preset
 *   packs.
 */
ChordPresetPackManager *
chord_preset_pack_manager_new (bool scan_for_packs);

int
chord_preset_pack_manager_get_num_packs (
  const ChordPresetPackManager * self);

ChordPresetPack *
chord_preset_pack_manager_get_pack_at (
  const ChordPresetPackManager * self,
  int                            idx);

NONNULL
ChordPresetPack *
chord_preset_pack_manager_get_pack_for_preset (
  ChordPresetPackManager * self,
  const ChordPreset *      pset);

int
chord_preset_pack_manager_get_pack_index (
  ChordPresetPackManager * self,
  ChordPresetPack *        pack);

/**
 * Returns the preset index in its pack.
 */
int
chord_preset_pack_manager_get_pset_index (
  ChordPresetPackManager * self,
  ChordPreset *            pset);

/**
 * Add a copy of the given preset.
 */
void
chord_preset_pack_manager_add_preset (
  ChordPresetPackManager * self,
  ChordPresetPack *        pack,
  const ChordPreset *      pset,
  bool                     serialize);

/**
 * Add a copy of the given pack.
 */
void
chord_preset_pack_manager_add_pack (
  ChordPresetPackManager * self,
  const ChordPresetPack *  pack,
  bool                     serialize);

void
chord_preset_pack_manager_delete_pack (
  ChordPresetPackManager * self,
  ChordPresetPack *        pack,
  bool                     serialize);

void
chord_preset_pack_manager_delete_preset (
  ChordPresetPackManager * self,
  ChordPreset *            pset,
  bool                     serialize);

/**
 * Serializes the chord presets.
 *
 * @return Whether successful.
 */
WARN_UNUSED_RESULT
bool
chord_preset_pack_manager_serialize (
  ChordPresetPackManager * self,
  GError **                error);

void
chord_preset_pack_manager_free (
  const ChordPresetPackManager * self);

/**
 * @}
 */

#endif
