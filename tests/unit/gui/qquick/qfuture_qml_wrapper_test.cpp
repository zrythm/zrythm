// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <stdexcept>

#include "gui/qquick/qfuture_qml_wrapper.h"

#include <QSignalSpy>
#include <QTest>

#include "helpers/scoped_qcoreapplication.h"

#include <gtest/gtest.h>

namespace zrythm::gui::qquick
{

class QFutureQmlWrapperTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    app_ = std::make_unique<test_helpers::ScopedQCoreApplication> ();
  }

  void TearDown () override { app_.reset (); }

  std::unique_ptr<test_helpers::ScopedQCoreApplication> app_;
};

TEST_F (QFutureQmlWrapperTest, ExceptionEmitsFailedWithMessage)
{
  QPromise<QString> promise;
  promise.start ();
  QFutureQmlWrapperT<QString> wrapper (promise.future ());
  QSignalSpy succeeded_spy (&wrapper, &QFutureQmlWrapper::succeeded);
  QSignalSpy failed_spy (&wrapper, &QFutureQmlWrapper::failed);
  QSignalSpy canceled_spy (&wrapper, &QFutureQmlWrapper::canceled);

  promise.setException (std::make_exception_ptr (std::runtime_error ("boom")));
  promise.finish ();

  ASSERT_TRUE (failed_spy.wait ());
  EXPECT_EQ (failed_spy.count (), 1);
  EXPECT_EQ (failed_spy.at (0).at (0).toString (), QStringLiteral ("boom"));
  EXPECT_EQ (succeeded_spy.count (), 0);
  EXPECT_EQ (canceled_spy.count (), 0);
}

TEST_F (QFutureQmlWrapperTest, ResultEmitsSucceededExactlyOnce)
{
  QPromise<QString> promise;
  promise.start ();
  QFutureQmlWrapperT<QString> wrapper (promise.future ());
  QSignalSpy succeeded_spy (&wrapper, &QFutureQmlWrapper::succeeded);
  QSignalSpy failed_spy (&wrapper, &QFutureQmlWrapper::failed);
  QSignalSpy canceled_spy (&wrapper, &QFutureQmlWrapper::canceled);

  promise.addResult (QStringLiteral ("/tmp/project"));
  promise.finish ();

  QTRY_COMPARE (succeeded_spy.count (), 1);
  EXPECT_EQ (failed_spy.count (), 0);
  EXPECT_EQ (canceled_spy.count (), 0);
  EXPECT_EQ (wrapper.resultVar ().toString (), QStringLiteral ("/tmp/project"));
}

TEST_F (QFutureQmlWrapperTest, CancelEmitsCanceled)
{
  QPromise<QString> promise;
  promise.start ();
  QFutureQmlWrapperT<QString> wrapper (promise.future ());
  QSignalSpy succeeded_spy (&wrapper, &QFutureQmlWrapper::succeeded);
  QSignalSpy failed_spy (&wrapper, &QFutureQmlWrapper::failed);
  QSignalSpy canceled_spy (&wrapper, &QFutureQmlWrapper::canceled);

  promise.future ().cancel ();
  promise.finish ();

  ASSERT_TRUE (canceled_spy.wait ());
  EXPECT_EQ (canceled_spy.count (), 1);
  EXPECT_EQ (succeeded_spy.count (), 0);
  EXPECT_EQ (failed_spy.count (), 0);
}

TEST_F (QFutureQmlWrapperTest, FinishedBeforeWrappingStillEmitsOutcome)
{
  QPromise<QString> promise;
  promise.start ();
  promise.setException (
    std::make_exception_ptr (std::runtime_error ("too late")));
  promise.finish ();

  QFutureQmlWrapperT<QString> wrapper (promise.future ());
  QSignalSpy                  failed_spy (&wrapper, &QFutureQmlWrapper::failed);

  ASSERT_TRUE (failed_spy.wait ());
  EXPECT_EQ (failed_spy.at (0).at (0).toString (), QStringLiteral ("too late"));
}

TEST_F (QFutureQmlWrapperTest, CancelAfterSuccessStillEmitsOnlySucceeded)
{
  QPromise<QString> promise;
  promise.start ();
  QFutureQmlWrapperT<QString> wrapper (promise.future ());
  QSignalSpy succeeded_spy (&wrapper, &QFutureQmlWrapper::succeeded);
  QSignalSpy failed_spy (&wrapper, &QFutureQmlWrapper::failed);
  QSignalSpy canceled_spy (&wrapper, &QFutureQmlWrapper::canceled);

  promise.addResult (QStringLiteral ("/tmp/project"));
  promise.finish ();
  QTRY_COMPARE (succeeded_spy.count (), 1);

  promise.future ().cancel ();
  QCoreApplication::processEvents ();
  QCoreApplication::processEvents ();

  EXPECT_EQ (succeeded_spy.count (), 1);
  EXPECT_EQ (failed_spy.count (), 0);
  EXPECT_EQ (canceled_spy.count (), 0);
}

}
