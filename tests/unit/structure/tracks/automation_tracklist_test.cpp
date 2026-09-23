// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/parameter.h"
#include "dsp/tempo_map.h"
#include "dsp/tempo_map_qml_adapter.h"
#include "dsp/timebase.h"
#include "structure/tracks/automation_track.h"
#include "structure/tracks/automation_tracklist.h"
#include "utils/exceptions.h"
#include "utils/object_registry.h"
#include "utils/registry_utils.h"

#include <gtest/gtest.h>

namespace zrythm::structure::tracks
{
class AutomationTracklistTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    tempo_map = std::make_unique<dsp::TempoMap> (units::sample_rate (44100.0));
    tempo_map_wrapper = std::make_unique<dsp::TempoMapWrapper> (*tempo_map);
    registry = std::make_unique<utils::ObjectRegistry> ();

    param_id1 = utils::create_object<dsp::ProcessorParameter> (
      *registry, *registry, dsp::ProcessorParameter::UniqueId (u8"test_param1"),
      dsp::ParameterRange (dsp::ParameterRange::Type::Linear, 0.0f, 1.0f),
      u8"Test Parameter 1");

    param_id2 = utils::create_object<dsp::ProcessorParameter> (
      *registry, *registry, dsp::ProcessorParameter::UniqueId (u8"test_param2"),
      dsp::ParameterRange (dsp::ParameterRange::Type::Linear, 0.0f, 1.0f),
      u8"Test Parameter 2");

    automation_tracklist = std::make_unique<AutomationTracklist> (
      AutomationTrackHolder::Dependencies{
        .tempo_map_ = *tempo_map_wrapper, .registry_ = *registry });
  }

  AutomationTrackUuidReference
  create_automation_track (dsp::ProcessorParameterUuidReference param_id)
  {
    return utils::create_object<AutomationTrack> (
      *registry, *tempo_map_wrapper, *registry, std::move (param_id));
  }

  std::unique_ptr<dsp::TempoMap>                      tempo_map;
  std::unique_ptr<dsp::TempoMapWrapper>               tempo_map_wrapper;
  std::unique_ptr<utils::ObjectRegistry>              registry;
  std::optional<dsp::ProcessorParameterUuidReference> param_id1;
  std::optional<dsp::ProcessorParameterUuidReference> param_id2;
  std::unique_ptr<AutomationTracklist>                automation_tracklist;
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
  auto   at = create_automation_track (*param_id1);
  auto * track_ptr = at.get ();
  automation_tracklist->add_automation_track (at);
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
  auto   at1 = create_automation_track (*param_id1);
  auto * track1_ptr = at1.get ();
  automation_tracklist->add_automation_track (at1);

  auto   at2 = create_automation_track (*param_id2);
  auto * track2_ptr = at2.get ();
  automation_tracklist->add_automation_track (at2);

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
  auto   at = create_automation_track (*param_id1);
  auto * track_ptr = at.get ();
  automation_tracklist->add_automation_track (at);
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
  holder->setCreatedByUser (true);
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
  auto   at1 = create_automation_track (*param_id1);
  auto * track1_ptr = at1.get ();
  automation_tracklist->add_automation_track (at1);

  auto   at2 = create_automation_track (*param_id2);
  auto * track2_ptr = at2.get ();
  automation_tracklist->add_automation_track (at2);

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
  auto   at1 = create_automation_track (*param_id1);
  auto * track1_ptr = at1.get ();
  automation_tracklist->add_automation_track (at1);

  auto   at2 = create_automation_track (*param_id2);
  auto * track2_ptr = at2.get ();
  automation_tracklist->add_automation_track (at2);

  // Set both visible for navigation tests
  auto * holder1 =
    automation_tracklist
      ->data (
        automation_tracklist->index (0),
        AutomationTracklist::AutomationTrackHolderRole)
      .value<AutomationTrackHolder *> ();
  holder1->setCreatedByUser (true);
  holder1->setVisible (true);

  auto * holder2 =
    automation_tracklist
      ->data (
        automation_tracklist->index (1),
        AutomationTracklist::AutomationTrackHolderRole)
      .value<AutomationTrackHolder *> ();
  holder2->setCreatedByUser (true);
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
    auto at = create_automation_track (*param_id1);
    automation_tracklist->add_automation_track (at);
  }
  automation_tracklist->automation_track_at (0)->add_object (
    utils::create_object<arrangement::AutomationClip> (
      *registry, *tempo_map_wrapper, *registry));

  EXPECT_EQ (
    automation_tracklist->automation_track_at (0)->get_children_vector ().size (),
    1);

  automation_tracklist->clear_arranger_objects ();

  // Clearing should remove all automation clips
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
  auto at = create_automation_track (*param_id1);
  automation_tracklist->add_automation_track (at);
  // Should return the new track since it's not created by user
  auto * holder =
    automation_tracklist->get_first_invisible_automation_track_holder ();
  EXPECT_NE (holder, nullptr);
  EXPECT_FALSE (holder->created_by_user_);

  // Mark as created and visible
  holder->setCreatedByUser (true);
  holder->setVisible (true);

  // Now should return nullptr since no invisible tracks
  EXPECT_EQ (
    automation_tracklist->get_first_invisible_automation_track_holder (),
    nullptr);
}

TEST_F (AutomationTracklistTest, AutomationVisibilityShowsFirstTrack)
{
  {
    auto at = create_automation_track (*param_id1);
    automation_tracklist->add_automation_track (at);
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
  auto at1 = create_automation_track (*param_id1);
  automation_tracklist->add_automation_track (at1);

  auto at2 = create_automation_track (*param_id2);
  automation_tracklist->add_automation_track (at2);

  // Set visibility for one track
  auto * holder =
    automation_tracklist
      ->data (
        automation_tracklist->index (0),
        AutomationTracklist::AutomationTrackHolderRole)
      .value<AutomationTrackHolder *> ();
  holder->setCreatedByUser (true);
  holder->setVisible (true);

  // Set visibility of the tracklist itself
  automation_tracklist->setAutomationVisible (true);

  // Serialize to JSON
  nlohmann::json j;
  to_json (j, *automation_tracklist);

  // Create dummy tracklist with same dependencies
  auto dummy_tracklist =
    std::make_unique<AutomationTracklist> (AutomationTrackHolder::Dependencies{
      .tempo_map_ = *tempo_map_wrapper, .registry_ = *registry });

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

TEST_F (AutomationTracklistTest, SerializationPreservesRegions)
{
  // Create an automation track with a clip containing a point
  auto   at1 = create_automation_track (*param_id1);
  auto * track_ptr = at1.get ();
  automation_tracklist->add_automation_track (at1);

  auto clip_ref = utils::create_object<arrangement::AutomationClip> (
    *registry, *tempo_map_wrapper, *registry);
  auto * clip = clip_ref.get_object_as<arrangement::AutomationClip> ();
  track_ptr->add_object (clip_ref);

  auto point_ref = utils::create_object<arrangement::AutomationPoint> (
    *registry, *tempo_map_wrapper);
  auto * point = point_ref.get_object_as<arrangement::AutomationPoint> ();
  point->setValue (0.75f);
  clip->add_object (point_ref);

  ASSERT_EQ (track_ptr->get_children_vector ().size (), 1);

  // Serialize
  nlohmann::json j;
  to_json (j, *automation_tracklist);

  // Deserialize into new tracklist
  auto tracklist2 =
    std::make_unique<AutomationTracklist> (AutomationTrackHolder::Dependencies{
      .tempo_map_ = *tempo_map_wrapper, .registry_ = *registry });
  from_json (j, *tracklist2);

  // Verify the automation track retained its clip
  auto * deserialized_at = tracklist2->automation_track_at (0);
  ASSERT_NE (deserialized_at, nullptr);
  EXPECT_EQ (deserialized_at->get_children_vector ().size (), 1)
    << "Automation track lost its clips during serialization round-trip";
}

// Deserialization attaches the automation tracks the JSON references
// (registered objects are shared, not copied)
TEST_F (AutomationTracklistTest, FromJsonAttachesRegisteredTracks)
{
  automation_tracklist->add_automation_track (
    create_automation_track (*param_id1));
  automation_tracklist->add_automation_track (
    create_automation_track (*param_id2));

  nlohmann::json j;
  to_json (j, *automation_tracklist);

  auto loaded =
    std::make_unique<AutomationTracklist> (AutomationTrackHolder::Dependencies{
      .tempo_map_ = *tempo_map_wrapper, .registry_ = *registry });
  from_json (j, *loaded);

  ASSERT_EQ (loaded->rowCount (), 2);
  for (int i = 0; i < 2; ++i)
    {
      EXPECT_EQ (
        loaded->automation_track_at (i)->get_uuid (),
        automation_tracklist->automation_track_at (i)->get_uuid ());
    }
}

// Attached automation tracks follow the timebase provider the tracklist
// was created with
TEST_F (AutomationTracklistTest, FromJsonKeepsTimebaseProviders)
{
  dsp::TimebaseProvider tbp;
  automation_tracklist = std::make_unique<
    AutomationTracklist> (AutomationTrackHolder::Dependencies{
    .tempo_map_ = *tempo_map_wrapper,
    .registry_ = *registry,
    .timebase_provider_ = &tbp });
  automation_tracklist->add_automation_track (
    create_automation_track (*param_id1));

  nlohmann::json j;
  to_json (j, *automation_tracklist);

  auto loaded = std::make_unique<
    AutomationTracklist> (AutomationTrackHolder::Dependencies{
    .tempo_map_ = *tempo_map_wrapper,
    .registry_ = *registry,
    .timebase_provider_ = &tbp });
  from_json (j, *loaded);

  EXPECT_EQ (loaded->automation_track_at (0)->timebaseProvider (), &tbp);
}

// An automationTrackId that does not reference an automation track is
// rejected at load time
TEST_F (AutomationTracklistTest, FromJsonRefusesNonAutomationTrackId)
{
  automation_tracklist->add_automation_track (
    create_automation_track (*param_id1));

  nlohmann::json j;
  to_json (j, *automation_tracklist);
  j["automationTracks"][0]["automationTrackId"] =
    type_safe::get (param_id1->id ());

  auto loaded =
    std::make_unique<AutomationTracklist> (AutomationTrackHolder::Dependencies{
      .tempo_map_ = *tempo_map_wrapper, .registry_ = *registry });
  EXPECT_THROW ({ from_json (j, *loaded); }, utils::ZrythmException);
  EXPECT_EQ (loaded->rowCount (), 0);
}

// Cloning produces new registered automation tracks with the same
// parameter, metadata and clips
TEST_F (AutomationTracklistTest, InitFromClonesAutomationTracks)
{
  auto at_ref = create_automation_track (*param_id1);
  automation_tracklist->add_automation_track (at_ref);

  auto clip_ref = utils::create_object<arrangement::AutomationClip> (
    *registry, *tempo_map_wrapper, *registry);
  at_ref.get ()->add_object (clip_ref);

  auto * holder =
    automation_tracklist
      ->data (
        automation_tracklist->index (0),
        AutomationTracklist::AutomationTrackHolderRole)
      .value<AutomationTrackHolder *> ();
  holder->setCreatedByUser (true);
  holder->setVisible (true);

  auto cloned =
    std::make_unique<AutomationTracklist> (AutomationTrackHolder::Dependencies{
      .tempo_map_ = *tempo_map_wrapper, .registry_ = *registry });
  // The destination starts with a basic automation track, as a freshly
  // constructed track would
  cloned->add_automation_track (create_automation_track (*param_id2));
  init_from (
    *cloned, *automation_tracklist, utils::ObjectCloneType::NewIdentity);

  // Cloning replaces the destination's tracks instead of appending
  ASSERT_EQ (cloned->rowCount (), 1);
  auto * cloned_at = cloned->automation_track_at (0);
  ASSERT_NE (cloned_at, nullptr);
  EXPECT_NE (cloned_at->get_uuid (), at_ref.id ());
  EXPECT_TRUE (utils::contains (*registry, cloned_at->get_uuid ()));
  EXPECT_EQ (cloned_at->parameter (), at_ref.get ()->parameter ());
  EXPECT_EQ (cloned_at->get_children_vector ().size (), 1);
  auto * cloned_holder =
    cloned
      ->data (cloned->index (0), AutomationTracklist::AutomationTrackHolderRole)
      .value<AutomationTrackHolder *> ();
  EXPECT_TRUE (cloned_holder->created_by_user_);
  EXPECT_TRUE (cloned_holder->visible ());

  // The clone's UUID matches what its holder serializes, and the
  // tracklist survives a serialization roundtrip
  nlohmann::json j;
  to_json (j, *cloned);
  EXPECT_EQ (
    j["automationTracks"][0]["automationTrackId"].get<QUuid> (),
    type_safe::get (cloned_at->get_uuid ()));
  auto reloaded =
    std::make_unique<AutomationTracklist> (AutomationTrackHolder::Dependencies{
      .tempo_map_ = *tempo_map_wrapper, .registry_ = *registry });
  from_json (j, *reloaded);
  ASSERT_EQ (reloaded->rowCount (), 1);
}

// A cloned automation track shares the source's parameter, and a
// parameter holds at most one automation provider: the clone must not
// take over the provider, and destroying the clone must not unhook the
// source's automation
TEST_F (AutomationTracklistTest, InitFromCloneKeepsSourceProviderHooked)
{
  auto at_ref = create_automation_track (*param_id1);
  automation_tracklist->add_automation_track (at_ref);
  auto * param = at_ref.get ()->parameter ();
  ASSERT_TRUE (param->hasAutomationProvider ());

  auto cloned =
    std::make_unique<AutomationTracklist> (AutomationTrackHolder::Dependencies{
      .tempo_map_ = *tempo_map_wrapper, .registry_ = *registry });
  init_from (
    *cloned, *automation_tracklist, utils::ObjectCloneType::NewIdentity);
  EXPECT_TRUE (param->hasAutomationProvider ());

  cloned.reset ();
  EXPECT_TRUE (param->hasAutomationProvider ());
}

// Adding an automation track rewires its timebase to the target
// tracklist's provider (automation tracks moved between tracklists rely
// on this)
TEST_F (AutomationTracklistTest, AddAutomationTrackRewiresTimebaseProvider)
{
  dsp::TimebaseProvider source_tbp;
  dsp::TimebaseProvider target_tbp;

  automation_tracklist = std::make_unique<
    AutomationTracklist> (AutomationTrackHolder::Dependencies{
    .tempo_map_ = *tempo_map_wrapper,
    .registry_ = *registry,
    .timebase_provider_ = &source_tbp });
  auto at_ref = create_automation_track (*param_id1);
  automation_tracklist->add_automation_track (at_ref);
  ASSERT_EQ (at_ref.get ()->timebaseProvider (), &source_tbp);

  AutomationTracklist target (
    AutomationTrackHolder::Dependencies{
      .tempo_map_ = *tempo_map_wrapper,
      .registry_ = *registry,
      .timebase_provider_ = &target_tbp });
  target.add_automation_track (at_ref);
  EXPECT_EQ (at_ref.get ()->timebaseProvider (), &target_tbp);
}

} // namespace zrythm::structure::tracks
