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

#include <QtQmlIntegration>

#include "juce_wrapper.h"

namespace zrythm::plugins::discovery

{

/**
 * @brief Scans for plugins in an out-of-process manner.
 *
 * This class is responsible for scanning for audio plugins in an out-of-process
 * manner, using a separate child process to perform the scanning. This is done
 * to avoid potential crashes or hangs in the main process when scanning for
 * plugins.
 *
 * The scanning is performed asynchronously, and a `scanCompleted()` signal is
 * emitted when the scan is finished.
 *
 * TODO: write unit tests.
 */
class OutOfProcessPluginScanner final
    : public QObject,
      public juce::KnownPluginList::CustomScanner
{
  Q_OBJECT

public:
  OutOfProcessPluginScanner (QObject * parent = nullptr);
  ~OutOfProcessPluginScanner () override;

  bool findPluginTypesFor (
    juce::AudioPluginFormat                   &format,
    juce::OwnedArray<juce::PluginDescription> &result,
    const juce::String                        &fileOrIdentifier) override;

  /**
   * @brief Used to let Qt know when the scan is done.
   */
  Q_SIGNAL void scanCompleted ();

private:
  /**
   * @brief Coordinates the execution of a subprocess to scan for plugins.
   *
   * This class is responsible for managing the communication with a child
   * process that is used to scan for audio plugins. It handles sending messages
   * to the child process, receiving responses, and managing the state of the
   * communication.
   *
   * The class is designed to be used in a thread-safe manner, with a mutex and
   * condition variable to synchronize access to the shared state.
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
      State                             state;
      std::unique_ptr<juce::XmlElement> xml;
    };

    Response getResponse ();

    using ChildProcessCoordinator::sendMessageToWorker;

  private:
    void handleMessageFromWorker (const juce::MemoryBlock &mb) override;

    void handleConnectionLost () override;

    std::mutex              mutex;
    std::condition_variable condvar;

    std::unique_ptr<juce::XmlElement> pluginDescription;
    bool                              connectionLost = false;
    bool                              gotResult = false;

    JUCE_DECLARE_NON_MOVEABLE (SubprocessCoordinator)
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SubprocessCoordinator)
  }; // class Superprocess

  /*  Scans for a plugin with format 'formatName' and ID 'fileOrIdentifier'
    using a subprocess, and adds discovered plugin descriptions to 'result'.

     Returns true on success.

     Failure indicates that the subprocess is unrecoverable and should be
    terminated.
  */
  bool add_plugin_descriptions (
    juce::AudioPluginFormat                   &format,
    const juce::String                        &file_or_identifier,
    juce::OwnedArray<juce::PluginDescription> &result);

  std::unique_ptr<SubprocessCoordinator> coordinator_;

  JUCE_DECLARE_NON_MOVEABLE (OutOfProcessPluginScanner)
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OutOfProcessPluginScanner)
};

} // namespace zrythm::plugins::discovery
