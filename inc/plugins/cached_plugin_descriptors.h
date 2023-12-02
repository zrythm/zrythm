// SPDX-FileCopyrightText: © 2020-2021 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * Plugin descriptors.
 */

#ifndef __PLUGINS_CACHED_PLUGIN_DESCRIPTORS_H__
#define __PLUGINS_CACHED_PLUGIN_DESCRIPTORS_H__

#include "plugins/plugin_descriptor.h"
#include "utils/yaml.h"

/**
 * @addtogroup plugins
 *
 * @{
 */

#define CACHED_PLUGIN_DESCRIPTORS_SCHEMA_VERSION 4

/**
 * Descriptors to be cached.
 */
typedef struct CachedPluginDescriptors
{
  /** Version of the file. */
  int schema_version;

  /** Valid descriptors. */
  PluginDescriptor * descriptors[90000];
  int                num_descriptors;

  /** Blacklisted paths and hashes, to skip
   * when scanning */
  PluginDescriptor * blacklisted[90000];
  int                num_blacklisted;
} CachedPluginDescriptors;

static const cyaml_schema_field_t cached_plugin_descriptors_fields_schema[] = {
  YAML_FIELD_INT (CachedPluginDescriptors, schema_version),
  YAML_FIELD_FIXED_SIZE_PTR_ARRAY_VAR_COUNT (
    CachedPluginDescriptors,
    descriptors,
    plugin_descriptor_schema),
  YAML_FIELD_FIXED_SIZE_PTR_ARRAY_VAR_COUNT (
    CachedPluginDescriptors,
    blacklisted,
    plugin_descriptor_schema),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t cached_plugin_descriptors_schema = {
  YAML_VALUE_PTR (
    CachedPluginDescriptors,
    cached_plugin_descriptors_fields_schema),
};

/**
 * Reads the file and fills up the object.
 */
CachedPluginDescriptors *
cached_plugin_descriptors_new (void);

void
cached_plugin_descriptors_serialize_to_file (CachedPluginDescriptors * self);

/**
 * Returns if the plugin at the given path is
 * blacklisted or not.
 */
int
cached_plugin_descriptors_is_blacklisted (
  CachedPluginDescriptors * self,
  const char *              abs_path);

/**
 * Finds a descriptor matching the given one's
 * unique identifiers.
 *
 * @param check_valid Whether to check valid
 *   descriptors.
 * @param check_blacklisted Whether to check
 *   blacklisted descriptors.
 *
 * @return The found descriptor or NULL.
 */
const PluginDescriptor *
cached_plugin_descriptors_find (
  CachedPluginDescriptors * self,
  const PluginDescriptor *  descr,
  bool                      check_valid,
  bool                      check_blacklisted);

/**
 * Returns the PluginDescriptor's corresponding to
 * the .so/.dll file at the given path, if it
 * exists and the MD5 hash matches.
 *
 * @note The returned array must be free'd but not
 *   the descriptors.
 *
 * @return NULL-terminated array.
 */
PluginDescriptor **
cached_plugin_descriptors_get (
  CachedPluginDescriptors * self,
  const char *              abs_path);

/**
 * Appends a descriptor to the cache.
 *
 * @param serialize 1 to serialize the updated cache
 *   now.
 */
void
cached_plugin_descriptors_blacklist (
  CachedPluginDescriptors * self,
  const char *              abs_path,
  int                       _serialize);

/**
 * Replaces a descriptor in the cache.
 *
 * @param serialize Whether to serialize the updated
 *   cache now.
 * @param new_descr A new descriptor to replace
 *   with. Note that this will be cloned, not used
 *   directly.
 */
void
cached_plugin_descriptors_replace (
  CachedPluginDescriptors * self,
  const PluginDescriptor *  _new_descr,
  bool                      _serialize);

/**
 * Appends a descriptor to the cache.
 *
 * @param serialize Whether to serialize the updated
 *   cache now.
 */
void
cached_plugin_descriptors_add (
  CachedPluginDescriptors * self,
  const PluginDescriptor *  descr,
  int                       _serialize);

/**
 * Clears the descriptors and removes the cache file.
 */
void
cached_plugin_descriptors_clear (CachedPluginDescriptors * self);

void
cached_plugin_descriptors_free (CachedPluginDescriptors * self);

/**
 * @}
 */

#endif
