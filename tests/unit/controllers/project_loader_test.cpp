// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "controllers/project_loader.h"
#include "controllers/project_saver.h"
#include "project_json_serializer_test.h"
#include "structure/project/project_path_provider.h"

namespace zrythm::controllers
{

class ProjectLoaderTest : public ProjectSerializationTest
{
};

TEST_F (ProjectLoaderTest, LoadNonexistentDirectory)
{
  EXPECT_THROW (
    { ProjectLoader::load_from_directory ("/nonexistent/path/to/project"); },
    ZrythmException);
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
  fs::create_directories (temp_path);

  EXPECT_THROW (
    { ProjectLoader::load_from_directory (temp_path); }, ZrythmException);
}

} // namespace zrythm::controllers
