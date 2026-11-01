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

#include "zrythm-config.h"

#include <chrono>
#include <string_view>

#include "plugins/out_of_process_scanner.h"
#include "utils/exceptions.h"
#include "utils/format_juce.h"
#include "utils/logger.h"

#include <QCoreApplication>
#include <QProcessEnvironment>

using namespace Qt::StringLiterals;
using namespace std::chrono_literals;
using namespace std::literals;
using zrythm::utils::ZrythmException;

constexpr auto PLUGIN_SCAN_TIMEOUT = 6s;

using namespace zrythm::plugins::discovery;

OutOfProcessPluginScanner::SubprocessCoordinator::SubprocessCoordinator ()
{
  const auto path_from_env = QProcessEnvironment::systemEnvironment ().value (
    u"ZRYTHM_PLUGIN_SCANNER_PATH"_s);
  juce::File path;
  if (!path_from_env.isEmpty ())
    {
      path = utils::Utf8String::from_qstring (path_from_env).to_juce_file ();
    }
  else
    {
      path =
        utils::Utf8String::from_path (
          utils::Utf8String::from_qstring (qApp->applicationDirPath ()).to_path ()
          / u8"plugin-scanner")
          .to_juce_file ();
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

  if (!condvar.wait_for (lock, std::chrono::milliseconds{ 50 }, [&] {
        return gotResult || connectionLost;
      }))
    return { .state = State::Timeout, .payload = {} };

  const auto state = connectionLost ? State::ConnectionLost : State::GotResult;
  connectionLost = false;
  gotResult = false;

  return { .state = state, .payload = std::move (payload) };
}

void
OutOfProcessPluginScanner::SubprocessCoordinator::handleMessageFromWorker (
  const juce::MemoryBlock &mb)
{
  const std::lock_guard<std::mutex> lock{ mutex };
  payload = utils::Utf8String::from_juce_string (mb.toString ());
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

PluginScanResult
OutOfProcessPluginScanner::scan (
  const utils::Utf8String      &format_name,
  const utils::Utf8String      &identifier,
  const std::function<bool ()> &cancelled)
{
  z_debug ("Scanning {} {}", format_name, identifier);

  auto result = add_plugin_descriptions (format_name, identifier, cancelled);
  if (!result.has_value ())
    coordinator_ = nullptr;
  return result;
}

PluginScanResult
OutOfProcessPluginScanner::add_plugin_descriptions (
  const utils::Utf8String      &format_name,
  const utils::Utf8String      &identifier,
  const std::function<bool ()> &cancelled)
{
  if (cancelled ())
    return std::vector<std::unique_ptr<PluginDescriptor>>{};

  if (coordinator_ == nullptr)
    {
      // A scanner that cannot start is an environment failure that affects
      // every identifier, so the failure propagates and aborts the whole scan
      try
        {
          coordinator_ = std::make_unique<SubprocessCoordinator> ();
        }
      catch (const std::exception &e)
        {
          throw ZrythmException (
            fmt::format ("Failed to launch plugin scanner: {}", e.what ()));
        }
    }

  juce::MemoryBlock        block;
  juce::MemoryOutputStream stream{ block, true };
  stream.writeString (format_name.to_juce_string ());
  stream.writeString (identifier.to_juce_string ());

  z_trace ("Sending scan request for {} {}", format_name, identifier);

  if (!coordinator_->sendMessageToWorker (block))
    return std::unexpected (
      utils::Utf8String::from_utf8_encoded_string (
        "Failed to send plugin scan request"));

  const auto start_time = std::chrono::steady_clock::now ();

  for (;;)
    {
      const auto response = coordinator_->getResponse ();

      if (response.state == SubprocessCoordinator::State::Timeout)
        {
          if (cancelled ())
            {
              // The abandoned subprocess is replaced by a fresh one on the
              // next scan
              coordinator_ = nullptr;
              return std::vector<std::unique_ptr<PluginDescriptor>>{};
            }
          const auto current_time = std::chrono::steady_clock::now ();
          if (current_time - start_time > PLUGIN_SCAN_TIMEOUT)
            {
              z_warning (
                "Scan for '{}' timed out after {} seconds", identifier,
                PLUGIN_SCAN_TIMEOUT.count ());
              return std::unexpected (
                utils::Utf8String::from_utf8_encoded_string (
                  fmt::format (
                    "Plugin scan timed out after {} seconds",
                    PLUGIN_SCAN_TIMEOUT.count ())));
            }

          continue;
        }

      if (response.state == SubprocessCoordinator::State::ConnectionLost)
        return std::unexpected (
          utils::Utf8String::from_utf8_encoded_string (
            "Plugin scanner connection lost"));

      return deserialize_plugin_scan_response (response.payload.view ());
    }
}
