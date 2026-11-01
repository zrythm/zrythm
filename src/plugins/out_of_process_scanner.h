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

#pragma once

#include "plugins/plugin_scan_protocol.h"

#include <QObject>
#include <QtQmlIntegration/qqmlintegration.h>

namespace zrythm::plugins::discovery
{

/**
 * @brief Scans plugins in a separate scanner process.
 *
 * Plugin binaries are loaded only by the scanner subprocess, so scanner crashes
 * and hangs cannot take down the application.
 */
class OutOfProcessPluginScanner final : public QObject, public PluginScanner
{
  Q_OBJECT

public:
  OutOfProcessPluginScanner (QObject * parent = nullptr);
  ~OutOfProcessPluginScanner () override;

  /**
   * @brief Scans one plugin identifier in the subprocess.
   *
   * @param format_name Name reported by PluginScanFormat::format_name().
   * @param identifier File path or format-specific plugin identifier.
   * @param cancelled Polled while waiting for the subprocess; the scan aborts
   * with no plugins once it returns true.
   *
   * @return The descriptors found (possibly none), or a scan/transport error.
   */
  [[nodiscard]] PluginScanResult scan (
    const utils::Utf8String      &format_name,
    const utils::Utf8String      &identifier,
    const std::function<bool ()> &cancelled) override;

private:
  /**
   * @brief Coordinates communication with the scanner subprocess.
   */
  class SubprocessCoordinator final : private juce::ChildProcessCoordinator
  {
  public:
    SubprocessCoordinator ();
    ~SubprocessCoordinator () override;

    enum class State
    {
      Timeout,
      GotResult,
      ConnectionLost,
    };

    struct Response
    {
      State             state;
      utils::Utf8String payload;
    };

    Response getResponse ();

    using ChildProcessCoordinator::sendMessageToWorker;

  private:
    void handleMessageFromWorker (const juce::MemoryBlock &mb) override;

    void handleConnectionLost () override;

    std::mutex              mutex;
    std::condition_variable condvar;

    utils::Utf8String payload;
    bool              connectionLost = false;
    bool              gotResult = false;

    JUCE_DECLARE_NON_MOVEABLE (SubprocessCoordinator)
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SubprocessCoordinator)
  };

  [[nodiscard]] PluginScanResult add_plugin_descriptions (
    const utils::Utf8String      &format_name,
    const utils::Utf8String      &identifier,
    const std::function<bool ()> &cancelled);

  std::unique_ptr<SubprocessCoordinator> coordinator_;

  JUCE_DECLARE_NON_MOVEABLE (OutOfProcessPluginScanner)
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OutOfProcessPluginScanner)
};

} // namespace zrythm::plugins::discovery
