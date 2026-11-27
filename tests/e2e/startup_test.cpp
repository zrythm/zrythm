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

// --project with --new-project is refused before a project is opened
TEST_F (StartupTest, ProjectOptionConflictsWithNewProject)
{
  const auto project =
    launcher_.make_directory (QStringLiteral ("project"))
    + QStringLiteral ("/missing.zpj");
  const auto parent_dir = launcher_.make_directory (QStringLiteral ("projects"));
  const auto result = launcher_.run (
    {
      QStringLiteral ("--dummy"),
      QStringLiteral ("--quit-after-startup"),
      QStringLiteral ("--project"),
      project,
      QStringLiteral ("--new-project"),
      parent_dir,
    },
    kExitsWithoutEventLoop);
  expect_exit_code (result, EXIT_FAILURE);
  EXPECT_TRUE (result.standard_error.contains (QStringLiteral ("--project")))
    << result.describe ();
  EXPECT_TRUE (result.standard_error.contains (QStringLiteral ("--new-project")))
    << result.describe ();
  EXPECT_EQ (
    count_lines_holding (
      result.standard_output, QStringLiteral ("Loading project from")),
    0)
    << result.describe ();
  EXPECT_TRUE (ZrythmLauncher::tree_entries (parent_dir).isEmpty ())
    << to_utf8 (parent_dir);
}

// --new-project given twice is refused before a project is created
TEST_F (StartupTest, NewProjectOptionGivenTwiceExitsWithFailure)
{
  const auto parent_dir = launcher_.make_directory (QStringLiteral ("projects"));
  const auto result = launcher_.run (
    {
      QStringLiteral ("--dummy"),
      QStringLiteral ("--quit-after-startup"),
      QStringLiteral ("--new-project"),
      parent_dir,
      QStringLiteral ("--new-project"),
      parent_dir + QStringLiteral ("/again"),
    },
    kExitsWithoutEventLoop);
  expect_exit_code (result, EXIT_FAILURE);
  EXPECT_TRUE (result.standard_error.contains (QStringLiteral ("--new-project")))
    << result.describe ();
  EXPECT_TRUE (ZrythmLauncher::tree_entries (parent_dir).isEmpty ())
    << to_utf8 (parent_dir);
}

// An empty or blank directory of --new-project is refused before a
// project is created
TEST_F (StartupTest, NewProjectOptionWithoutDirectoryExitsWithFailure)
{
  for (
    const auto &directory : {
      QString{},
      QStringLiteral ("   "),
    })
    {
      const auto result = launcher_.run (
        {
          QStringLiteral ("--dummy"),
          QStringLiteral ("--quit-after-startup"),
          QStringLiteral ("--new-project"),
          directory,
        },
        kExitsWithoutEventLoop);
      expect_exit_code (result, EXIT_FAILURE);
      EXPECT_TRUE (
        result.standard_error.contains (QStringLiteral ("--new-project")))
        << result.describe ();
      EXPECT_TRUE (
        ZrythmLauncher::tree_entries (launcher_.working_dir ()).isEmpty ())
        << to_utf8 (launcher_.working_dir ());
    }
}

// A project argument with --new-project is refused before a project is
// opened
TEST_F (StartupTest, PositionalProjectConflictsWithNewProject)
{
  const auto project =
    launcher_.make_directory (QStringLiteral ("project"))
    + QStringLiteral ("/missing.zpj");
  const auto parent_dir = launcher_.make_directory (QStringLiteral ("projects"));
  const auto result = launcher_.run (
    {
      QStringLiteral ("--dummy"),
      QStringLiteral ("--quit-after-startup"),
      QStringLiteral ("--new-project"),
      parent_dir,
      project,
    },
    kExitsWithoutEventLoop);
  expect_exit_code (result, EXIT_FAILURE);
  EXPECT_TRUE (result.standard_error.contains (QStringLiteral ("--new-project")))
    << result.describe ();
  EXPECT_EQ (
    count_lines_holding (
      result.standard_output, QStringLiteral ("Loading project from")),
    0)
    << result.describe ();
  EXPECT_TRUE (ZrythmLauncher::tree_entries (parent_dir).isEmpty ())
    << to_utf8 (parent_dir);
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
