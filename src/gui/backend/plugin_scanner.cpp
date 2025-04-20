// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
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

#include "gui/backend/plugin_protocol_paths.h"
#include "gui/backend/plugin_scanner.h"

using namespace zrythm::gui::old_dsp::plugins;
using namespace zrythm::gui::old_dsp::plugins::scanner_private;
using namespace Qt::StringLiterals;

constexpr auto PLUGIN_SCAN_TIMEOUT = std::chrono::seconds (4);

//==============================================================================

OutOfProcessPluginScanner::SubprocessCoordinator::SubprocessCoordinator ()
{
  const auto path_from_env = QProcessEnvironment::systemEnvironment ().value (
    u"ZRYTHM_PLUGIN_SCANNER_PATH"_s);
  juce::File path;
  if (!path_from_env.isEmpty ())
    {
      path = juce::File (path_from_env.toStdString ());
    }
  else
    {
      path = juce::File (
        (fs::path (qApp->applicationDirPath ().toStdString ()) / "plugin-scanner")
          .string ());
    }
  z_trace ("Scanning plugins with {}", path.getFullPathName ());
  if (!launchWorkerProcess (path, ZRYTHM_PLUGIN_SCANNER_UUID, 0, 0))
    {
      z_warning ("Failed to launch plugin scanner worker process");
      throw ZrythmException ("Failed to launch plugin scanner");
    }

  // z_debug ("Plugin scanner worker process launched");
}
OutOfProcessPluginScanner::SubprocessCoordinator::~SubprocessCoordinator () =
  default;

OutOfProcessPluginScanner::SubprocessCoordinator::Response
OutOfProcessPluginScanner::SubprocessCoordinator::getResponse ()
{
  std::unique_lock<std::mutex> lock{ mutex };

  // z_debug ("Waiting for response...");

  if (!condvar.wait_for (lock, std::chrono::milliseconds{ 50 }, [&] {
        return gotResult || connectionLost;
      }))
    return { State::Timeout, nullptr };

  const auto state = connectionLost ? State::ConnectionLost : State::GotResult;
  connectionLost = false;
  gotResult = false;

  return { state, std::move (pluginDescription) };
}

void
OutOfProcessPluginScanner::SubprocessCoordinator::handleMessageFromWorker (
  const juce::MemoryBlock &mb)
{
  const std::lock_guard<std::mutex> lock{ mutex };
  z_trace ("Received message from plugin scanner worker:\n{}", mb.toString ());
  pluginDescription = parseXML (mb.toString ());
  gotResult = true;
  condvar.notify_one ();
}

void
OutOfProcessPluginScanner::SubprocessCoordinator::handleConnectionLost ()
{
  const std::lock_guard<std::mutex> lock{ mutex };
  z_info ("Connection lost with worker");
  connectionLost = true;
  condvar.notify_one ();
}

//==============================================================================

OutOfProcessPluginScanner::OutOfProcessPluginScanner (QObject * parent)
    : QObject (parent)
{
}
OutOfProcessPluginScanner::~OutOfProcessPluginScanner () = default;

bool
OutOfProcessPluginScanner::findPluginTypesFor (
  juce::AudioPluginFormat                   &format,
  juce::OwnedArray<juce::PluginDescription> &result,
  const juce::String                        &fileOrIdentifier)
{
  z_debug ("Scanning {}", fileOrIdentifier);

  if (add_plugin_descriptions (format, fileOrIdentifier, result))
    return true;

  coordinator_ = nullptr;
  return false;
}

bool
OutOfProcessPluginScanner::add_plugin_descriptions (
  juce::AudioPluginFormat                   &format,
  const juce::String                        &file_or_identifier,
  juce::OwnedArray<juce::PluginDescription> &result)
{
  if (coordinator_ == nullptr)
    coordinator_ = std::make_unique<SubprocessCoordinator> ();

  juce::MemoryBlock        block;
  juce::MemoryOutputStream stream{ block, true };
  stream.writeString (format.getName ());
  stream.writeString (file_or_identifier);

  z_trace (
    "Sending scan request for {} {}", format.getName (), file_or_identifier);

  if (!coordinator_->sendMessageToWorker (block))
    return false;

  const auto start_time = std::chrono::steady_clock::now ();

  for (;;)
    {
      if (shouldExit ())
        return true;

      const auto response = coordinator_->getResponse ();

      if (response.state == SubprocessCoordinator::State::Timeout)
        {
          // Check if the total time elapsed exceeds the timeout
          auto current_time = std::chrono::steady_clock::now ();
          if (current_time - start_time > PLUGIN_SCAN_TIMEOUT)
            {
              z_warning (
                "Scan for '{}' timed out after {} seconds", file_or_identifier,
                PLUGIN_SCAN_TIMEOUT.count ());
              return false;
            }

          // plugin is still being scanned - try again
          continue;
        }

      if (response.xml != nullptr)
        {
          for (const auto * item : response.xml->getChildIterator ())
            {
              auto desc = std::make_unique<juce::PluginDescription> ();

              if (desc->loadFromXml (*item))
                {
                  z_debug ("Adding plugin: {}", desc->name);
                  result.add (std::move (desc));
                  // Q_EMIT pluginsAdded ();
                }
            }
        }

      return (response.state == SubprocessCoordinator::State::GotResult);
    }
}

//==============================================================================

Worker::Worker (PluginScanner &scanner) : scanner_ (scanner) { }

void
Worker::process ()
{
  z_info ("Scanning for plugins...");

  scanner_.known_plugin_list_->setCustomScanner (
    std::make_unique<OutOfProcessPluginScanner> ());

  // Initialize the format manager
  juce::AudioPluginFormatManager formatManager;
  formatManager.addDefaultFormats ();
#if ZRYTHM_WITH_JUCE_CLAP_HOSTING
  formatManager.addFormat (new juce::CLAPPluginFormat ());
#endif

  // Iterate through available formats
  for (auto * format : formatManager.getFormats ())
    {
      z_debug ("Scanning plugins for format {}", format->getName ());
      const auto protocol = Protocol::from_juce_format_name (format->getName ());
      const auto paths = PluginProtocolPaths::get_for_protocol (protocol);
      // auto defaultLocations = format->getDefaultLocationsToSearch ();
      auto identifiers = format->searchPathsForPlugins (
        paths->get_as_juce_file_search_path (), true, true);
      for (const auto &identifier : identifiers)
        {
          scanner_.set_currently_scanning_plugin (
            QString::fromStdString (identifier.toStdString ()));
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

PluginScanner::PluginScanner (
  std::shared_ptr<juce::KnownPluginList> known_plugins,
  QObject *                              parent)
    : QObject (parent), known_plugin_list_ (known_plugins),
      currently_scanning_plugin_ (tr ("Scanning..."))
{
}

void
PluginScanner::beginScan ()
{
  Q_EMIT scanningFinished ();
  return;
  auto * scan_thread = new QThread ();
  auto * worker = new Worker (*this);
  worker->moveToThread (scan_thread);
  QObject::connect (scan_thread, &QThread::started, worker, &Worker::process);
  QObject::connect (worker, &Worker::finished, scan_thread, &QThread::quit);
  QObject::connect (worker, &Worker::finished, worker, &Worker::deleteLater);
  QObject::connect (
    scan_thread, &QThread::finished, scan_thread, &QThread::deleteLater);
  QObject::connect (
    worker, &Worker::finished, this, &PluginScanner::scan_finished);
  scan_thread->start ();
}

void
PluginScanner::scan_finished ()
{
  auto xml = known_plugin_list_->createXml ();
  auto str = xml->toString ();
  z_debug ("Plugins:\n{}", str);
  Q_EMIT scanningFinished ();
}

QString
PluginScanner::getCurrentlyScanningPlugin () const
{
  QMutexLocker locker (&currently_scanning_plugin_mutex_);
  return currently_scanning_plugin_;
}

void
PluginScanner::set_currently_scanning_plugin (const QString &plugin)
{
  QMutexLocker locker (&currently_scanning_plugin_mutex_);
  if (currently_scanning_plugin_ == plugin)
    return;

  currently_scanning_plugin_ = plugin;

  Q_EMIT currentlyScanningPluginChanged (currently_scanning_plugin_);
}

PluginScanner::~PluginScanner () = default;
