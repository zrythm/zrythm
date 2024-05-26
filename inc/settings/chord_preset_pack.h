// SPDX-FileCopyrightText: © 2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * Chord preset pack.
 */

#ifndef __SETTINGS_CHORD_PRESET_PACK_H__
#define __SETTINGS_CHORD_PRESET_PACK_H__

#include "zrythm-config.h"

#include "settings/chord_preset.h"
#include "utils/yaml.h"

/**
 * @addtogroup settings
 *
 * @{
 */

/**
 * Chord preset pack.
 */
typedef struct ChordPresetPack
{
  /** Pack name. */
  char * name;

  /** Presets. */
  GPtrArray * presets;

  /** Whether this is a standard preset pack (not user-defined). */
  bool is_standard;
} ChordPresetPack;

ChordPresetPack *
chord_preset_pack_new_empty (void);

ChordPresetPack *
chord_preset_pack_new (const char * name, bool is_standard);

bool
chord_preset_pack_contains_name (const ChordPresetPack * self, const char * name);

bool
chord_preset_pack_contains_preset (
  const ChordPresetPack * self,
  const ChordPreset *     pset);

char *
chord_preset_pack_serialize_to_json_str (
  const ChordPresetPack * self,
  GError **               error);

ChordPresetPack *
chord_preset_pack_deserialize_from_json_str (const char * json, GError ** error);

/**
 * @note The given preset is cloned so the caller is
 *   still responsible for @ref pset.
 */
void
chord_preset_pack_add_preset (ChordPresetPack * self, const ChordPreset * pset);

void
chord_preset_pack_delete_preset (ChordPresetPack * self, ChordPreset * pset);

const char *
chord_preset_pack_get_name (const ChordPresetPack * self);

void
chord_preset_pack_set_name (ChordPresetPack * self, const char * name);

ChordPresetPack *
chord_preset_pack_clone (const ChordPresetPack * src);

GMenuModel *
chord_preset_pack_generate_context_menu (const ChordPresetPack * self);

void
chord_preset_pack_free (ChordPresetPack * self);

void
chord_preset_pack_destroy_cb (void * self);

/**
 * @}
 */

#endif
