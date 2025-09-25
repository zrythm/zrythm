// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/tempo_map.h"

#include "./arranger_object_owner_test.h"

namespace zrythm::structure::arrangement
{

class ArrangerObjectOwnerTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    tempo_map = std::make_unique<dsp::TempoMap> (units::sample_rate (44100.0));
    owner = std::make_unique<MockArrangerObjectOwner> (
      registry, file_audio_source_registry);
  }

  ArrangerObjectUuidReference create_midi_note ()
  {
    // Create MidiNote using registry
    return registry.create_object<MidiNote> (*tempo_map, owner.get ());
  }

  std::unique_ptr<dsp::TempoMap>           tempo_map;
  ArrangerObjectRegistry                   registry;
  dsp::FileAudioSourceRegistry             file_audio_source_registry;
  std::unique_ptr<MockArrangerObjectOwner> owner;
};

TEST_F (ArrangerObjectOwnerTest, AddObject)
{
  auto obj_ref = create_midi_note ();

  // Add object to owner
  owner->add_object (obj_ref);

  // Verify object was added
  EXPECT_EQ (owner->get_children_vector ().size (), 1);
  EXPECT_EQ (owner->get_children_vector ()[0].id (), obj_ref.id ());
}

TEST_F (ArrangerObjectOwnerTest, RemoveObject)
{
  auto obj_ref = create_midi_note ();
  owner->add_object (obj_ref);

  // Remove object
  auto removed_ref = owner->remove_object (obj_ref.id ());

  // Verify object was removed
  EXPECT_EQ (owner->get_children_vector ().size (), 0);
  EXPECT_EQ (removed_ref.id (), obj_ref.id ());
}

TEST_F (ArrangerObjectOwnerTest, InsertObjectAtIndex)
{
  // Create two objects
  auto obj_ref1 = create_midi_note ();
  auto obj_ref2 = create_midi_note ();

  // Add first object
  owner->add_object (obj_ref1);

  // Insert second object at position 0
  owner->insert_object (obj_ref2, 0);

  // Verify order
  EXPECT_EQ (owner->get_children_vector ().size (), 2);
  EXPECT_EQ (owner->get_children_vector ()[0].id (), obj_ref2.id ());
  EXPECT_EQ (owner->get_children_vector ()[1].id (), obj_ref1.id ());
}

TEST_F (ArrangerObjectOwnerTest, ClearObjects)
{
  // Add multiple objects
  auto obj_ref1 = create_midi_note ();
  auto obj_ref2 = create_midi_note ();
  owner->add_object (obj_ref1);
  owner->add_object (obj_ref2);

  // Clear all objects
  owner->clear_objects ();

  // Verify objects were cleared
  EXPECT_EQ (owner->get_children_vector ().size (), 0);
}

TEST_F (ArrangerObjectOwnerTest, GetChildrenView)
{
  // Add multiple objects
  auto obj_ref1 = create_midi_note ();
  auto obj_ref2 = create_midi_note ();
  owner->add_object (obj_ref1);
  owner->add_object (obj_ref2);

  // Get children view
  auto view = owner->get_children_view ();

  // Verify objects in view
  EXPECT_EQ (std::ranges::distance (view), 2);
  EXPECT_TRUE (std::ranges::any_of (view, [&] (auto * obj) {
    return obj->get_uuid () == obj_ref1.id ();
  }));
  EXPECT_TRUE (std::ranges::any_of (view, [&] (auto * obj) {
    return obj->get_uuid () == obj_ref2.id ();
  }));
}

TEST_F (ArrangerObjectOwnerTest, AddTicksToChildren)
{
  auto obj_ref = create_midi_note ();
  owner->add_object (obj_ref);

  // Get initial position
  auto initial_ticks = obj_ref.get_object_as<MidiNote> ()->position ()->ticks ();

  // Add ticks
  double ticks_to_add = 10.0;
  owner->add_ticks_to_children (ticks_to_add);

  // Verify position changed
  EXPECT_DOUBLE_EQ (
    obj_ref.get_object_as<MidiNote> ()->position ()->ticks (),
    initial_ticks + ticks_to_add);
}

TEST_F (ArrangerObjectOwnerTest, Serialization)
{
  auto obj_ref = create_midi_note ();
  owner->add_object (obj_ref);

  // Serialize
  nlohmann::json j;
  to_json (j, *owner);

  // Create new owner
  auto new_owner = std::make_unique<MockArrangerObjectOwner> (
    registry, file_audio_source_registry);
  from_json (j, *new_owner);

  // Verify deserialization
  EXPECT_EQ (new_owner->get_children_vector ().size (), 1);
  EXPECT_EQ (new_owner->get_children_vector ()[0].id (), obj_ref.id ());
}

TEST_F (ArrangerObjectOwnerTest, ObjectCloning)
{
  auto obj_ref = create_midi_note ();
  owner->add_object (obj_ref);

  // Clone owner with new identities
  auto cloned_owner = std::make_unique<MockArrangerObjectOwner> (
    registry, file_audio_source_registry);
  init_from (*cloned_owner, *owner, utils::ObjectCloneType::NewIdentity);

  // Verify cloning
  EXPECT_EQ (cloned_owner->get_children_vector ().size (), 1);
  EXPECT_NE (cloned_owner->get_children_vector ()[0].id (), obj_ref.id ());
}

} // namespace zrythm::structure::arrangement
