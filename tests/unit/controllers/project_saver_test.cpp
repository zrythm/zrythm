// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "controllers/project_saver.h"
#include "project_json_serializer_test.h"
#include "structure/project/project_path_provider.h"

#include <QEventLoop>
#include <QFutureWatcher>

namespace zrythm::controllers
{

/// Helper to wait for a QFuture while processing Qt events
template <typename T>
void
waitForFutureWithEvents (QFuture<T> &future, int timeout_ms = 30000)
{
  QFutureWatcher<T> watcher;
  QEventLoop        loop;

  QObject::connect (
    &watcher, &QFutureWatcher<T>::finished, &loop, &QEventLoop::quit);

  watcher.setFuture (future);

  // Timeout to prevent infinite hang
  QTimer::singleShot (timeout_ms, &loop, &QEventLoop::quit);

  loop.exec ();
}

class ProjectSaverTest : public ProjectSerializationTest
{
};

TEST_F (ProjectSaverTest, CompressAndDecompress)
{
  const std::string test_data = "This is test data for compression";

  QByteArray src_data = QByteArray::fromRawData (
    test_data.c_str (), static_cast<qsizetype> (test_data.length ()));

  char * compressed_data = nullptr;
  size_t compressed_size = 0;

  EXPECT_NO_THROW ({
    ProjectSaver::compress (&compressed_data, &compressed_size, src_data);
  });

  EXPECT_NE (compressed_data, nullptr);
  EXPECT_GT (compressed_size, 0);

  // Now decompress
  QByteArray compressed_ba (
    compressed_data, static_cast<qsizetype> (compressed_size));
  char * decompressed_data = nullptr;
  size_t decompressed_size = 0;

  EXPECT_NO_THROW ({
    ProjectSaver::decompress (
      &decompressed_data, &decompressed_size, compressed_ba);
  });

  EXPECT_NE (decompressed_data, nullptr);
  EXPECT_EQ (decompressed_size, test_data.length ());

  std::string result (decompressed_data, decompressed_size);
  EXPECT_EQ (result, test_data);

  free (compressed_data);
  free (decompressed_data);
}

TEST_F (ProjectSaverTest, CompressEmptyData)
{
  QByteArray empty_data;

  char * compressed_data = nullptr;
  size_t compressed_size = 0;

  EXPECT_NO_THROW ({
    ProjectSaver::compress (&compressed_data, &compressed_size, empty_data);
  });

  free (compressed_data);
}

TEST_F (ProjectSaverTest, DecompressInvalidData)
{
  // Invalid compressed data
  char       invalid_data[] = "not valid compressed data";
  QByteArray invalid_ba (invalid_data, sizeof (invalid_data));

  char * decompressed_data = nullptr;
  size_t decompressed_size = 0;

  EXPECT_THROW (
    {
      ProjectSaver::decompress (
        &decompressed_data, &decompressed_size, invalid_ba);
    },
    ZrythmException);
}

TEST_F (ProjectSaverTest, MakeProjectDirs)
{
  auto test_project_dir = project_dir / "test_dirs";

  EXPECT_NO_THROW ({ ProjectSaver::make_project_dirs (test_project_dir); });

  // Verify directories were created
  EXPECT_TRUE (fs::exists (test_project_dir));

  const auto expected_dirs = {
    structure::project::ProjectPathProvider::ProjectPath::BackupsDir,
    structure::project::ProjectPathProvider::ProjectPath::ExportsDir,
    structure::project::ProjectPathProvider::ProjectPath::ExportStemsDir,
    structure::project::ProjectPathProvider::ProjectPath::AudioFilePoolDir,
    structure::project::ProjectPathProvider::ProjectPath::PluginStates,
    structure::project::ProjectPathProvider::ProjectPath::PLUGIN_EXT_COPIES,
    structure::project::ProjectPathProvider::ProjectPath::PLUGIN_EXT_LINKS,
  };

  for (auto dir_type : expected_dirs)
    {
      auto dir_path =
        test_project_dir
        / structure::project::ProjectPathProvider::get_path (dir_type);
      EXPECT_TRUE (fs::exists (dir_path))
        << "Directory not created: " << dir_path;
    }
}

TEST_F (ProjectSaverTest, GetExistingUncompressedTextNonexistentFile)
{
  auto nonexistent_dir = project_dir / "nonexistent";

  EXPECT_THROW (
    { ProjectSaver::get_existing_uncompressed_text (nonexistent_dir); },
    ZrythmException);
}

TEST_F (ProjectSaverTest, CompressDecompressRoundTrip)
{
  // Create a minimal project JSON
  auto        json = create_minimal_valid_project_json ();
  std::string json_str = json.dump (2);

  // Compress
  QByteArray src_data = QByteArray::fromRawData (
    json_str.c_str (), static_cast<qsizetype> (json_str.length ()));
  char * compressed_data = nullptr;
  size_t compressed_size = 0;

  EXPECT_NO_THROW ({
    ProjectSaver::compress (&compressed_data, &compressed_size, src_data);
  });

  // Write to file
  auto test_dir = project_dir / "roundtrip_test";
  ProjectSaver::make_project_dirs (test_dir);

  auto project_file_path =
    test_dir
    / structure::project::ProjectPathProvider::get_path (
      structure::project::ProjectPathProvider::ProjectPath::ProjectFile);
  utils::io::set_file_contents (
    project_file_path, compressed_data, compressed_size);
  free (compressed_data);

  // Read back and decompress
  EXPECT_NO_THROW ({
    auto text = ProjectSaver::get_existing_uncompressed_text (test_dir);
    EXPECT_FALSE (text.empty ());

    // Verify it's valid JSON with expected content
    auto parsed = nlohmann::json::parse (text);
    EXPECT_EQ (parsed["documentType"].get<std::string> (), "ZrythmProject");
    EXPECT_EQ (parsed["title"].get<std::string> (), "Test Project");
  });
}

TEST_F (ProjectSaverTest, SaveCreatesProjectFile)
{
  auto project = create_minimal_project ();
  create_ui_state_and_undo_stack (*project);

  auto save_dir = project_dir / "save_test";
  fs::create_directories (save_dir);

  // Start the engine (required for save to work)
  project->engine ()->set_running (true);

  auto future = ProjectSaver::save (
    *project, *ui_state, *undo_stack, TEST_APP_VERSION, save_dir, false);

  // Wait for the save to complete while processing Qt events
  waitForFutureWithEvents (future);

  ASSERT_TRUE (future.isFinished ()) << "Save operation timed out";

  auto result = future.result ();
  EXPECT_FALSE (result.isEmpty ());

  // Verify the result is the path
  EXPECT_EQ (
    result.toStdString (),
    utils::Utf8String::from_path (save_dir).to_qstring ().toStdString ());

  // Verify project file was created
  auto project_file_path =
    save_dir
    / structure::project::ProjectPathProvider::get_path (
      structure::project::ProjectPathProvider::ProjectPath::ProjectFile);
  EXPECT_TRUE (fs::exists (project_file_path))
    << "Project file not created: " << project_file_path;

  // Verify we can decompress and read the file
  EXPECT_NO_THROW ({
    auto text = ProjectSaver::get_existing_uncompressed_text (save_dir);
    EXPECT_FALSE (text.empty ());

    // Verify it's valid JSON
    auto json = nlohmann::json::parse (text);
    EXPECT_EQ (json["documentType"].get<std::string> (), "ZrythmProject");
  });
}

TEST_F (ProjectSaverTest, SaveReturnsProjectPath)
{
  auto project = create_minimal_project ();
  create_ui_state_and_undo_stack (*project);

  auto save_dir = project_dir / "path_test_project";
  fs::create_directories (save_dir);

  project->engine ()->set_running (true);

  auto future = ProjectSaver::save (
    *project, *ui_state, *undo_stack, TEST_APP_VERSION, save_dir, false);

  // Wait for the save to complete while processing Qt events
  waitForFutureWithEvents (future);

  ASSERT_TRUE (future.isFinished ()) << "Save operation timed out";

  auto result = future.result ();

  // The result should be the path
  EXPECT_EQ (
    result.toStdString (),
    utils::Utf8String::from_path (save_dir).to_qstring ().toStdString ());
}

TEST_F (ProjectSaverTest, SaveTwiceToSameDirectory)
{
  auto project = create_minimal_project ();
  create_ui_state_and_undo_stack (*project);

  auto save_dir = project_dir / "save_twice_test";
  fs::create_directories (save_dir);

  project->engine ()->set_running (true);

  // First save
  {
    auto future = ProjectSaver::save (
      *project, *ui_state, *undo_stack, TEST_APP_VERSION, save_dir, false);
    waitForFutureWithEvents (future);
    ASSERT_TRUE (future.isFinished ()) << "First save timed out";
    EXPECT_FALSE (future.result ().isEmpty ());
  }

  // Second save to the same directory should also succeed
  {
    auto future = ProjectSaver::save (
      *project, *ui_state, *undo_stack, TEST_APP_VERSION, save_dir, false);
    waitForFutureWithEvents (future);
    ASSERT_TRUE (future.isFinished ()) << "Second save timed out";
    EXPECT_FALSE (future.result ().isEmpty ());
  }

  // Verify project file still exists and is valid
  auto project_file_path =
    save_dir
    / structure::project::ProjectPathProvider::get_path (
      structure::project::ProjectPathProvider::ProjectPath::ProjectFile);
  EXPECT_TRUE (fs::exists (project_file_path));
}

} // namespace zrythm::controllers
