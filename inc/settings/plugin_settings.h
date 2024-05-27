// SPDX-FileCopyrightText: © 2021-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Plugin settings.
 */

#ifndef __SETTINGS_PLUGIN_SETTINGS_H__
#define __SETTINGS_PLUGIN_SETTINGS_H__

#include "zrythm-config.h"

#include "plugins/plugin_descriptor.h"

/**
 * @addtogroup settings
 *
 * @{
 */

/**
 * A setting for a specific plugin descriptor.
 */
typedef struct PluginSetting
{
  /** The descriptor of the plugin this setting is for. */
  PluginDescriptor * descr;

  /** Whether to instantiate this plugin with carla. */
  bool open_with_carla;

  /** Whether to force a generic UI. */
  bool force_generic_ui;

  /** Requested carla bridge mode. */
  CarlaBridgeMode bridge_mode;

  /** Last datetime instantiated (number of microseconds since
   * January 1, 1970 UTC). */
  gint64 last_instantiated_time;

  /** Number of times this plugin has been instantiated. */
  int num_instantiations;
} PluginSetting;

typedef struct PluginSettings
{
  /** Settings. */
  GPtrArray * settings;
} PluginSettings;

/**
 * Creates a plugin setting with the recommended
 * settings for the given plugin descriptor based
 * on the current setup.
 */
PluginSetting * NONNULL
plugin_setting_new_default (const PluginDescriptor * descr);

PluginSetting * NONNULL
plugin_setting_clone (const PluginSetting * src, bool validate);

bool
plugin_setting_is_equal (const PluginSetting * a, const PluginSetting * b);

/**
 * Makes sure the setting is valid in the current
 * run and changes any fields to make it conform.
 *
 * For example, if the setting is set to not open
 * with carla but the descriptor is for a VST plugin,
 * this will set \ref PluginSetting.open_with_carla
 * to true.
 */
NONNULL void
plugin_setting_validate (PluginSetting * self, bool print_result);

NONNULL void
plugin_setting_print (const PluginSetting * self);

/**
 * Creates necessary tracks at the end of the tracklist.
 *
 * This may happen asynchronously so the caller should not
 * expect the setting to be activated on return.
 */
NONNULL void
plugin_setting_activate (const PluginSetting * self);

/**
 * Increments the number of times this plugin has been
 * instantiated.
 *
 * @note This also serializes all plugin settings.
 */
NONNULL void
plugin_setting_increment_num_instantiations (PluginSetting * self);

/**
 * Frees the plugin setting.
 */
NONNULL void
plugin_setting_free (PluginSetting * self);

void
plugin_setting_free_closure (void * self, GClosure * closure);

/**
 * Reads the file and fills up the object.
 */
PluginSettings *
plugin_settings_read_or_new (void);

/**
 * Serializes the current settings.
 */
NONNULL void
plugin_settings_serialize_to_file (PluginSettings * self);

/**
 * Finds a setting for the given plugin descriptor.
 *
 * @return The found setting or NULL.
 */
NONNULL PluginSetting *
plugin_settings_find (PluginSettings * self, const PluginDescriptor * descr);

/**
 * Replaces a setting or appends a setting to the
 * cache.
 *
 * This clones the setting before adding it.
 *
 * @param serialize Whether to serialize the updated
 *   cache now.
 */
NONNULL void
plugin_settings_set (
  PluginSettings * self,
  PluginSetting *  setting,
  bool             _serialize);

NONNULL void
plugin_settings_free (PluginSettings * self);

/**
 * @}
 */

#endif
