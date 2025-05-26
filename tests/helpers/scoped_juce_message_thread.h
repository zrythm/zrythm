// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "utils/types.h"

namespace zrythm::test_helpers
{
/**
 * @brief JUCE message thread mock.
 *
 * Taken from https://forum.juce.com/t/unit-test-with-catch-c/26827/8
 */
class ScopedJuceMessageThread : juce::Thread
{
public:
  ScopedJuceMessageThread () : juce::Thread ("message thread")
  {
    startThread ();
    auto wasSignaled = semaphore_.wait ();
    juce::ignoreUnused (wasSignaled);
  }

  ~ScopedJuceMessageThread () override
  {
    juce::MessageManager::getInstance ()->stopDispatchLoop ();
    stopThread (10);
  }

  Z_DISABLE_COPY_MOVE (ScopedJuceMessageThread)

private:
  void run () override
  {
    initializer_ = std::make_unique<juce::ScopedJuceInitialiser_GUI> ();
    semaphore_.signal ();
    juce::MessageManager::getInstance ()->runDispatchLoop ();
  }

  juce::WaitableEvent                              semaphore_;
  std::unique_ptr<juce::ScopedJuceInitialiser_GUI> initializer_;
};

/**
 * @brief ScopedJuceMessageThread wrapped as a base class.
 */
class ScopedJuceApplication
{
private:
  ScopedJuceMessageThread message_thread_;
};

}
