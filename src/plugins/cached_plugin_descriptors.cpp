// SPDX-FileCopyrightText: © 2020-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "io/serialization/plugin.h"
#include "plugins/cached_plugin_descriptors.h"
#include "utils/error.h"
#include "utils/file.h"
#include "utils/io.h"
#include "utils/objects.h"
#include "utils/string.h"
#include "zrythm.h"

#include <glib/gi18n.h>

#define CACHED_PLUGIN_DESCRIPTORS_JSON_TYPE "Zrythm Cached Plugins"
#define CACHED_PLUGIN_DESCRIPTORS_JSON_FILENAME "cached-plugins.json"

static CachedPluginDescriptors *
create_new (void)
{
  CachedPluginDescriptors * self = object_new (CachedPluginDescriptors);
  self->descriptors =
    g_ptr_array_new_full (100, (GDestroyNotify) plugin_descriptor_free);
  self->blacklisted_sha1s = g_ptr_array_new_full (100, (GDestroyNotify) g_free);

  return self;
}

static char *
get_cached_plugin_descriptors_file_path (void)
{
  char * zrythm_dir = gZrythmDirMgr->get_dir (ZRYTHM_DIR_USER_TOP);
  g_return_val_if_fail (zrythm_dir, NULL);

  return g_build_filename (
    zrythm_dir, CACHED_PLUGIN_DESCRIPTORS_JSON_FILENAME, NULL);
}

void
cached_plugin_descriptors_serialize_to_file (CachedPluginDescriptors * self)
{
  g_message ("Serializing cached plugin descriptors...");

  /* create a mutable doc */
  yyjson_mut_doc * doc = yyjson_mut_doc_new (NULL);
  yyjson_mut_val * root = yyjson_mut_obj (doc);
  g_return_if_fail (root);
  yyjson_mut_doc_set_root (doc, root);

  yyjson_mut_obj_add_str (
    doc, root, "type", CACHED_PLUGIN_DESCRIPTORS_JSON_TYPE);
  yyjson_mut_obj_add_int (
    doc, root, "format", CACHED_PLUGIN_DESCRIPTORS_SCHEMA_VERSION);

  yyjson_mut_val * descriptors_arr =
    yyjson_mut_obj_add_arr (doc, root, "descriptors");
  for (size_t i = 0; i < self->descriptors->len; i++)
    {
      const PluginDescriptor * descr =
        (const PluginDescriptor *) g_ptr_array_index (self->descriptors, i);
      yyjson_mut_val * descr_obj = yyjson_mut_arr_add_obj (doc, descriptors_arr);
      GError * err = NULL;
      plugin_descriptor_serialize_to_json (doc, descr_obj, descr, &err);
      g_return_if_fail (err == NULL);
    }
  yyjson_mut_val * blacklisted_sha1s_arr =
    yyjson_mut_obj_add_arr (doc, root, "blacklistedSha1s");
  for (size_t i = 0; i < self->blacklisted_sha1s->len; i++)
    {
      const char * str =
        (const char *) g_ptr_array_index (self->blacklisted_sha1s, i);
      yyjson_mut_arr_add_str (doc, blacklisted_sha1s_arr, str);
    }

  /* to string */
  char * json = yyjson_mut_write (doc, YYJSON_WRITE_PRETTY_TWO_SPACES, NULL);
  yyjson_mut_doc_free (doc);
  g_message ("done writing json to string");
  g_return_if_fail (json);

  char * path = get_cached_plugin_descriptors_file_path ();
  g_return_if_fail (path && strlen (path) > 2);
  g_message ("Writing cached plugin descriptors to %s...", path);
  GError * err = NULL;
  g_file_set_contents (path, json, -1, &err);
  if (err != NULL)
    {
      g_warning ("Unable to write cached plugins file: %s", err->message);
      g_error_free (err);
      g_free (path);
      g_free (json);
      g_return_if_reached ();
    }

  g_free (path);
  g_free (json);
}

static CachedPluginDescriptors *
deserialize_from_json_str (const char * json, GError ** error)
{
  yyjson_doc * doc =
    yyjson_read_opts ((char *) json, strlen (json), 0, NULL, NULL);
  yyjson_val * root = yyjson_doc_get_root (doc);
  if (!root)
    {
      g_set_error_literal (error, 0, 0, "Failed to create root obj");
      return NULL;
    }

  yyjson_obj_iter it = yyjson_obj_iter_with (root);
  if (!yyjson_equals_str (
        yyjson_obj_iter_get (&it, "type"), CACHED_PLUGIN_DESCRIPTORS_JSON_TYPE))
    {
      g_set_error_literal (error, 0, 0, "Not a valid cached plugins file");
      return NULL;
    }

  int format = yyjson_get_int (yyjson_obj_iter_get (&it, "format"));
  if (format != CACHED_PLUGIN_DESCRIPTORS_SCHEMA_VERSION)
    {
      g_set_error_literal (error, 0, 0, "Invalid version");
      return NULL;
    }

  CachedPluginDescriptors * self = create_new ();

  yyjson_val *    descriptors_arr = yyjson_obj_iter_get (&it, "descriptors");
  yyjson_arr_iter descriptors_it = yyjson_arr_iter_with (descriptors_arr);
  yyjson_val *    descr_obj = NULL;
  while ((descr_obj = yyjson_arr_iter_next (&descriptors_it)))
    {
      PluginDescriptor * descr = plugin_descriptor_new ();
      g_ptr_array_add (self->descriptors, descr);
      plugin_descriptor_deserialize_from_json (doc, descr_obj, descr, error);
    }
  yyjson_val * blacklisted_sha1s_arr =
    yyjson_obj_iter_get (&it, "blacklistedSha1s");
  yyjson_arr_iter blacklisted_sha1s_it =
    yyjson_arr_iter_with (blacklisted_sha1s_arr);
  yyjson_val * blacklisted_sha1_obj = NULL;
  while ((blacklisted_sha1_obj = yyjson_arr_iter_next (&blacklisted_sha1s_it)))
    {
      g_ptr_array_add (
        self->blacklisted_sha1s,
        g_strdup (yyjson_get_str (blacklisted_sha1_obj)));
    }

  yyjson_doc_free (doc);

  return self;
}

CachedPluginDescriptors *
cached_plugin_descriptors_read_or_new (void)
{
  GError * err = NULL;
  char *   path = get_cached_plugin_descriptors_file_path ();
  if (!file_exists (path))
    {
      g_message ("Cached plugin descriptors file at %s does not exist", path);
return_new_instance:
      g_free (path);
      CachedPluginDescriptors * self = create_new ();
      return self;
    }
  char * json = NULL;
  g_file_get_contents (path, &json, NULL, &err);
  if (err != NULL)
    {
      g_critical ("Failed to create CachedPluginDescriptors from %s", path);
      g_free (err);
      g_free (path);
      return NULL;
    }

  CachedPluginDescriptors * self = deserialize_from_json_str (json, &err);
  if (!self)
    {
      g_message (
        "Found invalid cached plugin descriptor file (error: %s). "
        "Purging file and creating a new one.",
        err->message);
      g_error_free (err);
      GFile * file = g_file_new_for_path (path);
      g_file_delete (file, NULL, NULL);
      g_object_unref (file);
      g_free (json);
      goto return_new_instance;
    }
  g_free (json);
  g_free (path);

  for (size_t i = 0; i < self->descriptors->len; i++)
    {
      PluginDescriptor * descr =
        (PluginDescriptor *) g_ptr_array_index (self->descriptors, i);
      descr->category =
        plugin_descriptor_string_to_category (descr->category_str);
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
      g_warning ("Failed to delete cached plugin descriptors file");
    }
  g_free (path);
  g_object_unref (file);
}

bool
cached_plugin_descriptors_is_blacklisted (
  CachedPluginDescriptors * self,
  const char *              sha1)
{
  for (size_t i = 0; i < self->blacklisted_sha1s->len; i++)
    {
      const char * cur_sha1 =
        (const char *) g_ptr_array_index (self->blacklisted_sha1s, i);
      if (string_is_equal (cur_sha1, sha1))
        {
          return true;
        }
    }
  return false;
}

unsigned int
cached_plugin_descriptors_find (
  CachedPluginDescriptors * self,
  GPtrArray *               arr,
  const PluginDescriptor *  descr,
  const char *              sha1,
  bool                      check_valid,
  bool                      check_blacklisted)
{
  unsigned int num_found = 0;
  if (check_valid)
    {
      for (size_t i = 0; i < self->descriptors->len; i++)
        {
          PluginDescriptor * cur_descr =
            (PluginDescriptor *) g_ptr_array_index (self->descriptors, i);
          if (
            (sha1 && string_is_equal (cur_descr->sha1, sha1))
            || (descr && plugin_descriptor_is_same_plugin (cur_descr, descr)))
            {
              g_ptr_array_add (arr, cur_descr);
              num_found++;
            }
        }
    }
  if (check_blacklisted)
    {
      for (size_t i = 0; i < self->blacklisted_sha1s->len; i++)
        {
          char * cur_sha1 =
            (char *) g_ptr_array_index (self->blacklisted_sha1s, i);
          if (sha1 && string_is_equal (cur_sha1, sha1))
            {
              g_ptr_array_add (arr, cur_sha1);
              num_found++;
            }
        }
    }

  return num_found;
}

void
cached_plugin_descriptors_blacklist (
  CachedPluginDescriptors * self,
  const char *              sha1,
  bool                      _serialize)
{
  g_return_if_fail (sha1 && self);

  g_ptr_array_add (self->blacklisted_sha1s, g_strdup (sha1));
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
  PluginDescriptor * new_descr = plugin_descriptor_clone (descr);
  if (descr->path)
    {
      GFile * file = g_file_new_for_path (descr->path);
      new_descr->ghash = g_file_hash (file);
      g_object_unref (file);
    }
  g_ptr_array_add (self->descriptors, new_descr);

  if (_serialize)
    {
      cached_plugin_descriptors_serialize_to_file (self);
    }
}

/**
 * Clears the descriptors and removes the cache file.
 */
void
cached_plugin_descriptors_clear (CachedPluginDescriptors * self)
{
  g_ptr_array_remove_range (self->descriptors, 0, self->descriptors->len);
  g_ptr_array_remove_range (
    self->blacklisted_sha1s, 0, self->blacklisted_sha1s->len);

  delete_file ();
}

void
cached_plugin_descriptors_free (CachedPluginDescriptors * self)
{
  object_free_w_func_and_null (g_ptr_array_unref, self->descriptors);
  object_free_w_func_and_null (g_ptr_array_unref, self->blacklisted_sha1s);

  object_zero_and_free (self);
}
