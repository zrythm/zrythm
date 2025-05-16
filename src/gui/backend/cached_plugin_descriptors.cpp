// SPDX-FileCopyrightText: © 2020-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/backend/zrythm.h"
#include "gui/backend/zrythm_application.h"
#include "utils/directory_manager.h"
#include "utils/io.h"
#include "utils/string.h"

#include "./cached_plugin_descriptors.h"

constexpr const char * CACHED_PLUGIN_DESCRIPTORS_JSON_FILENAME =
  "cached-plugins.json";

using namespace zrythm;
namespace zrythm::gui::old_dsp::plugins
{

fs::path
CachedPluginDescriptors::get_file_path ()
{
  auto zrythm_dir =
    dynamic_cast<ZrythmApplication *> (qApp)->get_directory_manager ().get_dir (
      IDirectoryManager::DirectoryType::USER_TOP);
  z_return_val_if_fail (!zrythm_dir.empty (), "");

  return fs::path (zrythm_dir) / CACHED_PLUGIN_DESCRIPTORS_JSON_FILENAME;
}

void
CachedPluginDescriptors::serialize_to_file ()
{
  z_info ("Serializing cached plugin descriptors...");

  nlohmann::json json = *this;
  auto           json_str = json.dump ();

  auto path = get_file_path ();
  z_return_if_fail (
    !path.empty () && path.is_absolute () && path.has_parent_path ());
  z_debug ("Writing cached plugin descriptors to {}...", path);
  try
    {
      utils::io::set_file_contents (
        path, utils::Utf8String::from_utf8_encoded_string (json_str));
    }
  catch (const ZrythmException &e)
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
      json =
        utils::Utf8String::from_qstring (utils::io::read_file_contents (path))
          .str ();
    }
  catch (const ZrythmException &e)
    {
      z_warning ("Failed to create CachedPluginDescriptors from {}", path);
      return ret;
    }

  try
    {
      auto nlohmann_json = nlohmann::json::parse (json);
      ret = std::make_unique<CachedPluginDescriptors> ();
      from_json (nlohmann_json, *ret);
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

// FIXME: do this in the deserialize override of
// zrythm::gui::old_dsp::plugins::PluginDescriptor
#if 0
for (size_t i = 0; i < self->descriptors->len; i++)
  {
    zrythm::gui::old_dsp::plugins::PluginDescriptor * descr =
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

std::vector<std::unique_ptr<PluginDescriptor>>
CachedPluginDescriptors::get_valid_descriptors_for_sha1 (std::string_view sha1)
{
  z_return_val_if_fail (!sha1.empty (), {});

  // auto sha1_exists = [sha1] (const auto &descr) {
  // return descr->sha1_ == sha1;
  // };

  std::vector<std::unique_ptr<PluginDescriptor>> res;
  for (auto &descr : descriptors_)
    {
      if (descr->sha1_ == sha1)
        {
          res.emplace_back (descr->clone_unique ());
        }
    }
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
          [sha1] (const auto &cur_descr) { return cur_descr->sha1_ == sha1; }))
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

  blacklisted_sha1s_.emplace_back (sha1);
  if (serialize)
    {
      serialize_to_file_no_throw ();
    }
}

void
CachedPluginDescriptors::add (
  const zrythm::gui::old_dsp::plugins::PluginDescriptor &descr,
  bool                                                   serialize)
{
  auto new_descr = descr.clone_unique ();
  if (!descr.path_.empty ())
    {
      // auto file = Gio::File::create_for_path (descr.path_);
      // new_descr->ghash_ = file->hash ();
    }
  descriptors_.emplace_back (std::move (new_descr));

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

void
from_json (const nlohmann::json &j, CachedPluginDescriptors &p)
{
  [[maybe_unused]] const auto major_ver =
    j.at (utils::serialization::kDocumentTypeKey).get<int> ();
  [[maybe_unused]] const auto minor_ver =
    j.at (utils::serialization::kFormatMinorKey).get<int> ();
  j.at (CachedPluginDescriptors::kDescriptorsKey).get_to (p.descriptors_);
  j.at (CachedPluginDescriptors::kBlacklistedSha1sKey)
    .get_to (p.blacklisted_sha1s_);
}

} // namespace zrythm::gui::old_dsp::plugin