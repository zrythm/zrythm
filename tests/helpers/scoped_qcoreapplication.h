// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <functional>
#include <memory>

#include <QCoreApplication>

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
    int     argc = 0;
    char ** argv = nullptr;
    app_ = std::make_unique<QCoreApplication> (argc, argv);
  }

  /**
   * @brief Processes QCoreApplication events until @p cond returns true;
   *
   */
  static void process_events_until_true (const std::function<bool ()> &cond)
  {
    constexpr auto max_calls = 100;
    for (int i = 0; i < max_calls && !cond (); ++i)
      {
        QCoreApplication::processEvents (QEventLoop::AllEvents, 50);
      }
  }

private:
  std::unique_ptr<QCoreApplication> app_;
};
}
