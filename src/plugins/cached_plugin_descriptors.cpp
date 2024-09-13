// SPDX-FileCopyrightText: © 2020-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "plugins/cached_plugin_descriptors.h"
#include "utils/string.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

#include <giomm.h>

constexpr const char * CACHED_PLUGIN_DESCRIPTORS_JSON_FILENAME =
  "cached-plugins.json";

fs::path
CachedPluginDescriptors::get_file_path ()
{
  auto * dir_mgr = ZrythmDirectoryManager::getInstance ();
  auto   zrythm_dir = dir_mgr->get_dir (ZrythmDirType::USER_TOP);
  z_return_val_if_fail (!zrythm_dir.empty (), "");

  return fs::path (zrythm_dir) / CACHED_PLUGIN_DESCRIPTORS_JSON_FILENAME;
}

void
CachedPluginDescriptors::define_fields (const Context &ctx)
{
  using T = ISerializable<CachedPluginDescriptors>;
  T::serialize_fields (
    ctx, T::make_field ("descriptors", descriptors_),
    T::make_field ("blacklistedSha1s", blacklisted_sha1s_));
}

void
CachedPluginDescriptors::serialize_to_file ()
{
  z_info ("Serializing cached plugin descriptors...");

  auto json_str = serialize_to_json_string ();

  auto path = get_file_path ();
  z_return_if_fail (
    !path.empty () && path.is_absolute () && path.has_parent_path ());
  z_debug ("Writing cached plugin descriptors to {}...", path);
  try
    {
      Glib::file_set_contents (path.string (), json_str.c_str ());
    }
  catch (const Glib::FileError &e)
    {
      throw ZrythmException (format_str (
        "Unable to write cached plugin descriptors: {}", e.what ()));
    }
}

std::unique_ptr<CachedPluginDescriptors>
CachedPluginDescriptors::read_or_new ()
{
  auto ret = std::make_unique<CachedPluginDescriptors> ();
  auto path = get_file_path ();
  if (!fs::exists (path))
    {
      z_info ("Cached plugin descriptors file at {} does not exist", path);
      return ret;
    }

  std::string json;
  try
    {
      json = Glib::file_get_contents (path.string ());
    }
  catch (const Glib::Error &e)
    {
      z_warning ("Failed to create CachedPluginDescriptors from {}", path);
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

// FIXME: do this in the deserialize override of PluginDescriptor
#if 0
for (size_t i = 0; i < self->descriptors->len; i++)
  {
    PluginDescriptor * descr =
      (PluginDescriptor *) g_ptr_array_index (self->descriptors, i);
    descr->category = plugin_descriptor_string_to_category (descr->category_str);
  }
#endif

  return ret;
}

void
CachedPluginDescriptors::delete_file ()
{
  auto path = get_file_path ();
  z_return_if_fail (
    !path.empty () && path.is_absolute () && path.has_parent_path ());
  try
    {
      fs::remove (path);
    }
  catch (const fs::filesystem_error &e)
    {
      throw ZrythmException (format_str (
        "Failed to remove invalid cached plugin descriptors file: {}",
        e.what ()));
    }
}

bool
CachedPluginDescriptors::is_blacklisted (std::string_view sha1)
{
  return std::any_of (
    blacklisted_sha1s_.begin (), blacklisted_sha1s_.end (),
    [sha1] (const auto &blacklisted_sha1) { return blacklisted_sha1 == sha1; });
}

std::vector<PluginDescriptor>
CachedPluginDescriptors::get_valid_descriptors_for_sha1 (std::string_view sha1)
{
  z_return_val_if_fail (!sha1.empty (), {});

  auto sha1_exists = [sha1] (const auto &descr) { return descr.sha1_ == sha1; };

  std::vector<PluginDescriptor> res;
  std::ranges::copy_if (descriptors_, std::back_inserter (res), sha1_exists);
  return res;
}

bool
CachedPluginDescriptors::contains_sha1 (
  std::string_view sha1,
  bool             check_valid,
  bool             check_blacklisted)
{
  z_return_val_if_fail (!sha1.empty (), false);
  if (check_valid)
    {
      if (
        std::any_of (
          descriptors_.begin (), descriptors_.end (),
          [sha1] (const auto &cur_descr) { return cur_descr.sha1_ == sha1; }))
        {
          return true;
        }
    }
  if (check_blacklisted)
    {
      if (
        std::any_of (
          blacklisted_sha1s_.begin (), blacklisted_sha1s_.end (),
          [sha1] (const auto &cur_sha1) { return cur_sha1 == sha1; }))
        {
          return true;
        }
    }

  return false;
}

void
CachedPluginDescriptors::blacklist (std::string_view sha1, bool serialize)
{
  z_return_if_fail (!sha1.empty ());

  blacklisted_sha1s_.push_back (std::string (sha1));
  if (serialize)
    {
      serialize_to_file_no_throw ();
    }
}

void
CachedPluginDescriptors::add (const PluginDescriptor &descr, bool serialize)
{
  PluginDescriptor new_descr = descr;
  if (!descr.path_.empty ())
    {
      auto file = Gio::File::create_for_path (descr.path_.string ());
      new_descr.ghash_ = file->hash ();
    }
  descriptors_.push_back (new_descr);

  if (serialize)
    {
      serialize_to_file_no_throw ();
    }
}

void
CachedPluginDescriptors::clear ()
{
  descriptors_.clear ();
  blacklisted_sha1s_.clear ();
  delete_file ();
}