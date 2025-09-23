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
    {
      std::lock_guard<std::mutex> lock (initializer_mutex_);
      // Signal the thread to exit first
      if (juce::MessageManager::getInstanceWithoutCreating () != nullptr)
        {
          juce::MessageManager::getInstance ()->stopDispatchLoop ();
        }
    }

    // Wait for the thread to exit completely to avoid data races
    // with JUCE's InternalMessageQueue file descriptors and atomic state
    stopThread (-1); // -1 means wait indefinitely

    // The thread should now be completely stopped, and the initializer_
    // should have been destroyed by the thread itself before exiting

    {
      std::lock_guard<std::mutex> lock (initializer_mutex_);
      initializer_.reset ();
    }
  }

  Z_DISABLE_COPY_MOVE (ScopedJuceMessageThread)

private:
  void run () override
  {
    {
      std::lock_guard<std::mutex> lock (initializer_mutex_);
      initializer_ = std::make_unique<juce::ScopedJuceInitialiser_GUI> ();
    }
    semaphore_.signal ();

    juce::MessageManager::getInstance ()->runDispatchLoop ();

    // Destroy the JUCE infrastructure from within the thread itself
    // to avoid race conditions with file descriptor cleanup
    {
      std::lock_guard<std::mutex> lock (initializer_mutex_);
      initializer_.reset ();
    }
  }

  juce::WaitableEvent                              semaphore_;
  mutable std::mutex                               initializer_mutex_;
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
