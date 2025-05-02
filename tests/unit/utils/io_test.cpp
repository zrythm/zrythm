// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <fstream>

#include "utils/gtest_wrapper.h"
#include "utils/io.h"

using namespace zrythm;

TEST (IoTest, PathSeparator)
{
  auto sep = zrythm::utils::io::get_path_separator_string ();
#ifdef _WIN32
  EXPECT_EQ (sep, ";");
#else
  EXPECT_EQ (sep, ":");
#endif
}

TEST (IoTest, DirectoryOperations)
{
  // Test directory creation and removal
  auto tmp_dir = zrythm::utils::io::make_tmp_dir ();
  EXPECT_TRUE (fs::exists (tmp_dir->path ().toStdString ()));

  // Test nested directory creation
  auto nested_path =
    fs::path (tmp_dir->path ().toStdString ()) / "a" / "b" / "c";
  EXPECT_NO_THROW (zrythm::utils::io::mkdir (nested_path.string ()));
  EXPECT_TRUE (fs::exists (nested_path));
}

TEST (IoTest, FileOperations)
{
  // Test file creation and removal
  auto tmp_file = zrythm::utils::io::make_tmp_file ();
  EXPECT_TRUE (fs::exists (tmp_file->fileName ().toStdString ()));

  // Test file contents
  std::string test_data = "Hello, World!";
  EXPECT_NO_THROW (zrythm::utils::io::set_file_contents (
    tmp_file->fileName ().toStdString (), test_data));

  auto read_data = zrythm::utils::io::read_file_contents (
    tmp_file->fileName ().toStdString ());
  EXPECT_EQ (read_data, QByteArray::fromStdString (test_data));
}

TEST (IoTest, PathManipulation)
{
  // Test path components
  std::string test_path = "path/to/file.txt";
  EXPECT_TRUE (zrythm::utils::io::get_dir (test_path).ends_with ("path/to"));
  EXPECT_EQ (zrythm::utils::io::file_get_ext (test_path), "txt");
  EXPECT_EQ (zrythm::utils::io::path_get_basename (test_path), "file.txt");
  EXPECT_EQ (
    zrythm::utils::io::path_get_basename_without_ext (test_path), "file");
}

TEST (IoTest, DirectoryListing)
{
  auto tmp_dir = zrythm::utils::io::make_tmp_dir ();

  // Create test files
  fs::path dir_path = tmp_dir->path ().toStdString ();
  EXPECT_NO_THROW (
    zrythm::utils::io::set_file_contents (dir_path / "file1.txt", "test1"));
  EXPECT_NO_THROW (
    zrythm::utils::io::set_file_contents (dir_path / "file2.txt", "test2"));

  // Test directory listing
  auto files = zrythm::utils::io::get_files_in_dir (dir_path.string ());
  EXPECT_EQ (files.size (), 2);

  // Test filtered listing
  auto txt_files = zrythm::utils::io::get_files_in_dir_ending_in (
    dir_path.string (), false, ".txt");
  EXPECT_EQ (txt_files.size (), 2);
}

TEST (IoTest, GetExtension)
{
  auto test_get_ext =
    [] (const std::string &file, const std::string &expected_ext) {
      EXPECT_EQ (zrythm::utils::io::file_get_ext (file), expected_ext);
    };

  test_get_ext ("abc.wav", "wav");
  test_get_ext ("abc.test.wav", "wav");
  test_get_ext ("abctestwav", "");
  test_get_ext ("abctestwav.", "");
  test_get_ext ("...", "");
}

TEST (IoTest, StripExtension)
{
  auto test_strip_ext =
    [] (const std::string &file, const std::string &expected) {
      EXPECT_EQ (zrythm::utils::io::file_strip_ext (file), expected);
    };

  test_strip_ext ("abc.wav", "abc");
  test_strip_ext ("abc.test.wav", "abc.test");
  test_strip_ext ("abctestwav", "abctestwav");
  test_strip_ext ("abctestwav.", "abctestwav");
  test_strip_ext ("...", "..");
}

TEST (IoTest, GetFilesInDirectory)
{
  // Create temporary directory and file
  auto tmp_dir = zrythm::utils::io::make_tmp_dir ();
  auto tmp_file = fs::path (tmp_dir->path ().toStdString ()) / "test.wav";
  std::ofstream (tmp_file) << "test data";

  // Test file listing
  auto files = zrythm::utils::io::get_files_in_dir_ending_in (
    tmp_dir->path ().toStdString (), false, ".wav");
  EXPECT_EQ (files.size (), 1);
  EXPECT_EQ (files[0], tmp_file);

  // Test non-existent directory
  EXPECT_ANY_THROW (
    utils::io::get_files_in_dir_ending_in ("/non-existent", true, ".wav"));
}

TEST (IoTest, QStringConversions)
{
  {
    QString  qstr = "test";
    fs::path path = utils::io::to_fs_path (qstr);
    EXPECT_EQ (path, fs::path ("test"));
  }

  // Test with non-ASCII characters
  {
    QString  qstr = "C:/テスト";
    fs::path path = utils::io::to_fs_path (qstr);
    EXPECT_EQ (path, fs::path (u"C:/テスト"));
  }

  {
    fs::path path = fs::path ("test");
    QString  qstr = utils::io::to_qstring (path);
    EXPECT_EQ (qstr, "test");
  }

  // Test with non-ASCII characters
  {
    fs::path path = fs::path (u"C:/テスト");
    QString  qstr = utils::io::to_qstring (path);
    EXPECT_EQ (qstr, "C:/テスト");
  }
}