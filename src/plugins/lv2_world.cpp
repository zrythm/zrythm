// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <filesystem>
#include <string>
#include <string_view>

#include "plugins/lv2_world.h"
#include "utils/exceptions.h"
#include "utils/logger.h"

#include <fmt/format.h>

namespace zrythm::plugins
{

Lv2World::Lv2World (const std::filesystem::path &spec_bundles_dir)
    : world_ (lilv_world_new ())
{
  if (world_ == nullptr)
    throw ZrythmException ("Failed to create an LV2 world");

  // Load the LV2 specification bundles, so that the plugin class hierarchy
  // and extension vocabularies resolve (without them, every plugin class
  // reads as the base "Plugin" class)
  std::error_code ec;
  int             loaded_bundles = 0;
  for (
    const auto &entry :
    std::filesystem::directory_iterator{ spec_bundles_dir, ec })
    {
      if (!entry.is_directory () || entry.path ().extension () != ".lv2")
        continue;

      const LilvNodeUPtr bundle_uri{ lilv_new_file_uri (
        world_.get (), nullptr, (entry.path () / "").string ().c_str ()) };
      if (bundle_uri == nullptr)
        continue;

      lilv_world_load_bundle (world_.get (), bundle_uri.get ());
      ++loaded_bundles;
    }
  if (ec)
    throw ZrythmException (
      fmt::format (
        "Failed to read the LV2 specification bundles from '{}': {}",
        spec_bundles_dir, ec.message ()));
  if (loaded_bundles == 0)
    throw ZrythmException (
      fmt::format (
        "No LV2 specification bundles found in '{}'", spec_bundles_dir));
  lilv_world_load_specifications (world_.get ());
  lilv_world_load_plugin_classes (world_.get ());
}

Lv2World::~Lv2World () = default;

const LilvPlugin *
Lv2World::find_plugin (
  const std::filesystem::path &bundle_dir,
  std::string_view             uri)
{
  // (bundle_dir / "") forces a trailing slash: lilv requires directories
  // to be passed with one
  const LilvNodeUPtr bundle_uri{ lilv_new_file_uri (
    world_.get (), nullptr, (bundle_dir / "").string ().c_str ()) };
  if (bundle_uri == nullptr)
    {
      z_warning ("Failed to create file URI for LV2 bundle '{}'", bundle_dir);
      return nullptr;
    }

  lilv_world_load_bundle (world_.get (), bundle_uri.get ());

  const LilvNodeUPtr uri_node{
    lilv_new_uri (world_.get (), std::string (uri).c_str ())
  };
  if (uri_node == nullptr)
    {
      return nullptr;
    }

  return lilv_plugins_get_by_uri (
    lilv_world_get_all_plugins (world_.get ()), uri_node.get ());
}

} // namespace zrythm::plugins
