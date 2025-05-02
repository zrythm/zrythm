// SPDX-FileCopyrightText: © 2018-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
/*
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 * ---
 *
 * Copyright (C) 2008-2012 Paul Davis
 * Copyright (C) David Robillard
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * ---
 */

#include "zrythm-config.h"

#include "gui/backend/backend/settings_manager.h"
#include "gui/backend/backend/zrythm.h"
#include "gui/backend/cached_plugin_descriptors.h"
#include "gui/backend/carla_discovery.h"
#include "gui/backend/plugin_manager.h"
#include "utils/directory_manager.h"
#include "utils/gtest_wrapper.h"

#include <QtConcurrent>

using namespace zrythm::gui::old_dsp::plugins;

PluginManager::PluginManager (QObject * parent)
    : QObject (parent),
      plugin_descriptors_ (std::make_unique<gui::PluginDescriptorList> ()),
      known_plugin_list_ (std::make_shared<juce::KnownPluginList> ()),
      // cached_plugin_descriptors_ (CachedPluginDescriptors::read_or_new ()),
      collections_ (PluginCollections::read_or_new ()),
      scanner_ (std::make_unique<PluginScanner> (known_plugin_list_))
#if HAVE_CARLA
      ,
      carla_discovery_ (std::make_unique<ZCarlaDiscovery> (*this))
#endif
{
}

PluginManager *
PluginManager::get_active_instance ()
{
  return Zrythm::getInstance ()->getPluginManager ();
}

void
PluginManager::add_category_and_author (
  std::string_view category,
  std::string_view author)
{
  if (!utils::string::is_ascii (category))
    {
      z_warning ("Ignoring non-ASCII plugin category name...");
    }
  if (
    !std::any_of (
      plugin_categories_.begin (), plugin_categories_.end (),
      [&] (const auto &cat) { return cat == category; }))
    {
      z_debug ("New category: {}", category);
      plugin_categories_.emplace_back (category);
    }

  if (!author.empty ())
    {
      if (
        !std::any_of (
          plugin_authors_.begin (), plugin_authors_.end (),
          [&] (const auto &cur_author) { return cur_author == author; }))
        {
          z_debug ("New author: {}", author);
          plugin_authors_.emplace_back (author);
        }
    }
}

bool
PluginManager::supports_protocol (Protocol::ProtocolType protocol)
{
  // List of protocols that are always supported
  const auto always_supported = {
    Protocol::ProtocolType::Internal, Protocol::ProtocolType::LV2,
    Protocol::ProtocolType::LADSPA,   Protocol::ProtocolType::VST,
    Protocol::ProtocolType::VST3,     Protocol::ProtocolType::SFZ,
    Protocol::ProtocolType::JSFX,     Protocol::ProtocolType::CLAP
  };

  // Check if the protocol is in the always supported list
  if (
    std::ranges::find (always_supported, protocol)
    != std::ranges::end (always_supported))
    {
#if HAVE_CARLA
      return true;
#else
      return false;
#endif
    }

#if HAVE_CARLA
  // Get the list of features supported by Carla
  const StringArray carla_supported_features (carla_get_supported_features ());

  // Map of protocols to their corresponding Carla feature names
  const auto feature_map = std::map<PluginProtocol, std::string>{
    { ProtocolType::SF2,  "sf2"  },
    { ProtocolType::DSSI, "osc"  },
    { ProtocolType::VST3, "vst3" },
    { ProtocolType::AU,   "au"   }
  };

  // Check if the protocol is in the feature map
  if (auto it = feature_map.find (protocol); it != feature_map.end ())
    {
      // Check if the corresponding feature is supported by Carla
      return std::ranges::any_of (
        carla_supported_features, [&] (const auto &feature) {
          return feature.toStdString () == it->second;
        });
    }
#endif

  // If not found in any of the above cases, return false
  return false;
}

void
PluginManager::add_descriptor (
  const zrythm::gui::old_dsp::plugins::PluginDescriptor &descr)
{
  z_return_if_fail (descr.protocol_ > Protocol::ProtocolType::Internal);
  plugin_descriptors_->addDescriptor (descr);
  add_category_and_author (descr.category_str_, descr.author_);
}

fs::path
PluginManager::get_known_plugins_xml_path ()
{
  QString local_app_data_path =
    QStandardPaths::writableLocation (QStandardPaths::AppLocalDataLocation);
  QDir dir (local_app_data_path);
  return utils::io::to_fs_path (dir.absoluteFilePath (u"known_plugins.xml"_s));
}

void
PluginManager::serialize_known_plugins ()
{
  const auto known_plugins_xml_path = get_known_plugins_xml_path ();
  z_return_if_fail (known_plugin_list_);
  if (known_plugin_list_->createXml ()->writeTo (
        juce::File (known_plugins_xml_path.string ())))
    {
      z_debug ("Saved known plugins to {}", known_plugins_xml_path.string ());
    }
  else
    {
      z_warning (
        "Failed to save known plugins to {}", known_plugins_xml_path.string ());
    }
}

void
PluginManager::deserialize_known_plugins ()
{
  const auto known_plugins_xml_path = get_known_plugins_xml_path ();
  known_plugin_list_->clear ();
  const juce::File jfile (known_plugins_xml_path.string ());
  if (jfile.existsAsFile ())
    {
      z_debug (
        "Loading known plugins from {}", known_plugins_xml_path.string ());
      const auto xml_doc = juce::XmlDocument::parse (jfile);
      if (xml_doc)
        {
          known_plugin_list_->recreateFromXml (*xml_doc);
        }
      else
        {
          z_warning (
            "Failed to load known plugins from {}",
            known_plugins_xml_path.string ());
        }
    }
  else
    {
      z_info (
        "No known plugins file found at {}", known_plugins_xml_path.string ());
    }
}

void
PluginManager::onScanFinished ()
{
  // serialize
  serialize_known_plugins ();

  // relay the signal
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
    scanner_.get (), &PluginScanner::currentlyScanningPluginChanged, this,
    &PluginManager::currentlyScanningPluginChanged);

  // get notified by the scanner when it has finished scanning
  QObject::connect (
    scanner_.get (), &PluginScanner::scanningFinished, this,
    &PluginManager::onScanFinished);

  deserialize_known_plugins ();

  scanner_->beginScan ();

#if 0
  for (
    size_t i = ENUM_VALUE_TO_INT (ProtocolType::LV2);
    i <= ENUM_VALUE_TO_INT (ProtocolType::JSFX); i++)
    {
      PluginProtocol cur = ENUM_INT_TO_VALUE (PluginProtocol, i);
      if (!supports_protocol (cur))
        continue;

      carla_discovery_->start (CarlaBackend::BINARY_NATIVE, cur);
      /* also scan 32-bit on windows */
#  ifdef _WIN32
      carla_discovery_->start (CarlaBackend::BINARY_WIN32, cur);
#  endif
    }

  // Connect to the event loop using QTimer
  auto * timer = new QTimer (this);
  connect (
    timer, &QTimer::timeout, this, &PluginManager::call_carla_discovery_idle);
  timer->start (
    0); // 0 ms interval means it will be called on every event loop iteration
#endif
}

std::unique_ptr<PluginDescriptor>
PluginManager::find_plugin_from_uri (std::string_view uri) const
{
// TODO
#if 0
  auto it = std::find_if (
    plugin_descriptors_.begin (), plugin_descriptors_.end (),
    [&uri] (const zrythm::gui::old_dsp::plugins::PluginDescriptor &descr) {
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

std::unique_ptr<PluginDescriptor>
PluginManager::find_from_descriptor (
  const zrythm::gui::old_dsp::plugins::PluginDescriptor &src_descr) const
{
// TODO
#if 0
  auto it = std::find_if (
    plugin_descriptors_.begin (), plugin_descriptors_.end (),
    [&src_descr] (const zrythm::gui::old_dsp::plugins::PluginDescriptor &descr) {
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

std::unique_ptr<PluginDescriptor>
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
  plugin_descriptors_->clear ();
  plugin_categories_.clear ();
  plugin_authors_.clear ();
}
