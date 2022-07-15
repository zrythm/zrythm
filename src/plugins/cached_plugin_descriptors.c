// SPDX-FileCopyrightText: © 2020-2021 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "plugins/cached_plugin_descriptors.h"
#include "utils/file.h"
#include "utils/objects.h"
#include "utils/string.h"
#include "zrythm.h"

static char *
get_cached_plugin_descriptors_file_path (void)
{
  char * zrythm_dir = zrythm_get_dir (ZRYTHM_DIR_USER_TOP);
  g_return_val_if_fail (zrythm_dir, NULL);

  return g_build_filename (
    zrythm_dir, "cached_plugin_descriptors.yaml", NULL);
}

void
cached_plugin_descriptors_serialize_to_file (
  CachedPluginDescriptors * self)
{
  g_message ("Serializing cached plugin descriptors...");
  char * yaml =
    yaml_serialize (self, &cached_plugin_descriptors_schema);
  g_return_if_fail (yaml);
  GError * err = NULL;
  char *   path = get_cached_plugin_descriptors_file_path ();
  g_return_if_fail (path && strlen (path) > 2);
  g_message (
    "Writing cached plugin descriptors to %s...", path);
  g_file_set_contents (path, yaml, -1, &err);
  if (err != NULL)
    {
      g_warning (
        "Unable to write cached plugin descriptors "
        "file: %s",
        err->message);
      g_error_free (err);
      g_free (path);
      g_free (yaml);
      g_return_if_reached ();
    }
  g_free (path);
  g_free (yaml);
}

static bool
is_yaml_our_version (const char * yaml)
{
  bool same_version = false;
  char version_str[120];
  sprintf (
    version_str, "schema_version: %d\n",
    CACHED_PLUGIN_DESCRIPTORS_SCHEMA_VERSION);
  same_version = g_str_has_prefix (yaml, version_str);
  if (!same_version)
    {
      sprintf (
        version_str, "---\nschema_version: %d\n",
        CACHED_PLUGIN_DESCRIPTORS_SCHEMA_VERSION);
      same_version = g_str_has_prefix (yaml, version_str);
    }

  return same_version;
}

/**
 * Reads the file and fills up the object.
 */
CachedPluginDescriptors *
cached_plugin_descriptors_new (void)
{
  GError * err = NULL;
  char *   path = get_cached_plugin_descriptors_file_path ();
  if (!file_exists (path))
    {
      g_message (
        "Cached plugin descriptors file at %s does "
        "not exist",
        path);
return_new_instance:
      g_free (path);
      CachedPluginDescriptors * self =
        object_new (CachedPluginDescriptors);
      self->schema_version =
        CACHED_PLUGIN_DESCRIPTORS_SCHEMA_VERSION;
      return self;
    }
  char * yaml = NULL;
  g_file_get_contents (path, &yaml, NULL, &err);
  if (err != NULL)
    {
      g_critical (
        "Failed to create CachedPluginDescriptors "
        "from %s",
        path);
      g_free (err);
      g_free (yaml);
      g_free (path);
      return NULL;
    }

  /* if not same version, purge file and return
   * a new instance */
  if (!is_yaml_our_version (yaml))
    {
      g_message (
        "Found old plugin descriptor file version. "
        "Purging file and creating a new one.");
      GFile * file = g_file_new_for_path (path);
      g_file_delete (file, NULL, NULL);
      g_object_unref (file);
      g_free (yaml);
      goto return_new_instance;
    }

  CachedPluginDescriptors * self = (CachedPluginDescriptors *)
    yaml_deserialize (yaml, &cached_plugin_descriptors_schema);
  if (!self)
    {
      g_critical (
        "Failed to deserialize "
        "CachedPluginDescriptors from %s",
        path);
      g_free (yaml);
      g_free (path);
      return NULL;
    }
  g_free (yaml);
  g_free (path);

  for (int i = 0; i < self->num_descriptors; i++)
    {
      self->descriptors[i]
        ->category = plugin_descriptor_string_to_category (
        self->descriptors[i]->category_str);
    }

  return self;
}

static void
delete_file (void)
{
  char *  path = get_cached_plugin_descriptors_file_path ();
  GFile * file = g_file_new_for_path (path);
  if (!g_file_delete (file, NULL, NULL))
    {
      g_warning (
        "Failed to delete cached plugin descriptors "
        "file");
    }
  g_free (path);
  g_object_unref (file);
}

/**
 * Returns if the plugin at the given path is
 * blacklisted or not.
 */
int
cached_plugin_descriptors_is_blacklisted (
  CachedPluginDescriptors * self,
  const char *              abs_path)
{
  for (int i = 0; i < self->num_blacklisted; i++)
    {
      PluginDescriptor * descr = self->blacklisted[i];
      GFile * file = g_file_new_for_path (descr->path);
      if (
        string_is_equal (descr->path, abs_path)
        && descr->ghash == g_file_hash (file))
        {
          g_object_unref (file);
          return 1;
        }
      g_object_unref (file);
    }
  return 0;
}

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
  bool                      check_blacklisted)
{
  if (check_valid)
    {
      for (int i = 0; i < self->num_descriptors; i++)
        {
          PluginDescriptor * cur_descr = self->descriptors[i];
          if (plugin_descriptor_is_same_plugin (
                cur_descr, descr))
            {
              return cur_descr;
            }
        }
    }
  if (check_blacklisted)
    {
      for (int i = 0; i < self->num_blacklisted; i++)
        {
          PluginDescriptor * cur_descr = self->blacklisted[i];
          if (plugin_descriptor_is_same_plugin (
                cur_descr, descr))
            {
              return cur_descr;
            }
        }
    }

  return NULL;
}

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
  const char *              abs_path)
{
  PluginDescriptor ** descriptors =
    object_new_n (1, PluginDescriptor *);
  int num_descriptors = 0;

  g_debug ("Getting cached descriptors for %s", abs_path);

  for (int i = 0; i < self->num_descriptors; i++)
    {
      PluginDescriptor * descr = self->descriptors[i];

      /* skip LV2 since they don't have paths */
      if (descr->protocol == PROT_LV2)
        continue;

      GFile * file = g_file_new_for_path (descr->path);
      if (string_is_equal (descr->path, abs_path))
        {
          if (descr->ghash == g_file_hash (file))
            {
              num_descriptors++;
              descriptors = g_realloc (
                descriptors,
                (size_t) (num_descriptors + 1)
                  * sizeof (PluginDescriptor *));
              descriptors[num_descriptors - 1] = descr;
            }
          else
            {
              g_debug (
                "hash differs %u != %u", descr->ghash,
                g_file_hash (file));
            }
        }
      g_object_unref (file);
    }

  if (num_descriptors == 0)
    {
      free (descriptors);
      return NULL;
    }

  /* NULL-terminate */
  descriptors[num_descriptors] = NULL;

  return descriptors;
}

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
  int                       _serialize)
{
  g_return_if_fail (abs_path && self);

  PluginDescriptor * new_descr = object_new (PluginDescriptor);
  new_descr->path = g_strdup (abs_path);
  GFile * file = g_file_new_for_path (abs_path);
  new_descr->ghash = g_file_hash (file);
  g_object_unref (file);
  self->blacklisted[self->num_blacklisted++] = new_descr;
  if (_serialize)
    {
      cached_plugin_descriptors_serialize_to_file (self);
    }
}

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
  bool                      _serialize)
{
  PluginDescriptor * new_descr =
    plugin_descriptor_clone (_new_descr);

  for (int i = 0; i < self->num_descriptors; i++)
    {
      PluginDescriptor * cur_descr = self->descriptors[i];
      if (plugin_descriptor_is_same_plugin (
            cur_descr, new_descr))
        {
          self->descriptors[i] = new_descr;
          plugin_descriptor_free (cur_descr);
          goto check_serialize;
        }
    }
  for (int i = 0; i < self->num_blacklisted; i++)
    {
      PluginDescriptor * cur_descr = self->blacklisted[i];
      if (plugin_descriptor_is_same_plugin (
            cur_descr, new_descr))
        {
          self->descriptors[i] = new_descr;
          plugin_descriptor_free (cur_descr);
          goto check_serialize;
        }
    }
  /* plugin not found, add instead */
  cached_plugin_descriptors_add (self, _new_descr, _serialize);
  plugin_descriptor_free (new_descr);
  return;

check_serialize:
  if (_serialize)
    {
      cached_plugin_descriptors_serialize_to_file (self);
    }
}

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
  int                       _serialize)
{
  PluginDescriptor * new_descr =
    plugin_descriptor_clone (descr);
  if (descr->path)
    {
      GFile * file = g_file_new_for_path (descr->path);
      new_descr->ghash = g_file_hash (file);
      g_object_unref (file);
    }
  self->descriptors[self->num_descriptors++] = new_descr;

  if (_serialize)
    {
      cached_plugin_descriptors_serialize_to_file (self);
    }
}

/**
 * Clears the descriptors and removes the cache file.
 */
void
cached_plugin_descriptors_clear (
  CachedPluginDescriptors * self)
{
  for (int i = 0; i < self->num_descriptors; i++)
    {
      plugin_descriptor_free (self->descriptors[i]);
    }
  self->num_descriptors = 0;

  delete_file ();
}

void
cached_plugin_descriptors_free (CachedPluginDescriptors * self)
{
  for (int i = 0; i < self->num_descriptors; i++)
    {
      object_free_w_func_and_null (
        plugin_descriptor_free, self->descriptors[i]);
    }
  for (int i = 0; i < self->num_blacklisted; i++)
    {
      object_free_w_func_and_null (
        plugin_descriptor_free, self->blacklisted[i]);
    }

  object_zero_and_free (self);
}
