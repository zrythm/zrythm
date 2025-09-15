// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/parameter.h"
#include "dsp/tempo_map.h"
#include "structure/tracks/automation_track.h"
#include "structure/tracks/automation_tracklist.h"

#include <gtest/gtest.h>

namespace zrythm::structure::tracks
{
class AutomationTracklistTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    tempo_map = std::make_unique<dsp::TempoMap> (44100.0);
    tempo_map_wrapper = std::make_unique<dsp::TempoMapWrapper> (*tempo_map);

    // Create test parameters
    param_id1 = processor_param_registry.create_object<dsp::ProcessorParameter> (
      port_registry, dsp::ProcessorParameter::UniqueId (u8"test_param1"),
      dsp::ParameterRange (dsp::ParameterRange::Type::Linear, 0.0f, 1.0f),
      u8"Test Parameter 1");

    param_id2 = processor_param_registry.create_object<dsp::ProcessorParameter> (
      port_registry, dsp::ProcessorParameter::UniqueId (u8"test_param2"),
      dsp::ParameterRange (dsp::ParameterRange::Type::Linear, 0.0f, 1.0f),
      u8"Test Parameter 2");

    // Create automation tracklist
    automation_tracklist = std::make_unique<
      AutomationTracklist> (AutomationTrackHolder::Dependencies{
      .tempo_map_ = *tempo_map_wrapper,
      .file_audio_source_registry_ = file_audio_source_registry,
      .port_registry_ = port_registry,
      .param_registry_ = processor_param_registry,
      .object_registry_ = obj_registry });
  }

  // Helper to create an automation track
  utils::QObjectUniquePtr<AutomationTrack>
  create_automation_track (dsp::ProcessorParameterUuidReference param_id)
  {
    return utils::make_qobject_unique<AutomationTrack> (
      *tempo_map_wrapper, file_audio_source_registry, obj_registry,
      std::move (param_id));
  }

  std::unique_ptr<dsp::TempoMap>        tempo_map;
  std::unique_ptr<dsp::TempoMapWrapper> tempo_map_wrapper;
  dsp::PortRegistry                     port_registry;
  dsp::ProcessorParameterRegistry processor_param_registry{ port_registry };
  dsp::ProcessorParameterUuidReference param_id1{ processor_param_registry };
  dsp::ProcessorParameterUuidReference param_id2{ processor_param_registry };
  structure::arrangement::ArrangerObjectRegistry obj_registry;
  dsp::FileAudioSourceRegistry                   file_audio_source_registry;
  std::unique_ptr<AutomationTracklist>           automation_tracklist;
};

TEST_F (AutomationTracklistTest, InitialState)
{
  EXPECT_EQ (automation_tracklist->rowCount (), 0);
  EXPECT_FALSE (automation_tracklist->automationVisible ());
}

TEST_F (AutomationTracklistTest, AutomationVisibility)
{
  automation_tracklist->setAutomationVisible (true);
  EXPECT_TRUE (automation_tracklist->automationVisible ());

  automation_tracklist->setAutomationVisible (false);
  EXPECT_FALSE (automation_tracklist->automationVisible ());
}

TEST_F (AutomationTracklistTest, AddAutomationTrack)
{
  auto   at = create_automation_track (param_id1);
  auto * track_ptr = at.get ();
  automation_tracklist->add_automation_track (std::move (at));

  EXPECT_EQ (automation_tracklist->rowCount (), 1);

  auto * holder =
    automation_tracklist
      ->data (
        automation_tracklist->index (0),
        AutomationTracklist::AutomationTrackHolderRole)
      .value<AutomationTrackHolder *> ();
  EXPECT_EQ (holder->automationTrack (), track_ptr);
}

TEST_F (AutomationTracklistTest, RemoveAutomationTrack)
{
  auto   at1 = create_automation_track (param_id1);
  auto * track1_ptr = at1.get ();
  automation_tracklist->add_automation_track (std::move (at1));

  auto   at2 = create_automation_track (param_id2);
  auto * track2_ptr = at2.get ();
  automation_tracklist->add_automation_track (std::move (at2));

  EXPECT_EQ (automation_tracklist->rowCount (), 2);

  // Remove first track
  auto holder = automation_tracklist->remove_automation_track (*track1_ptr);
  EXPECT_EQ (automation_tracklist->rowCount (), 1);
  EXPECT_EQ (holder->automationTrack (), track1_ptr);

  // Verify remaining track
  auto * remaining_holder =
    automation_tracklist
      ->data (
        automation_tracklist->index (0),
        AutomationTracklist::AutomationTrackHolderRole)
      .value<AutomationTrackHolder *> ();
  EXPECT_EQ (remaining_holder->automationTrack (), track2_ptr);
}

TEST_F (AutomationTracklistTest, VisibilityManagement)
{
  auto   at = create_automation_track (param_id1);
  auto * track_ptr = at.get ();
  automation_tracklist->add_automation_track (std::move (at));

  auto * holder =
    automation_tracklist
      ->data (
        automation_tracklist->index (0),
        AutomationTracklist::AutomationTrackHolderRole)
      .value<AutomationTrackHolder *> ();

  // Initially should not be visible or created by user
  EXPECT_FALSE (holder->visible ());
  EXPECT_FALSE (holder->created_by_user_);

  // Show track
  holder->created_by_user_ = true;
  holder->setVisible (true);
  EXPECT_TRUE (holder->visible ());

  // Hide track
  automation_tracklist->hideAutomationTrack (track_ptr);
  EXPECT_FALSE (holder->visible ());

  // Show next available track
  automation_tracklist->showNextAvailableAutomationTrack (track_ptr);
  EXPECT_TRUE (holder->visible ());
}

TEST_F (AutomationTracklistTest, TrackMovement)
{
  auto   at1 = create_automation_track (param_id1);
  auto * track1_ptr = at1.get ();
  automation_tracklist->add_automation_track (std::move (at1));

  auto   at2 = create_automation_track (param_id2);
  auto * track2_ptr = at2.get ();
  automation_tracklist->add_automation_track (std::move (at2));

  // Move track2 to position 0 (swap)
  automation_tracklist->set_automation_track_index (*track2_ptr, 0, false);

  // Verify new order
  auto * holder0 =
    automation_tracklist
      ->data (
        automation_tracklist->index (0),
        AutomationTracklist::AutomationTrackHolderRole)
      .value<AutomationTrackHolder *> ();
  EXPECT_EQ (holder0->automationTrack (), track2_ptr);

  auto * holder1 =
    automation_tracklist
      ->data (
        automation_tracklist->index (1),
        AutomationTracklist::AutomationTrackHolderRole)
      .value<AutomationTrackHolder *> ();
  EXPECT_EQ (holder1->automationTrack (), track1_ptr);

  // Move track2 to position 1 (push down)
  automation_tracklist->set_automation_track_index (*track2_ptr, 1, true);

  // Verify new order
  holder0 =
    automation_tracklist
      ->data (
        automation_tracklist->index (0),
        AutomationTracklist::AutomationTrackHolderRole)
      .value<AutomationTrackHolder *> ();
  EXPECT_EQ (holder0->automationTrack (), track1_ptr);

  holder1 =
    automation_tracklist
      ->data (
        automation_tracklist->index (1),
        AutomationTracklist::AutomationTrackHolderRole)
      .value<AutomationTrackHolder *> ();
  EXPECT_EQ (holder1->automationTrack (), track2_ptr);
}

TEST_F (AutomationTracklistTest, FindTracks)
{
  auto   at1 = create_automation_track (param_id1);
  auto * track1_ptr = at1.get ();
  automation_tracklist->add_automation_track (std::move (at1));

  auto   at2 = create_automation_track (param_id2);
  auto * track2_ptr = at2.get ();
  automation_tracklist->add_automation_track (std::move (at2));

  // Set both visible for navigation tests
  auto * holder1 =
    automation_tracklist
      ->data (
        automation_tracklist->index (0),
        AutomationTracklist::AutomationTrackHolderRole)
      .value<AutomationTrackHolder *> ();
  holder1->created_by_user_ = true;
  holder1->setVisible (true);

  auto * holder2 =
    automation_tracklist
      ->data (
        automation_tracklist->index (1),
        AutomationTracklist::AutomationTrackHolderRole)
      .value<AutomationTrackHolder *> ();
  holder2->created_by_user_ = true;
  holder2->setVisible (true);

  // Test navigation
  EXPECT_EQ (
    automation_tracklist->get_next_visible_automation_track (*track1_ptr),
    track2_ptr);
  EXPECT_EQ (
    automation_tracklist->get_previous_visible_automation_track (*track2_ptr),
    track1_ptr);

  // Test count between tracks
  EXPECT_EQ (
    automation_tracklist->get_visible_automation_track_count_between (
      *track1_ptr, *track2_ptr),
    1);
  EXPECT_EQ (
    automation_tracklist->get_visible_automation_track_count_between (
      *track2_ptr, *track1_ptr),
    -1);
}

TEST_F (AutomationTracklistTest, ClearObjects)
{
  {
    auto at = create_automation_track (param_id1);
    automation_tracklist->add_automation_track (std::move (at));
  }
  automation_tracklist->automation_track_at (0)->add_object (
    obj_registry.create_object<arrangement::AutomationRegion> (
      *tempo_map, obj_registry, file_audio_source_registry));

  EXPECT_EQ (
    automation_tracklist->automation_track_at (0)->get_children_vector ().size (),
    1);

  automation_tracklist->clear_arranger_objects ();

  // Clearing should remove all automation regions
  EXPECT_EQ (
    automation_tracklist->automation_track_at (0)->get_children_vector ().size (),
    0);
}

TEST_F (AutomationTracklistTest, FirstInvisibleTrack)
{
  // Initially no tracks, should return nullptr
  EXPECT_EQ (
    automation_tracklist->get_first_invisible_automation_track_holder (),
    nullptr);

  // Add a track
  auto at = create_automation_track (param_id1);
  automation_tracklist->add_automation_track (std::move (at));

  // Should return the new track since it's not created by user
  auto * holder =
    automation_tracklist->get_first_invisible_automation_track_holder ();
  EXPECT_NE (holder, nullptr);
  EXPECT_FALSE (holder->created_by_user_);

  // Mark as created and visible
  holder->created_by_user_ = true;
  holder->setVisible (true);

  // Now should return nullptr since no invisible tracks
  EXPECT_EQ (
    automation_tracklist->get_first_invisible_automation_track_holder (),
    nullptr);
}

TEST_F (AutomationTracklistTest, AutomationVisibilityShowsFirstTrack)
{
  {
    auto at = create_automation_track (param_id1);
    automation_tracklist->add_automation_track (std::move (at));
  }

  // When setting automation visible and no tracks are visible, should show
  // first track
  automation_tracklist->setAutomationVisible (true);

  auto * holder =
    automation_tracklist
      ->data (
        automation_tracklist->index (0),
        AutomationTracklist::AutomationTrackHolderRole)
      .value<AutomationTrackHolder *> ();
  EXPECT_TRUE (holder->visible ());
  EXPECT_TRUE (holder->created_by_user_);
}

TEST_F (AutomationTracklistTest, Serialization)
{
  // Add tracks to tracklist
  auto at1 = create_automation_track (param_id1);
  automation_tracklist->add_automation_track (std::move (at1));

  auto at2 = create_automation_track (param_id2);
  automation_tracklist->add_automation_track (std::move (at2));

  // Set visibility for one track
  auto * holder =
    automation_tracklist
      ->data (
        automation_tracklist->index (0),
        AutomationTracklist::AutomationTrackHolderRole)
      .value<AutomationTrackHolder *> ();
  holder->created_by_user_ = true;
  holder->setVisible (true);

  // Set visibility of the tracklist itself
  automation_tracklist->setAutomationVisible (true);

  // Serialize to JSON
  nlohmann::json j;
  to_json (j, *automation_tracklist);

  // Create dummy tracklist with same dependencies
  auto dummy_tracklist = std::make_unique<
    AutomationTracklist> (AutomationTrackHolder::Dependencies{
    .tempo_map_ = *tempo_map_wrapper,
    .file_audio_source_registry_ = file_audio_source_registry,
    .port_registry_ = port_registry,
    .param_registry_ = processor_param_registry,
    .object_registry_ = obj_registry });

  // Deserialize into dummy tracklist
  from_json (j, *dummy_tracklist);

  // Verify serialization/deserialization
  EXPECT_EQ (dummy_tracklist->rowCount (), 2);
  EXPECT_TRUE (dummy_tracklist->automationVisible ());

  // Verify track properties
  auto * dummy_holder0 =
    dummy_tracklist
      ->data (
        dummy_tracklist->index (0), AutomationTracklist::AutomationTrackHolderRole)
      .value<AutomationTrackHolder *> ();
  EXPECT_TRUE (dummy_holder0->visible ());
  EXPECT_TRUE (dummy_holder0->created_by_user_);

  auto * dummy_holder1 =
    dummy_tracklist
      ->data (
        dummy_tracklist->index (1), AutomationTracklist::AutomationTrackHolderRole)
      .value<AutomationTrackHolder *> ();
  EXPECT_FALSE (dummy_holder1->visible ());
  EXPECT_FALSE (dummy_holder1->created_by_user_);

  // Verify track parameters
  EXPECT_EQ (
    dummy_holder0->automationTrack ()->parameter ()->label (),
    "Test Parameter 1");
  EXPECT_EQ (
    dummy_holder1->automationTrack ()->parameter ()->label (),
    "Test Parameter 2");
}

} // namespace zrythm::structure::tracks
