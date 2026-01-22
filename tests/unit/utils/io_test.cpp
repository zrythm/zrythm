// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <fstream>

#include "utils/io_utils.h"

#include <gtest/gtest.h>

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
  EXPECT_TRUE (
    fs::exists (utils::Utf8String::from_qstring (tmp_dir->path ()).to_path ()));

  // Test nested directory creation
  auto nested_path =
    utils::Utf8String::from_qstring (tmp_dir->path ()).to_path () / u8"a"
    / u8"b" / u8"c";
  EXPECT_NO_THROW (utils::io::mkdir (nested_path));
  EXPECT_TRUE (fs::exists (nested_path));
}

TEST (IoTest, FileOperations)
{
  // Test file creation and removal
  auto tmp_file = zrythm::utils::io::make_tmp_file ();
  EXPECT_TRUE (
    fs::exists (
      utils::Utf8String::from_qstring (tmp_file->fileName ()).to_path ()));

  // Test file contents
  utils::Utf8String test_data = u8"Hello, World!";
  EXPECT_NO_THROW (
    zrythm::utils::io::set_file_contents (
      utils::Utf8String::from_qstring (tmp_file->fileName ()).to_path (),
      test_data));

  auto read_data = zrythm::utils::io::read_file_contents (
    utils::Utf8String::from_qstring (tmp_file->fileName ()).to_path ());
  EXPECT_EQ (read_data, QByteArray::fromStdString (test_data.str ()));
}

TEST (IoTest, PathManipulation)
{
  // Test path components
  std::string test_path = "path/to/file.txt";
  EXPECT_TRUE (utils::io::get_dir (test_path).string ().ends_with ("path/to"));
  EXPECT_EQ (zrythm::utils::io::file_get_ext (test_path), "txt");
  EXPECT_EQ (zrythm::utils::io::path_get_basename (test_path), "file.txt");
  EXPECT_EQ (
    zrythm::utils::io::path_get_basename_without_ext (test_path), "file");
}

TEST (IoTest, DirectoryListing)
{
  auto tmp_dir = zrythm::utils::io::make_tmp_dir ();

  // Create test files
  fs::path dir_path =
    utils::Utf8String::from_qstring (tmp_dir->path ()).to_path ();
  EXPECT_NO_THROW (
    zrythm::utils::io::set_file_contents (dir_path / "file1.txt", u8"test1"));
  EXPECT_NO_THROW (
    zrythm::utils::io::set_file_contents (dir_path / "file2.txt", u8"test2"));

  // Test directory listing
  auto files = zrythm::utils::io::get_files_in_dir (dir_path.string ());
  EXPECT_EQ (files.size (), 2);

  // Test filtered listing
  auto txt_files =
    zrythm::utils::io::get_files_in_dir_ending_in (dir_path, false, u8".txt");
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
  auto tmp_file =
    utils::Utf8String::from_qstring (tmp_dir->path ()).to_path () / "test.wav";
  std::ofstream (tmp_file) << "test data";

  // Test file listing
  auto files = zrythm::utils::io::get_files_in_dir_ending_in (
    utils::Utf8String::from_qstring (tmp_dir->path ()).to_path (), false,
    u8".wav");
  EXPECT_EQ (files.size (), 1);
  EXPECT_EQ (files[0], tmp_file);

  // Test non-existent directory
  EXPECT_ANY_THROW (
    utils::io::get_files_in_dir_ending_in ("/non-existent", true, u8".wav"));
}

TEST (IoTest, QStringConversions)
{
  {
    QString  qstr = "test";
    fs::path path = utils::Utf8String::from_qstring (qstr).to_path ();
    EXPECT_EQ (path, fs::path ("test"));
  }

  // Test with non-ASCII characters
  {
    QString  qstr = "C:/テスト";
    fs::path path = utils::Utf8String::from_qstring (qstr).to_path ();
    EXPECT_EQ (path, fs::path (u"C:/テスト"));
  }

  {
    fs::path path = fs::path ("test");
    QString  qstr = utils::Utf8String::from_path (path).to_qstring ();
    EXPECT_EQ (qstr, "test");
  }

  // Test with non-ASCII characters
  {
    fs::path path = fs::path (u"C:/テスト");
    QString  qstr = utils::Utf8String::from_path (path).to_qstring ();
    EXPECT_EQ (qstr, "C:/テスト");
  }
}

TEST (IoTest, MoveFile)
{
  // Test successful file move
  {
    auto tmp_dir = zrythm::utils::io::make_tmp_dir ();
    auto dir_path =
      utils::Utf8String::from_qstring (tmp_dir->path ()).to_path ();

    auto src_file = dir_path / "source.txt";
    auto dest_file = dir_path / "destination.txt";

    // Create source file with content
    utils::Utf8String test_data = u8"Test content for move";
    EXPECT_NO_THROW (utils::io::set_file_contents (src_file, test_data));
    EXPECT_TRUE (fs::exists (src_file));

    // Move file
    EXPECT_NO_THROW (utils::io::move_file (dest_file, src_file));

    // Verify source is removed and destination exists
    EXPECT_FALSE (fs::exists (src_file));
    EXPECT_TRUE (fs::exists (dest_file));

    // Verify content is preserved
    auto read_data = utils::io::read_file_contents (dest_file);
    EXPECT_EQ (read_data, QByteArray::fromStdString (test_data.str ()));
  }

  // Test moving to non-existent directory (should fail)
  {
    auto tmp_dir = zrythm::utils::io::make_tmp_dir ();
    auto dir_path =
      utils::Utf8String::from_qstring (tmp_dir->path ()).to_path ();

    auto src_file = dir_path / "source.txt";
    auto dest_file = dir_path / "nonexistent" / "destination.txt";

    // Create source file
    utils::io::set_file_contents (src_file, u8"test");
    EXPECT_TRUE (fs::exists (src_file));

    // Attempt to move to non-existent directory
    EXPECT_THROW (utils::io::move_file (dest_file, src_file), ZrythmException);

    // Source file should still exist
    EXPECT_TRUE (fs::exists (src_file));
  }

  // Test moving non-existent source file (should fail)
  {
    auto tmp_dir = zrythm::utils::io::make_tmp_dir ();
    auto dir_path =
      utils::Utf8String::from_qstring (tmp_dir->path ()).to_path ();

    auto src_file = dir_path / "nonexistent.txt";
    auto dest_file = dir_path / "destination.txt";

    EXPECT_THROW (utils::io::move_file (dest_file, src_file), ZrythmException);
  }

  // Test moving file with UTF-8 content
  {
    auto tmp_dir = zrythm::utils::io::make_tmp_dir ();
    auto dir_path =
      utils::Utf8String::from_qstring (tmp_dir->path ()).to_path ();

    auto src_file = dir_path / "source_utf8.txt";
    auto dest_file = dir_path / "dest_utf8.txt";

    // Create source file with UTF-8 content
    utils::Utf8String test_data = u8"Hello, 世界! Γεια σας! Привет!";
    EXPECT_NO_THROW (utils::io::set_file_contents (src_file, test_data));

    // Move file
    EXPECT_NO_THROW (utils::io::move_file (dest_file, src_file));

    // Verify content is preserved
    auto read_data = utils::io::read_file_contents (dest_file);
    EXPECT_EQ (read_data, QByteArray::fromStdString (test_data.str ()));
  }
}
