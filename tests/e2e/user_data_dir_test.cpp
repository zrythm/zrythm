// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <cstdlib>

#include <QDir>

#include "launcher.h"

namespace zrythm::e2e
{

class UserDataDirTest : public ::testing::Test
{
protected:
  ZrythmLauncher launcher_;
};

// A run with --user-data-dir writes its log into the directory, and
// nothing into the working directory and the home directory
TEST_F (UserDataDirTest, StartupWritesOnlyInsideTheDirectory)
{
  const auto working_dir_before =
    ZrythmLauncher::tree_entries (launcher_.working_dir ());
  const auto home_before = ZrythmLauncher::tree_entries (launcher_.home_dir ());

  const auto result = launcher_.run (
    { QStringLiteral ("--dummy"), QStringLiteral ("--quit-after-startup") },
    kQuitAfterStartup);
  expect_exit_code (result, EXIT_SUCCESS);

  const QDir cache_dir (launcher_.user_data_dir () + QStringLiteral ("/cache"));
  EXPECT_FALSE (
    cache_dir.entryList ({ QStringLiteral ("log_*") }, QDir::Files).isEmpty ())
    << to_utf8 (cache_dir.absolutePath ());
  EXPECT_EQ (
    ZrythmLauncher::tree_entries (launcher_.working_dir ()), working_dir_before);
#ifndef Q_OS_WIN
  // Qt takes the locations of the user profile from the operating
  // system on Windows, so the home directory of the runs is not private
  // there
  EXPECT_EQ (ZrythmLauncher::tree_entries (launcher_.home_dir ()), home_before);
#endif
}

// A --user-data-dir that is a file ends startup with an error
TEST_F (UserDataDirTest, FileAsDirectoryExitsWithFailure)
{
  const auto file = launcher_.make_file (QStringLiteral ("not-a-directory"));
  const auto result = launcher_.run (
    { QStringLiteral ("--dummy") }, kExitsWithoutEventLoop, file);
  expect_exit_code (result, EXIT_FAILURE);
  EXPECT_TRUE (
    result.standard_error.contains (QStringLiteral ("Fatal error at startup")))
    << result.describe ();
  EXPECT_TRUE (result.standard_error.contains (
    QStringLiteral ("Cannot create the directory")))
    << result.describe ();
}

} // namespace zrythm::e2e
