// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <filesystem>
#include <memory>

#include "structure/project/project_directory.h"
#include "structure/project/project_path_provider.h"
#include "utils/io_utils.h"

#include <gtest/gtest.h>

namespace zrythm::structure::project
{

class ProjectDirectoryTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    tmp_dir_ = utils::io::make_tmp_dir ();
    tmp_path_ = utils::Utf8String::from_qstring (tmp_dir_->path ()).to_path ();
  }

  std::unique_ptr<QTemporaryDir> tmp_dir_;
  std::filesystem::path          tmp_path_;
};

TEST_F (ProjectDirectoryTest, ClassifiesMissingPathAsNonExistent)
{
  const auto dir = tmp_path_ / "no-such-project";
  EXPECT_EQ (classify_directory (dir), DirectoryContent::NonExistent);
}

TEST_F (ProjectDirectoryTest, ClassifiesEmptyDirectoryAsEmpty)
{
  const auto dir = tmp_path_ / "empty";
  utils::io::mkdir (dir);
  EXPECT_EQ (classify_directory (dir), DirectoryContent::Empty);
}

TEST_F (
  ProjectDirectoryTest,
  ClassifiesDirectoryWithProjectFileAsContainsProjectFile)
{
  const auto dir = tmp_path_ / "song";
  utils::io::mkdir (dir);
  utils::io::set_file_contents (
    dir
      / ProjectPathProvider::get_path (
        ProjectPathProvider::ProjectPath::ProjectFile),
    utils::Utf8String ());
  EXPECT_EQ (classify_directory (dir), DirectoryContent::ContainsProjectFile);
}

TEST_F (ProjectDirectoryTest, ProjectFileWinsOverOtherFilesInClassification)
{
  const auto dir = tmp_path_ / "mixed";
  utils::io::mkdir (dir);
  utils::io::set_file_contents (dir / "notes.txt", utils::Utf8String ());
  utils::io::set_file_contents (
    dir
      / ProjectPathProvider::get_path (
        ProjectPathProvider::ProjectPath::ProjectFile),
    utils::Utf8String ());
  EXPECT_EQ (classify_directory (dir), DirectoryContent::ContainsProjectFile);
}

TEST_F (ProjectDirectoryTest, DirectoryNamedProjectFileIsOtherContent)
{
  const auto dir = tmp_path_ / "weird";
  utils::io::mkdir (dir);
  utils::io::mkdir (
    dir
    / ProjectPathProvider::get_path (
      ProjectPathProvider::ProjectPath::ProjectFile));
  EXPECT_EQ (classify_directory (dir), DirectoryContent::OtherContent);
}

TEST_F (ProjectDirectoryTest, ClassifiesDirectoryWithOtherFilesAsOtherContent)
{
  const auto dir = tmp_path_ / "photos";
  utils::io::mkdir (dir);
  utils::io::set_file_contents (dir / "img_001.jpg", utils::Utf8String ());
  EXPECT_EQ (classify_directory (dir), DirectoryContent::OtherContent);
}

TEST_F (ProjectDirectoryTest, ClassifiesNonDirectoryPathAsOtherContent)
{
  const auto file = tmp_path_ / "plain-file";
  utils::io::set_file_contents (file, utils::Utf8String ());
  EXPECT_EQ (classify_directory (file), DirectoryContent::OtherContent);
}

TEST_F (ProjectDirectoryTest, SaveTargetIgnoresTrailingSeparatorOnParent)
{
  const auto parent = tmp_path_ / "songs";
  utils::io::mkdir (parent / "My Song");
  utils::io::set_file_contents (
    parent / "My Song"
      / ProjectPathProvider::get_path (
        ProjectPathProvider::ProjectPath::ProjectFile),
    utils::Utf8String ());
  EXPECT_EQ (
    classify_save_target (parent / "", u8"My Song"),
    DirectoryContent::ContainsProjectFile);
  EXPECT_EQ (
    classify_save_target (tmp_path_ / "nope" / "", u8"My Song"),
    DirectoryContent::NonExistent);
}

} // namespace zrythm::structure::project
