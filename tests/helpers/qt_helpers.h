// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <chrono>

#include <QDeadlineTimer>
#include <QFuture>
#include <QTest>

namespace zrythm::test_helpers
{

/**
 * @brief Timeout used when waiting on QFutures (project save/load can be
 * slow).
 */
inline constexpr std::chrono::milliseconds kFutureWaitTimeout{ 30'000 };

/**
 * @brief Waits for a QFuture to complete while processing Qt events.
 *
 * Uses QTest::qWaitFor() so the event loop keeps running: QFuture
 * continuations queued on the main thread would deadlock if the caller
 * blocked with waitForFinished().
 *
 * @param future The QFuture to wait for.
 * @param timeout Maximum time to wait.
 * @return true if the future completed, false if it timed out.
 */
template <typename T>
bool
waitForFutureWithEvents (
  QFuture<T>    &future,
  QDeadlineTimer timeout = QDeadlineTimer (kFutureWaitTimeout))
{
  return QTest::qWaitFor (
    [&future] () { return future.isFinished (); }, timeout);
}

} // namespace zrythm::test_helpers
