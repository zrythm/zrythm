// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/tempo_map.h"
#include "structure/arrangement/arranger_object_all.h"

#include <gtest/gtest.h>

namespace zrythm::structure::arrangement
{
class ChordRegionTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    tempo_map = std::make_unique<dsp::TempoMap> (44100.0);
    region = std::make_unique<ChordRegion> (
      *tempo_map, registry, file_audio_source_registry, nullptr);

    // Set up region properties
    region->position ()->setTicks (100);
    region->bounds ()->length ()->setTicks (200);
  }

  auto create_chord_object (int chord_index)
  {
    // Create ChordObject using registry
    auto chord_ref =
      registry.create_object<ChordObject> (*tempo_map, region.get ());
    chord_ref.get_object_as<ChordObject> ()->setChordDescriptorIndex (
      chord_index);
    return chord_ref;
  }

  void
  add_chord_object (int chord_index, double position_ticks, double length_ticks)
  {
    auto chord_ref = create_chord_object (chord_index);
    chord_ref.get_object_as<ChordObject> ()->position ()->setTicks (
      position_ticks);
    region->add_object (chord_ref);
  }

  std::unique_ptr<dsp::TempoMap> tempo_map;
  ArrangerObjectRegistry         registry;
  dsp::FileAudioSourceRegistry   file_audio_source_registry;
  std::unique_ptr<ChordRegion>   region;
};

TEST_F (ChordRegionTest, InitialState)
{
  EXPECT_EQ (region->type (), ArrangerObject::Type::ChordRegion);
  EXPECT_EQ (region->position ()->ticks (), 100);
  EXPECT_NE (region->bounds (), nullptr);
  EXPECT_NE (region->loopRange (), nullptr);
  EXPECT_NE (region->name (), nullptr);
  EXPECT_NE (region->color (), nullptr);
  EXPECT_NE (region->mute (), nullptr);
  EXPECT_EQ (region->get_children_vector ().size (), 0);
}

TEST_F (ChordRegionTest, AddChordObject)
{
  // Add a chord object
  add_chord_object (3, 100, 50);

  // Verify object was added
  EXPECT_EQ (region->get_children_vector ().size (), 1);
  auto chord_obj = region->get_children_view ()[0];
  EXPECT_EQ (chord_obj->chordDescriptorIndex (), 3);
  EXPECT_EQ (chord_obj->position ()->ticks (), 100);
}

TEST_F (ChordRegionTest, RemoveChordObject)
{
  // Add a chord object
  auto chord_ref = create_chord_object (2);
  region->add_object (chord_ref);

  // Remove object
  auto removed_ref = region->remove_object (chord_ref.id ());

  // Verify object was removed
  EXPECT_EQ (region->get_children_vector ().size (), 0);
  EXPECT_EQ (removed_ref.id (), chord_ref.id ());
}

TEST_F (ChordRegionTest, ChordIndexChange)
{
  // Add a chord object
  add_chord_object (1, 100, 50);

  // Change chord index
  region->get_children_view ()[0]->setChordDescriptorIndex (5);

  // Verify change
  EXPECT_EQ (region->get_children_view ()[0]->chordDescriptorIndex (), 5);
}

TEST_F (ChordRegionTest, MuteFunctionality)
{
  // Add a chord object
  add_chord_object (4, 100, 50);

  // Mute the chord
  region->get_children_view ()[0]->mute ()->setMuted (true);

  // Verify mute
  EXPECT_TRUE (region->get_children_view ()[0]->mute ()->muted ());
}

TEST_F (ChordRegionTest, Serialization)
{
  // Add a chord object
  add_chord_object (2, 150, 75);
  const auto chord_id = region->get_children_view ()[0]->get_uuid ();

  // Serialize
  nlohmann::json j;
  to_json (j, *region);

  // Create new region
  auto new_region = std::make_unique<ChordRegion> (
    *tempo_map, registry, file_audio_source_registry, nullptr);
  from_json (j, *new_region);

  // Verify deserialization
  EXPECT_EQ (new_region->get_children_vector ().size (), 1);
  auto chord_obj = new_region->get_children_view ()[0];
  EXPECT_EQ (chord_obj->chordDescriptorIndex (), 2);
  EXPECT_EQ (chord_obj->position ()->ticks (), 150);
  EXPECT_EQ (chord_obj->get_uuid (), chord_id);
}

TEST_F (ChordRegionTest, ObjectCloning)
{
  // Add a chord object
  add_chord_object (3, 200, 100);

  // Clone region with new identities
  auto cloned_region = std::make_unique<ChordRegion> (
    *tempo_map, registry, file_audio_source_registry, nullptr);
  init_from (*cloned_region, *region, utils::ObjectCloneType::NewIdentity);

  // Verify cloning
  EXPECT_EQ (cloned_region->get_children_vector ().size (), 1);
  auto cloned_chord = cloned_region->get_children_view ()[0];
  EXPECT_NE (
    cloned_chord->get_uuid (), region->get_children_view ()[0]->get_uuid ());
  EXPECT_EQ (cloned_chord->chordDescriptorIndex (), 3);
  EXPECT_EQ (cloned_chord->position ()->ticks (), 200);
}

} // namespace zrythm::structure::arrangement
