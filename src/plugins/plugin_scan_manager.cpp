// SPDX-FileCopyrightText: © 2024-2026 Alexandros Theodotou <alex@zrythm.org>
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
#include "utils/format_juce.h"
#include "utils/logger.h"

using namespace zrythm::plugins;
using namespace zrythm::plugins::scanner_private;

//==============================================================================

Worker::Worker (
  PluginScanManager                       &scanner,
  std::shared_ptr<PluginCatalog>           catalog,
  std::shared_ptr<PluginScanFormatManager> format_manager,
  PluginScanner *                          plugin_scanner,
  std::uint64_t                            scan_generation)
    : scanner_ (scanner), catalog_ (std::move (catalog)),
      format_manager_ (std::move (format_manager)),
      plugin_scanner_ (plugin_scanner), scan_generation_ (scan_generation)
{
}

void
Worker::requestStop ()
{
  should_stop_ = true;
}

void
Worker::process ()
{
  try
    {
      process_scan ();
    }
  catch (const std::exception &e)
    {
      z_warning ("Plugin scan stopped after an error: {}", e.what ());
    }
  catch (...)
    {
      z_warning ("Plugin scan stopped after an unknown error");
    }

  z_debug ("Scanning in thread finished");
  Q_EMIT finished ();
}

void
Worker::process_scan ()
{
  z_info ("Scanning for plugins...");

  for (auto * format : format_manager_->formats ())
    {
      if (should_stop_)
        {
          z_debug ("Scanning cancelled");
          return;
        }

      z_debug ("Scanning plugins for format {}", format->format_name ());
      const auto protocol = format->protocol ();
      const auto paths = scanner_.plugin_paths_provider_ (protocol);
      // A protocol without configured search paths is not scanned. Formats
      // that ignore search paths (e.g. AudioUnit, which enumerates the
      // system component registry) are also bound by this rule.
      if (paths->empty ())
        {
          z_debug ("Skipping {}: no search paths", format->format_name ());
          continue;
        }
      const auto identifiers = format->search_identifiers (*paths, true);
      for (const auto &identifier : identifiers)
        {
          if (should_stop_)
            {
              z_debug ("Scanning cancelled");
              return;
            }

          scanner_.set_currently_scanning_plugin (identifier.to_qstring ());
          const auto scan_state = catalog_->scan_state (
            protocol, identifier, [&] (const PluginDescriptor &descriptor) {
              return format->plugin_needs_rescanning (descriptor);
            });
          if (
            scan_state == PluginCatalog::ScanState::UpToDate
            || scan_state == PluginCatalog::ScanState::Blacklisted)
            continue;

          auto result =
            plugin_scanner_->scan (format->format_name (), identifier, [this] {
              return should_stop_.load ();
            });
          if (should_stop_)
            {
              z_debug ("Scanning cancelled");
              return;
            }
          if (!result.has_value ())
            {
              z_warning (
                "Scan failed for {} {}: {}", format->format_name (), identifier,
                result.error ());
              QMetaObject::invokeMethod (
                &scanner_,
                [manager = &scanner_, generation = scan_generation_, protocol,
                 identifier] {
                  if (manager->active_generation_ != generation)
                    return;
                  manager->catalog_->blacklist_identifier (protocol, identifier);
                },
                Qt::QueuedConnection);
              continue;
            }

          auto descriptors =
            std::make_shared<std::vector<std::unique_ptr<PluginDescriptor>>> (
              std::move (*result));
          for (const auto &descriptor : *descriptors)
            {
              if (descriptor->thread () != catalog_->thread ())
                descriptor->moveToThread (catalog_->thread ());
            }

          QMetaObject::invokeMethod (
            &scanner_,
            [manager = &scanner_, generation = scan_generation_,
             catalog = catalog_, protocol, identifier,
             scan_results = std::move (descriptors)] () mutable {
              if (manager->active_generation_ != generation)
                return;
              const auto replacement = catalog->replace_results_for_scan (
                protocol, identifier, std::move (*scan_results));
              if (!replacement.has_value ())
                {
                  z_warning (
                    "Rejecting scan results for {}: {}", identifier.view (),
                    replacement.error ().view ());
                  catalog->blacklist_identifier (protocol, identifier);
                }
            },
            Qt::QueuedConnection);
        }

      if (should_stop_)
        {
          z_debug ("Scanning cancelled");
          return;
        }
    }
}

//==============================================================================

PluginScanManager::PluginScanManager (
  std::shared_ptr<PluginCatalog>           catalog,
  std::shared_ptr<PluginScanFormatManager> format_manager,
  ProtocolPluginPathsProvider              plugin_paths_provider,
  std::unique_ptr<PluginScanner>           plugin_scanner,
  QObject *                                parent)
    : QObject (parent), catalog_ (std::move (catalog)),
      plugin_paths_provider_ (std::move (plugin_paths_provider)),
      format_manager_ (std::move (format_manager)),
      plugin_scanner_ (std::move (plugin_scanner)),
      currently_scanning_plugin_ (tr ("Scanning..."))
{
}

void
PluginScanManager::requestStop ()
{
  if (worker_)
    {
      worker_->requestStop ();
    }
}

void
PluginScanManager::beginScan ()
{
  if (scan_thread_ != nullptr)
    {
      if (scan_thread_->isRunning ())
        {
          // the active scan finishes first, and the new scan starts from its
          // completion callback
          restart_pending_ = true;
          requestStop ();
          return;
        }

      scan_thread_->quit ();
      scan_thread_->wait ();
      worker_.reset ();
      scan_thread_.reset ();
    }

  start_scan ();
}

void
PluginScanManager::start_scan ()
{
  const auto scan_generation = ++active_generation_;
  scan_thread_ = utils::make_qobject_unique<QThread> ();
  worker_ = utils::make_qobject_unique<Worker> (
    *this, catalog_, format_manager_, plugin_scanner_.get (), scan_generation);
  worker_->moveToThread (scan_thread_.get ());
  QObject::connect (
    scan_thread_.get (), &QThread::started, worker_.get (), &Worker::process);
  // the worker emits finished on the scan thread, and quit() stops that
  // thread's event loop immediately
  QObject::connect (
    worker_.get (), &Worker::finished, scan_thread_.get (), &QThread::quit,
    Qt::DirectConnection);
  // queued catalog updates are posted before the thread finishes, so completion
  // is delivered after every result of the generation
  QObject::connect (
    scan_thread_.get (), &QThread::finished, this,
    [this, scan_generation] { finish_scan (scan_generation); },
    Qt::QueuedConnection);
  scan_thread_->start ();
}

void
PluginScanManager::finish_scan (std::uint64_t scan_generation)
{
  if (scan_generation != active_generation_)
    return;

  worker_.reset ();
  scan_thread_.reset ();

  if (restart_pending_)
    {
      restart_pending_ = false;
      start_scan ();
      return;
    }

  scan_finished ();
}

void
PluginScanManager::scan_finished ()
{
  z_debug (
    "Plugin scan finished ({} descriptors)", catalog_->descriptor_count ());
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
      // Disconnect all signals between the scan objects and this manager
      if (worker_)
        {
          worker_->disconnect ();
          disconnect (worker_.get ());
        }
      scan_thread_->disconnect ();
      disconnect (scan_thread_.get ());

      // Request worker to stop gracefully
      requestStop ();

      // Request thread to quit
      scan_thread_->quit ();

      // Wait for thread to finish
      scan_thread_->wait ();

      // Clean up the worker and thread objects
      worker_.reset ();
      scan_thread_.reset ();
    }
}
