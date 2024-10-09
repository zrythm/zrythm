// SPDX-FileCopyrightText: © 2020-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "common/plugins/collections.h"
#include "common/utils/directory_manager.h"
#include "common/utils/string.h"
#include "gui/backend/backend/zrythm.h"

#include <glib/gi18n.h>

constexpr const char * PLUGIN_COLLECTIONS_JSON_FILENAME =
  "plugin-collections.json";

using namespace zrythm::plugins;

bool
PluginCollection::contains_descriptor (
  const zrythm::plugins::PluginDescriptor &descr) const
{
  return std::any_of (
    descriptors_.begin (), descriptors_.end (), [&descr] (const auto &cur_descr) {
      return descr.is_same_plugin (*cur_descr);
    });
}

void
PluginCollection::add_descriptor (const zrythm::plugins::PluginDescriptor &descr)
{
  if (contains_descriptor (descr))
    {
      return;
    }

  auto new_descr = descr.clone_unique ();
  if (!descr.path_.empty ())
    {
      auto file = Gio::File::create_for_path (descr.path_.string ());
      new_descr->ghash_ = file->hash ();
    }
  descriptors_.emplace_back (std::move (new_descr));
}

void
PluginCollection::remove_descriptor (
  const zrythm::plugins::PluginDescriptor &descr)
{
  std::erase_if (descriptors_, [&descr] (const auto &cur_descr) {
    return cur_descr->is_same_plugin (descr);
  });
}

#if 0
Glib::RefPtr<Gio::MenuModel>
PluginCollection::generate_context_menu () const
{
  auto menu = Gio::Menu::create ();

  // TODO icon name edit-rename
  menu->append (_ ("Rename"), "app.plugin-collection-rename");

  // TODO icon name edit-delete
  menu->append (_ ("Delete"), "app.plugin-collection-remove");

  return menu;
}
#endif

void
PluginCollection::init_after_cloning (const PluginCollection &other)
{
  name_ = other.name_;
  description_ = other.description_;
  clone_unique_ptr_container (descriptors_, other.descriptors_);
}

/* ============================================================================ */

fs::path
PluginCollections::get_file_path ()
{
  auto dir_mgr = DirectoryManager::getInstance ();
  auto zrythm_dir = dir_mgr->get_dir (DirectoryManager::DirectoryType::USER_TOP);
  z_return_val_if_fail (!zrythm_dir.empty (), "");

  return fs::path (zrythm_dir) / PLUGIN_COLLECTIONS_JSON_FILENAME;
}

void
PluginCollections::serialize_to_file () const
{
  z_info ("Serializing plugin collections...");
  auto json = serialize_to_json_string ();
  auto path = get_file_path ();
  z_return_if_fail (
    !path.empty () && path.is_absolute () && path.has_parent_path ());
  z_debug ("Writing plugin collections to {}...", path);
  try
    {
      Glib::file_set_contents (path.string (), json.c_str ());
    }
  catch (const Glib::FileError &e)
    {
      throw ZrythmException (
        format_str ("Unable to write plugin collections: {}", e.what ()));
    }
}

void
PluginCollections::delete_file ()
{
  auto path = get_file_path ();
  if (!fs::exists (path))
    {
      z_info ("Plugin collections file at {} does not exist", path);
      return;
    }
  z_debug ("Deleting plugin collections file at {}", path);
  try
    {
      fs::remove (path);
    }
  catch (const fs::filesystem_error &e)
    {
      z_warning ("Failed to delete plugin collections file: {}", e.what ());
    }
}

std::unique_ptr<PluginCollections>
PluginCollections::read_or_new ()
{
  auto ret = std::make_unique<PluginCollections> ();
  auto path = get_file_path ();
  if (!fs::exists (path))
    {
      z_info ("Plugin collections file at {} does not exist", path);
      return ret;
    }

  std::string json;
  try
    {
      json = Glib::file_get_contents (path.string ());
    }
  catch (const Glib::Error &e)
    {
      z_warning ("Failed to create PluginCollections from {}", path);
      return ret;
    }

  try
    {
      ret->deserialize_from_json_string (json.c_str ());
    }
  catch (const ZrythmException &e)
    {
      z_warning (
        "Found invalid cached plugin descriptor file (error: {}). "
        "Purging file and creating a new one.",
        e.what ());

      try
        {
          delete_file ();
        }
      catch (const ZrythmException &e2)
        {
          z_warning (e2.what ());
        }

      return {};
    }

  return ret;
}

void
PluginCollections::add (
  const zrythm::plugins::PluginCollection &collection,
  bool                                     serialize)
{
  collections_.emplace_back (collection.clone_unique ());

  if (serialize)
    {
      serialize_to_file_no_throw ();
    }
}

const zrythm::plugins::PluginCollection *
PluginCollections::find_from_name (std::string_view name) const
{
  auto it = std::find_if (
    collections_.begin (), collections_.end (),
    [&name] (const auto &collection) { return collection->name_ == name; });

  return it == collections_.end () ? nullptr : (*it).get ();
}

void
PluginCollections::remove (PluginCollection &collection, bool serialize)
{
  std::erase_if (collections_, [&collection] (const auto &cur_collection) {
    return cur_collection->name_ == collection.name_;
  });

  if (serialize)
    {
      serialize_to_file_no_throw ();
    }
}