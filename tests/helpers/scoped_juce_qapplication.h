// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <QObject>
#include <QTimer>

#include "helpers/scoped_qcoreapplication.h"

#include <juce_wrapper.h>

namespace zrythm::test_helpers
{
/**
 * @brief QApplication wrapper that also spins the JUCE message loop.
 *
 * To be inherited from (or used as a member) in test classes.
 */
class ScopedJuceQApplication : public ScopedQCoreApplication
{
public:
  ScopedJuceQApplication ()
  {
    QObject::connect (&timer_, &QTimer::timeout, qApp, [] () {
      juce::MessageManager::getInstance ()->runDispatchLoopUntil (10);
    });
    timer_.start ();
    juce::MessageManager::getInstance ()->runDispatchLoopUntil (0);
  }
  ~ScopedJuceQApplication () = default;
  ScopedJuceQApplication (const ScopedJuceQApplication &) = delete;
  ScopedJuceQApplication &operator= (const ScopedJuceQApplication &) = delete;
  ScopedJuceQApplication (ScopedJuceQApplication &&) = delete;
  ScopedJuceQApplication &operator= (ScopedJuceQApplication &&) = delete;

private:
  juce::ScopedJuceInitialiser_GUI initializer_;
  QTimer                          timer_;
};

}
