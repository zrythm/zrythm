// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/tempo_map.h"
#include "structure/arrangement/arranger_object_all.h"
#include "structure/tracks/track_lane_list.h"

#include <QObject>
#include <QSignalSpy>

#include "helpers/scoped_qcoreapplication.h"

#include "unit/dsp/graph_helpers.h"
#include <gtest/gtest.h>

namespace zrythm::structure::tracks
{

class TrackLaneListTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    scoped_qapplication_ =
      std::make_unique<test_helpers::ScopedQCoreApplication> ();

    obj_registry_ = std::make_unique<arrangement::ArrangerObjectRegistry> ();
    file_audio_source_registry_ =
      std::make_unique<dsp::FileAudioSourceRegistry> ();

    // Create dependencies for TrackLane
    TrackLane::TrackLaneDependencies deps{
      .obj_registry_ = *obj_registry_,
      .file_audio_source_registry_ = *file_audio_source_registry_,
    };

    lane_list_ = std::make_unique<TrackLaneList> (
      deps.obj_registry_, deps.file_audio_source_registry_);
  }

  void TearDown () override { lane_list_.reset (); }

  std::unique_ptr<arrangement::ArrangerObjectRegistry> obj_registry_;
  std::unique_ptr<dsp::FileAudioSourceRegistry> file_audio_source_registry_;
  bool                                          soloed_lanes_exist_ = false;
  std::unique_ptr<TrackLaneList>                lane_list_;
  std::unique_ptr<test_helpers::ScopedQCoreApplication> scoped_qapplication_;
};

TEST_F (TrackLaneListTest, ConstructionAndBasicProperties)
{
  // Test default construction
  EXPECT_NE (lane_list_, nullptr);
  EXPECT_EQ (lane_list_->size (), 0);
  EXPECT_TRUE (lane_list_->empty ());
}

TEST_F (TrackLaneListTest, AddLanes)
{
  // Test adding lanes
  auto * lane1 = lane_list_->addLane ();
  EXPECT_EQ (lane_list_->size (), 1);
  EXPECT_FALSE (lane_list_->empty ());
  EXPECT_EQ (lane1->name (), QString ("Lane 1"));

  auto * lane2 = lane_list_->addLane ();
  EXPECT_EQ (lane_list_->size (), 2);
  EXPECT_EQ (lane2->name (), QString ("Lane 2"));

  auto * lane3 = lane_list_->addLane ();
  EXPECT_EQ (lane_list_->size (), 3);
  EXPECT_EQ (lane3->name (), QString ("Lane 3"));

  // Test lane ownership
  EXPECT_EQ (lane1->parent (), lane_list_.get ());
  EXPECT_EQ (lane2->parent (), lane_list_.get ());
  EXPECT_EQ (lane3->parent (), lane_list_.get ());
}

TEST_F (TrackLaneListTest, InsertLanes)
{
  // Add initial lanes
  lane_list_->addLane ();
  lane_list_->addLane ();
  lane_list_->addLane ();

  // Test inserting at beginning
  auto * inserted_lane = lane_list_->insertLane (0);
  EXPECT_EQ (lane_list_->size (), 4);
  EXPECT_EQ (inserted_lane->name (), QString ("Lane 1"));
  EXPECT_EQ (lane_list_->at (0), inserted_lane);
  EXPECT_EQ (lane_list_->at (1)->name (), QString ("Lane 2"));

  // Test inserting in middle
  auto * middle_lane = lane_list_->insertLane (2);
  EXPECT_EQ (lane_list_->size (), 5);
  EXPECT_EQ (middle_lane->name (), QString ("Lane 3"));
  EXPECT_EQ (lane_list_->at (2), middle_lane);

  // Test inserting at end
  auto * end_lane = lane_list_->insertLane (5);
  EXPECT_EQ (lane_list_->size (), 6);
  EXPECT_EQ (end_lane->name (), QString ("Lane 6"));
  EXPECT_EQ (lane_list_->at (5), end_lane);
}

TEST_F (TrackLaneListTest, RemoveLanes)
{
  // Add test lanes
  auto * lane1 = lane_list_->addLane ();
  lane_list_->addLane ();
  auto * lane3 = lane_list_->addLane ();

  // Test removing from middle
  lane_list_->removeLane (1);
  EXPECT_EQ (lane_list_->size (), 2);
  EXPECT_EQ (lane_list_->at (0), lane1);
  EXPECT_EQ (lane_list_->at (1), lane3);

  // Test removing from beginning
  lane_list_->removeLane (0);
  EXPECT_EQ (lane_list_->size (), 1);
  EXPECT_EQ (lane_list_->at (0), lane3);

  // Test removing last lane
  lane_list_->removeLane (0);
  EXPECT_EQ (lane_list_->size (), 0);
  EXPECT_TRUE (lane_list_->empty ());
}

TEST_F (TrackLaneListTest, MoveLanes)
{
  // Add test lanes
  auto * lane0 = lane_list_->addLane ();
  auto * lane1 = lane_list_->addLane ();
  auto * lane2 = lane_list_->addLane ();
  auto * lane3 = lane_list_->addLane ();

  // Test moving lane up (earlier in list)
  lane_list_->moveLane (2, 0); // Move lane2 to position 0
  EXPECT_EQ (lane_list_->size (), 4);
  EXPECT_EQ (lane_list_->at (0), lane2);
  EXPECT_EQ (lane_list_->at (1), lane0);
  EXPECT_EQ (lane_list_->at (2), lane1);
  EXPECT_EQ (lane_list_->at (3), lane3);

  // Test moving lane down (later in list)
  lane_list_->moveLane (1, 3); // Move lane1 to position 3
  EXPECT_EQ (lane_list_->at (0), lane2);
  EXPECT_EQ (lane_list_->at (1), lane1);
  EXPECT_EQ (lane_list_->at (2), lane3);
  EXPECT_EQ (lane_list_->at (3), lane0);

  // Test moving to same position (no-op)
  lane_list_->moveLane (1, 1);
  EXPECT_EQ (lane_list_->at (0), lane2);
  EXPECT_EQ (lane_list_->at (1), lane1);
  EXPECT_EQ (lane_list_->at (2), lane3);
  EXPECT_EQ (lane_list_->at (3), lane0);
}

TEST_F (TrackLaneListTest, LaneAccess)
{
  // Add test lanes
  auto * lane1 = lane_list_->addLane ();
  auto * lane2 = lane_list_->addLane ();
  auto * lane3 = lane_list_->addLane ();

  // Test at() access
  EXPECT_EQ (lane_list_->at (0), lane1);
  EXPECT_EQ (lane_list_->at (1), lane2);
  EXPECT_EQ (lane_list_->at (2), lane3);

  // Test bounds checking
  EXPECT_THROW (lane_list_->at (3), std::out_of_range);
  EXPECT_THROW (lane_list_->at (-1), std::out_of_range);
}

TEST_F (TrackLaneListTest, LaneNaming)
{
  // Test automatic naming
  auto * lane1 = lane_list_->addLane ();
  EXPECT_EQ (lane1->name (), QString ("Lane 1"));

  auto * lane2 = lane_list_->addLane ();
  EXPECT_EQ (lane2->name (), QString ("Lane 2"));

  // Test naming after removal
  lane_list_->removeLane (0);
  auto * lane3 = lane_list_->addLane ();
  EXPECT_EQ (
    lane3->name (), QString ("Lane 2")); // All lanes should be renumbered

  // Test naming after insertion
  auto * inserted_lane = lane_list_->insertLane (0);
  EXPECT_EQ (inserted_lane->name (), QString ("Lane 1")); // Should renumber

  // Test naming after move
  lane_list_->moveLane (0, 2);
  EXPECT_EQ (lane_list_->at (0)->name (), QString ("Lane 1"));
  EXPECT_EQ (lane_list_->at (1)->name (), QString ("Lane 2"));
  EXPECT_EQ (lane_list_->at (2)->name (), QString ("Lane 3"));
}

TEST_F (TrackLaneListTest, JsonSerializationRoundtrip)
{
  // Add test lanes with properties
  auto * lane1 = lane_list_->addLane ();
  lane1->setName ("MIDI Lane");
  lane1->setHeight (64.0);
  lane1->setMuted (false);
  lane1->setSoloed (true);

  auto * lane2 = lane_list_->addLane ();
  lane2->setName ("Audio Lane");
  lane2->setHeight (48.0);
  lane2->setMuted (true);
  lane2->setSoloed (false);

  lane_list_->setLanesVisible (true);

  // Serialize to JSON
  nlohmann::json j = *lane_list_;

  // Create new list from JSON
  TrackLane::TrackLaneDependencies deps{
    .obj_registry_ = *obj_registry_,
    .file_audio_source_registry_ = *file_audio_source_registry_,
    .soloed_lanes_exist_func_ = [this] () { return soloed_lanes_exist_; }
  };
  TrackLaneList deserialized_list (
    deps.obj_registry_, deps.file_audio_source_registry_);

  // Deserialize from JSON
  from_json (j, deserialized_list);

  // Verify properties
  EXPECT_TRUE (deserialized_list.lanesVisible ());

  EXPECT_EQ (deserialized_list.size (), 2);
  EXPECT_EQ (deserialized_list.at (0)->name (), QString ("MIDI Lane"));
  EXPECT_DOUBLE_EQ (deserialized_list.at (0)->height (), 64.0);
  EXPECT_FALSE (deserialized_list.at (0)->muted ());
  EXPECT_TRUE (deserialized_list.at (0)->soloed ());

  EXPECT_EQ (deserialized_list.at (1)->name (), QString ("Audio Lane"));
  EXPECT_DOUBLE_EQ (deserialized_list.at (1)->height (), 48.0);
  EXPECT_TRUE (deserialized_list.at (1)->muted ());
  EXPECT_FALSE (deserialized_list.at (1)->soloed ());
}

TEST_F (TrackLaneListTest, EmptyListSerialization)
{
  // Test serialization of empty list
  EXPECT_TRUE (lane_list_->empty ());

  nlohmann::json j = *lane_list_;

  TrackLane::TrackLaneDependencies deps{
    .obj_registry_ = *obj_registry_,
    .file_audio_source_registry_ = *file_audio_source_registry_,
    .soloed_lanes_exist_func_ = [this] () { return soloed_lanes_exist_; }
  };
  TrackLaneList deserialized_list (
    deps.obj_registry_, deps.file_audio_source_registry_);

  from_json (j, deserialized_list);

  EXPECT_TRUE (deserialized_list.empty ());
  EXPECT_EQ (deserialized_list.size (), 0);
}

TEST_F (TrackLaneListTest, SingleLaneOperations)
{
  // Test single lane operations
  auto * lane = lane_list_->addLane ();
  EXPECT_EQ (lane_list_->size (), 1);
  EXPECT_EQ (lane_list_->at (0), lane);

  // Test removing single lane
  lane_list_->removeLane (0);
  EXPECT_EQ (lane_list_->size (), 0);
  EXPECT_TRUE (lane_list_->empty ());
}

TEST_F (TrackLaneListTest, LargeNumberOfLanes)
{
  // Test with many lanes
  const int                num_lanes = 100;
  std::vector<TrackLane *> lanes;

  lanes.reserve (num_lanes);
  for (int i = 0; i < num_lanes; ++i)
    {
      lanes.push_back (lane_list_->addLane ());
    }

  EXPECT_EQ (lane_list_->size (), num_lanes);

  // Verify all lanes have correct names
  for (int i = 0; i < num_lanes; ++i)
    {
      EXPECT_EQ (lane_list_->at (i)->name (), QString ("Lane %1").arg (i + 1));
    }

  // Test removing from middle
  lane_list_->removeLane (50);
  EXPECT_EQ (lane_list_->size (), num_lanes - 1);
  EXPECT_EQ (lane_list_->at (50)->name (), QString ("Lane 51"));
}

TEST_F (TrackLaneListTest, LaneOwnership)
{
  // Test that lanes are properly parented to the list
  auto * lane1 = lane_list_->addLane ();
  auto * lane2 = lane_list_->addLane ();

  EXPECT_EQ (lane1->parent (), lane_list_.get ());
  EXPECT_EQ (lane2->parent (), lane_list_.get ());

  // Test that removed lanes are properly deleted
  lane_list_->removeLane (0);
  EXPECT_EQ (lane_list_->size (), 1);
}

TEST_F (TrackLaneListTest, Iteration)
{
  // Add test lanes
  auto * lane1 = lane_list_->addLane ();
  auto * lane2 = lane_list_->addLane ();
  auto * lane3 = lane_list_->addLane ();

  // Test iteration using range-based for
  std::vector<TrackLane *> collected_lanes;
  for (auto * lane : lane_list_->lanes_view ())
    {
      collected_lanes.push_back (lane);
    }

  EXPECT_EQ (collected_lanes.size (), 3);
  EXPECT_EQ (collected_lanes[0], lane1);
  EXPECT_EQ (collected_lanes[1], lane2);
  EXPECT_EQ (collected_lanes[2], lane3);

  // Test const iteration
  const TrackLaneList           &const_list = *lane_list_;
  std::vector<const TrackLane *> const_collected_lanes;
  for (const auto * lane : const_list.lanes_view ())
    {
      const_collected_lanes.push_back (lane);
    }

  EXPECT_EQ (const_collected_lanes.size (), 3);
  EXPECT_EQ (const_collected_lanes[0], lane1);
  EXPECT_EQ (const_collected_lanes[1], lane2);
  EXPECT_EQ (const_collected_lanes[2], lane3);
}

TEST_F (TrackLaneListTest, EdgeCases)
{
  // Test operations on empty list
  EXPECT_THROW (lane_list_->at (0), std::out_of_range);
  EXPECT_THROW (lane_list_->removeLane (0), std::out_of_range);
  EXPECT_THROW (lane_list_->moveLane (0, 1), std::out_of_range);

  // Test invalid indices
  lane_list_->addLane ();
  EXPECT_THROW (lane_list_->at (-1), std::out_of_range);
  EXPECT_THROW (lane_list_->at (1), std::out_of_range);
  EXPECT_THROW (lane_list_->insertLane (-1), std::out_of_range);
  EXPECT_THROW (lane_list_->insertLane (2), std::out_of_range);
  EXPECT_THROW (lane_list_->moveLane (-1, 0), std::out_of_range);
  EXPECT_THROW (lane_list_->moveLane (1, 0), std::out_of_range);
  EXPECT_THROW (lane_list_->moveLane (0, -1), std::out_of_range);
  EXPECT_THROW (lane_list_->moveLane (0, 2), std::out_of_range);
}

TEST_F (TrackLaneListTest, LanesVisibleProperty)
{
  // Test default value
  EXPECT_FALSE (lane_list_->lanesVisible ());

  // Test setting lanes visible
  bool signal_emitted = false;
  QObject::connect (
    lane_list_.get (), &TrackLaneList::lanesVisibleChanged, lane_list_.get (),
    [&signal_emitted] (bool visible) { signal_emitted = true; });

  lane_list_->setLanesVisible (true);
  EXPECT_TRUE (lane_list_->lanesVisible ());
  EXPECT_TRUE (signal_emitted);

  // Test setting same value (should not emit signal)
  signal_emitted = false;
  lane_list_->setLanesVisible (true);
  EXPECT_FALSE (signal_emitted);
  EXPECT_TRUE (lane_list_->lanesVisible ());

  // Test setting back to false
  lane_list_->setLanesVisible (false);
  EXPECT_FALSE (lane_list_->lanesVisible ());
}

TEST_F (TrackLaneListTest, GetVisibleLaneHeights)
{
  // Test with no lanes
  lane_list_->setLanesVisible (true);
  EXPECT_DOUBLE_EQ (lane_list_->get_visible_lane_heights (), 0.0);

  // Test with lanes visible
  auto * lane1 = lane_list_->addLane ();
  lane1->setHeight (64.0);
  auto * lane2 = lane_list_->addLane ();
  lane2->setHeight (48.0);
  auto * lane3 = lane_list_->addLane ();
  lane3->setHeight (32.0);

  EXPECT_DOUBLE_EQ (lane_list_->get_visible_lane_heights (), 144.0);

  // Test with lanes hidden
  lane_list_->setLanesVisible (false);
  EXPECT_DOUBLE_EQ (lane_list_->get_visible_lane_heights (), 0.0);

  // Test with lanes visible again
  lane_list_->setLanesVisible (true);
  EXPECT_DOUBLE_EQ (lane_list_->get_visible_lane_heights (), 144.0);
}

TEST_F (TrackLaneListTest, CreateMissingLanes)
{
  // Test creating missing lanes
  EXPECT_EQ (lane_list_->size (), 0);

  lane_list_->create_missing_lanes (2);
  EXPECT_EQ (lane_list_->size (), 4); // Creates lanes 0, 1, 2, 3

  // Test creating fewer lanes (should not remove)
  lane_list_->create_missing_lanes (1);
  EXPECT_EQ (lane_list_->size (), 4);

  // Test creating more lanes
  lane_list_->create_missing_lanes (4);
  EXPECT_EQ (lane_list_->size (), 6); // Creates lanes 3, 4, 5

  // Verify lane names
  for (size_t i = 0; i < lane_list_->size (); ++i)
    {
      EXPECT_EQ (lane_list_->at (i)->name (), QString ("Lane %1").arg (i + 1));
    }
}

TEST_F (TrackLaneListTest, RemoveEmptyLastLanes)
{
  // Test with no lanes
  lane_list_->remove_empty_last_lanes ();
  EXPECT_EQ (lane_list_->size (), 0);

  // Add lanes with no regions (all empty)
  lane_list_->addLane ();
  lane_list_->addLane ();
  lane_list_->addLane ();
  EXPECT_EQ (lane_list_->size (), 3);

  // Remove empty last lanes (should keep at least one)
  lane_list_->remove_empty_last_lanes ();
  EXPECT_EQ (lane_list_->size (), 1);

  // Add more lanes
  lane_list_->addLane ();
  lane_list_->addLane ();
  EXPECT_EQ (lane_list_->size (), 3);

  // Remove empty last lanes (should keep only first lane)
  lane_list_->remove_empty_last_lanes ();
  EXPECT_EQ (lane_list_->size (), 1);
}

// Test laneObjectsNeedRecache signal emission on lane object changes
TEST_F (TrackLaneListTest, LaneObjectsNeedRecacheSignal)
{
  // Add a lane
  auto * lane = lane_list_->addLane ();

  QSignalSpy recacheSpy (
    lane_list_.get (), &TrackLaneList::laneObjectsNeedRecache);

  // Create a MIDI region and add it to the lane
  auto tempo_map =
    std::make_unique<dsp::TempoMap> (units::sample_rate (44100.0));
  auto region_ref = obj_registry_->create_object<arrangement::MidiRegion> (
    *tempo_map, *obj_registry_, *file_audio_source_registry_, lane);

  // Add the region to the lane - this should trigger the signal
  lane->arrangement::ArrangerObjectOwner<arrangement::MidiRegion>::add_object (
    region_ref);

  // Wait for queued connections to process
  QCoreApplication::processEvents ();

  // Verify the signal was emitted
  EXPECT_GT (recacheSpy.count (), 0);
}

// Test signal connections when lanes are added/removed
TEST_F (TrackLaneListTest, SignalConnectionsOnLaneOperations)
{
  QSignalSpy recacheSpy (
    lane_list_.get (), &TrackLaneList::laneObjectsNeedRecache);

  // Add a lane
  auto * lane = lane_list_->addLane ();

  // Clear any signals from lane addition
  recacheSpy.clear ();

  // Create and add a MIDI region to the lane
  auto tempo_map =
    std::make_unique<dsp::TempoMap> (units::sample_rate (44100.0));
  auto region_ref = obj_registry_->create_object<arrangement::MidiRegion> (
    *tempo_map, *obj_registry_, *file_audio_source_registry_, lane);

  lane->arrangement::ArrangerObjectOwner<arrangement::MidiRegion>::add_object (
    region_ref);

  // Wait for queued connections to process
  QCoreApplication::processEvents ();

  // Verify signal was emitted
  EXPECT_GT (recacheSpy.count (), 0);

  // Remove the lane - this should disconnect signals
  recacheSpy.clear ();
  lane_list_->removeLane (0);

  // The signal connections should be disconnected, so no more signals
  // should be emitted even if we try to trigger them
  EXPECT_EQ (recacheSpy.count (), 0);
}

// Test ExpandableTickRange calculation in lane context
TEST_F (TrackLaneListTest, ExpandableTickRangeInLaneContext)
{
  // Add a lane
  auto * lane = lane_list_->addLane ();

  QSignalSpy recacheSpy (
    lane_list_.get (), &TrackLaneList::laneObjectsNeedRecache);

  // Create a MIDI region with specific position and bounds
  auto tempo_map =
    std::make_unique<dsp::TempoMap> (units::sample_rate (44100.0));
  auto region_ref = obj_registry_->create_object<arrangement::MidiRegion> (
    *tempo_map, *obj_registry_, *file_audio_source_registry_, lane);
  auto * region = region_ref.get_object_as<arrangement::MidiRegion> ();
  region->position ()->setTicks (100.0);
  region->bounds ()->setLengthTicks (50.0);

  // Add the region to the lane
  lane->arrangement::ArrangerObjectOwner<arrangement::MidiRegion>::add_object (
    region_ref);

  // Wait for queued connections to process
  QCoreApplication::processEvents ();

  // Verify the signal was emitted with correct range
  EXPECT_GT (recacheSpy.count (), 0);

  // Check that the signal contains the expected range
  if (recacheSpy.count () > 0)
    {
      auto firstSignal = recacheSpy.takeFirst ();
      auto range = firstSignal.at (0).value<utils::ExpandableTickRange> ();
      auto rangeOpt = range.range ();

      if (rangeOpt.has_value ())
        {
          EXPECT_EQ (rangeOpt->first, 100.0);
          EXPECT_EQ (rangeOpt->second, 150.0);
        }
    }
}

// Test signal connections and disconnections work correctly
TEST_F (TrackLaneListTest, SignalConnectionManagement)
{
  QSignalSpy recacheSpy (
    lane_list_.get (), &TrackLaneList::laneObjectsNeedRecache);

  // Add multiple lanes
  auto * lane1 = lane_list_->addLane ();
  auto * lane2 = lane_list_->addLane ();

  // Clear any initial signals
  recacheSpy.clear ();

  // Add objects to both lanes
  auto tempo_map =
    std::make_unique<dsp::TempoMap> (units::sample_rate (44100.0));

  // Add to first lane
  auto region1_ref = obj_registry_->create_object<arrangement::MidiRegion> (
    *tempo_map, *obj_registry_, *file_audio_source_registry_, lane1);
  lane1->arrangement::ArrangerObjectOwner<arrangement::MidiRegion>::add_object (
    region1_ref);

  // Add to second lane
  auto region2_ref = obj_registry_->create_object<arrangement::MidiRegion> (
    *tempo_map, *obj_registry_, *file_audio_source_registry_, lane2);
  lane2->arrangement::ArrangerObjectOwner<arrangement::MidiRegion>::add_object (
    region2_ref);

  // Wait for queued connections to process
  QCoreApplication::processEvents ();

  // Should have signals from both lanes
  EXPECT_GE (recacheSpy.count (), 2);

  // Remove one lane
  recacheSpy.clear ();
  lane_list_->removeLane (0);

  // Only the remaining lane should trigger signals
  auto region3_ref = obj_registry_->create_object<arrangement::MidiRegion> (
    *tempo_map, *obj_registry_, *file_audio_source_registry_,
    lane_list_->at (0));
  lane_list_->at (0)->arrangement::
    ArrangerObjectOwner<arrangement::MidiRegion>::add_object (region3_ref);

  QCoreApplication::processEvents ();

  // Should get signal from remaining lane only
  EXPECT_EQ (recacheSpy.count (), 1);
}

} // namespace zrythm::structure::tracks
