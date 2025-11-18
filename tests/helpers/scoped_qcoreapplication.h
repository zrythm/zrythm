// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

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

private:
  std::unique_ptr<QCoreApplication> app_;
};
}
