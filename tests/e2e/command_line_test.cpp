// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <cstdlib>

#include "launcher.h"

namespace zrythm::e2e
{

class CommandLineTest : public ::testing::Test
{
protected:
  ZrythmLauncher launcher_;
};

// --version prints the name and the version and exits with success
TEST_F (CommandLineTest, VersionExitsWithSuccess)
{
  const auto result =
    launcher_.run ({ QStringLiteral ("--version") }, kExitsWithoutEventLoop);
  expect_exit_code (result, EXIT_SUCCESS);
  EXPECT_TRUE (result.standard_output.startsWith (QStringLiteral ("Zrythm ")))
    << result.describe ();
  EXPECT_TRUE (result.standard_output.contains (
    QString::fromUtf8 (ZRYTHM_E2E_APP_VERSION)))
    << result.describe ();
}

// --help lists the options of the application
TEST_F (CommandLineTest, HelpListsTheOptions)
{
  const auto result =
    launcher_.run ({ QStringLiteral ("--help") }, kExitsWithoutEventLoop);
  expect_exit_code (result, EXIT_SUCCESS);
  for (
    const auto &option : {
      "--project <project>",
      "--new-project <directory>",
      "--user-data-dir <directory>",
      "--dummy",
      "--quit-after-startup",
    })
    {
      EXPECT_TRUE (result.standard_output.contains (QString::fromUtf8 (option)))
        << result.describe ();
    }
}

// --verify-installation passes in a complete build tree
TEST_F (CommandLineTest, VerifyInstallationExitsWithSuccess)
{
  const auto result = launcher_.run (
    { QStringLiteral ("--verify-installation") }, kExitsWithoutEventLoop);
  expect_exit_code (result, EXIT_SUCCESS);
  EXPECT_TRUE (
    result.standard_output.contains (QStringLiteral ("installation OK")))
    << result.describe ();
}

} // namespace zrythm::e2e
