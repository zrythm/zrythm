// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <memory>

#include "dsp/tempo_map.h"
#include "dsp/tempo_map_qml_adapter.h"
#include "structure/arrangement/marker.h"

#include "helpers/mock_qobject.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

namespace zrythm::structure::arrangement
{
class MarkerTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    tempo_map = std::make_unique<dsp::TempoMap> (units::sample_rate (44100.0));
    tempo_map_wrapper = std::make_unique<dsp::TempoMapWrapper> (*tempo_map);
    parent = std::make_unique<MockQObject> ();
    marker = std::make_unique<Marker> (
      *tempo_map_wrapper, Marker::MarkerType::Custom, parent.get ());
  }

  std::unique_ptr<dsp::TempoMap>        tempo_map;
  std::unique_ptr<dsp::TempoMapWrapper> tempo_map_wrapper;
  std::unique_ptr<MockQObject>          parent;
  std::unique_ptr<Marker>               marker;
};

// Test initial state
TEST_F (MarkerTest, InitialState)
{
  EXPECT_EQ (marker->type (), ArrangerObject::Type::Marker);
  EXPECT_EQ (marker->position ()->ticks (), 0);
  EXPECT_EQ (marker->name ()->name (), "");
  EXPECT_EQ (marker->markerType (), Marker::MarkerType::Custom);
}

// Test marker types
TEST_F (MarkerTest, MarkerTypes)
{
  // Default is custom
  EXPECT_FALSE (marker->isStartMarker ());
  EXPECT_FALSE (marker->isEndMarker ());

  // Test start marker
  auto start_marker = std::make_unique<Marker> (
    *tempo_map_wrapper, Marker::MarkerType::Start, parent.get ());
  EXPECT_TRUE (start_marker->isStartMarker ());
  EXPECT_FALSE (start_marker->isEndMarker ());

  // Test end marker
  auto end_marker = std::make_unique<Marker> (
    *tempo_map_wrapper, Marker::MarkerType::End, parent.get ());
  EXPECT_FALSE (end_marker->isStartMarker ());
  EXPECT_TRUE (end_marker->isEndMarker ());
}

// Test serialization/deserialization
TEST_F (MarkerTest, Serialization)
{
  // Set initial state
  marker->position ()->setTicks (2000);
  marker->name ()->setName ("Chorus");

  // Serialize
  nlohmann::json j;
  to_json (j, *marker);

  // Create new marker
  auto new_marker = std::make_unique<Marker> (
    *tempo_map_wrapper, Marker::MarkerType::Custom, parent.get ());
  from_json (j, *new_marker);

  // Verify state
  EXPECT_EQ (new_marker->position ()->ticks (), 2000);
  EXPECT_EQ (new_marker->name ()->name (), "Chorus");
}

// Test copying
TEST_F (MarkerTest, Copying)
{
  // Set initial state
  marker->position ()->setTicks (3000);
  marker->name ()->setName ("Outro");

  // Create target
  auto target = std::make_unique<Marker> (
    *tempo_map_wrapper, Marker::MarkerType::Custom, parent.get ());

  // Copy
  init_from (*target, *marker, utils::ObjectCloneType::Snapshot);

  // Verify state
  EXPECT_EQ (target->position ()->ticks (), 3000);
  EXPECT_EQ (target->name ()->name (), "Outro");
}

// Test edge cases
TEST_F (MarkerTest, EdgeCases)
{
  // Long name
  marker->name ()->setName (
    "This is a very long marker name that should be supported");
  EXPECT_EQ (
    marker->name ()->name (),
    "This is a very long marker name that should be supported");

  // Special characters in name
  marker->name ()->setName ("Section 1: Intro (A-B)");
  EXPECT_EQ (marker->name ()->name (), "Section 1: Intro (A-B)");

  // Empty name
  marker->name ()->setName ("");
  EXPECT_EQ (marker->name ()->name (), "");

  // Negative position (no constraint, stored as-is)
  marker->position ()->setTicks (-1000.0);
  EXPECT_DOUBLE_EQ (marker->position ()->ticks (), -1000.0);
}

} // namespace zrythm::structure::arrangement
