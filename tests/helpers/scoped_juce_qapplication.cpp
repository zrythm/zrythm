// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "helpers/scoped_juce_qapplication.h"

namespace zrythm::test_helpers
{

ScopedJuceQApplication::ScopedJuceQApplication ()
{
  QObject::connect (&timer_, &QTimer::timeout, qApp, [] () {
    juce::MessageManager::getInstance ()->runDispatchLoopUntil (10);
  });
  timer_.start ();
  juce::MessageManager::getInstance ()->runDispatchLoopUntil (0);
}

ScopedJuceQApplication::~ScopedJuceQApplication () = default;

}
