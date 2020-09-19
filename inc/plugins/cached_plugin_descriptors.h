/*
 * Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
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

#ifndef __PLUGINS_CACHED_PLUGIN_DESCRIPTORS_H__
#define __PLUGINS_CACHED_PLUGIN_DESCRIPTORS_H__

#include "plugins/plugin_descriptor.h"
#include "utils/yaml.h"

/**
 * @addtogroup plugins
 *
 * @{
 */

/**
 * Descriptors to be cached.
 */
typedef struct CachedPluginDescriptors
{
  /** Version of the file. */
  unsigned int        version;

  PluginDescriptor *  descriptors[10000];
  int                 num_descriptors;

  /** Blacklisted paths and hashes, to skip
   * when scanning */
  PluginDescriptor *  blacklisted[1000];
  int                 num_blacklisted;
} CachedPluginDescriptors;

static const cyaml_schema_field_t
cached_plugin_descriptors_fields_schema[] =
{
  CYAML_FIELD_UINT (
    "version", CYAML_FLAG_DEFAULT,
    CachedPluginDescriptors, version),
  CYAML_FIELD_SEQUENCE_COUNT (
    "descriptors", CYAML_FLAG_DEFAULT,
    CachedPluginDescriptors, descriptors,
    num_descriptors,
    &plugin_descriptor_schema, 0, CYAML_UNLIMITED),
  CYAML_FIELD_SEQUENCE_COUNT (
    "blacklisted", CYAML_FLAG_DEFAULT,
    CachedPluginDescriptors, blacklisted,
    num_blacklisted,
    &plugin_descriptor_schema, 0, CYAML_UNLIMITED),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t
cached_plugin_descriptors_schema =
{
  CYAML_VALUE_MAPPING (
    CYAML_FLAG_POINTER,
    CachedPluginDescriptors,
    cached_plugin_descriptors_fields_schema),
};

/**
 * Reads the file and fills up the object.
 */
CachedPluginDescriptors *
cached_plugin_descriptors_new (void);

void
cached_plugin_descriptors_serialize_to_file (
  CachedPluginDescriptors * self);

/**
 * Returns if the plugin at the given path is
 * blacklisted or not.
 */
int
cached_plugin_descriptors_is_blacklisted (
  CachedPluginDescriptors * self,
  const char *           abs_path);

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
  const char *           abs_path,
  int                    _serialize);

/**
 * Appends a descriptor to the cache.
 *
 * @param serialize 1 to serialize the updated cache
 *   now.
 */
void
cached_plugin_descriptors_add (
  CachedPluginDescriptors * self,
  PluginDescriptor *     descr,
  int                    _serialize);

/**
 * Clears the descriptors and removes the cache file.
 */
void
cached_plugin_descriptors_clear (
  CachedPluginDescriptors * self);

void
cached_plugin_descriptors_free (
  CachedPluginDescriptors * self);

/**
 * @}
 */

SERIALIZE_INC (
  CachedPluginDescriptors, cached_plugin_descriptors);
DESERIALIZE_INC (
  CachedPluginDescriptors, cached_plugin_descriptors);

#endif
