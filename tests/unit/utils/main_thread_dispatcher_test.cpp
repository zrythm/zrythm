// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <cstdint>
#include <ranges>
#include <thread>

#include "utils/main_thread_dispatcher.h"

#include <QCoreApplication>
#include <QObject>
#include <QTest>

#include "helpers/scoped_qcoreapplication.h"

#include <gtest/gtest.h>

using namespace zrythm::test_helpers;
using namespace std::chrono_literals;

namespace zrythm::utils
{

struct MainThreadDispatcherTestRequest
{
  uint32_t a = 0;
  uint32_t b = 0;
};

class MainThreadDispatcherTest : public ::testing::Test
{
protected:
  using TestRequest = MainThreadDispatcherTestRequest;

  void SetUp () override
  {
    app_ = std::make_unique<ScopedQCoreApplication> ();
    context_ = std::make_unique<QObject> ();
    dispatcher_ = std::make_unique<MainThreadDispatcher<TestRequest>> (
      *context_, 10ms,
      [this] (const TestRequest &request) { handled_.push_back (request); });
  }

  void TearDown () override
  {
    dispatcher_.reset ();
    context_.reset ();
    app_.reset ();
  }

  std::unique_ptr<ScopedQCoreApplication>            app_;
  std::unique_ptr<QObject>                           context_;
  std::unique_ptr<MainThreadDispatcher<TestRequest>> dispatcher_;
  std::vector<TestRequest>                           handled_;
};

TEST_F (MainThreadDispatcherTest, PostFromContextThreadIsSynchronous)
{
  EXPECT_TRUE (dispatcher_->post ({ .a = 1, .b = 2 }));
  ASSERT_EQ (handled_.size (), 1);
  EXPECT_EQ (handled_[0].a, 1u);
  EXPECT_EQ (handled_[0].b, 2u);
}

TEST_F (MainThreadDispatcherTest, PostFromOtherThreadIsDeliveredOnPump)
{
  {
    std::jthread poster ([this] {
      EXPECT_TRUE (dispatcher_->post ({ .a = 3, .b = 4 }));
    });
  }

  // Not delivered synchronously
  EXPECT_TRUE (handled_.empty ());

  ASSERT_TRUE (QTest::qWaitFor ([this] { return !handled_.empty (); }, 2000));
  EXPECT_EQ (handled_[0].a, 3u);
  EXPECT_EQ (handled_[0].b, 4u);
}

TEST_F (MainThreadDispatcherTest, PostFromContextThreadDrainsQueueFirst)
{
  MainThreadDispatcher<TestRequest> local_dispatcher (
    *context_, 10s,
    [this] (const TestRequest &request) { handled_.push_back (request); });

  {
    std::jthread poster ([&] {
      EXPECT_TRUE (local_dispatcher.post ({ .a = 1, .b = 0 }));
    });
  }

  // Posting from the context thread must handle the queued request first
  EXPECT_TRUE (local_dispatcher.post ({ .a = 2, .b = 0 }));
  ASSERT_EQ (handled_.size (), 2);
  EXPECT_EQ (handled_[0].a, 1u);
  EXPECT_EQ (handled_[1].a, 2u);
}

TEST_F (MainThreadDispatcherTest, PostsAreDroppedWhenFifoIsFull)
{
  // The pump interval is long and the event loop is never run, so the
  // fifo never drains and fills up
  MainThreadDispatcher<TestRequest, 4> small_dispatcher (
    *context_, 10s, [] (const TestRequest &) { });

  int accepted = 0;
  {
    std::jthread poster ([&] {
      for (const auto i : std::views::iota (0, 8))
        {
          if (small_dispatcher.post ({ .a = static_cast<uint32_t> (i), .b = 0 }))
            ++accepted;
        }
    });
  }

  EXPECT_EQ (accepted, 4);
}

} // namespace zrythm::utils
