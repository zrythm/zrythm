// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "project_json_serializer_test.h"

namespace zrythm::structure::project
{

// ============================================================================
// Top-Level Document Structure Tests
// ============================================================================

TEST_F (ProjectSerializationTest, SerializeProject_TopLevelFields)
{
  auto project = create_minimal_project ();
  ASSERT_NE (project, nullptr);

  const std::string test_title = "Test Serialization Project";

  nlohmann::json j =
    ProjectJsonSerializer::serialize (*project, TEST_APP_VERSION, test_title);

  EXPECT_EQ (j["documentType"].get<std::string> (), "ZrythmProject");
  EXPECT_EQ (j["schemaVersion"]["major"].get<int> (), 2);
  EXPECT_EQ (j["schemaVersion"]["minor"].get<int> (), 1);
  EXPECT_EQ (j["appVersion"]["major"].get<int> (), 2);
  EXPECT_EQ (j["appVersion"]["minor"].get<int> (), 0);
  EXPECT_EQ (j["title"].get<std::string> (), test_title);

  EXPECT_TRUE (j.contains ("datetime"));
  auto datetime = j["datetime"].get<std::string> ();
  EXPECT_FALSE (datetime.empty ());
  EXPECT_TRUE (datetime.find ('T') != std::string::npos);
}

TEST_F (ProjectSerializationTest, SerializeProject_ProjectDataExists)
{
  auto project = create_minimal_project ();
  ASSERT_NE (project, nullptr);

  nlohmann::json j =
    ProjectJsonSerializer::serialize (*project, TEST_APP_VERSION, "Test");

  EXPECT_TRUE (j.contains ("projectData"));
  EXPECT_TRUE (j["projectData"].is_object ());
}

// ============================================================================
// Required Fields Tests
// ============================================================================

TEST_F (ProjectSerializationTest, SerializeProject_RequiredFields)
{
  auto project = create_minimal_project ();
  ASSERT_NE (project, nullptr);

  nlohmann::json j =
    ProjectJsonSerializer::serialize (*project, TEST_APP_VERSION, "Test");
  auto &pd = j["projectData"];

  EXPECT_TRUE (pd.contains ("tempoMap"));
  EXPECT_TRUE (pd.contains ("transport"));
  EXPECT_TRUE (pd.contains ("tracklist"));
  EXPECT_TRUE (pd.contains ("registries"));
}

// ============================================================================
// TempoMap Structure Tests
// ============================================================================

TEST_F (ProjectSerializationTest, SerializeProject_TempoMapStructure)
{
  auto project = create_minimal_project ();
  ASSERT_NE (project, nullptr);

  nlohmann::json j =
    ProjectJsonSerializer::serialize (*project, TEST_APP_VERSION, "Test");
  auto &tempo_map = j["projectData"]["tempoMap"];

  EXPECT_TRUE (tempo_map.contains ("timeSignatures"));
  EXPECT_TRUE (tempo_map["timeSignatures"].is_array ());
  EXPECT_TRUE (tempo_map.contains ("tempoChanges"));
  EXPECT_TRUE (tempo_map["tempoChanges"].is_array ());
}

// ============================================================================
// Transport Structure Tests
// ============================================================================

TEST_F (ProjectSerializationTest, SerializeProject_TransportStructure)
{
  auto project = create_minimal_project ();
  ASSERT_NE (project, nullptr);

  nlohmann::json j =
    ProjectJsonSerializer::serialize (*project, TEST_APP_VERSION, "Test");
  auto &transport = j["projectData"]["transport"];

  EXPECT_TRUE (transport.contains ("playheadPosition"));
  EXPECT_TRUE (transport.contains ("cuePosition"));
  EXPECT_TRUE (transport.contains ("loopStartPosition"));
  EXPECT_TRUE (transport.contains ("loopEndPosition"));
  EXPECT_TRUE (transport.contains ("punchInPosition"));
  EXPECT_TRUE (transport.contains ("punchOutPosition"));
}

// ============================================================================
// Tracklist Structure Tests
// ============================================================================

TEST_F (ProjectSerializationTest, SerializeProject_TracklistStructure)
{
  auto project = create_minimal_project ();
  ASSERT_NE (project, nullptr);

  nlohmann::json j =
    ProjectJsonSerializer::serialize (*project, TEST_APP_VERSION, "Test");
  auto &tracklist = j["projectData"]["tracklist"];

  EXPECT_TRUE (tracklist.contains ("tracks"));
  EXPECT_TRUE (tracklist["tracks"].is_array ());
  EXPECT_TRUE (tracklist.contains ("pinnedTracksCutoff"));
  EXPECT_TRUE (tracklist["pinnedTracksCutoff"].is_number ());
  EXPECT_TRUE (tracklist.contains ("trackRoutes"));
  EXPECT_TRUE (tracklist["trackRoutes"].is_array ());
}

TEST_F (ProjectSerializationTest, SerializeProject_TracklistTracksAreUuids)
{
  auto project = create_minimal_project ();
  ASSERT_NE (project, nullptr);

  nlohmann::json j =
    ProjectJsonSerializer::serialize (*project, TEST_APP_VERSION, "Test");
  auto &tracks = j["projectData"]["tracklist"]["tracks"];

  for (const auto &track_id : tracks)
    {
      EXPECT_TRUE (track_id.is_string ());
      assert_valid_uuid (track_id.get<std::string> ());
    }
}

TEST_F (ProjectSerializationTest, SerializeProject_TrackRoutesAreUuidPairs)
{
  auto project = create_minimal_project ();
  ASSERT_NE (project, nullptr);

  nlohmann::json j =
    ProjectJsonSerializer::serialize (*project, TEST_APP_VERSION, "Test");
  auto &routes = j["projectData"]["tracklist"]["trackRoutes"];

  for (const auto &route : routes)
    {
      EXPECT_TRUE (route.is_array ());
      EXPECT_EQ (route.size (), 2);
      EXPECT_TRUE (route[0].is_string ());
      assert_valid_uuid (route[0].get<std::string> ());
      EXPECT_TRUE (route[1].is_string ());
      assert_valid_uuid (route[1].get<std::string> ());
    }
}

TEST_F (ProjectSerializationTest, SerializeProject_FolderParentsFormat)
{
  auto project = create_minimal_project ();
  ASSERT_NE (project, nullptr);

  nlohmann::json j =
    ProjectJsonSerializer::serialize (*project, TEST_APP_VERSION, "Test");

  auto &tracklist = j["projectData"]["tracklist"];
  if (tracklist.contains ("folderParents"))
    {
      auto &folder_parents = tracklist["folderParents"];
      EXPECT_TRUE (folder_parents.is_array ());

      for (const auto &entry : folder_parents)
        {
          EXPECT_TRUE (entry.is_array ());
          EXPECT_EQ (entry.size (), 2);
          EXPECT_TRUE (entry[0].is_string ());
          EXPECT_TRUE (entry[1].is_string ());
          assert_valid_uuid (entry[0].get<std::string> ());
          assert_valid_uuid (entry[1].get<std::string> ());
        }
    }
}

TEST_F (ProjectSerializationTest, SerializeProject_ExpandedTracksFormat)
{
  auto project = create_minimal_project ();
  ASSERT_NE (project, nullptr);

  nlohmann::json j =
    ProjectJsonSerializer::serialize (*project, TEST_APP_VERSION, "Test");

  auto &tracklist = j["projectData"]["tracklist"];
  if (tracklist.contains ("expandedTracks"))
    {
      auto &expanded = tracklist["expandedTracks"];
      EXPECT_TRUE (expanded.is_array ());

      for (const auto &uuid : expanded)
        {
          EXPECT_TRUE (uuid.is_string ());
          assert_valid_uuid (uuid.get<std::string> ());
        }
    }
}

// ============================================================================
// Registries Structure Tests
// ============================================================================

TEST_F (ProjectSerializationTest, SerializeProject_RegistriesExist)
{
  auto project = create_minimal_project ();
  ASSERT_NE (project, nullptr);

  nlohmann::json j =
    ProjectJsonSerializer::serialize (*project, TEST_APP_VERSION, "Test");
  auto &registries = j["projectData"]["registries"];

  EXPECT_TRUE (registries.contains ("portRegistry"));
  EXPECT_TRUE (registries.contains ("paramRegistry"));
  EXPECT_TRUE (registries.contains ("pluginRegistry"));
  EXPECT_TRUE (registries.contains ("trackRegistry"));
  EXPECT_TRUE (registries.contains ("arrangerObjectRegistry"));
  EXPECT_TRUE (registries.contains ("fileAudioSourceRegistry"));

  EXPECT_TRUE (registries["portRegistry"].is_array ());
  EXPECT_TRUE (registries["paramRegistry"].is_array ());
  EXPECT_TRUE (registries["pluginRegistry"].is_array ());
  EXPECT_TRUE (registries["trackRegistry"].is_array ());
  EXPECT_TRUE (registries["arrangerObjectRegistry"].is_array ());
  EXPECT_TRUE (registries["fileAudioSourceRegistry"].is_array ());
}

TEST_F (ProjectSerializationTest, SerializeProject_PortRegistryStructure)
{
  auto project = create_minimal_project ();
  ASSERT_NE (project, nullptr);

  nlohmann::json j =
    ProjectJsonSerializer::serialize (*project, TEST_APP_VERSION, "Test");
  auto &ports = j["projectData"]["registries"]["portRegistry"];

  for (const auto &port : ports)
    {
      EXPECT_TRUE (port.contains ("id"));
      assert_valid_uuid (port["id"].get<std::string> ());
      EXPECT_TRUE (port.contains ("type"));
      EXPECT_TRUE (port["type"].is_number ());
      EXPECT_TRUE (port.contains ("flow"));
      EXPECT_TRUE (port["flow"].is_number ());
    }
}

TEST_F (ProjectSerializationTest, SerializeProject_ParamRegistryStructure)
{
  auto project = create_minimal_project ();
  ASSERT_NE (project, nullptr);

  nlohmann::json j =
    ProjectJsonSerializer::serialize (*project, TEST_APP_VERSION, "Test");
  auto &params = j["projectData"]["registries"]["paramRegistry"];

  for (const auto &param : params)
    {
      EXPECT_TRUE (param.contains ("id"));
      assert_valid_uuid (param["id"].get<std::string> ());
      EXPECT_TRUE (param.contains ("uniqueId"));
      EXPECT_TRUE (param["uniqueId"].is_string ());
      EXPECT_TRUE (param.contains ("baseValue"));
      EXPECT_TRUE (param["baseValue"].is_number ());
      EXPECT_GE (param["baseValue"].get<double> (), 0.0);
      EXPECT_LE (param["baseValue"].get<double> (), 1.0);
    }
}

TEST_F (ProjectSerializationTest, SerializeProject_TrackRegistryStructure)
{
  auto project = create_minimal_project ();
  ASSERT_NE (project, nullptr);

  nlohmann::json j =
    ProjectJsonSerializer::serialize (*project, TEST_APP_VERSION, "Test");
  auto &tracks = j["projectData"]["registries"]["trackRegistry"];

  for (const auto &track : tracks)
    {
      EXPECT_TRUE (track.contains ("id"));
      assert_valid_uuid (track["id"].get<std::string> ());
      EXPECT_TRUE (track.contains ("type"));
      auto track_type = track["type"].get<int> ();
      EXPECT_GE (track_type, 0);
      EXPECT_LE (track_type, 11);
      EXPECT_TRUE (track.contains ("name"));
      EXPECT_TRUE (track["name"].is_string ());

      if (track.contains ("color"))
        {
          assert_valid_color (track["color"].get<std::string> ());
        }
    }
}

TEST_F (ProjectSerializationTest, SerializeProject_PluginRegistryStructure)
{
  auto project = create_minimal_project ();
  ASSERT_NE (project, nullptr);

  nlohmann::json j =
    ProjectJsonSerializer::serialize (*project, TEST_APP_VERSION, "Test");
  auto &plugins = j["projectData"]["registries"]["pluginRegistry"];

  for (const auto &plugin : plugins)
    {
      EXPECT_TRUE (plugin.contains ("id"));
      assert_valid_uuid (plugin["id"].get<std::string> ());
      EXPECT_TRUE (plugin.contains ("protocol"));
      EXPECT_TRUE (plugin["protocol"].is_number ());
      auto protocol = plugin["protocol"].get<int> ();
      EXPECT_GE (protocol, 0);
      EXPECT_LE (protocol, 10);
      EXPECT_TRUE (plugin.contains ("inputPorts"));
      EXPECT_TRUE (plugin["inputPorts"].is_array ());
      EXPECT_TRUE (plugin.contains ("outputPorts"));
      EXPECT_TRUE (plugin["outputPorts"].is_array ());
      EXPECT_TRUE (plugin.contains ("parameters"));
      EXPECT_TRUE (plugin["parameters"].is_array ());
      EXPECT_TRUE (plugin.contains ("setting"));
    }
}

TEST_F (
  ProjectSerializationTest,
  SerializeProject_FileAudioSourceRegistryStructure)
{
  auto project = create_minimal_project ();
  ASSERT_NE (project, nullptr);

  nlohmann::json j =
    ProjectJsonSerializer::serialize (*project, TEST_APP_VERSION, "Test");
  auto &sources = j["projectData"]["registries"]["fileAudioSourceRegistry"];

  for (const auto &source : sources)
    {
      EXPECT_TRUE (source.contains ("id"));
      assert_valid_uuid (source["id"].get<std::string> ());
      EXPECT_TRUE (source.contains ("filename"));
      EXPECT_TRUE (source["filename"].is_string ());
      EXPECT_TRUE (source.contains ("filePath"));
      EXPECT_TRUE (source["filePath"].is_string ());

      if (source.contains ("sampleRate"))
        {
          EXPECT_TRUE (source["sampleRate"].is_number ());
          EXPECT_GT (source["sampleRate"].get<int> (), 0);
        }

      if (source.contains ("channels"))
        {
          auto channels = source["channels"].get<int> ();
          EXPECT_TRUE (channels == 1 || channels == 2);
        }

      if (source.contains ("frames"))
        {
          EXPECT_TRUE (source["frames"].is_number ());
          EXPECT_GE (source["frames"].get<int> (), 0);
        }

      if (source.contains ("bitsPerSample"))
        {
          auto bits = source["bitsPerSample"].get<int> ();
          EXPECT_GE (bits, 8);
          EXPECT_LE (bits, 32);
        }

      if (source.contains ("format"))
        {
          auto format = source["format"].get<int> ();
          EXPECT_GE (format, 1);
          EXPECT_LE (format, 4);
        }
    }
}

// ============================================================================
// Optional Fields Tests
// ============================================================================

TEST_F (ProjectSerializationTest, SerializeProject_OptionalFields)
{
  auto project = create_minimal_project ();
  ASSERT_NE (project, nullptr);

  nlohmann::json j =
    ProjectJsonSerializer::serialize (*project, TEST_APP_VERSION, "Test");
  auto &pd = j["projectData"];

  EXPECT_TRUE (pd.contains ("snapGridTimeline"));
  EXPECT_TRUE (pd.contains ("snapGridEditor"));
  EXPECT_TRUE (pd.contains ("audioPool"));
  EXPECT_TRUE (pd.contains ("portConnectionsManager"));
  EXPECT_TRUE (pd.contains ("tempoObjectManager"));
  EXPECT_TRUE (pd.contains ("clipLauncher"));
}

TEST_F (ProjectSerializationTest, SerializeProject_AudioPoolStructure)
{
  auto project = create_minimal_project ();
  ASSERT_NE (project, nullptr);

  nlohmann::json j =
    ProjectJsonSerializer::serialize (*project, TEST_APP_VERSION, "Test");
  auto &audio_pool = j["projectData"]["audioPool"];

  EXPECT_TRUE (audio_pool.contains ("fileHashes"));
  EXPECT_TRUE (audio_pool["fileHashes"].is_array ());

  for (const auto &entry : audio_pool["fileHashes"])
    {
      EXPECT_TRUE (entry.is_array ());
      EXPECT_EQ (entry.size (), 2);
      EXPECT_TRUE (entry[0].is_string ());
      assert_valid_uuid (entry[0].get<std::string> ());
      EXPECT_TRUE (entry[1].is_number ());
    }
}

TEST_F (ProjectSerializationTest, SerializeProject_SnapGridStructure)
{
  auto project = create_minimal_project ();
  ASSERT_NE (project, nullptr);

  nlohmann::json j =
    ProjectJsonSerializer::serialize (*project, TEST_APP_VERSION, "Test");

  auto &snap_timeline = j["projectData"]["snapGridTimeline"];
  EXPECT_TRUE (snap_timeline.is_object ());

  auto &snap_editor = j["projectData"]["snapGridEditor"];
  EXPECT_TRUE (snap_editor.is_object ());
}

// ============================================================================
// Schema Validation Test
// ============================================================================

TEST_F (ProjectSerializationTest, SerializeProject_ValidatesAgainstSchema)
{
  auto project = create_minimal_project ();
  ASSERT_NE (project, nullptr);

  nlohmann::json j = ProjectJsonSerializer::serialize (
    *project, TEST_APP_VERSION_WITH_PATCH, "Schema Test");

  EXPECT_NO_THROW ({ ProjectJsonSerializer::validate_json (j); });
}

// ============================================================================
// Consistency Tests
// ============================================================================

TEST_F (ProjectSerializationTest, SerializeProject_TrackRegistryMatchesTracklist)
{
  auto project = create_minimal_project ();
  ASSERT_NE (project, nullptr);

  nlohmann::json j =
    ProjectJsonSerializer::serialize (*project, TEST_APP_VERSION, "Test");

  std::set<std::string> registry_ids;
  for (const auto &track : j["projectData"]["registries"]["trackRegistry"])
    {
      registry_ids.insert (track["id"].get<std::string> ());
    }

  std::set<std::string> tracklist_ids;
  for (const auto &track_id : j["projectData"]["tracklist"]["tracks"])
    {
      tracklist_ids.insert (track_id.get<std::string> ());
    }

  for (const auto &id : tracklist_ids)
    {
      EXPECT_TRUE (registry_ids.contains (id))
        << "Track " << id << " in tracklist but not in trackRegistry";
    }
}

TEST_F (ProjectSerializationTest, SerializeProject_PortReferencesAreValid)
{
  auto project = create_minimal_project ();
  ASSERT_NE (project, nullptr);

  nlohmann::json j =
    ProjectJsonSerializer::serialize (*project, TEST_APP_VERSION, "Test");

  std::set<std::string> port_ids;
  for (const auto &port : j["projectData"]["registries"]["portRegistry"])
    {
      port_ids.insert (port["id"].get<std::string> ());
    }

  for (const auto &track : j["projectData"]["registries"]["trackRegistry"])
    {
      if (track.contains ("processor"))
        {
          auto &processor = track["processor"];
          if (processor.contains ("inputPorts"))
            {
              for (const auto &port_ref : processor["inputPorts"])
                {
                  EXPECT_TRUE (port_ids.contains (port_ref.get<std::string> ()))
                    << "Processor input port not found in portRegistry: "
                    << port_ref.get<std::string> ();
                }
            }
          if (processor.contains ("outputPorts"))
            {
              for (const auto &port_ref : processor["outputPorts"])
                {
                  EXPECT_TRUE (port_ids.contains (port_ref.get<std::string> ()))
                    << "Processor output port not found in portRegistry: "
                    << port_ref.get<std::string> ();
                }
            }
        }
    }
}

// ============================================================================
// Deserialization Tests
// ============================================================================

TEST_F (ProjectSerializationTest, DeserializeProject_MinimalValidJson)
{
  auto j = create_minimal_valid_project_json ();

  auto project = create_minimal_project ();
  ASSERT_NE (project, nullptr);

  EXPECT_NO_THROW ({ ProjectJsonSerializer::deserialize (j, *project); });
}

TEST_F (ProjectSerializationTest, DeserializeProject_ValidatesFirst)
{
  auto j = create_minimal_valid_project_json ();
  j.erase ("documentType");

  auto project = create_minimal_project ();
  ASSERT_NE (project, nullptr);

  EXPECT_THROW (
    { ProjectJsonSerializer::deserialize (j, *project); },
    utils::exceptions::ZrythmException);
}

TEST_F (ProjectSerializationTest, DeserializeProject_FutureMajorVersion_Rejected)
{
  auto j = create_minimal_valid_project_json ();
  j["schemaVersion"]["major"] = ProjectJsonSerializer::SCHEMA_VERSION.major + 1;

  auto project = create_minimal_project ();
  ASSERT_NE (project, nullptr);

  EXPECT_THROW (
    { ProjectJsonSerializer::deserialize (j, *project); },
    utils::exceptions::ZrythmException);
}

TEST_F (ProjectSerializationTest, Deserialize_OlderMajorVersionSucceeds)
{
  auto j = create_minimal_valid_project_json ();
  j["schemaVersion"]["major"] = 1;

  auto project = create_minimal_project ();
  ASSERT_NE (project, nullptr);

  EXPECT_NO_THROW ({ ProjectJsonSerializer::deserialize (j, *project); });
}

TEST_F (ProjectSerializationTest, Deserialize_InvalidRegistryDataRejected)
{
  auto j = create_minimal_valid_project_json ();

  nlohmann::json bad_port;
  bad_port["id"] = "660e8400-e29b-41d4-a716-446655440001";
  j["projectData"]["registries"]["portRegistry"].push_back (bad_port);

  EXPECT_THROW (
    { ProjectJsonSerializer::validate_json (j); },
    utils::exceptions::ZrythmException);
}
}
