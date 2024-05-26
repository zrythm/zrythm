// SPDX-FileCopyrightText: © 2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * Chord preset.
 */

#ifndef __SETTINGS_CHORD_PRESET_H__
#define __SETTINGS_CHORD_PRESET_H__

#include "zrythm-config.h"

#include "dsp/chord_descriptor.h"
#include "utils/yaml.h"

typedef struct ChordPresetPack ChordPresetPack;

/**
 * @addtogroup settings
 *
 * @{
 */

/**
 * A preset of chord descriptors.
 */
typedef struct ChordPreset
{
  /** Preset name. */
  char * name;

  /** Chord descriptors. */
  ChordDescriptor * descr[12];

  /** Pointer to owner pack. */
  ChordPresetPack * pack;
} ChordPreset;

ChordPreset *
chord_preset_new (void);

ChordPreset *
chord_preset_new_from_name (const char * name);

NONNULL ChordPreset *
chord_preset_clone (const ChordPreset * src);

/**
 * Gets informational text.
 *
 * Must be free'd by caller.
 */
char *
chord_preset_get_info_text (const ChordPreset * self);

const char *
chord_preset_get_name (const ChordPreset * self);

void
chord_preset_set_name (ChordPreset * self, const char * name);

GMenuModel *
chord_preset_generate_context_menu (const ChordPreset * self);

/**
 * Frees the plugin setting.
 */
NONNULL void
chord_preset_free (ChordPreset * self);

/**
 * @}
 */

#endif
