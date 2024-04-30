// SPDX-FileCopyrightText: © 2020-2021 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Plugin preset.
 */

#ifndef __PLUGINS_PLUGIN_PRESET_H__
#define __PLUGINS_PLUGIN_PRESET_H__

#include "plugins/plugin_identifier.h"
#include "utils/yaml.h"

/**
 * @addtogroup plugins
 *
 * @{
 */

#define PLUGIN_BANK_SCHEMA_VERSION 1
#define PLUGIN_PRESET_IDENTIFIER_SCHEMA_VERSION 1
#define PLUGIN_PRESET_SCHEMA_VERSION 1

/**
 * Preset identifier.
 */
typedef struct PluginPresetIdentifier
{
  /** Index in bank, or -1 if this is used for
   * a bank. */
  int idx;

  /** Bank index in plugin. */
  int bank_idx;

  /** Plugin identifier. */
  PluginIdentifier plugin_id;
} PluginPresetIdentifier;

/**
 * Plugin preset.
 */
typedef struct PluginPreset
{
  /** Human readable name. */
  char * name;

  /** URI if LV2. */
  char * uri;

  /** Carla program index. */
  int carla_program;

  PluginPresetIdentifier id;
} PluginPreset;

/**
 * A plugin bank containing presets.
 *
 * If the plugin has no banks, there must be a
 * default bank that will contain all the presets.
 */
typedef struct PluginBank
{
  /** Presets in this bank. */
  PluginPreset ** presets;
  int             num_presets;
  size_t          presets_size;

  /** URI if LV2. */
  char * uri;

  /** Human readable name. */
  char * name;

  PluginPresetIdentifier id;
} PluginBank;

PluginBank *
plugin_bank_new (void);

PluginPreset *
plugin_preset_new (void);

NONNULL void
plugin_preset_identifier_init (PluginPresetIdentifier * id);

/**
 * @}
 */

#endif
