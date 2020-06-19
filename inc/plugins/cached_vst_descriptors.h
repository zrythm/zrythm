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

#ifndef __PLUGINS_CACHED_VST_DESCRIPTORS_H__
#define __PLUGINS_CACHED_VST_DESCRIPTORS_H__

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
typedef struct CachedVstDescriptors
{
  /** Version of the file. */
  unsigned int        version;

  PluginDescriptor *  descriptors[10000];
  int                 num_descriptors;

  /** Blacklisted paths and hashes, to skip
   * when scanning */
  PluginDescriptor *  blacklisted[1000];
  int                 num_blacklisted;
} CachedVstDescriptors;

static const cyaml_schema_field_t
cached_vst_descriptors_fields_schema[] =
{
  CYAML_FIELD_UINT (
    "version", CYAML_FLAG_DEFAULT,
    CachedVstDescriptors, version),
  CYAML_FIELD_SEQUENCE_COUNT (
    "descriptors", CYAML_FLAG_DEFAULT,
    CachedVstDescriptors, descriptors,
    num_descriptors,
    &plugin_descriptor_schema, 0, CYAML_UNLIMITED),
  CYAML_FIELD_SEQUENCE_COUNT (
    "blacklisted", CYAML_FLAG_DEFAULT,
    CachedVstDescriptors, blacklisted,
    num_blacklisted,
    &plugin_descriptor_schema, 0, CYAML_UNLIMITED),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t
cached_vst_descriptors_schema =
{
  CYAML_VALUE_MAPPING (
    CYAML_FLAG_POINTER,
    CachedVstDescriptors,
    cached_vst_descriptors_fields_schema),
};

/**
 * Reads the file and fills up the object.
 */
CachedVstDescriptors *
cached_vst_descriptors_new (void);

void
cached_vst_descriptors_serialize_to_file (
  CachedVstDescriptors * self);

/**
 * Returns if the plugin at the given path is
 * blacklisted or not.
 */
int
cached_vst_descriptors_is_blacklisted (
  CachedVstDescriptors * self,
  const char *           abs_path);

/**
 * Returns the PluginDescriptor corresponding to the
 * .so/.dll file at the given path, if it exists and
 * the MD5 hash matches.
 */
PluginDescriptor *
cached_vst_descriptors_get (
  CachedVstDescriptors * self,
  const char *           abs_path);

/**
 * Appends a descriptor to the cache.
 *
 * @param serialize 1 to serialize the updated cache
 *   now.
 */
void
cached_vst_descriptors_blacklist (
  CachedVstDescriptors * self,
  const char *           abs_path,
  int                    _serialize);

/**
 * Appends a descriptor to the cache.
 *
 * @param serialize 1 to serialize the updated cache
 *   now.
 */
void
cached_vst_descriptors_add (
  CachedVstDescriptors * self,
  PluginDescriptor *     descr,
  int                    _serialize);

/**
 * Clears the descriptors and removes the cache file.
 */
void
cached_vst_descriptors_clear (
  CachedVstDescriptors * self);

void
cached_vst_descriptors_free (
  CachedVstDescriptors * self);

/**
 * @}
 */

SERIALIZE_INC (
  CachedVstDescriptors, cached_vst_descriptors);
DESERIALIZE_INC (
  CachedVstDescriptors, cached_vst_descriptors);

#endif
