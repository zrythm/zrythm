// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "frontends/gui/backend/juce_event_loop_bridge.h"

#ifdef __linux__

#  include <latch>
#  include <thread>

#  include <sys/socket.h>

#  include <QCoreApplication>
#  include <QTest>
#  include <QThread>

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

  juce::ScopedJuceInitialiser_GUI                      juce_initializer;
  zrythm::frontends::gui::backend::JuceEventLoopBridge bridge;

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

// A descriptor registered from another thread is watched by the Qt event
// loop, and its callback runs on the thread of that loop
TEST (JuceEventLoopBridgeTest, DispatchesDescriptorRegisteredFromAnotherThread)
{
  zrythm::test_helpers::ScopedQCoreApplication app;

  juce::ScopedJuceInitialiser_GUI                      juce_initializer;
  zrythm::frontends::gui::backend::JuceEventLoopBridge bridge;

  int fds[2];
  ASSERT_EQ (::socketpair (AF_LOCAL, SOCK_STREAM, 0, fds), 0);

  QThread *    callback_thread = nullptr;
  std::latch   registered (1);
  std::latch   dispatch_checked (1);
  std::jthread registrant ([&] {
    juce::LinuxEventLoop::registerFdCallback (fds[0], [&callback_thread] (int) {
      callback_thread = QThread::currentThread ();
    });
    registered.count_down ();
    dispatch_checked.wait ();
    juce::LinuxEventLoop::unregisterFdCallback (fds[0]);
  });
  registered.wait ();

  const char byte = 1;
  EXPECT_EQ (::write (fds[1], &byte, 1), 1);

  const bool dispatched = QTest::qWaitFor ([&callback_thread] () {
    return callback_thread != nullptr;
  });
  dispatch_checked.count_down ();
  registrant.join ();

  EXPECT_TRUE (dispatched);
  EXPECT_EQ (callback_thread, QCoreApplication::instance ()->thread ());

  ::close (fds[0]);
  ::close (fds[1]);
}

// A callback may unregister its own descriptor. It runs once, and a
// callback registered for the same descriptor afterwards is dispatched too
TEST (
  JuceEventLoopBridgeTest,
  DispatchesDescriptorRegisteredAgainAfterItsCallbackUnregistersIt)
{
  zrythm::test_helpers::ScopedQCoreApplication app;

  juce::ScopedJuceInitialiser_GUI                      juce_initializer;
  zrythm::frontends::gui::backend::JuceEventLoopBridge bridge;

  int fds[2];
  ASSERT_EQ (::socketpair (AF_LOCAL, SOCK_STREAM, 0, fds), 0);

  int unregistering_callback_calls = 0;
  juce::LinuxEventLoop::registerFdCallback (
    fds[0], [&unregistering_callback_calls, fd = fds[0]] (int) {
      ++unregistering_callback_calls;
      juce::LinuxEventLoop::unregisterFdCallback (fd);
    });

  const char byte = 1;
  ASSERT_EQ (::write (fds[1], &byte, 1), 1);
  EXPECT_TRUE (QTest::qWaitFor ([&unregistering_callback_calls] () {
    return unregistering_callback_calls > 0;
  }));
  EXPECT_EQ (unregistering_callback_calls, 1);

  bool next_callback_called = false;
  juce::LinuxEventLoop::registerFdCallback (
    fds[0], [&next_callback_called] (int) { next_callback_called = true; });
  ASSERT_EQ (::write (fds[1], &byte, 1), 1);
  EXPECT_TRUE (QTest::qWaitFor ([&next_callback_called] () {
    return next_callback_called;
  }));

  juce::LinuxEventLoop::unregisterFdCallback (fds[0]);
  ::close (fds[0]);
  ::close (fds[1]);
}

} // namespace

#endif // __linux__
