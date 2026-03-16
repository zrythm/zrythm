// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <QEventLoop>
#include <QFuture>
#include <QFutureWatcher>
#include <QTimer>

namespace zrythm::test_helpers
{

/**
 * @brief Waits for a QFuture to complete while processing Qt events.
 *
 * This is necessary when the QFuture uses Qt continuations that need to run
 * on the main thread. Using waitForFinished() on the main thread would cause
 * a deadlock since it blocks the event loop.
 *
 * @param future The QFuture to wait for.
 * @param timeout_ms Maximum time to wait in milliseconds (default: 30 seconds).
 * @return true if the future completed, false if it timed out.
 */
template <typename T>
bool
waitForFutureWithEvents (QFuture<T> &future, int timeout_ms = 30000)
{
  if (future.isFinished ())
    return true;

  QFutureWatcher<T> watcher;
  QEventLoop        loop;
  bool              timed_out = false;

  QObject::connect (
    &watcher, &QFutureWatcher<T>::finished, &loop, [&loop, &timed_out] () {
      timed_out = false;
      loop.quit ();
    });

  watcher.setFuture (future);

  // Timeout to prevent infinite hang
  QTimer::singleShot (timeout_ms, &loop, [&loop, &timed_out] () {
    timed_out = true;
    loop.quit ();
  });

  loop.exec ();

  return !timed_out && future.isFinished ();
}

} // namespace zrythm::test_helpers
