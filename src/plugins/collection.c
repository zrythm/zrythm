// clang-format off
// SPDX-FileCopyrightText: © 2020-2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
// clang-format on

#include "plugins/collection.h"
#include "utils/arrays.h"
#include "utils/gtk.h"
#include "utils/mem.h"
#include "utils/objects.h"

void
plugin_collection_init_loaded (PluginCollection * self)
{
  g_return_if_fail (self);

  for (size_t i = 0; i < self->descriptors->len; i++)
    {
      PluginDescriptor * descr = g_ptr_array_index (self->descriptors, i);
      descr->category =
        plugin_descriptor_string_to_category (descr->category_str);
    }
}

/**
 * Creates a new plugin collection.
 */
PluginCollection *
plugin_collection_new (void)
{
  PluginCollection * self = object_new (PluginCollection);

  self->descriptors =
    g_ptr_array_new_full (1, (GDestroyNotify) plugin_descriptor_free);

  return self;
}

/**
 * Clones a plugin collection.
 */
PluginCollection *
plugin_collection_clone (const PluginCollection * self)
{
  PluginCollection * clone = plugin_collection_new ();

  for (size_t i = 0; i < self->descriptors->len; i++)
    {
      PluginDescriptor * descr = g_ptr_array_index (self->descriptors, i);
      g_ptr_array_add (clone->descriptors, plugin_descriptor_clone (descr));
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
plugin_collection_set_name (PluginCollection * self, const char * name)
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
  for (size_t i = 0; i < self->descriptors->len; i++)
    {
      PluginDescriptor * cur_descr = g_ptr_array_index (self->descriptors, i);
      if (
        (match_pointer && descr == cur_descr)
        || (!match_pointer && plugin_descriptor_is_same_plugin (descr, cur_descr)))
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
  if (plugin_collection_contains_descriptor (self, descr, false))
    {
      return;
    }

  PluginDescriptor * new_descr = plugin_descriptor_clone (descr);
  if (descr->path)
    {
      GFile * file = g_file_new_for_path (descr->path);
      new_descr->ghash = g_file_hash (file);
      g_object_unref (file);
    }
  g_ptr_array_add (self->descriptors, new_descr);
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
  for (size_t i = 0; i < self->descriptors->len; i++)
    {
      PluginDescriptor * cur_descr = g_ptr_array_index (self->descriptors, i);
      if (plugin_descriptor_is_same_plugin (_descr, cur_descr))
        {
          g_ptr_array_remove (self->descriptors, cur_descr);
          found = true;
          break;
        }
    }

  g_warn_if_fail (found);
}

GMenuModel *
plugin_collection_generate_context_menu (const PluginCollection * self)
{
  GMenu *     menu = g_menu_new ();
  GMenuItem * menuitem;

  menuitem = z_gtk_create_menu_item (
    _ ("Rename"), "edit-rename", "app.plugin-collection-rename");
  g_menu_append_item (menu, menuitem);

  menuitem = z_gtk_create_menu_item (
    _ ("Delete"), "edit-delete", "app.plugin-collection-remove");
  g_menu_append_item (menu, menuitem);

  return G_MENU_MODEL (menu);
}

/**
 * Removes all the descriptors.
 */
void
plugin_collection_clear (PluginCollection * self)
{
  g_ptr_array_remove_range (self->descriptors, 0, self->descriptors->len);
}

void
plugin_collection_free (PluginCollection * self)
{
  object_free_w_func_and_null (g_ptr_array_unref, self->descriptors);
  g_free_and_null (self->name);
  g_free_and_null (self->description);

  object_zero_and_free (self);
}
