// SPDX-FileCopyrightText: © 2020-2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "plugins/collections.h"
#include "utils/error.h"
#include "utils/file.h"
#include "utils/objects.h"
#include "utils/string.h"
#include "zrythm.h"

#include <glib/gi18n.h>

static char *
get_plugin_collections_file_path (void)
{
  char * zrythm_dir = zrythm_get_dir (ZRYTHM_DIR_USER_TOP);
  g_return_val_if_fail (zrythm_dir, NULL);

  return g_build_filename (zrythm_dir, "plugin_collections.yaml", NULL);
}

void
plugin_collections_serialize_to_file (PluginCollections * self)
{
  g_message ("Serializing plugin collections...");
  GError * err = NULL;
  char *   yaml = yaml_serialize (self, &plugin_collections_schema, &err);
  if (!yaml)
    {
      HANDLE_ERROR_LITERAL (err, _ ("Failed to serialize plugin collections"));
      return;
    }
  char * path = get_plugin_collections_file_path ();
  g_return_if_fail (path && strlen (path) > 2);
  g_message ("Writing plugin collections to %s...", path);
  g_file_set_contents (path, yaml, -1, &err);
  if (err != NULL)
    {
      g_warning (
        "Unable to write plugin collections "
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
    version_str, "schema_version: %d\n", PLUGIN_COLLECTIONS_SCHEMA_VERSION);
  same_version = g_str_has_prefix (yaml, version_str);
  if (!same_version)
    {
      sprintf (
        version_str, "---\nschema_version: %d\n",
        PLUGIN_COLLECTIONS_SCHEMA_VERSION);
      same_version = g_str_has_prefix (yaml, version_str);
    }

  return same_version;
}

/**
 * Reads the file and fills up the object.
 */
PluginCollections *
plugin_collections_new (void)
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
      PluginCollections * self = object_new (PluginCollections);
      self->schema_version = PLUGIN_COLLECTIONS_SCHEMA_VERSION;
      return self;
    }
  char * yaml = NULL;
  g_file_get_contents (path, &yaml, NULL, &err);
  if (err != NULL)
    {
      g_critical (
        "Failed to create PluginCollections "
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
        "Found old plugin collections file version. "
        "Backing up file and creating a new one.");
      GFile * file = g_file_new_for_path (path);
      char *  backup_path = g_strdup_printf ("%s.bak", path);
      GFile * backup_file = g_file_new_for_path (backup_path);
      g_file_move (
        file, backup_file, G_FILE_COPY_OVERWRITE, NULL, NULL, NULL, NULL);
      g_object_unref (backup_file);
      g_object_unref (file);
      g_free (backup_path);
      goto return_new_instance;
    }

  PluginCollections * self = (PluginCollections *) yaml_deserialize (
    yaml, &plugin_collections_schema, &err);
  if (!self)
    {
      g_critical (
        "Failed to deserialize "
        "PluginCollections from %s:\n%s",
        path, err->message);
      g_free (err);
      g_free (yaml);
      g_free (path);
      return NULL;
    }
  g_free (yaml);
  g_free (path);

  for (int i = 0; i < self->num_collections; i++)
    {
      plugin_collection_init_loaded (self->collections[i]);
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
  self->collections[self->num_collections++] = new_collection;

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
  for (int i = 0; i < self->num_collections; i++)
    {
      PluginCollection * collection = self->collections[i];
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
  bool found = false;
  for (int i = self->num_collections - 1; i >= 0; i--)
    {
      PluginCollection * collection = self->collections[i];

      if (collection == _collection)
        {
          for (int j = i; j < self->num_collections - 1; j++)
            {
              self->collections[j] = self->collections[j + 1];
            }
          self->num_collections--;
          found = true;
          break;
        }
    }

  g_warn_if_fail (found);

  if (serialize)
    {
      plugin_collections_serialize_to_file (self);
    }
}

void
plugin_collections_free (PluginCollections * self)
{
  for (int i = 0; i < self->num_collections; i++)
    {
      object_free_w_func_and_null (plugin_collection_free, self->collections[i]);
    }

  object_zero_and_free (self);
}
