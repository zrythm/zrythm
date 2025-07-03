// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <memory>

#include "dsp/atomic_position.h"
#include "dsp/atomic_position_qml_adapter.h"
#include "dsp/tempo_map.h"
#include "structure/arrangement/region.h"

#include "helpers/mock_qobject.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace zrythm::structure::arrangement
{
class RegionMixinTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    tempo_map = std::make_unique<dsp::TempoMap> (44100.0);
    start_position = std::make_unique<dsp::AtomicPosition> (*tempo_map);
    parent = std::make_unique<MockQObject> ();
    start_position_adapter = std::make_unique<dsp::AtomicPositionQmlAdapter> (
      *start_position, parent.get ());

    // Create region mixin
    region = std::make_unique<RegionMixin> (*start_position_adapter);
  }

  std::unique_ptr<dsp::TempoMap>                 tempo_map;
  std::unique_ptr<dsp::AtomicPosition>           start_position;
  std::unique_ptr<MockQObject>                   parent;
  std::unique_ptr<dsp::AtomicPositionQmlAdapter> start_position_adapter;
  std::unique_ptr<RegionMixin>                   region;
};

// Test initial state
TEST_F (RegionMixinTest, InitialState)
{
  // Verify all components are created
  EXPECT_NE (region->bounds (), nullptr);
  EXPECT_NE (region->loopRange (), nullptr);
  EXPECT_NE (region->name (), nullptr);
  EXPECT_NE (region->color (), nullptr);
  EXPECT_NE (region->mute (), nullptr);

  // Verify initial property values
  EXPECT_EQ (start_position_adapter->samples (), 0);
  EXPECT_EQ (region->loopRange ()->clipStartPosition ()->samples (), 0);
  EXPECT_TRUE (region->name ()->name ().isEmpty ());
  EXPECT_FALSE (region->color ()->useColor ());
  EXPECT_FALSE (region->mute ()->muted ());
}

// Test property access
TEST_F (RegionMixinTest, PropertyAccess)
{
  // Set values through the mixin
  region->bounds ()->length ()->setSamples (5000);
  region->loopRange ()->loopStartPosition ()->setSamples (1000);
  region->name ()->setName ("Test Region");
  region->color ()->setColor (QColor (255, 0, 0));
  region->mute ()->setMuted (true);

  // Verify values
  EXPECT_EQ (region->bounds ()->length ()->samples (), 5000);
  EXPECT_EQ (region->loopRange ()->loopStartPosition ()->samples (), 1000);
  EXPECT_EQ (region->name ()->name (), "Test Region");
  EXPECT_EQ (region->color ()->color (), QColor (255, 0, 0));
  EXPECT_TRUE (region->mute ()->muted ());
}

// Test serialization/deserialization
TEST_F (RegionMixinTest, Serialization)
{
  // Set values
  region->bounds ()->length ()->setSamples (3000);
  region->loopRange ()->loopEndPosition ()->setSamples (2500);
  region->name ()->setName ("Serialized Region");
  region->color ()->setColor (QColor (0, 255, 0));
  region->mute ()->setMuted (true);

  // Serialize
  nlohmann::json j;
  to_json (j, *region);

  // Create new region
  auto new_region = std::make_unique<RegionMixin> (*start_position_adapter);
  from_json (j, *new_region);

  // Verify
  EXPECT_EQ (new_region->bounds ()->length ()->samples (), 3000);
  EXPECT_EQ (new_region->loopRange ()->loopEndPosition ()->samples (), 2500);
  EXPECT_EQ (new_region->name ()->name (), "Serialized Region");
  EXPECT_EQ (new_region->color ()->color (), QColor (0, 255, 0));
  EXPECT_TRUE (new_region->mute ()->muted ());
}

// Test copying with init_from
TEST_F (RegionMixinTest, Copying)
{
  // Set values
  region->bounds ()->length ()->setSamples (4000);
  region->loopRange ()->clipStartPosition ()->setSamples (500);
  region->name ()->setName ("Copied Region");
  region->color ()->setColor (QColor (0, 0, 255));
  region->mute ()->setMuted (false);

  // Create target
  auto target = std::make_unique<RegionMixin> (*start_position_adapter);

  // Copy
  init_from (*target, *region, utils::ObjectCloneType::Snapshot);

  // Verify
  EXPECT_EQ (target->bounds ()->length ()->samples (), 4000);
  EXPECT_EQ (target->loopRange ()->clipStartPosition ()->samples (), 500);
  EXPECT_EQ (target->name ()->name (), "Copied Region");
  EXPECT_EQ (target->color ()->color (), QColor (0, 0, 255));
  EXPECT_FALSE (target->mute ()->muted ());
}

// Test component relationships
TEST_F (RegionMixinTest, ComponentRelationships)
{
  // Verify that loop range tracks bounds length by default
  region->bounds ()->length ()->setSamples (5000);
  EXPECT_EQ (region->loopRange ()->loopEndPosition ()->samples (), 5000);

  // Disable tracking and verify independence
  region->loopRange ()->setTrackLength (false);
  region->bounds ()->length ()->setSamples (6000);
  EXPECT_EQ (region->loopRange ()->loopEndPosition ()->samples (), 5000);
}

} // namespace zrythm::structure::arrangement
