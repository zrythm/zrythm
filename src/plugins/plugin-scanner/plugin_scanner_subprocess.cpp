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

#include "plugins/lv2_plugin_format.h"
#include "plugins/lv2_world.h"
#include "plugins/plugin_scan_format.h"
#include "plugins/plugin_scan_protocol.h"

#include "plugin_scanner_subprocess.h"
#include <fmt/format.h>

namespace zrythm::plugins::scanner
{

PluginScannerSubprocess::PluginScannerSubprocess ()
    : file_logger_ (
        std::make_unique<juce::FileLogger> (
          juce::File::getSpecialLocation (
            juce::File::SpecialLocationType::tempDirectory)
            .getChildFile ("plugin_scanner_subprocess_log.txt"),
          "PluginScannerSubprocess"))
{
  juce::Logger::setCurrentLogger (file_logger_.get ());
}

void
PluginScannerSubprocess::handleMessageFromCoordinator (
  const juce::MemoryBlock &mb)
{
  if (mb.isEmpty ())
    return;

  juce::Logger::writeToLog ("Message received");

  const std::lock_guard<std::mutex> lock (mutex);

  if (const auto results = do_scan (mb))
    {
      send_results (*results);
    }
  else
    {
      pending_blocks_.emplace (mb);
      triggerAsyncUpdate ();
    }
}

void
PluginScannerSubprocess::handleConnectionLost ()
{
  juce::Logger::writeToLog ("Connection lost - exiting");
  juce::JUCEApplicationBase::quit ();
}

void
PluginScannerSubprocess::handleAsyncUpdate ()
{
  for (;;)
    {
      const std::lock_guard<std::mutex> lock (mutex);

      if (pending_blocks_.empty ())
        return;

      // this runs on the message thread, so the scan always produces a
      // definitive result here - which must be sent even when empty
      if (const auto results = do_scan (pending_blocks_.front ()))
        {
          send_results (*results);
        }

      pending_blocks_.pop ();
    }
}

std::optional<discovery::PluginScanResult>
PluginScannerSubprocess::do_scan (const juce::MemoryBlock &block)
{
  juce::MemoryInputStream stream{ block, false };
  const auto              format_name =
    utils::Utf8String::from_juce_string (stream.readString ());
  const auto identifier =
    utils::Utf8String::from_juce_string (stream.readString ());

  juce::Logger::writeToLog (
    "Scanning " + format_name.to_juce_string ()
    + " identifier: " + identifier.to_juce_string ());

  auto * const matching_format = format_manager_->find_format (format_name);
  if (matching_format == nullptr)
    {
      // An unregistered format is an environment condition that applies to
      // every identifier of the format: the scan answers with no plugins and
      // the identifier is not recorded as failed
      juce::Logger::writeToLog (
        "No format registered for '" + format_name.to_juce_string ()
        + "': reporting no plugins");
      return discovery::PluginScanResult{
        std::vector<std::unique_ptr<plugins::PluginDescriptor>>{}
      };
    }

  if (
    !juce::MessageManager::getInstance ()->isThisTheMessageThread ()
    && matching_format->scan_uses_message_thread (identifier))
    return std::nullopt;

  juce::Logger::writeToLog (
    "Scanning identifier: " + identifier.to_juce_string ());
  auto result = matching_format->scan_identifier (identifier);

  juce::Logger::writeToLog (
    result.has_value ()
      ? "Found " + juce::String (result->size ())
          + " plugins in identifier: " + identifier.to_juce_string ()
      : "Scan failed for identifier: " + identifier.to_juce_string ());
  return result;
}

void
PluginScannerSubprocess::send_results (
  const discovery::PluginScanResult &results)
{
  const auto payload = discovery::serialize_plugin_scan_response (results);

  juce::Logger::writeToLog ("Sending scan response to coordinator...");

  const auto juce_payload = payload.to_juce_string ();
  sendMessageToCoordinator (
    { juce_payload.toRawUTF8 (), juce_payload.getNumBytesAsUTF8 () });
}

void
PluginScannerSubprocess::initialise (const juce::String &commandLineParameters)
{
  // formats must be initialized before starting to receive messages
  juce::Logger::writeToLog ("Creating scan formats");
  try
    {
      format_manager_ = std::make_unique<plugins::PluginScanFormatManager> (
        std::make_shared<plugins::Lv2World> (
          plugins::Lv2PluginFormat::get_spec_bundles_dir ()));
    }
  catch (const std::exception &e)
    {
      juce::Logger::writeToLog (
        "LV2 format unavailable: " + juce::String (e.what ()));
      format_manager_ =
        std::make_unique<plugins::PluginScanFormatManager> (nullptr);
    }
  for (const auto * format : format_manager_->formats ())
    {
      juce::Logger::writeToLog (
        "Found format: " + format->format_name ().to_juce_string ());
    }

  // start timer to end the subprocess if it takes too long
  start_time_ = std::chrono::steady_clock::now ();
  startTimer (60 * 1000); // 1 minute

  if (
    initialiseFromCommandLine (commandLineParameters, ZRYTHM_PLUGIN_SCANNER_UUID))
    {
      juce::Logger::writeToLog ("Initialized");
    }
  else
    {
      const auto cmd_array = getCommandLineParameterArray ();
      if (cmd_array.size () >= 2)
        {
          const auto &format_name = cmd_array[0];
          const auto &identifier = cmd_array[1];
          juce::Logger::writeToLog (
            "Attempting to scan: " + format_name + " | " + identifier);
          juce::MemoryBlock        block;
          juce::MemoryOutputStream stream{ block, true };
          stream.writeString (format_name);
          stream.writeString (identifier);
          handleMessageFromCoordinator (block);
          juce::JUCEApplicationBase::quit ();
        }
      else
        {
          juce::Logger::writeToLog (
            "Failed to initialize (cmd line: " + commandLineParameters + ")");
          juce::JUCEApplicationBase::quit ();
        }
    }
}

bool
PluginScannerSubprocess::has_timed_out () const
{
  constexpr auto TIMEOUT_DURATION = std::chrono::minutes (1);
  auto           current_time = std::chrono::steady_clock::now ();
  return (current_time - start_time_) > TIMEOUT_DURATION;
}

void
PluginScannerSubprocess::timerCallback ()
{
  if (has_timed_out ())
    {
      juce::Logger::writeToLog ("Subprocess timed out - exiting");
      juce::JUCEApplicationBase::quit ();
    }
}

void
PluginScannerSubprocess::shutdown ()
{
  juce::Logger::writeToLog ("Shutting down");
}

void
PluginScannerSubprocess::anotherInstanceStarted (const juce::String &commandLine)
{
  juce::Logger::writeToLog ("Another instance started");
}

void
PluginScannerSubprocess::systemRequestedQuit ()
{
  juce::Logger::writeToLog ("System requested quit");
}

void
PluginScannerSubprocess::suspended ()
{
  juce::Logger::writeToLog ("Suspended");
}

void
PluginScannerSubprocess::resumed ()
{
  juce::Logger::writeToLog ("Resumed");
}

PluginScannerSubprocess::~PluginScannerSubprocess ()
{
  juce::Logger::setCurrentLogger (nullptr);
}
}

JUCE_BEGIN_IGNORE_WARNINGS_GCC_LIKE ("-Wcast-qual")
START_JUCE_APPLICATION (zrythm::plugins::scanner::PluginScannerSubprocess)
JUCE_END_IGNORE_WARNINGS_GCC_LIKE
