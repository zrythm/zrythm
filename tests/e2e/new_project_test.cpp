// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <cstdlib>
#include <string>

#include <QCryptographicHash>
#include <QDir>
#include <QFile>

#include "launcher.h"

namespace zrythm::e2e
{

namespace
{

/** Runs --new-project @p parent_dir, then quits after the startup. */
RunResult
run_new_project (const ZrythmLauncher &launcher, const QString &parent_dir)
{
  return launcher.run (
    {
      QStringLiteral ("--dummy"),
      QStringLiteral ("--quit-after-startup"),
      QStringLiteral ("--new-project"),
      parent_dir,
    },
    kQuitAfterStartup);
}

/** Returns the SHA-256 of the file at @p path in hex digits. */
std::string
sha256_of_file (const QString &path)
{
  QFile              file (path);
  QCryptographicHash hash (QCryptographicHash::Sha256);
  hash.addData (&file);
  return to_utf8 (QString::fromLatin1 (hash.result ().toHex ()));
}

} // namespace

class NewProjectTest : public ::testing::Test
{
protected:
  ZrythmLauncher launcher_;
};

// --new-project creates a project that a later run opens
TEST_F (NewProjectTest, CreatesAProjectThatLoads)
{
  const auto parent_dir = launcher_.make_directory (QStringLiteral ("projects"));

  const auto created = run_new_project (launcher_, parent_dir);
  expect_exit_code (created, EXIT_SUCCESS);
  EXPECT_FALSE (has_error_lines (created.standard_output))
    << created.describe ();
  EXPECT_EQ (
    count_lines_holding (
      created.standard_output, QStringLiteral ("a project was requested: true")),
    1)
    << created.describe ();
  const QDir project_dir (parent_dir + QStringLiteral ("/Untitled Project"));
  const auto project_file =
    project_dir.filePath (QStringLiteral ("project.zpj"));
  ASSERT_TRUE (QFile::exists (project_file)) << to_utf8 (project_file);

  const auto opened = launcher_.run (
    {
      QStringLiteral ("--dummy"),
      QStringLiteral ("--quit-after-startup"),
      QStringLiteral ("--project"),
      project_dir.absolutePath (),
    },
    kQuitAfterStartup);
  expect_exit_code (opened, EXIT_SUCCESS);
  EXPECT_FALSE (has_error_lines (opened.standard_output)) << opened.describe ();
  EXPECT_EQ (
    count_lines_holding (
      opened.standard_output, QStringLiteral ("Loading project from")),
    1)
    << opened.describe ();
}

// A second --new-project into the same directory leaves the project as
// it is
TEST_F (NewProjectTest, RefusesToReplaceAnExistingProject)
{
  const auto parent_dir = launcher_.make_directory (QStringLiteral ("projects"));
  const auto first = run_new_project (launcher_, parent_dir);
  ASSERT_NO_FATAL_FAILURE (expect_exit_code (first, EXIT_SUCCESS));

  const auto project_file =
    parent_dir + QStringLiteral ("/Untitled Project/project.zpj");
  const auto hash_before = sha256_of_file (project_file);

  const auto second = run_new_project (launcher_, parent_dir);
  expect_exit_code (second, EXIT_FAILURE);
  EXPECT_EQ (
    count_lines_holding (
      second.standard_output, QStringLiteral ("a project already exists at")),
    1)
    << second.describe ();
  EXPECT_EQ (sha256_of_file (project_file), hash_before);
}

// A parent directory that cannot be created ends the run with a failure
TEST_F (NewProjectTest, ParentThatCannotBeCreatedExitsWithFailure)
{
  const auto file = launcher_.make_file (QStringLiteral ("not-a-directory"));
  const auto result =
    run_new_project (launcher_, file + QStringLiteral ("/projects"));
  expect_exit_code (result, EXIT_FAILURE);
  EXPECT_EQ (
    count_lines_holding (
      result.standard_output, QStringLiteral ("Failed to create project")),
    1)
    << result.describe ();
  EXPECT_EQ (
    count_lines_holding (
      result.standard_output, QStringLiteral ("Startup did not finish within")),
    0)
    << result.describe ();
}

} // namespace zrythm::e2e
