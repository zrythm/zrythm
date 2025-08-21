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

#pragma once

#include "zrythm-config.h"

#include "juce_wrapper.h"

namespace zrythm::plugins::scanner
{

/**
 * @brief Plugin scanner subprocess (JUCE application).
 */
class PluginScannerSubprocess final
    : private juce::ChildProcessWorker,
      private juce::AsyncUpdater,
      private juce::Timer,
      public juce::JUCEApplication
{
public:
  PluginScannerSubprocess ();
  ~PluginScannerSubprocess () override;
  JUCE_DECLARE_NON_COPYABLE (PluginScannerSubprocess)
  JUCE_DECLARE_NON_MOVEABLE (PluginScannerSubprocess)

private:
  void handleMessageFromCoordinator (const juce::MemoryBlock &mb) override;
  void handleConnectionLost () override;
  void handleAsyncUpdate () override;
  const juce::String getApplicationName () override
  {
    return "Zrythm Plugin Scanner";
  }
  const juce::String getApplicationVersion () override { return "v1"; }
  bool               moreThanOneInstanceAllowed () override { return true; }
  void initialise (const juce::String &commandLineParameters) override;
  void shutdown () override;
  void anotherInstanceStarted (const juce::String &commandLine) override;
  void systemRequestedQuit () override;
  void suspended () override;
  void resumed () override;

  juce::OwnedArray<juce::PluginDescription>
       do_scan (const juce::MemoryBlock &block);
  void send_results (const juce::OwnedArray<juce::PluginDescription> &results);

  bool has_timed_out () const;
  void timerCallback () override;

private:
  std::mutex mutex;

  /**
   * @brief Format name & identifier stream.
   */
  std::queue<juce::MemoryBlock>     pending_blocks_;
  juce::AudioPluginFormatManager    format_manager_;
  std::unique_ptr<juce::FileLogger> file_logger_;

private:
  std::chrono::steady_clock::time_point start_time_;
};

} // namespace zrythm::plugins::scanner
