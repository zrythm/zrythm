// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <cstdlib>

#include "launcher.h"

namespace zrythm::e2e
{

class StartupTest : public ::testing::Test
{
protected:
  ZrythmLauncher launcher_;
};

// A run with no requested project ends by itself and logs no error
TEST_F (StartupTest, GreeterOnlyRunExitsWithSuccess)
{
  const auto result = launcher_.run (
    { QStringLiteral ("--dummy"), QStringLiteral ("--quit-after-startup") },
    kQuitAfterStartup);
  expect_exit_code (result, EXIT_SUCCESS);
  EXPECT_EQ (
    count_lines_holding (
      result.standard_output, QStringLiteral ("Shutting down...")),
    1)
    << result.describe ();
  EXPECT_FALSE (has_error_lines (result.standard_output)) << result.describe ();
}

// A missing project is loaded once, the run reports the failure and ends
TEST_F (StartupTest, MissingProjectExitsWithFailure)
{
  const auto missing_project =
    launcher_.make_directory (QStringLiteral ("missing-project"))
    + QStringLiteral ("/missing.zpj");
  const auto result = launcher_.run (
    {
      QStringLiteral ("--dummy"),
      QStringLiteral ("--quit-after-startup"),
      QStringLiteral ("--project"),
      missing_project,
    },
    kQuitAfterStartup);
  expect_exit_code (result, EXIT_FAILURE);
  EXPECT_EQ (
    count_lines_holding (
      result.standard_output, QStringLiteral ("Loading project from")),
    1)
    << result.describe ();
  EXPECT_EQ (
    count_lines_holding (
      result.standard_output, QStringLiteral ("Startup did not finish within")),
    0)
    << result.describe ();
}

// The launcher ends an application that does not exit
TEST (LauncherTest, EndsAChildThatKeepsRunning)
{
  ZrythmLauncher launcher;
  const auto     result =
    launcher.run ({ QStringLiteral ("--dummy") }, std::chrono::seconds{ 10 });
  EXPECT_TRUE (result.ended_by_launcher) << result.describe ();
}

} // namespace zrythm::e2e
