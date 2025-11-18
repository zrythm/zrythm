// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "utils/debouncer.h"

#include <QTest>
#include <QtSystemDetection>

#include "helpers/scoped_qcoreapplication.h"

#include <gtest/gtest.h>

using namespace zrythm::test_helpers;
using namespace std::chrono_literals;

namespace zrythm::utils
{

class DebouncerTest : public ::testing::Test
{
protected:
  // On Mac there are some issues...
#ifdef Q_OS_MACOS
  static constexpr auto DEFAULT_DELAY = 1000ms;
#else
  static constexpr auto DEFAULT_DELAY = 100ms;
#endif

  // Gets a reasonable time after given delay so that we can be sure the
  // callback has been executed after the returned time
  static auto get_reasonable_time_after_delay (const auto &delay)
  {
    return (delay * 3) / 2;
  }
  static auto get_reasonable_time_after_default_delay ()
  {
    return get_reasonable_time_after_delay (DEFAULT_DELAY);
  }

  void SetUp () override
  {
    // Set up QCoreApplication for Qt event handling
    app_ = std::make_unique<ScopedQCoreApplication> ();
    callback_count_ = 0;
    debouncer = std::make_unique<Debouncer> (DEFAULT_DELAY, [this] () {
      callback_count_++;
    });
  }

  void TearDown () override { }

  std::unique_ptr<ScopedQCoreApplication> app_;
  std::unique_ptr<Debouncer>              debouncer;
  int                                     callback_count_{ 0 };
};

TEST_F (DebouncerTest, BasicExecution)
{
  debouncer->debounce ();

  // Process events to trigger timer
  QTest::qWait (get_reasonable_time_after_default_delay ());

  EXPECT_EQ (callback_count_, 1);
}

TEST_F (DebouncerTest, MultipleCallsDebounced)
{
  // Call debouncer multiple times rapidly
  debouncer->debounce ();
  debouncer->debounce ();
  debouncer->debounce ();

  QTest::qWait (get_reasonable_time_after_default_delay ());

  // Should only execute once due to debouncing
  EXPECT_EQ (callback_count_, 1);
}

TEST_F (DebouncerTest, TimerResetOnNewCall)
{
  const auto custom_delay = DEFAULT_DELAY * 2;
  debouncer->set_delay (custom_delay);

  // First call
  debouncer->debounce ();

  // Wait for half the time, then call again - should reset timer
  QTest::qWait (custom_delay / 2);
  debouncer->debounce ();

  // Wait for original delay time - should not have fired yet
  QTest::qWait (custom_delay / 2);
  EXPECT_EQ (callback_count_, 0);

  // Wait for the remaining time - should fire now
  QTest::qWait (get_reasonable_time_after_delay (custom_delay));
  EXPECT_EQ (callback_count_, 1);
}

TEST_F (DebouncerTest, CustomDelay)
{
  const auto custom_delay = DEFAULT_DELAY * 2;
  debouncer->set_delay (custom_delay);

  debouncer->debounce ();

  // Should not fire before custom delay
  QTest::qWait (DEFAULT_DELAY);
  EXPECT_EQ (callback_count_, 0);

  // Should fire after custom delay
  QTest::qWait (get_reasonable_time_after_delay (custom_delay));
  EXPECT_EQ (callback_count_, 1);
}

TEST_F (DebouncerTest, GetSetDelay)
{
  const auto custom_delay = 500ms;
  debouncer->set_delay (custom_delay);

  EXPECT_EQ (debouncer->get_delay (), custom_delay);
}

TEST_F (DebouncerTest, IsPending)
{
  // Initially not pending
  EXPECT_FALSE (debouncer->is_pending ());

  debouncer->debounce ();
  EXPECT_TRUE (debouncer->is_pending ());

  QTest::qWait (get_reasonable_time_after_default_delay ());
  EXPECT_FALSE (debouncer->is_pending ());
}

TEST_F (DebouncerTest, Cancel)
{
  debouncer->debounce ();
  EXPECT_TRUE (debouncer->is_pending ());

  debouncer->cancel ();
  EXPECT_FALSE (debouncer->is_pending ());

  // Wait to ensure callback doesn't fire
  QTest::qWait (get_reasonable_time_after_default_delay ());
  EXPECT_EQ (callback_count_, 0);
}

TEST_F (DebouncerTest, MultipleSeparateExecutions)
{
  // First execution
  debouncer->debounce ();
  QTest::qWait (get_reasonable_time_after_default_delay ());
  EXPECT_EQ (callback_count_, 1);

  // Second execution
  debouncer->debounce ();
  QTest::qWait (get_reasonable_time_after_default_delay ());
  EXPECT_EQ (callback_count_, 2);
}

TEST_F (DebouncerTest, OperatorCall)
{
  // Test using operator() instead of debounce()
  (*debouncer) ();

  QTest::qWait (get_reasonable_time_after_default_delay ());

  EXPECT_EQ (callback_count_, 1);
}

TEST_F (DebouncerTest, RapidCallsWithCancellation)
{
  debouncer->debounce ();
  debouncer->debounce ();

  EXPECT_TRUE (debouncer->is_pending ());

  debouncer->cancel ();
  EXPECT_FALSE (debouncer->is_pending ());

  // New call after cancellation should work
  debouncer->debounce ();
  EXPECT_TRUE (debouncer->is_pending ());

  QTest::qWait (get_reasonable_time_after_default_delay ());
  EXPECT_EQ (callback_count_, 1);
}

TEST_F (DebouncerTest, ZeroDelay)
{
  // Test with zero delay - should execute immediately
  Debouncer zero_delay_debouncer (0ms, [this] () { callback_count_++; });

  zero_delay_debouncer.debounce ();

  // Process events
  QTest::qWait (10ms);

  EXPECT_EQ (callback_count_, 1);
}

TEST_F (DebouncerTest, VeryShortDelay)
{
  // Test with very short delay
  Debouncer short_delay_debouncer (1ms, [this] () { callback_count_++; });

  short_delay_debouncer.debounce ();
  short_delay_debouncer.debounce ();

  // Process events
  QTest::qWait (get_reasonable_time_after_default_delay ());

  // Should still only execute once
  EXPECT_EQ (callback_count_, 1);
}

} // namespace zrythm::utils
