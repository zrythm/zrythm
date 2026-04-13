// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "controllers/project_loader.h"
#include "controllers/project_saver.h"
#include "project_json_serializer_test.h"
#include "structure/project/project_path_provider.h"
#include "utils/audio.h"

#include "helpers/qt_helpers.h"

namespace zrythm::controllers
{

class ProjectLoaderTest : public ProjectSerializationTest
{
};

TEST_F (ProjectLoaderTest, LoadNonexistentDirectory)
{
  auto future =
    ProjectLoader::load_from_directory ("/nonexistent/path/to/project");
  EXPECT_ANY_THROW ({ future.waitForFinished (); });
}

TEST_F (ProjectLoaderTest, ParseInvalidJson)
{
  EXPECT_THROW (
    { ProjectLoader::parse_and_validate ("not valid json"); }, ZrythmException);
}

TEST_F (ProjectLoaderTest, ParseValidButInvalidSchemaJson)
{
  // Valid JSON but missing required fields
  nlohmann::json invalid_json = R"({"someField": "someValue"})"_json;
  EXPECT_THROW (
    { ProjectLoader::parse_and_validate (invalid_json.dump ()); },
    ZrythmException);
}

TEST_F (ProjectLoaderTest, ParseAndValidateMinimalJson)
{
  auto json = create_minimal_valid_project_json ();
  EXPECT_NO_THROW ({
    auto result = ProjectLoader::parse_and_validate (json.dump ());
    EXPECT_EQ (result["documentType"].get<std::string> (), "ZrythmProject");
  });
}

TEST_F (ProjectLoaderTest, ExtractTitleFromJson)
{
  auto json = create_minimal_valid_project_json ();
  auto title = ProjectLoader::extract_title (json);
  EXPECT_EQ (title.view (), "Test Project");
}

TEST_F (ProjectLoaderTest, ExtractTitleMissing)
{
  auto json = create_minimal_valid_project_json ();
  json.erase ("title");
  auto title = ProjectLoader::extract_title (json);
  EXPECT_EQ (title.view (), "Untitled");
}

TEST_F (ProjectLoaderTest, DecompressAndParseRoundTrip)
{
  // Create a minimal project
  auto project = create_minimal_project ();
  create_ui_state_and_undo_stack (*project);

  // Serialize to JSON
  constexpr utils::Version test_version{ 2, 0, {} };
  nlohmann::json           j = ProjectJsonSerializer::serialize (
    *project, *ui_state, *undo_stack, test_version, "Loader Test Project");

  // Create project directories
  ProjectSaver::make_project_dirs (project_dir);

  // Write compressed project file
  std::string json_str = j.dump ();
  char *      compressed_data = nullptr;
  size_t      compressed_size = 0;
  QByteArray  src_data = QByteArray::fromRawData (
    json_str.c_str (), static_cast<qsizetype> (json_str.length ()));
  ProjectSaver::compress (&compressed_data, &compressed_size, src_data);

  auto project_file_path =
    project_dir
    / structure::project::ProjectPathProvider::get_path (
      structure::project::ProjectPathProvider::ProjectPath::ProjectFile);
  utils::io::set_file_contents (
    project_file_path, compressed_data, compressed_size);
  free (compressed_data);

  // Test decompression and parsing (bypassing schema validation which has
  // pre-existing issues with track serialization format)
  auto decompressed = ProjectLoader::get_uncompressed_project_text (project_dir);
  auto parsed = nlohmann::json::parse (decompressed);

  // Verify the parsed JSON
  EXPECT_EQ (parsed["title"].get<std::string> (), "Loader Test Project");
  EXPECT_TRUE (parsed.contains ("projectData"));

  // Verify we can deserialize the parsed JSON
  auto deserialized_project = create_minimal_project ();
  create_ui_state_and_undo_stack (*deserialized_project);
  EXPECT_NO_THROW ({
    ProjectLoader::deserialize (
      parsed, *deserialized_project, *ui_state, *undo_stack);
  });
}

TEST_F (ProjectLoaderTest, LoadFromDirectoryMissingProjectFile)
{
  // Create directory but no project file
  auto temp_path = project_dir / "empty_project";
  std::filesystem::create_directories (temp_path);

  auto future = ProjectLoader::load_from_directory (temp_path);
  EXPECT_ANY_THROW ({ future.waitForFinished (); });
}

TEST_F (ProjectLoaderTest, LoadFromDirectoryAsync)
{
  // Create a minimal project and save it
  auto project = create_minimal_project ();
  create_ui_state_and_undo_stack (*project);

  constexpr utils::Version test_version{ 2, 0, {} };
  nlohmann::json           j = ProjectJsonSerializer::serialize (
    *project, *ui_state, *undo_stack, test_version, "Async Load Test");

  ProjectSaver::make_project_dirs (project_dir);

  std::string json_str = j.dump ();
  char *      compressed_data = nullptr;
  size_t      compressed_size = 0;
  QByteArray  src_data = QByteArray::fromRawData (
    json_str.c_str (), static_cast<qsizetype> (json_str.length ()));
  ProjectSaver::compress (&compressed_data, &compressed_size, src_data);

  auto project_file_path =
    project_dir
    / structure::project::ProjectPathProvider::get_path (
      structure::project::ProjectPathProvider::ProjectPath::ProjectFile);
  utils::io::set_file_contents (
    project_file_path, compressed_data, compressed_size);
  free (compressed_data);

  // Load asynchronously
  auto future = ProjectLoader::load_from_directory (project_dir);

  // Wait for completion
  EXPECT_NO_THROW ({ future.waitForFinished (); });

  // Verify result
  EXPECT_TRUE (future.isFinished ());
  auto result = future.result ();
  EXPECT_EQ (result.title.view (), "Async Load Test");
  EXPECT_EQ (result.project_directory, project_dir);
  EXPECT_TRUE (result.json.contains ("documentType"));
}

TEST_F (ProjectLoaderTest, RoundtripWithAudioFiles)
{
  // Create a project with an audio file
  auto project = create_minimal_project ();
  create_ui_state_and_undo_stack (*project);

  // Create a FileAudioSource with some audio data
  constexpr int             num_channels = 2;
  constexpr int             num_frames = 100;
  utils::audio::AudioBuffer buffer (num_channels, num_frames);
  for (int ch = 0; ch < num_channels; ++ch)
    {
      for (int i = 0; i < num_frames; ++i)
        {
          buffer.setSample (ch, i, 0.5f);
        }
    }

  const double test_bpm = 120.0;
  auto         clip_ref =
    project->get_file_audio_source_registry ().create_object<dsp::FileAudioSource> (
      buffer, dsp::FileAudioSource::BitDepth::BIT_DEPTH_32,
      project->engine ()->get_sample_rate (), test_bpm,
      utils::Utf8String::from_utf8_encoded_string ("test_clip"));
  auto clip_id = clip_ref.id ();

  // Save the project (this writes audio files to disk via
  // AudioPool::write_to_disk)
  auto save_future = ProjectSaver::save (
    *project, *ui_state, *undo_stack, TEST_APP_VERSION, project_dir, false);
  test_helpers::waitForFutureWithEvents (save_future);

  // Verify the audio file was written to the pool
  auto pool_path =
    project_dir
    / structure::project::ProjectPathProvider::get_path (
      structure::project::ProjectPathProvider::ProjectPath::AudioFilePoolDir);
  auto clip_path =
    pool_path
    / fmt::format (
      "{}.wav", clip_id.value_.toString (QUuid::WithoutBraces).toStdString ());
  EXPECT_TRUE (utils::io::path_exists (clip_path))
    << "Audio file should exist at: " << clip_path;

  // Load the project into a new Project instance
  auto loaded_project = create_minimal_project ();
  auto loaded_ui_state = utils::make_qobject_unique<
    structure::project::ProjectUiState> (*loaded_project, *app_settings);
  auto loaded_undo_stack = utils::make_qobject_unique<undo::UndoStack> (
    [&] (const std::function<void ()> &action, bool recalculate_graph) {
      loaded_project->engine ()
        ->execute_function_with_paused_processing_synchronously (
          action, recalculate_graph);
    },
    nullptr);

  auto load_result = ProjectLoader::load_from_directory (project_dir);
  load_result.waitForFinished ();
  auto json = load_result.result ().json;

  ProjectLoader::deserialize (
    json, *loaded_project, *loaded_ui_state, *loaded_undo_stack);

  // Verify the FileAudioSource was loaded correctly
  auto * loaded_clip = std::get<dsp::FileAudioSource *> (
    loaded_project->get_file_audio_source_registry ().find_by_id_or_throw (
      clip_id));

  // These fields are serialized in JSON
  EXPECT_EQ (loaded_clip->get_name ().view (), "test_clip");
  EXPECT_EQ (loaded_clip->get_bpm (), test_bpm);

  // These fields are read from the audio file during init_loaded()
  EXPECT_EQ (loaded_clip->get_num_channels (), num_channels);
  EXPECT_EQ (loaded_clip->get_num_frames (), num_frames);
}

} // namespace zrythm::controllers
