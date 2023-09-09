// SPDX-FileCopyrightText: © 2020 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "plugins/collection.h"
#include "utils/arrays.h"
#include "utils/gtk.h"
#include "utils/mem.h"
#include "utils/objects.h"

void
plugin_collection_init_loaded (PluginCollection * self)
{
  g_return_if_fail (self);

  self->descriptors_size = (size_t) self->num_descriptors;
  if (self->descriptors_size == 0)
    {
      self->descriptors_size = 1;
      self->descriptors = object_new_n (
        self->descriptors_size, PluginDescriptor *);
    }
  for (int i = 0; i < self->num_descriptors; i++)
    {
      self->descriptors[i]
        ->category = plugin_descriptor_string_to_category (
        self->descriptors[i]->category_str);
    }
}

/**
 * Creates a new plugin collection.
 */
PluginCollection *
plugin_collection_new (void)
{
  PluginCollection * self = object_new (PluginCollection);

  self->schema_version = PLUGIN_COLLECTION_SCHEMA_VERSION;
  self->name = g_strdup ("");
  self->descriptors_size = 1;
  self->descriptors =
    object_new_n (self->descriptors_size, PluginDescriptor *);

  return self;
}

/**
 * Clones a plugin collection.
 */
PluginCollection *
plugin_collection_clone (const PluginCollection * self)
{
  PluginCollection * clone = plugin_collection_new ();

  for (int i = 0; i < self->num_descriptors; i++)
    {
      clone->descriptors[clone->num_descriptors++] =
        plugin_descriptor_clone (self->descriptors[i]);
    }

  clone->name = g_strdup (self->name);
  if (self->description)
    {
      clone->description = g_strdup (self->description);
    }

  return clone;
}

char *
plugin_collection_get_name (PluginCollection * self)
{
  return self->name;
}

void
plugin_collection_set_name (
  PluginCollection * self,
  const char *       name)
{
  self->name = g_strdup (name);
}

/**
 * Returns whether the collection contains the
 * given descriptor.
 *
 * @param match_pointer Whether to check pointers
 *   or the descriptor details.
 */
bool
plugin_collection_contains_descriptor (
  PluginCollection *       self,
  const PluginDescriptor * descr,
  bool                     match_pointer)
{
  for (int i = 0; i < self->num_descriptors; i++)
    {
      if ((match_pointer &&
           descr == self->descriptors[i]) ||
          (!match_pointer &&
           plugin_descriptor_is_same_plugin (
             descr, self->descriptors[i])))
        {
          return true;
        }
    }
  return false;
}

/**
 * Appends a descriptor to the collection.
 */
void
plugin_collection_add_descriptor (
  PluginCollection *       self,
  const PluginDescriptor * descr)
{
  if (plugin_collection_contains_descriptor (
        self, descr, false))
    {
      return;
    }

  PluginDescriptor * new_descr =
    plugin_descriptor_clone (descr);
  if (descr->path)
    {
      GFile * file = g_file_new_for_path (descr->path);
      new_descr->ghash = g_file_hash (file);
      g_object_unref (file);
    }
  array_double_size_if_full (
    self->descriptors, self->num_descriptors,
    self->descriptors_size, PluginDescriptor *);
  self->descriptors[self->num_descriptors++] = new_descr;
}

/**
 * Removes the descriptor matching the given one from
 * the collection.
 */
void
plugin_collection_remove_descriptor (
  PluginCollection *       self,
  const PluginDescriptor * _descr)
{
  bool found = false;
  for (int i = 0; i < self->num_descriptors; i++)
    {
      PluginDescriptor * descr = self->descriptors[i];
      if (plugin_descriptor_is_same_plugin (_descr, descr))
        {
          for (int j = i; j < self->num_descriptors - 1; j++)
            {
              self->descriptors[j] = self->descriptors[j + 1];
            }
          self->num_descriptors--;
          found = true;
          break;
        }
    }

  g_warn_if_fail (found);
}

GMenuModel *
plugin_collection_generate_context_menu (
  const PluginCollection * self)
{
  GMenu *     menu = g_menu_new ();
  GMenuItem * menuitem;

  menuitem = z_gtk_create_menu_item (
    _ ("Rename"), "edit-rename",
    "app.plugin-collection-rename");
  g_menu_append_item (menu, menuitem);

  menuitem = z_gtk_create_menu_item (
    _ ("Delete"), "edit-delete",
    "app.plugin-collection-remove");
  g_menu_append_item (menu, menuitem);

  return G_MENU_MODEL (menu);
}

/**
 * Removes all the descriptors.
 */
void
plugin_collection_clear (PluginCollection * self)
{
  for (int i = 0; i < self->num_descriptors; i++)
    {
      plugin_descriptor_free (self->descriptors[i]);
    }
  self->num_descriptors = 0;
}

void
plugin_collection_free (PluginCollection * self)
{
  for (int i = 0; i < self->num_descriptors; i++)
    {
      object_free_w_func_and_null (
        plugin_descriptor_free, self->descriptors[i]);
    }
  object_zero_and_free (self->descriptors);

  g_free_and_null (self->name);
  g_free_and_null (self->description);
}
