// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "project_json_serializer_test.h"

namespace zrythm::structure::project
{

// ============================================================================
// Round-Trip Tests
// ============================================================================

TEST_F (ProjectSerializationTest, RoundTrip_MinimalProject)
{
  auto original_project = create_minimal_project ();
  ASSERT_NE (original_project, nullptr);

  nlohmann::json j1 = ProjectJsonSerializer::serialize (
    *original_project, TEST_APP_VERSION, "RoundTrip Test");

  EXPECT_NO_THROW ({ ProjectJsonSerializer::validate_json (j1); });

  auto deserialized_project = create_minimal_project ();
  ASSERT_NE (deserialized_project, nullptr);

  EXPECT_NO_THROW ({
    ProjectJsonSerializer::deserialize (j1, *deserialized_project);
  });

  nlohmann::json j2 = ProjectJsonSerializer::serialize (
    *deserialized_project, TEST_APP_VERSION, "RoundTrip Test");

  EXPECT_NO_THROW ({ ProjectJsonSerializer::validate_json (j2); });

  EXPECT_EQ (j2["documentType"], j1["documentType"]);
  EXPECT_EQ (j2["schemaVersion"], j1["schemaVersion"]);
  EXPECT_EQ (j2["appVersion"], j1["appVersion"]);
  EXPECT_EQ (j2["title"], j1["title"]);

  EXPECT_TRUE (j2["projectData"].contains ("tempoMap"));
  EXPECT_TRUE (j2["projectData"].contains ("transport"));
  EXPECT_TRUE (j2["projectData"].contains ("tracklist"));
  EXPECT_TRUE (j2["projectData"].contains ("registries"));

  auto &tm1 = j1["projectData"]["tempoMap"];
  auto &tm2 = j2["projectData"]["tempoMap"];
  EXPECT_EQ (tm1["timeSignatures"].size (), tm2["timeSignatures"].size ())
    << "timeSignatures count mismatch";
  EXPECT_EQ (tm1["tempoChanges"].size (), tm2["tempoChanges"].size ())
    << "tempoChanges count mismatch";

  auto &tr1 = j1["projectData"]["transport"];
  auto &tr2 = j2["projectData"]["transport"];
  if (tr1.contains ("playheadPosition") && tr2.contains ("playheadPosition"))
    {
      EXPECT_EQ (
        tr1["playheadPosition"]["value"], tr2["playheadPosition"]["value"])
        << "Playhead position value mismatch";
    }
}

TEST_F (ProjectSerializationTest, RoundTrip_RegistriesPreserved)
{
  auto original_project = create_minimal_project ();
  ASSERT_NE (original_project, nullptr);

  nlohmann::json j1 = ProjectJsonSerializer::serialize (
    *original_project, TEST_APP_VERSION, "Test");

  auto deserialized_project = create_minimal_project ();
  ASSERT_NE (deserialized_project, nullptr);
  ProjectJsonSerializer::deserialize (j1, *deserialized_project);

  nlohmann::json j2 = ProjectJsonSerializer::serialize (
    *deserialized_project, TEST_APP_VERSION, "Test");

  auto &regs1 = j1["projectData"]["registries"];
  auto &regs2 = j2["projectData"]["registries"];

  EXPECT_EQ (regs1["portRegistry"].size (), regs2["portRegistry"].size ());
  EXPECT_EQ (regs1["paramRegistry"].size (), regs2["paramRegistry"].size ());
  EXPECT_EQ (regs1["trackRegistry"].size (), regs2["trackRegistry"].size ());
  EXPECT_EQ (regs1["pluginRegistry"].size (), regs2["pluginRegistry"].size ());
  EXPECT_EQ (
    regs1["arrangerObjectRegistry"].size (),
    regs2["arrangerObjectRegistry"].size ());
  EXPECT_EQ (
    regs1["fileAudioSourceRegistry"].size (),
    regs2["fileAudioSourceRegistry"].size ());

  // Verify actual port IDs are preserved (not just counts)
  std::set<std::string> port_ids_1, port_ids_2;
  for (const auto &port : regs1["portRegistry"])
    port_ids_1.insert (port["id"].get<std::string> ());
  for (const auto &port : regs2["portRegistry"])
    port_ids_2.insert (port["id"].get<std::string> ());
  EXPECT_EQ (port_ids_1, port_ids_2)
    << "Port IDs not preserved across round-trip";

  // Verify track IDs are preserved
  std::set<std::string> track_ids_1, track_ids_2;
  for (const auto &track : regs1["trackRegistry"])
    track_ids_1.insert (track["id"].get<std::string> ());
  for (const auto &track : regs2["trackRegistry"])
    track_ids_2.insert (track["id"].get<std::string> ());
  EXPECT_EQ (track_ids_1, track_ids_2)
    << "Track IDs not preserved across round-trip";

  // Verify arranger object IDs are preserved
  std::set<std::string> obj_ids_1, obj_ids_2;
  for (const auto &obj : regs1["arrangerObjectRegistry"])
    obj_ids_1.insert (obj["id"].get<std::string> ());
  for (const auto &obj : regs2["arrangerObjectRegistry"])
    obj_ids_2.insert (obj["id"].get<std::string> ());
  EXPECT_EQ (obj_ids_1, obj_ids_2)
    << "Arranger object IDs not preserved across round-trip";
}

TEST_F (ProjectSerializationTest, RoundTrip_TracklistPreserved)
{
  auto original_project = create_minimal_project ();
  ASSERT_NE (original_project, nullptr);

  nlohmann::json j1 = ProjectJsonSerializer::serialize (
    *original_project, TEST_APP_VERSION, "Test");

  auto deserialized_project = create_minimal_project ();
  ASSERT_NE (deserialized_project, nullptr);
  ProjectJsonSerializer::deserialize (j1, *deserialized_project);

  nlohmann::json j2 = ProjectJsonSerializer::serialize (
    *deserialized_project, TEST_APP_VERSION, "Test");

  auto &tl1 = j1["projectData"]["tracklist"];
  auto &tl2 = j2["projectData"]["tracklist"];

  EXPECT_EQ (tl1["tracks"].size (), tl2["tracks"].size ());
  EXPECT_EQ (
    tl1["pinnedTracksCutoff"].get<int> (),
    tl2["pinnedTracksCutoff"].get<int> ());
  EXPECT_EQ (tl1["trackRoutes"].size (), tl2["trackRoutes"].size ());

  // Verify track order is preserved (exact UUID sequence)
  auto &tracks1 = tl1["tracks"];
  auto &tracks2 = tl2["tracks"];
  for (size_t i = 0; i < tracks1.size (); ++i)
    {
      EXPECT_EQ (tracks1[i], tracks2[i])
        << "Track at index " << i << " differs after round-trip";
    }

  // Verify track routes are preserved exactly
  auto &routes1 = tl1["trackRoutes"];
  auto &routes2 = tl2["trackRoutes"];
  for (size_t i = 0; i < routes1.size (); ++i)
    {
      EXPECT_EQ (routes1[i].size (), routes2[i].size ())
        << "Route " << i << " size differs";
      if (routes1[i].size () == 2 && routes2[i].size () == 2)
        {
          EXPECT_EQ (routes1[i][0], routes2[i][0])
            << "Route " << i << " source differs";
          EXPECT_EQ (routes1[i][1], routes2[i][1])
            << "Route " << i << " destination differs";
        }
    }
}

TEST_F (ProjectSerializationTest, RoundTrip_OptionalFieldsMissing)
{
  // Create minimal JSON with required fields only
  auto j = create_minimal_valid_project_json ();

  auto project = create_minimal_project ();
  ASSERT_NE (project, nullptr);

  // Should deserialize without throwing (optional fields use defaults)
  EXPECT_NO_THROW ({ ProjectJsonSerializer::deserialize (j, *project); });

  // Serialize back and verify it validates
  nlohmann::json j2 =
    ProjectJsonSerializer::serialize (*project, TEST_APP_VERSION, "Test");
  EXPECT_NO_THROW ({ ProjectJsonSerializer::validate_json (j2); });
}

TEST_F (ProjectSerializationTest, RoundTrip_FolderHierarchyPreserved)
{
  auto original_project = create_minimal_project ();
  ASSERT_NE (original_project, nullptr);

  nlohmann::json j1 = ProjectJsonSerializer::serialize (
    *original_project, TEST_APP_VERSION, "Test");

  auto deserialized_project = create_minimal_project ();
  ASSERT_NE (deserialized_project, nullptr);
  ProjectJsonSerializer::deserialize (j1, *deserialized_project);

  nlohmann::json j2 = ProjectJsonSerializer::serialize (
    *deserialized_project, TEST_APP_VERSION, "Test");

  // Verify folderParents and expandedTracks are preserved (if present)
  auto &tl1 = j1["projectData"]["tracklist"];
  auto &tl2 = j2["projectData"]["tracklist"];

  // Check folderParents - verify exact pairs are preserved
  if (tl1.contains ("folderParents"))
    {
      EXPECT_TRUE (tl2.contains ("folderParents"))
        << "folderParents should be preserved";

      auto &fp1 = tl1["folderParents"];
      auto &fp2 = tl2["folderParents"];
      EXPECT_EQ (fp1.size (), fp2.size ());

      // Build set of pairs from first serialization
      std::set<std::pair<std::string, std::string>> pairs1;
      for (const auto &entry : fp1)
        {
          pairs1.emplace (
            entry[0].get<std::string> (), entry[1].get<std::string> ());
        }
      // Verify all pairs exist in second serialization
      for (const auto &entry : fp2)
        {
          auto pair = std::make_pair (
            entry[0].get<std::string> (), entry[1].get<std::string> ());
          EXPECT_TRUE (pairs1.contains (pair))
            << "Folder parent pair (" << pair.first << ", " << pair.second
            << ") not preserved";
        }
    }

  // Check expandedTracks - verify exact UUIDs are preserved
  if (tl1.contains ("expandedTracks"))
    {
      EXPECT_TRUE (tl2.contains ("expandedTracks"))
        << "expandedTracks should be preserved";

      std::set<std::string> expanded1, expanded2;
      for (const auto &uuid : tl1["expandedTracks"])
        expanded1.insert (uuid.get<std::string> ());
      for (const auto &uuid : tl2["expandedTracks"])
        expanded2.insert (uuid.get<std::string> ());

      EXPECT_EQ (expanded1, expanded2) << "Expanded tracks set not preserved";
    }
}

TEST_F (ProjectSerializationTest, RoundTrip_ClipLauncherPreserved)
{
  auto original_project = create_minimal_project ();
  ASSERT_NE (original_project, nullptr);

  nlohmann::json j1 = ProjectJsonSerializer::serialize (
    *original_project, TEST_APP_VERSION, "Test");

  auto deserialized_project = create_minimal_project ();
  ASSERT_NE (deserialized_project, nullptr);
  ProjectJsonSerializer::deserialize (j1, *deserialized_project);

  nlohmann::json j2 = ProjectJsonSerializer::serialize (
    *deserialized_project, TEST_APP_VERSION, "Test");

  auto &cl1 = j1["projectData"]["clipLauncher"];
  auto &cl2 = j2["projectData"]["clipLauncher"];

  EXPECT_EQ (cl1["scenes"].size (), cl2["scenes"].size ())
    << "Scene count not preserved";

  for (size_t i = 0; i < cl1["scenes"].size () && i < cl2["scenes"].size (); ++i)
    {
      auto &scene1 = cl1["scenes"][i];
      auto &scene2 = cl2["scenes"][i];

      if (scene1.contains ("name"))
        {
          EXPECT_EQ (scene1["name"], scene2["name"])
            << "Scene " << i << " name mismatch";
        }

      if (scene1.contains ("clipSlots") && scene2.contains ("clipSlots"))
        {
          EXPECT_EQ (scene1["clipSlots"].size (), scene2["clipSlots"].size ())
            << "Scene " << i << " clip slots count mismatch";
        }
    }
}

// ============================================================================
// Property-Based Round-Trip Tests
// ============================================================================

/// Property-based test: All UUIDs should be preserved across round-trip
TEST_F (ProjectSerializationTest, RoundTrip_AllUuidsPreserved)
{
  auto original_project = create_minimal_project ();
  ASSERT_NE (original_project, nullptr);

  nlohmann::json j1 = ProjectJsonSerializer::serialize (
    *original_project, TEST_APP_VERSION, "Test");

  auto deserialized_project = create_minimal_project ();
  ASSERT_NE (deserialized_project, nullptr);
  ProjectJsonSerializer::deserialize (j1, *deserialized_project);

  nlohmann::json j2 = ProjectJsonSerializer::serialize (
    *deserialized_project, TEST_APP_VERSION, "Test");

  // Extract all UUIDs from both serializations
  auto uuids1 = extract_all_uuids (j1);
  auto uuids2 = extract_all_uuids (j2);

  // All UUIDs from j1 should exist in j2
  for (const auto &uuid : uuids1)
    {
      EXPECT_TRUE (uuids2.contains (uuid))
        << "UUID " << uuid << " was not preserved across round-trip";
    }

  // And vice versa - no new UUIDs should appear
  for (const auto &uuid : uuids2)
    {
      EXPECT_TRUE (uuids1.contains (uuid))
        << "UUID " << uuid << " appeared after round-trip (was not in original)";
    }
}

/// Property-based test: Object count should be consistent
TEST_F (ProjectSerializationTest, RoundTrip_ObjectCountConsistent)
{
  auto original_project = create_minimal_project ();
  ASSERT_NE (original_project, nullptr);

  nlohmann::json j1 = ProjectJsonSerializer::serialize (
    *original_project, TEST_APP_VERSION, "Test");

  auto deserialized_project = create_minimal_project ();
  ASSERT_NE (deserialized_project, nullptr);
  ProjectJsonSerializer::deserialize (j1, *deserialized_project);

  nlohmann::json j2 = ProjectJsonSerializer::serialize (
    *deserialized_project, TEST_APP_VERSION, "Test");

  // Object count should be identical
  EXPECT_EQ (count_objects (j1), count_objects (j2))
    << "Object count changed across round-trip";
}

/// Property-based test: Registry sizes should be preserved
TEST_F (ProjectSerializationTest, RoundTrip_AllRegistrySizesPreserved)
{
  auto original_project = create_minimal_project ();
  ASSERT_NE (original_project, nullptr);

  nlohmann::json j1 = ProjectJsonSerializer::serialize (
    *original_project, TEST_APP_VERSION, "Test");

  auto deserialized_project = create_minimal_project ();
  ASSERT_NE (deserialized_project, nullptr);
  ProjectJsonSerializer::deserialize (j1, *deserialized_project);

  nlohmann::json j2 = ProjectJsonSerializer::serialize (
    *deserialized_project, TEST_APP_VERSION, "Test");

  auto &regs1 = j1["projectData"]["registries"];
  auto &regs2 = j2["projectData"]["registries"];

  // Check each registry size
  const std::array<std::string, 6> registry_names = {
    "portRegistry",  "paramRegistry",          "pluginRegistry",
    "trackRegistry", "arrangerObjectRegistry", "fileAudioSourceRegistry",
  };

  for (const auto &name : registry_names)
    {
      EXPECT_EQ (regs1[name].size (), regs2[name].size ())
        << "Registry " << name << " size changed across round-trip";
    }
}

/// Property-based test: Second round-trip should be identical to first
TEST_F (ProjectSerializationTest, RoundTrip_Idempotent)
{
  auto project1 = create_minimal_project ();
  ASSERT_NE (project1, nullptr);

  // First serialization
  nlohmann::json j1 =
    ProjectJsonSerializer::serialize (*project1, TEST_APP_VERSION, "Test");

  // Deserialize into project2
  auto project2 = create_minimal_project ();
  ASSERT_NE (project2, nullptr);
  ProjectJsonSerializer::deserialize (j1, *project2);

  // Second serialization
  nlohmann::json j2 =
    ProjectJsonSerializer::serialize (*project2, TEST_APP_VERSION, "Test");

  // Deserialize into project3
  auto project3 = create_minimal_project ();
  ASSERT_NE (project3, nullptr);
  ProjectJsonSerializer::deserialize (j2, *project3);

  // Third serialization - should be identical to second
  nlohmann::json j3 =
    ProjectJsonSerializer::serialize (*project3, TEST_APP_VERSION, "Test");

  // j2 and j3 should have identical UUID sets
  auto uuids2 = extract_all_uuids (j2);
  auto uuids3 = extract_all_uuids (j3);
  EXPECT_EQ (uuids2, uuids3) << "UUIDs not stable after second round-trip";

  // j2 and j3 should have identical object counts
  EXPECT_EQ (count_objects (j2), count_objects (j3))
    << "Object count not stable after second round-trip";
}
}
