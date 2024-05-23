// SPDX-FileCopyrightText: © 2020-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "io/serialization/plugin.h"
#include "plugins/collections.h"
#include "utils/error.h"
#include "utils/file.h"
#include "utils/objects.h"
#include "utils/string.h"
#include "zrythm.h"

#include <glib/gi18n.h>

#define PLUGIN_COLLECTIONS_JSON_TYPE "Zrythm Plugin Collections"
#define PLUGIN_COLLECTIONS_JSON_FILENAME "plugin-collections.json"

static PluginCollections *
create_new (void)
{
  PluginCollections * self = object_new (PluginCollections);
  self->collections =
    g_ptr_array_new_full (1, (GDestroyNotify) plugin_collection_free);
  return self;
}

static char *
get_plugin_collections_file_path (void)
{
  char * zrythm_dir = zrythm_get_dir (ZRYTHM_DIR_USER_TOP);
  g_return_val_if_fail (zrythm_dir, NULL);

  return g_build_filename (zrythm_dir, "plugin_collections.json", NULL);
}

static char *
serialize_to_json_str (const PluginCollections * self, GError ** error)
{
  /* create a mutable doc */
  yyjson_mut_doc * doc = yyjson_mut_doc_new (NULL);
  yyjson_mut_val * root = yyjson_mut_obj (doc);
  if (!root)
    {
      g_set_error_literal (error, 0, 0, "Failed to create root obj");
      return NULL;
    }
  yyjson_mut_doc_set_root (doc, root);

  yyjson_mut_obj_add_str (doc, root, "type", PLUGIN_COLLECTIONS_JSON_TYPE);
  yyjson_mut_obj_add_int (
    doc, root, "format", PLUGIN_COLLECTIONS_SCHEMA_VERSION);
  yyjson_mut_val * collections_arr =
    yyjson_mut_obj_add_arr (doc, root, "collections");
  for (size_t i = 0; i < self->collections->len; i++)
    {
      const PluginCollection * col =
        (PluginCollection *) g_ptr_array_index (self->collections, i);
      yyjson_mut_val * col_obj = yyjson_mut_arr_add_obj (doc, collections_arr);
      plugin_collection_serialize_to_json (doc, col_obj, col, error);
    }

  char * json = yyjson_mut_write (doc, YYJSON_WRITE_PRETTY_TWO_SPACES, NULL);
  g_message ("done writing json to string");

  yyjson_mut_doc_free (doc);

  return json;
}

static PluginCollections *
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
        yyjson_obj_iter_get (&it, "type"), PLUGIN_COLLECTIONS_JSON_TYPE))
    {
      g_set_error_literal (error, 0, 0, "Not a valid plugin collections file");
      return NULL;
    }

  int format = yyjson_get_int (yyjson_obj_iter_get (&it, "format"));
  if (format != PLUGIN_COLLECTIONS_SCHEMA_VERSION)
    {
      g_set_error_literal (error, 0, 0, "Invalid version");
      return NULL;
    }

  PluginCollections * self = create_new ();

  yyjson_val *    collections_arr = yyjson_obj_iter_get (&it, "collections");
  yyjson_arr_iter collections_it = yyjson_arr_iter_with (collections_arr);
  yyjson_val *    col_obj = NULL;
  while ((col_obj = yyjson_arr_iter_next (&collections_it)))
    {
      PluginCollection * col = plugin_collection_new ();
      g_ptr_array_add (self->collections, col);
      plugin_collection_deserialize_from_json (doc, col_obj, col, error);
    }

  yyjson_doc_free (doc);

  return self;
}

void
plugin_collections_serialize_to_file (PluginCollections * self)
{
  g_message ("Serializing plugin collections...");
  GError * err = NULL;
  char *   json = serialize_to_json_str (self, &err);
  if (!json)
    {
      HANDLE_ERROR_LITERAL (err, _ ("Failed to serialize plugin collections"));
      return;
    }
  char * path = get_plugin_collections_file_path ();
  g_return_if_fail (path && strlen (path) > 2);
  g_message ("Writing plugin collections to %s...", path);
  g_file_set_contents (path, json, -1, &err);
  if (err != NULL)
    {
      g_warning ("Unable to write plugin collections file: %s", err->message);
      g_error_free (err);
      g_free (path);
      g_free (json);
      g_return_if_reached ();
    }
  g_free (path);
  g_free (json);
}

/**
 * Reads the file and fills up the object.
 */
PluginCollections *
plugin_collections_read_or_new (void)
{
  GError * err = NULL;
  char *   path = get_plugin_collections_file_path ();
  if (!file_exists (path))
    {
      g_message (
        "Plugin collections file at %s does "
        "not exist",
        path);
return_new_instance:
      g_free (path);
      PluginCollections * self = create_new ();
      return self;
    }
  char * json = NULL;
  g_file_get_contents (path, &json, NULL, &err);
  if (err != NULL)
    {
      g_critical ("Failed to create PluginCollections from %s", path);
      g_free (err);
      g_free (json);
      g_free (path);
      return NULL;
    }

  PluginCollections * self = deserialize_from_json_str (json, &err);
  if (!self)
    {
      g_message (
        "Found invalid plugin collections file (error: %s). "
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

  for (size_t i = 0; i < self->collections->len; i++)
    {
      plugin_collection_init_loaded (
        (PluginCollection *) g_ptr_array_index (self->collections, i));
    }

  return self;
}

/**
 * Appends a collection.
 *
 * @param serialize Whether to serialize the updated
 *   cache now.
 */
void
plugin_collections_add (
  PluginCollections *      self,
  const PluginCollection * collection,
  bool                     serialize)
{
  PluginCollection * new_collection = plugin_collection_clone (collection);
  g_ptr_array_add (self->collections, new_collection);

  if (serialize)
    {
      plugin_collections_serialize_to_file (self);
    }
}

const PluginCollection *
plugin_collections_find_from_name (
  const PluginCollections * self,
  const char *              name)
{
  for (size_t i = 0; i < self->collections->len; i++)
    {
      const PluginCollection * collection =
        (PluginCollection *) g_ptr_array_index (self->collections, i);
      if (string_is_equal (collection->name, name))
        {
          return collection;
        }
    }

  return NULL;
}

/**
 * Removes the given collection.
 *
 * @param serialize Whether to serialize the updated
 *   cache now.
 */
void
plugin_collections_remove (
  PluginCollections * self,
  PluginCollection *  _collection,
  bool                serialize)
{
  bool found = g_ptr_array_remove (self->collections, _collection);
  ;
  g_warn_if_fail (found);

  if (serialize)
    {
      plugin_collections_serialize_to_file (self);
    }
}

void
plugin_collections_free (PluginCollections * self)
{
  object_free_w_func_and_null (g_ptr_array_unref, self->collections);

  object_zero_and_free (self);
}
