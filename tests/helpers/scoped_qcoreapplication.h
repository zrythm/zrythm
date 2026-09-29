// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <chrono>
#include <memory>

#include <QCoreApplication>
#include <QTest>

namespace zrythm::test_helpers
{
/**
 * @brief A base class that starts a QCoreApplication.
 *
 * This is needed in order to test signal functionality.
 *
 * @see https://forum.qt.io/post/804638
 */
class ScopedQCoreApplication
{
public:
  ScopedQCoreApplication ()
  {
    // Central knob for the QTest::qWaitFor()/QTRY_* timeout. Matches Qt's
    // own 5s default; stored explicitly so there is one place to tune it
    // (individual calls can still pass their own timeout).
    QTest::defaultTryTimeout.store (
      std::chrono::seconds (5), std::memory_order_relaxed);
    int     argc = 0;
    char ** argv = nullptr;
    app_ = std::make_unique<QCoreApplication> (argc, argv);
  }

private:
  std::unique_ptr<QCoreApplication> app_;
};
}
