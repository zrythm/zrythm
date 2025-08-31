// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
/*
 * This file incorporates work covered by the following copyright and
 * permission notice:
 * ---
 *
 * This file is part of the JUCE framework.
 * Copyright (c) Raw Material Software Limited
 *
 * JUCE is an open source framework subject to commercial or open source
 * licensing.
 *
 * By downloading, installing, or using the JUCE framework, or combining the
 * JUCE framework with any other source code, object code, content or any other
 * copyrightable work, you agree to the terms of the JUCE End User Licence
 * Agreement, and all incorporated terms including the JUCE Privacy Policy and
 * the JUCE Website Terms of Service, as applicable, which will bind you. If you
 * do not agree to the terms of these agreements, we will not license the JUCE
 * framework to you, and you must discontinue the installation or download
 * process and cease use of the JUCE framework.
 *
 * JUCE End User Licence Agreement: https://juce.com/legal/juce-8-licence/
 * JUCE Privacy Policy: https://juce.com/juce-privacy-policy
 * JUCE Website Terms of Service: https://juce.com/juce-website-terms-of-service/
 *
 * Or:
 *
 * You may also use this code under the terms of the AGPLv3:
 * https://www.gnu.org/licenses/agpl-3.0.en.html
 *
 * THE JUCE FRAMEWORK IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL
 * WARRANTIES, WHETHER EXPRESSED OR IMPLIED, INCLUDING WARRANTY OF
 * MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE, ARE DISCLAIMED.
 * ---
 * SPDX-FileCopyrightText: (c) Raw Material Software Limited
 * SPDX-License-Identifier: AGPL-3.0-only
 */

#include <utility>

#include "plugins/plugin_scan_manager.h"

using namespace zrythm::plugins;
using namespace zrythm::plugins::scanner_private;

//==============================================================================

//==============================================================================

Worker::Worker (PluginScanManager &scanner) : scanner_ (scanner) { }

void
Worker::process ()
{
  z_info ("Scanning for plugins...");

  // Iterate through available formats
  for (auto * format : scanner_.format_manager_->getFormats ())
    {
      z_debug ("Scanning plugins for format {}", format->getName ());
      const auto protocol = Protocol::from_juce_format_name (format->getName ());
      const auto paths = scanner_.plugin_paths_provider_ (protocol);
      // auto defaultLocations = format->getDefaultLocationsToSearch ();
      auto identifiers = format->searchPathsForPlugins (
        paths->get_as_juce_file_search_path (), true, true);
      for (const auto &identifier : identifiers)
        {
          scanner_.set_currently_scanning_plugin (
            utils::Utf8String::from_juce_string (identifier).to_qstring ());
          juce::OwnedArray<juce::PluginDescription> types;
          bool has_new = scanner_.known_plugin_list_->scanAndAddFile (
            identifier, true, types, *format);
          if (has_new)
            {
              z_info (
                "Found new plugins for identifier '{}' (total types {})",
                identifier, types.size ());
            }
          if (types.isEmpty ())
            {
              z_warning ("Blacklisting plugin: {}", identifier);
              scanner_.known_plugin_list_->addToBlacklist (identifier);
            }
        }
    }
  z_debug ("Scanning in thread finished");
  Q_EMIT finished ();
}

//==============================================================================

PluginScanManager::PluginScanManager (
  std::shared_ptr<juce::KnownPluginList>          known_plugins,
  std::shared_ptr<juce::AudioPluginFormatManager> format_manager,
  ProtocolPluginPathsProvider                     plugin_paths_provider,
  QObject *                                       parent)
    : QObject (parent), known_plugin_list_ (std::move (known_plugins)),
      plugin_paths_provider_ (std::move (plugin_paths_provider)),
      format_manager_ (std::move (format_manager)),
      currently_scanning_plugin_ (tr ("Scanning..."))
{
}

void
PluginScanManager::beginScan ()
{
  scan_thread_ = utils::make_qobject_unique<QThread> ();
  worker_ = utils::make_qobject_unique<Worker> (*this);
  worker_->moveToThread (scan_thread_.get ());
  QObject::connect (
    scan_thread_.get (), &QThread::started, worker_.get (), &Worker::process);
  QObject::connect (
    worker_.get (), &Worker::finished, scan_thread_.get (), &QThread::quit);
  QObject::connect (scan_thread_.get (), &QThread::finished, this, [this] {
    // Delete worker in main thread after thread finishes
    worker_->deleteLater ();
    scan_thread_->deleteLater ();
  });
  QObject::connect (
    worker_.get (), &Worker::finished, this, &PluginScanManager::scan_finished);
  scan_thread_->start ();
}

void
PluginScanManager::scan_finished ()
{
  auto xml = known_plugin_list_->createXml ();
  auto str = xml->toString ();
  z_debug ("Plugins:\n{}", str);
  Q_EMIT scanningFinished ();
}

QString
PluginScanManager::getCurrentlyScanningPlugin () const
{
  QMutexLocker locker (&currently_scanning_plugin_mutex_);
  return currently_scanning_plugin_;
}

void
PluginScanManager::set_currently_scanning_plugin (const QString &plugin)
{
  QMutexLocker locker (&currently_scanning_plugin_mutex_);
  if (currently_scanning_plugin_ == plugin)
    return;

  currently_scanning_plugin_ = plugin;

  Q_EMIT currentlyScanningPluginChanged (currently_scanning_plugin_);
}

PluginScanManager::~PluginScanManager ()
{
  if (scan_thread_)
    {
      scan_thread_->wait ();
    }
}
