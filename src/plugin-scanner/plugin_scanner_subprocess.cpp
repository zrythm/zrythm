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

#include "plugin_scanner_subprocess.h"

using namespace zrythm::gui::old_dsp::plugins;

PluginScannerSubprocess::PluginScannerSubprocess ()
    : file_logger_ (std::make_unique<juce::FileLogger> (
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

  if (const auto results = do_scan (mb); !results.isEmpty ())
    {
      send_results (results);
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

      if (const auto results = do_scan (pending_blocks_.front ());
          !results.isEmpty ())
        {
          send_results (results);
        }
      pending_blocks_.pop ();
    }
}

juce::OwnedArray<juce::PluginDescription>
PluginScannerSubprocess::do_scan (const juce::MemoryBlock &block)
{
  juce::MemoryInputStream stream{ block, false };
  const auto              formatName = stream.readString ();
  const auto              identifier = stream.readString ();

  juce::Logger::writeToLog (
    "Scanning " + formatName + " identifier: " + identifier);

  juce::PluginDescription pd;
  pd.fileOrIdentifier = identifier;
  pd.uniqueId = pd.deprecatedUid = 0;

  auto * const matchingFormat = [&] () -> juce::AudioPluginFormat * {
    for (auto * format : format_manager_.getFormats ())
      if (format->getName () == formatName)
        return format;
    return nullptr;
  }();

  juce::OwnedArray<juce::PluginDescription> results;

  if (matchingFormat != nullptr
        && (juce::MessageManager::getInstance()->isThisTheMessageThread()
            || matchingFormat->requiresUnblockedMessageThreadDuringCreation(pd)))
    {
      juce::Logger::writeToLog (
        "DEBUG: Attempting to find all types for identifier: " + identifier);
      matchingFormat->findAllTypesForFile (results, identifier);
    }

  juce::Logger::writeToLog (
    "Found " + juce::String (results.size ())
    + " plugins in identifier: " + identifier);

  return results;
}

void
PluginScannerSubprocess::send_results (
  const juce::OwnedArray<juce::PluginDescription> &results)
{
  juce::XmlElement xml ("LIST");

  for (const auto &desc : results)
    xml.addChildElement (desc->createXml ().release ());

  const auto str = xml.toString ();

  juce::Logger::writeToLog ("Sending XML to coordinator...");

  sendMessageToCoordinator ({ str.toRawUTF8 (), str.getNumBytesAsUTF8 () });
}

void
PluginScannerSubprocess::initialise (const juce::String &commandLineParameters)
{
  // formats must be initialized before starting to receive messages
  juce::Logger::writeToLog ("Adding default formats");
  format_manager_.addDefaultFormats ();
  format_manager_.addFormat (new juce::CLAPPluginFormat ());
  for (auto * format : format_manager_.getFormats ())
    {
      juce::Logger::writeToLog ("Found format: " + format->getName ());
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
          const auto format_name = cmd_array[0];
          const auto identifier = cmd_array[1];
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

START_JUCE_APPLICATION (PluginScannerSubprocess)
