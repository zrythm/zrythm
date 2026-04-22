// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "project_json_serializer_test.h"

namespace zrythm::controllers
{

using test_helpers::expect_registries_match;

// ============================================================================
// Round-Trip Tests
// ============================================================================

TEST_F (ProjectSerializationTest, RoundTrip_MinimalProject)
{
  auto original_project = create_minimal_project ();
  ASSERT_NE (original_project, nullptr);
  create_ui_state_and_undo_stack (*original_project);

  nlohmann::json j1 = ProjectJsonSerializer::serialize (
    *original_project, *ui_state, *undo_stack, TEST_APP_VERSION,
    "RoundTrip Test");

  EXPECT_NO_THROW ({ ProjectJsonSerializer::validate_json (j1); });

  auto deserialized_project = create_minimal_project ();
  ASSERT_NE (deserialized_project, nullptr);
  create_ui_state_and_undo_stack (*deserialized_project);

  EXPECT_NO_THROW ({
    ProjectJsonSerializer::deserialize (
      j1, *deserialized_project, *ui_state, *undo_stack);
  });

  nlohmann::json j2 = ProjectJsonSerializer::serialize (
    *deserialized_project, *ui_state, *undo_stack, TEST_APP_VERSION,
    "RoundTrip Test");

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
  create_ui_state_and_undo_stack (*original_project);

  nlohmann::json j1 = ProjectJsonSerializer::serialize (
    *original_project, *ui_state, *undo_stack, TEST_APP_VERSION, "Test");

  auto deserialized_project = create_minimal_project ();
  ASSERT_NE (deserialized_project, nullptr);
  create_ui_state_and_undo_stack (*deserialized_project);
  ProjectJsonSerializer::deserialize (
    j1, *deserialized_project, *ui_state, *undo_stack);

  nlohmann::json j2 = ProjectJsonSerializer::serialize (
    *deserialized_project, *ui_state, *undo_stack, TEST_APP_VERSION, "Test");

  expect_registries_match (j1, j2);
}

TEST_F (ProjectSerializationTest, RoundTrip_TracklistPreserved)
{
  auto original_project = create_minimal_project ();
  ASSERT_NE (original_project, nullptr);
  create_ui_state_and_undo_stack (*original_project);

  nlohmann::json j1 = ProjectJsonSerializer::serialize (
    *original_project, *ui_state, *undo_stack, TEST_APP_VERSION, "Test");

  auto deserialized_project = create_minimal_project ();
  ASSERT_NE (deserialized_project, nullptr);
  create_ui_state_and_undo_stack (*deserialized_project);
  ProjectJsonSerializer::deserialize (
    j1, *deserialized_project, *ui_state, *undo_stack);

  nlohmann::json j2 = ProjectJsonSerializer::serialize (
    *deserialized_project, *ui_state, *undo_stack, TEST_APP_VERSION, "Test");

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
  create_ui_state_and_undo_stack (*project);

  // Should deserialize without throwing (optional fields use defaults)
  EXPECT_NO_THROW ({
    ProjectJsonSerializer::deserialize (j, *project, *ui_state, *undo_stack);
  });

  // Serialize back and verify it validates
  nlohmann::json j2 = ProjectJsonSerializer::serialize (
    *project, *ui_state, *undo_stack, TEST_APP_VERSION, "Test");
  EXPECT_NO_THROW ({ ProjectJsonSerializer::validate_json (j2); });
}

TEST_F (ProjectSerializationTest, RoundTrip_FolderHierarchyPreserved)
{
  auto original_project = create_minimal_project ();
  ASSERT_NE (original_project, nullptr);
  create_ui_state_and_undo_stack (*original_project);

  nlohmann::json j1 = ProjectJsonSerializer::serialize (
    *original_project, *ui_state, *undo_stack, TEST_APP_VERSION, "Test");

  auto deserialized_project = create_minimal_project ();
  ASSERT_NE (deserialized_project, nullptr);
  create_ui_state_and_undo_stack (*deserialized_project);
  ProjectJsonSerializer::deserialize (
    j1, *deserialized_project, *ui_state, *undo_stack);

  nlohmann::json j2 = ProjectJsonSerializer::serialize (
    *deserialized_project, *ui_state, *undo_stack, TEST_APP_VERSION, "Test");

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
  create_ui_state_and_undo_stack (*original_project);

  nlohmann::json j1 = ProjectJsonSerializer::serialize (
    *original_project, *ui_state, *undo_stack, TEST_APP_VERSION, "Test");

  auto deserialized_project = create_minimal_project ();
  ASSERT_NE (deserialized_project, nullptr);
  create_ui_state_and_undo_stack (*deserialized_project);
  ProjectJsonSerializer::deserialize (
    j1, *deserialized_project, *ui_state, *undo_stack);

  nlohmann::json j2 = ProjectJsonSerializer::serialize (
    *deserialized_project, *ui_state, *undo_stack, TEST_APP_VERSION, "Test");

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

TEST_F (ProjectSerializationTest, RoundTrip_WithPlugin)
{
  auto original_project = create_minimal_project ();
  ASSERT_NE (original_project, nullptr);

  // Create a plugin in the project's plugin registry
  auto &plugin_registry = original_project->get_plugin_registry ();
  auto &project_port_registry = original_project->get_port_registry ();
  auto &project_param_registry = original_project->get_param_registry ();

  // Create plugin descriptor
  auto descriptor = std::make_unique<plugins::PluginDescriptor> ();
  descriptor->name_ = u8"Test EQ Plugin";
  descriptor->protocol_ = plugins::Protocol::ProtocolType::CLAP;

  plugins::PluginConfiguration config;
  config.descr_ = std::move (descriptor);

  // Create the plugin
  auto plugin_ref = plugin_registry.create_object<plugins::InternalPluginBase> (
    dsp::ProcessorBase::ProcessorBaseDependencies{
      .port_registry_ = project_port_registry,
      .param_registry_ = project_param_registry },
    nullptr);
  auto * plugin = plugin_ref.get_object_as<plugins::InternalPluginBase> ();
  plugin->set_configuration (config);

  create_ui_state_and_undo_stack (*original_project);

  // Serialize
  nlohmann::json j1 = ProjectJsonSerializer::serialize (
    *original_project, *ui_state, *undo_stack, TEST_APP_VERSION, "Plugin Test");

  // Validate against schema
  EXPECT_NO_THROW ({ ProjectJsonSerializer::validate_json (j1); });

  // Verify plugin is in the registry with correct structure
  auto &plugins1 = j1["projectData"]["registries"]["pluginRegistry"];
  EXPECT_EQ (plugins1.size (), 1);
  EXPECT_TRUE (plugins1[0].contains ("protocol"));
  EXPECT_TRUE (plugins1[0].contains ("configuration"));

  // Deserialize into new project
  auto deserialized_project = create_minimal_project ();
  ASSERT_NE (deserialized_project, nullptr);
  create_ui_state_and_undo_stack (*deserialized_project);
  ProjectJsonSerializer::deserialize (
    j1, *deserialized_project, *ui_state, *undo_stack);

  // Serialize again
  nlohmann::json j2 = ProjectJsonSerializer::serialize (
    *deserialized_project, *ui_state, *undo_stack, TEST_APP_VERSION,
    "Plugin Test");

  // Validate again
  EXPECT_NO_THROW ({ ProjectJsonSerializer::validate_json (j2); });

  // Verify all registries match (sizes and IDs)
  expect_registries_match (j1, j2);
}

/// List of non-singleton track types to test
using NonSingletonTrackTypes = ::testing::Types<
  structure::tracks::MidiTrack,
  structure::tracks::AudioTrack,
  structure::tracks::InstrumentTrack,
  structure::tracks::MidiGroupTrack,
  structure::tracks::AudioGroupTrack,
  structure::tracks::FolderTrack,
  structure::tracks::MidiBusTrack,
  structure::tracks::AudioBusTrack>;

template <typename TrackT>
class TrackRoundTripTest : public ProjectSerializationTest
{
};

TYPED_TEST_SUITE (TrackRoundTripTest, NonSingletonTrackTypes);

TYPED_TEST (TrackRoundTripTest, RoundTrip_PreservesTrack)
{
  using TrackType = TypeParam;

  auto original_project = this->create_minimal_project ();
  ASSERT_NE (original_project, nullptr);

  // Create a track using the project's track factory
  auto track_ref =
    original_project->track_factory_->template create_empty_track<TrackType> ();
  auto * track = track_ref.template get_object_as<TrackType> ();
  ASSERT_NE (track, nullptr);
  track->setName (u8"Test Track");

  // Add the track to the tracklist
  original_project->tracklist ()->collection ()->add_track (track_ref);

  this->create_ui_state_and_undo_stack (*original_project);

  // Serialize
  nlohmann::json j1 = ProjectJsonSerializer::serialize (
    *original_project, *this->ui_state, *this->undo_stack,
    this->TEST_APP_VERSION, "Track Test");

  // Validate against schema
  EXPECT_NO_THROW ({ ProjectJsonSerializer::validate_json (j1); });

  // Verify track appears in tracklist
  auto &tl1 = j1["projectData"]["tracklist"]["tracks"];
  EXPECT_GE (tl1.size (), 1) << "Tracklist should contain the track reference";

  // Deserialize into new project
  auto deserialized_project = this->create_minimal_project ();
  ASSERT_NE (deserialized_project, nullptr);
  this->create_ui_state_and_undo_stack (*deserialized_project);
  ProjectJsonSerializer::deserialize (
    j1, *deserialized_project, *this->ui_state, *this->undo_stack);

  // Serialize again
  nlohmann::json j2 = ProjectJsonSerializer::serialize (
    *deserialized_project, *this->ui_state, *this->undo_stack,
    this->TEST_APP_VERSION, "Track Test");

  // Validate again
  EXPECT_NO_THROW ({ ProjectJsonSerializer::validate_json (j2); });

  // Verify all registries match (sizes and IDs)
  expect_registries_match (j1, j2);

  // Verify automation track counts match for each track
  {
    auto &regs1 = j1["projectData"]["registries"];
    auto &regs2 = j2["projectData"]["registries"];

    // Build map of track ID -> track JSON for both serializations
    std::map<std::string, const nlohmann::json *> tracks_by_id1, tracks_by_id2;
    for (const auto &track_json : regs1["trackRegistry"])
      tracks_by_id1[track_json["id"].get<std::string> ()] = &track_json;
    for (const auto &track_json : regs2["trackRegistry"])
      tracks_by_id2[track_json["id"].get<std::string> ()] = &track_json;

    // Check automation track counts for each track
    for (const auto &[id, track1_ptr] : tracks_by_id1)
      {
        auto it2 = tracks_by_id2.find (id);
        if (it2 == tracks_by_id2.end ())
          continue;

        const auto &track1 = *track1_ptr;
        const auto &track2 = *it2->second;

        if (
          track1.contains ("automationTracklist")
          && track2.contains ("automationTracklist"))
          {
            const auto &at1 = track1["automationTracklist"]["automationTracks"];
            const auto &at2 = track2["automationTracklist"]["automationTracks"];

            EXPECT_EQ (at1.size (), at2.size ())
              << "Track " << id << " automation track count mismatch";

            if (at1.size () != at2.size ())
              {
                // Print which automation tracks differ
                std::set<std::string> at_ids1, at_ids2;
                for (const auto &at : at1)
                  at_ids1.insert (at["id"].get<std::string> ());
                for (const auto &at : at2)
                  at_ids2.insert (at["id"].get<std::string> ());

                for (const auto &at_id : at_ids1)
                  {
                    if (!at_ids2.contains (at_id))
                      std::cout
                        << "  Automation track " << at_id
                        << " from j1 missing in j2\n";
                  }
                for (const auto &at_id : at_ids2)
                  {
                    if (!at_ids1.contains (at_id))
                      std::cout
                        << "  Automation track " << at_id
                        << " from j2 missing in j1\n";
                  }
              }
          }
      }
  }

  // Verify tracklist track references are preserved
  auto &tl2 = j2["projectData"]["tracklist"]["tracks"];
  EXPECT_EQ (tl1.size (), tl2.size ()) << "Tracklist size mismatch";
  for (size_t i = 0; i < tl1.size () && i < tl2.size (); ++i)
    {
      EXPECT_EQ (tl1[i], tl2[i])
        << "Track reference at index " << i << " differs";
    }
}

// ============================================================================
// Arranger Object Round-Trip Tests
// ============================================================================
// These tests verify that C++ serialization of arranger objects produces
// schema-valid JSON. This catches mismatches between C++ serialization keys
// and schema property names (e.g., "clipStartPos" vs "clipStartPosition").
// ============================================================================

/// Test that a project with MIDI track + MIDI region + MIDI note validates
TEST_F (ProjectSerializationTest, RoundTrip_MidiRegionWithNote_Validates)
{
  auto original_project = create_minimal_project ();
  ASSERT_NE (original_project, nullptr);

  // Create MIDI track
  auto track_ref = original_project->track_factory_->create_empty_track<
    structure::tracks::MidiTrack> ();
  auto * track = track_ref.get_object_as<structure::tracks::MidiTrack> ();
  ASSERT_NE (track, nullptr);
  track->setName (u8"MIDI Track");
  original_project->tracklist ()->collection ()->add_track (track_ref);

  // Add MIDI region with note to the first lane
  {
    auto  &factory = *original_project->arrangerObjectFactory ();
    auto * lane = track->lanes ()->at (0);
    ASSERT_NE (lane, nullptr);

    auto region_ref =
      factory.get_builder<structure::arrangement::MidiRegion> ()
        .with_start_ticks (0)
        .with_end_ticks (960)
        .with_name (u8"MIDI Region")
        .build_in_registry ();
    auto * region =
      region_ref.get_object_as<structure::arrangement::MidiRegion> ();
    lane->structure::arrangement::ArrangerObjectOwner<
      structure::arrangement::MidiRegion>::add_object (region_ref);

    auto note_ref =
      factory.get_builder<structure::arrangement::MidiNote> ()
        .with_start_ticks (0)
        .with_pitch (60)
        .with_velocity (100)
        .build_in_registry ();
    region->add_object (note_ref);
  }

  create_ui_state_and_undo_stack (*original_project);

  // Serialize
  nlohmann::json j1 = ProjectJsonSerializer::serialize (
    *original_project, *ui_state, *undo_stack, TEST_APP_VERSION,
    "MIDI Region Test");

  // Validate against schema - this catches code/schema mismatches
  EXPECT_NO_THROW ({
    ProjectJsonSerializer::validate_json (j1);
  }) << "MIDI region serialization should produce schema-valid JSON";

  // Verify arranger objects are in the registry
  auto &obj_registry = j1["projectData"]["registries"]["arrangerObjectRegistry"];
  EXPECT_GE (obj_registry.size (), 2)
    << "Should have at least MIDI region and MIDI note";

  // Deserialize
  auto deserialized_project = create_minimal_project ();
  ASSERT_NE (deserialized_project, nullptr);
  create_ui_state_and_undo_stack (*deserialized_project);
  ProjectJsonSerializer::deserialize (
    j1, *deserialized_project, *ui_state, *undo_stack);

  // Serialize again
  nlohmann::json j2 = ProjectJsonSerializer::serialize (
    *deserialized_project, *ui_state, *undo_stack, TEST_APP_VERSION,
    "MIDI Region Test");

  // Validate again
  EXPECT_NO_THROW ({ ProjectJsonSerializer::validate_json (j2); });

  // Verify registries match
  expect_registries_match (j1, j2);
}

/// Test that a project with Audio track + Audio region validates
TEST_F (ProjectSerializationTest, RoundTrip_AudioRegion_Validates)
{
  auto original_project = create_minimal_project ();
  ASSERT_NE (original_project, nullptr);

  // Create Audio track
  auto track_ref = original_project->track_factory_->create_empty_track<
    structure::tracks::AudioTrack> ();
  auto * track = track_ref.get_object_as<structure::tracks::AudioTrack> ();
  ASSERT_NE (track, nullptr);
  track->setName (u8"Audio Track");
  original_project->tracklist ()->collection ()->add_track (track_ref);

  // Add audio region (has loop properties that must match schema) to first lane
  {
    auto  &factory = *original_project->arrangerObjectFactory ();
    auto * lane = track->lanes ()->at (0);
    ASSERT_NE (lane, nullptr);

    auto &file_registry = original_project->get_file_audio_source_registry ();
    auto  audio_source_ref = file_registry.create_object<dsp::FileAudioSource> (
      2, units::samples (1), units::sample_rate (44100), 120,
      u8"test_audio.wav");

    auto region_ref =
      factory.create_audio_region_with_clip (audio_source_ref, 0);
    auto * region =
      region_ref.get_object_as<structure::arrangement::AudioRegion> ();
    region->name ()->setName (u8"Audio Region");
    lane->structure::arrangement::ArrangerObjectOwner<
      structure::arrangement::AudioRegion>::add_object (region_ref);
  }

  create_ui_state_and_undo_stack (*original_project);

  // Serialize
  nlohmann::json j1 = ProjectJsonSerializer::serialize (
    *original_project, *ui_state, *undo_stack, TEST_APP_VERSION,
    "Audio Region Test");

  // Validate against schema - this catches clipStartPos/loopStartPos/loopEndPos
  // mismatches with schema's clipStartPosition/loopStartPosition/loopEndPosition
  EXPECT_NO_THROW ({ ProjectJsonSerializer::validate_json (j1); })
    << "Audio region serialization should produce schema-valid JSON. "
    << "Check for property name mismatches (e.g., clipStartPos vs clipStartPosition)";

  // Verify arranger objects are in the registry
  auto &obj_registry = j1["projectData"]["registries"]["arrangerObjectRegistry"];
  EXPECT_GE (obj_registry.size (), 2)
    << "Should have at least audio region and audio source object";

  // Deserialize
  auto deserialized_project = create_minimal_project ();
  ASSERT_NE (deserialized_project, nullptr);
  create_ui_state_and_undo_stack (*deserialized_project);
  ProjectJsonSerializer::deserialize (
    j1, *deserialized_project, *ui_state, *undo_stack);

  // Serialize again
  nlohmann::json j2 = ProjectJsonSerializer::serialize (
    *deserialized_project, *ui_state, *undo_stack, TEST_APP_VERSION,
    "Audio Region Test");

  // Validate again
  EXPECT_NO_THROW ({ ProjectJsonSerializer::validate_json (j2); });

  // Verify registries match
  expect_registries_match (j1, j2);
}

/// Test that a project with track + Automation region + Automation point
/// validates
TEST_F (ProjectSerializationTest, RoundTrip_AutomationRegionWithPoint_Validates)
{
  auto original_project = create_minimal_project ();
  ASSERT_NE (original_project, nullptr);

  // Create an Audio track (ChannelTrack that supports automation)
  auto track_ref = original_project->track_factory_->create_empty_track<
    structure::tracks::AudioTrack> ();
  auto * track = track_ref.get_object_as<structure::tracks::AudioTrack> ();
  ASSERT_NE (track, nullptr);
  track->setName (u8"Audio Track with Automation");
  original_project->tracklist ()->collection ()->add_track (track_ref);

  // Add automation region with point
  {
    auto &factory = *original_project->arrangerObjectFactory ();
    auto  region_ref =
      factory.get_builder<structure::arrangement::AutomationRegion> ()
        .with_start_ticks (0)
        .with_end_ticks (960)
        .with_name (u8"Automation Region")
        .build_in_registry ();
    auto * region =
      region_ref.get_object_as<structure::arrangement::AutomationRegion> ();
    track->automationTracklist ()->automation_track_at (0)->add_object (
      region_ref);

    auto point_ref =
      factory.get_builder<structure::arrangement::AutomationPoint> ()
        .with_start_ticks (0)
        .with_automatable_value (0.75)
        .build_in_registry ();
    region->add_object (point_ref);
  }

  create_ui_state_and_undo_stack (*original_project);

  // Serialize
  nlohmann::json j1 = ProjectJsonSerializer::serialize (
    *original_project, *ui_state, *undo_stack, TEST_APP_VERSION,
    "Automation Region Test");

  // Validate against schema - this catches code/schema mismatches for
  // automation regions (which also have loop properties like
  // clipStartPosition/loopStartPosition)
  EXPECT_NO_THROW ({
    ProjectJsonSerializer::validate_json (j1);
  }) << "Automation region serialization should produce schema-valid JSON";

  // Verify arranger objects are in the registry
  auto &obj_registry = j1["projectData"]["registries"]["arrangerObjectRegistry"];
  EXPECT_GE (obj_registry.size (), 2)
    << "Should have at least automation region and automation point";

  // Deserialize
  auto deserialized_project = create_minimal_project ();
  ASSERT_NE (deserialized_project, nullptr);
  create_ui_state_and_undo_stack (*deserialized_project);
  ProjectJsonSerializer::deserialize (
    j1, *deserialized_project, *ui_state, *undo_stack);

  // Serialize again
  nlohmann::json j2 = ProjectJsonSerializer::serialize (
    *deserialized_project, *ui_state, *undo_stack, TEST_APP_VERSION,
    "Automation Region Test");

  // Validate again
  EXPECT_NO_THROW ({ ProjectJsonSerializer::validate_json (j2); });

  // Verify registries match
  expect_registries_match (j1, j2);
}

/// Test that a project with Chord track + Chord region + Chord object validates
TEST_F (ProjectSerializationTest, RoundTrip_ChordRegionWithChord_Validates)
{
  auto original_project = create_minimal_project ();
  ASSERT_NE (original_project, nullptr);

  // Get the chord track (singleton)
  {
    auto track_ref = original_project->track_factory_->create_empty_track<
      structure::tracks::ChordTrack> ();
    original_project->tracklist ()->collection ()->add_track (track_ref);
    original_project->tracklist ()->singletonTracks ()->setChordTrack (
      track_ref.get_object_as<structure::tracks::ChordTrack> ());
  }
  auto * chord_track =
    original_project->tracklist ()->singletonTracks ()->chordTrack ();
  ASSERT_NE (chord_track, nullptr);

  // Add chord region with chord object
  {
    auto &factory = *original_project->arrangerObjectFactory ();
    auto  region_ref =
      factory.get_builder<structure::arrangement::ChordRegion> ()
        .with_start_ticks (0)
        .with_end_ticks (960)
        .with_name (u8"Chord Region")
        .build_in_registry ();
    auto * region =
      region_ref.get_object_as<structure::arrangement::ChordRegion> ();
    chord_track->structure::arrangement::ArrangerObjectOwner<
      structure::arrangement::ChordRegion>::add_object (region_ref);

    auto chord_ref =
      factory.get_builder<structure::arrangement::ChordObject> ()
        .with_start_ticks (0)
        .with_chord_descriptor (0)
        .build_in_registry ();
    region->add_object (chord_ref);
  }

  create_ui_state_and_undo_stack (*original_project);

  // Serialize
  nlohmann::json j1 = ProjectJsonSerializer::serialize (
    *original_project, *ui_state, *undo_stack, TEST_APP_VERSION,
    "Chord Region Test");

  // Validate against schema - this catches code/schema mismatches for chord
  // regions (which also have loop properties)
  EXPECT_NO_THROW ({
    ProjectJsonSerializer::validate_json (j1);
  }) << "Chord region serialization should produce schema-valid JSON";

  // Verify arranger objects are in the registry
  auto &obj_registry = j1["projectData"]["registries"]["arrangerObjectRegistry"];
  EXPECT_GE (obj_registry.size (), 2)
    << "Should have at least chord region and chord object";

  // Deserialize
  auto deserialized_project = create_minimal_project ();
  ASSERT_NE (deserialized_project, nullptr);
  create_ui_state_and_undo_stack (*deserialized_project);
  ProjectJsonSerializer::deserialize (
    j1, *deserialized_project, *ui_state, *undo_stack);

  // Serialize again
  nlohmann::json j2 = ProjectJsonSerializer::serialize (
    *deserialized_project, *ui_state, *undo_stack, TEST_APP_VERSION,
    "Chord Region Test");

  // Validate again
  EXPECT_NO_THROW ({ ProjectJsonSerializer::validate_json (j2); });

  // Verify registries match
  expect_registries_match (j1, j2);
}

// ============================================================================
// Property-Based Round-Trip Tests
// ============================================================================

/// Property-based test: All UUIDs should be preserved across round-trip
TEST_F (ProjectSerializationTest, RoundTrip_AllUuidsPreserved)
{
  auto original_project = create_minimal_project ();
  ASSERT_NE (original_project, nullptr);
  create_ui_state_and_undo_stack (*original_project);

  nlohmann::json j1 = ProjectJsonSerializer::serialize (
    *original_project, *ui_state, *undo_stack, TEST_APP_VERSION, "Test");

  auto deserialized_project = create_minimal_project ();
  ASSERT_NE (deserialized_project, nullptr);
  create_ui_state_and_undo_stack (*deserialized_project);
  ProjectJsonSerializer::deserialize (
    j1, *deserialized_project, *ui_state, *undo_stack);

  nlohmann::json j2 = ProjectJsonSerializer::serialize (
    *deserialized_project, *ui_state, *undo_stack, TEST_APP_VERSION, "Test");

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
  create_ui_state_and_undo_stack (*original_project);

  nlohmann::json j1 = ProjectJsonSerializer::serialize (
    *original_project, *ui_state, *undo_stack, TEST_APP_VERSION, "Test");

  auto deserialized_project = create_minimal_project ();
  ASSERT_NE (deserialized_project, nullptr);
  create_ui_state_and_undo_stack (*deserialized_project);
  ProjectJsonSerializer::deserialize (
    j1, *deserialized_project, *ui_state, *undo_stack);

  nlohmann::json j2 = ProjectJsonSerializer::serialize (
    *deserialized_project, *ui_state, *undo_stack, TEST_APP_VERSION, "Test");

  // Object count should be identical
  EXPECT_EQ (count_objects (j1), count_objects (j2))
    << "Object count changed across round-trip";
}

/// Property-based test: Second round-trip should be identical to first
TEST_F (ProjectSerializationTest, RoundTrip_Idempotent)
{
  auto project1 = create_minimal_project ();
  ASSERT_NE (project1, nullptr);
  create_ui_state_and_undo_stack (*project1);

  // First serialization
  nlohmann::json j1 = ProjectJsonSerializer::serialize (
    *project1, *ui_state, *undo_stack, TEST_APP_VERSION, "Test");

  // Deserialize into project2
  auto project2 = create_minimal_project ();
  ASSERT_NE (project2, nullptr);
  create_ui_state_and_undo_stack (*project2);
  ProjectJsonSerializer::deserialize (j1, *project2, *ui_state, *undo_stack);

  // Second serialization
  nlohmann::json j2 = ProjectJsonSerializer::serialize (
    *project2, *ui_state, *undo_stack, TEST_APP_VERSION, "Test");

  // Deserialize into project3
  auto project3 = create_minimal_project ();
  ASSERT_NE (project3, nullptr);
  create_ui_state_and_undo_stack (*project3);
  ProjectJsonSerializer::deserialize (j2, *project3, *ui_state, *undo_stack);

  // Third serialization - should be identical to second
  nlohmann::json j3 = ProjectJsonSerializer::serialize (
    *project3, *ui_state, *undo_stack, TEST_APP_VERSION, "Test");

  // j2 and j3 should have identical UUID sets
  auto uuids2 = extract_all_uuids (j2);
  auto uuids3 = extract_all_uuids (j3);
  EXPECT_EQ (uuids2, uuids3) << "UUIDs not stable after second round-trip";

  // j2 and j3 should have identical object counts
  EXPECT_EQ (count_objects (j2), count_objects (j3))
    << "Object count not stable after second round-trip";
}
}
