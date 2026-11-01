// SPDX-FileCopyrightText: © 2018-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#include <algorithm>
#include <memory>

#include "gui/backend/plugin_manager.h"
#include "gui/backend/plugin_protocol_paths.h"
#include "plugins/faust/faust_registry.h"
#include "plugins/lv2_plugin_format.h"
#include "plugins/lv2_world.h"
#include "plugins/out_of_process_scanner.h"
#include "plugins/plugin_scan_format.h"
#include "utils/io_utils.h"
#include "utils/logger.h"

#include <QtConcurrent>

#include <nlohmann/json.hpp>

using namespace Qt::StringLiterals;

using namespace zrythm::gui::old_dsp::plugins;

bool
zrythm::gui::old_dsp::plugins::known_plugin_file_missing (
  const zrythm::plugins::PluginDescriptor &descriptor)
{
  // internal (Faust) plugins are not file-based
  if (descriptor.protocol_ == zrythm::plugins::Protocol::ProtocolType::Internal)
    return false;

  const auto path = std::get_if<std::filesystem::path> (&descriptor.path_or_id_);
  if (path == nullptr || !path->is_absolute ())
    return false;

  return !std::filesystem::exists (*path);
}

PluginManager::PluginManager (
  zrythm::plugins::ProtocolPluginPathsProvider plugin_paths_provider,
  QObject *                                    parent)
    : QObject (parent), collections_ (PluginCollections::read_or_new ())
{
  // One LV2 world per process, shared by the scan format and the plugins
  // instantiated through get_lv2_world()
  lv2_world_ = std::make_shared<zrythm::plugins::Lv2World> (
    zrythm::plugins::Lv2PluginFormat::get_spec_bundles_dir ());
  scan_formats_ =
    std::make_shared<zrythm::plugins::PluginScanFormatManager> (lv2_world_);
  catalog_ = std::make_shared<zrythm::plugins::discovery::PluginCatalog> ();
  scanner_ = std::make_unique<zrythm::plugins::PluginScanManager> (
    catalog_, scan_formats_, std::move (plugin_paths_provider),
    std::make_unique<zrythm::plugins::discovery::OutOfProcessPluginScanner> ());

  add_internal_plugins_to_catalog ();
}

void
PluginManager::add_internal_plugins_to_catalog ()
{
  for (const auto &info : zrythm::plugins::faust::available_faust_plugins ())
    {
      catalog_->add_descriptor (
        zrythm::plugins::faust::make_faust_plugin_descriptor (info));
    }
}

void
PluginManager::add_category_and_author (
  const utils::Utf8String &category,
  const utils::Utf8String &author)
{
  if (!category.is_ascii ())
    {
      z_warning ("Ignoring non-ASCII plugin category name...");
    }
  if (!std::ranges::contains (plugin_categories_, category))
    {
      z_debug ("New category: {}", category);
      plugin_categories_.emplace_back (category);
    }

  if (!author.empty ())
    {
      if (!std::ranges::contains (plugin_authors_, author))
        {
          z_debug ("New author: {}", author);
          plugin_authors_.emplace_back (author);
        }
    }
}

std::filesystem::path
PluginManager::get_known_plugins_json_path ()
{
  QString local_app_data_path =
    QStandardPaths::writableLocation (QStandardPaths::AppLocalDataLocation);
  QDir dir (local_app_data_path);
  return utils::Utf8String::from_qstring (
    dir.absoluteFilePath (u"known_plugins.json"_s));
}

void
PluginManager::serialize_known_plugins ()
{
  const auto known_plugins_path = get_known_plugins_json_path ();
  const auto known_plugins_path_str =
    utils::Utf8String::from_path (known_plugins_path);

  try
    {
      utils::io::mkdir (known_plugins_path.parent_path ());
      utils::io::set_file_contents (
        known_plugins_path,
        utils::Utf8String::from_utf8_encoded_string (
          catalog_->serialize ().dump ()));
      z_debug ("Saved known plugins to {}", known_plugins_path_str);
    }
  catch (const std::exception &e)
    {
      z_warning (
        "Failed to save known plugins to {}: {}", known_plugins_path_str,
        e.what ());
    }
}

void
PluginManager::deserialize_known_plugins ()
{
  const auto known_plugins_path = get_known_plugins_json_path ();
  const auto known_plugins_path_str =
    utils::Utf8String::from_path (known_plugins_path);

  if (!std::filesystem::exists (known_plugins_path))
    {
      z_info ("No known plugins file found at {}", known_plugins_path_str);
      add_internal_plugins_to_catalog ();
      return;
    }

  try
    {
      const auto contents = utils::io::read_file_contents (known_plugins_path);
      const auto json = nlohmann::json::parse (utils::to_std_string (contents));
      catalog_->deserialize (json);
      z_debug ("Loaded known plugins from {}", known_plugins_path_str);

      int num_pruned = 0;
      for (const auto &descriptor : catalog_->descriptors ())
        {
          if (known_plugin_file_missing (*descriptor))
            {
              catalog_->remove_descriptor (*descriptor);
              ++num_pruned;
            }
        }
      if (num_pruned > 0)
        {
          z_info ("Pruned {} known plugins with missing files", num_pruned);
        }
    }
  catch (const std::exception &e)
    {
      z_warning (
        "Failed to load known plugins from {}: {}", known_plugins_path_str,
        e.what ());
      catalog_->clear ();
    }

  add_internal_plugins_to_catalog ();
}

void
PluginManager::onScannerScanFinished ()
{
  serialize_known_plugins ();
  Q_EMIT scanFinished ();
}

void
PluginManager::beginScan ()
{
  if (qEnvironmentVariableIsSet ("ZRYTHM_SKIP_PLUGIN_SCAN"))
    {
      Q_EMIT scanFinished ();
      return;
    }

  // relay currently scanning plugin
  QObject::connect (
    scanner_.get (),
    &::zrythm::plugins::PluginScanManager::currentlyScanningPluginChanged, this,
    &PluginManager::currentlyScanningPluginChanged);

  // get notified by the scanner when it has finished scanning
  QObject::connect (
    scanner_.get (), &::zrythm::plugins::PluginScanManager::scanningFinished,
    this, &PluginManager::onScannerScanFinished);

  deserialize_known_plugins ();

  scanner_->beginScan ();
}

std::unique_ptr<zrythm::plugins::PluginDescriptor>
PluginManager::find_plugin_from_uri (const utils::Utf8String &uri) const
{
// TODO
#if 0
  auto it = std::find_if (
    plugin_descriptors_.begin (), plugin_descriptors_.end (),
    [&uri] (const zrythm::plugins::PluginDescriptor &descr) {
      return uri == descr.uri_;
    });
  if (it != plugin_descriptors_.end ())
    {
      return std::make_unique<PluginDescriptor> (*it);
    }
  else
    {
      z_debug ("descriptor for URI {} not found", uri);
      return nullptr;
    }
#endif
  return nullptr;
}

std::unique_ptr<zrythm::plugins::PluginDescriptor>
PluginManager::find_from_descriptor (
  const zrythm::plugins::PluginDescriptor &src_descr) const
{
// TODO
#if 0
  auto it = std::find_if (
    plugin_descriptors_.begin (), plugin_descriptors_.end (),
    [&src_descr] (const zrythm::plugins::PluginDescriptor &descr) {
      return src_descr.is_same_plugin (descr);
    });
  if (it != plugin_descriptors_.end ())
    {
      return std::make_unique<PluginDescriptor> (*it);
    }
  else
    {
      z_debug ("descriptor for {} not found", src_descr.name_);
      return nullptr;
    }
#endif
  return nullptr;
}

std::unique_ptr<zrythm::plugins::PluginDescriptor>
PluginManager::pick_instrument () const
{
// TODO
#if 0
  auto it = std::find_if (
    plugin_descriptors_.begin (), plugin_descriptors_.end (),
    [] (const auto &descr) { return descr.is_instrument (); });
  if (it != plugin_descriptors_.end ())
    {
      return std::make_unique<PluginDescriptor> (*it);
    }
  else
    {
      z_debug ("no instrument found");
      return nullptr;
    }
#endif
  return nullptr;
}

void
PluginManager::clear_plugins ()
{
  catalog_->clear ();
  plugin_categories_.clear ();
  plugin_authors_.clear ();

  add_internal_plugins_to_catalog ();
}
