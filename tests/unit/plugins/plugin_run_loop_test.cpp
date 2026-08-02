// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "plugins/plugin_run_loop.h"

#include <QTest>

#include "helpers/scoped_qcoreapplication.h"

#include <gtest/gtest.h>

#if defined(Q_OS_UNIX)
#  include <unistd.h>
#endif

namespace zrythm::plugins
{

class PluginRunLoopTest
    : public ::testing::Test,
      public test_helpers::ScopedQCoreApplication
{
};

TEST_F (PluginRunLoopTest, TimerFiresRepeatedly)
{
  PluginRunLoop run_loop;
  int           fire_count = 0;
  run_loop.register_timer (std::chrono::milliseconds{ 5 }, [&] {
    ++fire_count;
  });
  EXPECT_TRUE (QTest::qWaitFor ([&] { return fire_count >= 3; }, 1000));
}

TEST_F (PluginRunLoopTest, TimerStopsAfterUnregister)
{
  PluginRunLoop run_loop;
  int           fire_count = 0;
  const auto    token = run_loop.register_timer (
    std::chrono::milliseconds{ 5 }, [&] { ++fire_count; });
  ASSERT_TRUE (QTest::qWaitFor ([&] { return fire_count >= 1; }, 1000));

  run_loop.unregister_timer (token);
  const auto count_at_unregister = fire_count;
  EXPECT_FALSE (
    QTest::qWaitFor ([&] { return fire_count > count_at_unregister; }, 150));
}

#if defined(Q_OS_UNIX)
TEST_F (PluginRunLoopTest, FdReadWatchFires)
{
  PluginRunLoop run_loop;
  int           fds[2];
  ASSERT_EQ (pipe (fds), 0);

  bool       read_fired = false;
  const auto token =
    run_loop.register_fd (fds[0], true, false, [&] (bool read_ready) {
      EXPECT_TRUE (read_ready);
      read_fired = true;
    });

  ASSERT_EQ (write (fds[1], "x", 1), 1);
  EXPECT_TRUE (QTest::qWaitFor ([&] { return read_fired; }, 1000));

  run_loop.unregister_fd (token);
  close (fds[0]);
  close (fds[1]);
}

TEST_F (PluginRunLoopTest, FdUpdateDisablesAndReenablesWatch)
{
  PluginRunLoop run_loop;
  int           fds[2];
  ASSERT_EQ (pipe (fds), 0);

  bool       read_fired = false;
  const auto token = run_loop.register_fd (fds[0], true, false, [&] (bool) {
    read_fired = true;
  });

  // Disabled watch must not fire
  run_loop.update_fd (token, false, false);
  ASSERT_EQ (write (fds[1], "x", 1), 1);
  EXPECT_FALSE (QTest::qWaitFor ([&] { return read_fired; }, 150));

  // Re-enabled watch fires
  run_loop.update_fd (token, true, false);
  ASSERT_EQ (write (fds[1], "x", 1), 1);
  EXPECT_TRUE (QTest::qWaitFor ([&] { return read_fired; }, 1000));

  run_loop.unregister_fd (token);
  close (fds[0]);
  close (fds[1]);
}

TEST_F (PluginRunLoopTest, FdUnregisterStopsWatch)
{
  PluginRunLoop run_loop;
  int           fds[2];
  ASSERT_EQ (pipe (fds), 0);

  int        fire_count = 0;
  const auto token = run_loop.register_fd (fds[0], true, false, [&] (bool) {
    ++fire_count;
  });

  ASSERT_EQ (write (fds[1], "x", 1), 1);
  ASSERT_TRUE (QTest::qWaitFor ([&] { return fire_count >= 1; }, 1000));

  run_loop.unregister_fd (token);
  // Drain the pipe so the fd stays readable
  char buf[8];
  ASSERT_GT (read (fds[0], buf, sizeof (buf)), 0);
  ASSERT_EQ (write (fds[1], "x", 1), 1);
  const auto count_at_unregister = fire_count;
  EXPECT_FALSE (
    QTest::qWaitFor ([&] { return fire_count > count_at_unregister; }, 150));

  close (fds[0]);
  close (fds[1]);
}
#endif

} // namespace zrythm::plugins
