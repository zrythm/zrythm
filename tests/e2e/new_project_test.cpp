// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <cstdlib>
#include <string>

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStringList>

#include "launcher.h"
#include "mcp_client.h"

namespace zrythm::e2e
{

namespace
{

/**
 * Runs --new-project @p parent_dir, then quits after the startup. With
 * @p template_dir, the new project starts from that template.
 */
RunResult
run_new_project (
  const ZrythmLauncher &launcher,
  const QString        &parent_dir,
  const QString        &template_dir = {})
{
  QStringList arguments{
    QStringLiteral ("--dummy"),
    QStringLiteral ("--quit-after-startup"),
    QStringLiteral ("--new-project"),
    parent_dir,
  };
  if (!template_dir.isEmpty ())
    {
      arguments
        << QStringLiteral ("--new-project-with-template") << template_dir;
    }
  return launcher.run (arguments, kQuitAfterStartup);
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

// A project created from a template is saved in the new directory, holds
// the tracks of the template and leaves the template as it is
TEST_F (NewProjectTest, CreatesAProjectFromTemplate)
{
  // The application makes the template and lists its tracks
  const auto templates_dir =
    launcher_.make_directory (QStringLiteral ("templates"));
  const auto template_run = run_and_list_tracks (
    launcher_,
    {
      QStringLiteral ("--dummy"),
      QStringLiteral ("--quit-after-startup"),
      QStringLiteral ("--new-project"),
      templates_dir,
    });
  ASSERT_NO_FATAL_FAILURE (expect_exit_code (template_run.run, EXIT_SUCCESS));
  ASSERT_FALSE (template_run.tracks.isEmpty ())
    << template_run.run.describe () << to_utf8 (template_run.last_answer);
  const QDir template_dir (templates_dir + QStringLiteral ("/Untitled Project"));
  const auto template_entries =
    ZrythmLauncher::tree_entries (template_dir.absolutePath ());

  const auto parent_dir = launcher_.make_directory (QStringLiteral ("projects"));
  const auto created =
    run_new_project (launcher_, parent_dir, template_dir.absolutePath ());
  expect_exit_code (created, EXIT_SUCCESS);
  EXPECT_FALSE (has_error_lines (created.standard_output))
    << created.describe ();

  // The project is saved in the new directory
  const QDir project_dir (parent_dir + QStringLiteral ("/Untitled Project"));
  EXPECT_TRUE (
    QFile::exists (project_dir.filePath (QStringLiteral ("project.zpj"))))
    << to_utf8 (project_dir.absolutePath ());

  // The template is only read
  EXPECT_EQ (
    ZrythmLauncher::tree_entries (template_dir.absolutePath ()),
    template_entries);

  // The saved project holds the tracks of the template
  const auto opened = run_and_list_tracks (
    launcher_,
    {
      QStringLiteral ("--dummy"),
      QStringLiteral ("--quit-after-startup"),
      QStringLiteral ("--project"),
      project_dir.absolutePath (),
    });
  expect_exit_code (opened.run, EXIT_SUCCESS);
  EXPECT_EQ (opened.tracks, template_run.tracks)
    << opened.run.describe ()
    << "tracks of the template: " << template_run.describe_tracks ()
    << "\ntracks of the project: " << opened.describe_tracks ();
}

// An empty template directory ends the run with a failure, and nothing
// is created
TEST_F (NewProjectTest, TemplateWithoutProjectExitsWithFailure)
{
  const auto template_dir =
    launcher_.make_directory (QStringLiteral ("template"));
  const auto parent_dir = launcher_.make_directory (QStringLiteral ("projects"));
  const auto result = run_new_project (launcher_, parent_dir, template_dir);
  expect_exit_code (result, EXIT_FAILURE);
  // The log states that the template directory is empty
  EXPECT_EQ (
    count_lines_holding (
      result.standard_output,
      QStringLiteral ("template directory '%1' is empty").arg (template_dir)),
    1)
    << result.describe ();
  EXPECT_EQ (
    count_lines_holding (
      result.standard_output, QStringLiteral ("Startup did not finish within")),
    0)
    << result.describe ();
  EXPECT_TRUE (ZrythmLauncher::tree_entries (parent_dir).isEmpty ())
    << to_utf8 (parent_dir);
}

// A template whose project file cannot be read ends the run with a
// failure, and nothing is created
TEST_F (NewProjectTest, TemplateThatCannotBeReadExitsWithFailure)
{
  const auto project_file =
    launcher_.make_file (QStringLiteral ("template/project.zpj"));
  QFile file (project_file);
  ASSERT_TRUE (file.open (QIODevice::WriteOnly | QIODevice::Truncate));
  ASSERT_GT (file.write ("not a compressed project"), 0);
  file.close ();
  const auto parent_dir = launcher_.make_directory (QStringLiteral ("projects"));
  const auto template_dir = QFileInfo (project_file).absolutePath ();
  const auto result = run_new_project (launcher_, parent_dir, template_dir);
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
  EXPECT_TRUE (ZrythmLauncher::tree_entries (parent_dir).isEmpty ())
    << to_utf8 (parent_dir);
}

// --new-project-with-template without --new-project is refused before a
// project is created
TEST_F (NewProjectTest, TemplateOptionNeedsNewProject)
{
  const auto template_dir =
    launcher_.make_directory (QStringLiteral ("template"));
  const auto result = launcher_.run (
    {
      QStringLiteral ("--dummy"),
      QStringLiteral ("--quit-after-startup"),
      QStringLiteral ("--new-project-with-template"),
      template_dir,
    },
    kExitsWithoutEventLoop);
  expect_exit_code (result, EXIT_FAILURE);
  EXPECT_TRUE (result.standard_error.contains (
    QStringLiteral ("--new-project-with-template")))
    << result.describe ();
  EXPECT_TRUE (ZrythmLauncher::tree_entries (template_dir).isEmpty ())
    << to_utf8 (template_dir);
}

// --new-project-with-template given twice is refused before a project is
// created
TEST_F (NewProjectTest, TemplateOptionGivenTwiceExitsWithFailure)
{
  const auto parent_dir = launcher_.make_directory (QStringLiteral ("projects"));
  const auto template_dir =
    launcher_.make_directory (QStringLiteral ("template"));
  const auto result = launcher_.run (
    {
      QStringLiteral ("--dummy"),
      QStringLiteral ("--quit-after-startup"),
      QStringLiteral ("--new-project"),
      parent_dir,
      QStringLiteral ("--new-project-with-template"),
      template_dir,
      QStringLiteral ("--new-project-with-template"),
      template_dir,
    },
    kExitsWithoutEventLoop);
  expect_exit_code (result, EXIT_FAILURE);
  EXPECT_TRUE (result.standard_error.contains (
    QStringLiteral ("--new-project-with-template")))
    << result.describe ();
  EXPECT_TRUE (ZrythmLauncher::tree_entries (parent_dir).isEmpty ())
    << to_utf8 (parent_dir);
}

// An empty value of --new-project-with-template is refused before a
// project is created
TEST_F (NewProjectTest, TemplateOptionWithoutDirectoryExitsWithFailure)
{
  const auto parent_dir = launcher_.make_directory (QStringLiteral ("projects"));
  const auto result = launcher_.run (
    {
      QStringLiteral ("--dummy"),
      QStringLiteral ("--quit-after-startup"),
      QStringLiteral ("--new-project"),
      parent_dir,
      QStringLiteral ("--new-project-with-template"),
      QString{},
    },
    kExitsWithoutEventLoop);
  expect_exit_code (result, EXIT_FAILURE);
  EXPECT_TRUE (result.standard_error.contains (
    QStringLiteral ("--new-project-with-template")))
    << result.describe ();
  EXPECT_TRUE (ZrythmLauncher::tree_entries (parent_dir).isEmpty ())
    << to_utf8 (parent_dir);
}

} // namespace zrythm::e2e
