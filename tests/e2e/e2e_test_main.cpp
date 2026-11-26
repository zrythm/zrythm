// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <QCoreApplication>

#include <gtest/gtest.h>

int
main (int argc, char ** argv)
{
  QCoreApplication application (argc, argv);
  ::testing::InitGoogleTest (&argc, argv);
  return RUN_ALL_TESTS ();
}
