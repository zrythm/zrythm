// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/juce_event_loop_bridge.h"

#ifdef __linux__

#  include <sys/socket.h>

#  include <QCoreApplication>
#  include <QTest>

#  include "helpers/scoped_qcoreapplication.h"

#  include <gtest/gtest.h>
#  include <juce_events/juce_events.h>
#  include <unistd.h>

namespace
{

// A descriptor registered with JUCE must have its callback invoked when
// the descriptor becomes readable, driven by the Qt event loop alone (no
// JUCE dispatch loop of its own is running)
TEST (JuceEventLoopBridgeTest, DispatchesRegisteredFdCallbacks)
{
  zrythm::test_helpers::ScopedQCoreApplication app;

  juce::ScopedJuceInitialiser_GUI           juce_initializer;
  zrythm::gui::backend::JuceEventLoopBridge bridge;

  int fds[2];
  ASSERT_EQ (::socketpair (AF_LOCAL, SOCK_STREAM, 0, fds), 0);

  bool called = false;
  juce::LinuxEventLoop::registerFdCallback (fds[0], [&called] (int) {
    called = true;
  });

  const char byte = 1;
  ASSERT_EQ (::write (fds[1], &byte, 1), 1);

  EXPECT_TRUE (QTest::qWaitFor ([&called] () { return called; }));

  juce::LinuxEventLoop::unregisterFdCallback (fds[0]);
  ::close (fds[0]);
  ::close (fds[1]);
}

} // namespace

#endif // __linux__
