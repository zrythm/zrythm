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

  // Add object to owner (use MidiNote base class explicitly)
  owner->ArrangerObjectOwner<MidiNote>::add_object (obj_ref);

  // Verify object was added
  EXPECT_EQ (
    owner->ArrangerObjectOwner<MidiNote>::get_children_vector ().size (), 1);
  EXPECT_EQ (
    owner->ArrangerObjectOwner<MidiNote>::get_children_vector ()[0].id (),
    obj_ref.id ());
}

TEST_F (ArrangerObjectOwnerTest, RemoveObject)
{
  auto obj_ref = create_midi_note ();
  owner->ArrangerObjectOwner<MidiNote>::add_object (obj_ref);

  // Remove object (use MidiNote base class explicitly)
  auto removed_ref =
    owner->ArrangerObjectOwner<MidiNote>::remove_object (obj_ref.id ());

  // Verify object was removed
  EXPECT_EQ (
    owner->ArrangerObjectOwner<MidiNote>::get_children_vector ().size (), 0);
  EXPECT_EQ (removed_ref.id (), obj_ref.id ());
}

TEST_F (ArrangerObjectOwnerTest, InsertObjectAtIndex)
{
  // Create two objects
  auto obj_ref1 = create_midi_note ();
  auto obj_ref2 = create_midi_note ();

  // Add first object
  owner->ArrangerObjectOwner<MidiNote>::add_object (obj_ref1);

  // Insert second object at position 0 (use MidiNote base class explicitly)
  owner->ArrangerObjectOwner<MidiNote>::insert_object (obj_ref2, 0);

  // Verify order
  EXPECT_EQ (
    owner->ArrangerObjectOwner<MidiNote>::get_children_vector ().size (), 2);
  EXPECT_EQ (
    owner->ArrangerObjectOwner<MidiNote>::get_children_vector ()[0].id (),
    obj_ref2.id ());
  EXPECT_EQ (
    owner->ArrangerObjectOwner<MidiNote>::get_children_vector ()[1].id (),
    obj_ref1.id ());
}

TEST_F (ArrangerObjectOwnerTest, ClearObjects)
{
  // Add multiple objects
  auto obj_ref1 = create_midi_note ();
  auto obj_ref2 = create_midi_note ();
  owner->ArrangerObjectOwner<MidiNote>::add_object (obj_ref1);
  owner->ArrangerObjectOwner<MidiNote>::add_object (obj_ref2);

  // Clear all objects (use MidiNote base class explicitly)
  owner->ArrangerObjectOwner<MidiNote>::clear_objects ();

  // Verify objects were cleared
  EXPECT_EQ (
    owner->ArrangerObjectOwner<MidiNote>::get_children_vector ().size (), 0);
}

TEST_F (ArrangerObjectOwnerTest, GetChildrenView)
{
  // Add multiple objects
  auto obj_ref1 = create_midi_note ();
  auto obj_ref2 = create_midi_note ();
  owner->ArrangerObjectOwner<MidiNote>::add_object (obj_ref1);
  owner->ArrangerObjectOwner<MidiNote>::add_object (obj_ref2);

  // Get children view (use MidiNote base class explicitly)
  auto view = owner->ArrangerObjectOwner<MidiNote>::get_children_view ();

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
  owner->ArrangerObjectOwner<MidiNote>::add_object (obj_ref);

  // Get initial position
  auto initial_ticks = obj_ref.get_object_as<MidiNote> ()->position ()->ticks ();

  // Add ticks (use MidiNote base class explicitly)
  double ticks_to_add = 10.0;
  owner->ArrangerObjectOwner<MidiNote>::add_ticks_to_children (ticks_to_add);

  // Verify position changed
  EXPECT_DOUBLE_EQ (
    obj_ref.get_object_as<MidiNote> ()->position ()->ticks (),
    initial_ticks + ticks_to_add);
}

TEST_F (ArrangerObjectOwnerTest, ContainsObject)
{
  // Create two objects
  auto obj_ref1 = create_midi_note ();
  auto obj_ref2 = create_midi_note ();

  // Initially, owner should not contain any objects (use MidiNote base class
  // explicitly)
  EXPECT_FALSE (
    owner->ArrangerObjectOwner<MidiNote>::contains_object (obj_ref1.id ()));
  EXPECT_FALSE (
    owner->ArrangerObjectOwner<MidiNote>::contains_object (obj_ref2.id ()));

  // Add first object
  owner->ArrangerObjectOwner<MidiNote>::add_object (obj_ref1);

  // Now owner should contain the first object but not the second
  EXPECT_TRUE (
    owner->ArrangerObjectOwner<MidiNote>::contains_object (obj_ref1.id ()));
  EXPECT_FALSE (
    owner->ArrangerObjectOwner<MidiNote>::contains_object (obj_ref2.id ()));

  // Add second object
  owner->ArrangerObjectOwner<MidiNote>::add_object (obj_ref2);

  // Now owner should contain both objects
  EXPECT_TRUE (
    owner->ArrangerObjectOwner<MidiNote>::contains_object (obj_ref1.id ()));
  EXPECT_TRUE (
    owner->ArrangerObjectOwner<MidiNote>::contains_object (obj_ref2.id ()));

  // Remove first object
  owner->ArrangerObjectOwner<MidiNote>::remove_object (obj_ref1.id ());

  // Now owner should only contain the second object
  EXPECT_FALSE (
    owner->ArrangerObjectOwner<MidiNote>::contains_object (obj_ref1.id ()));
  EXPECT_TRUE (
    owner->ArrangerObjectOwner<MidiNote>::contains_object (obj_ref2.id ()));

  // Clear all objects
  owner->ArrangerObjectOwner<MidiNote>::clear_objects ();

  // Now owner should not contain any objects
  EXPECT_FALSE (
    owner->ArrangerObjectOwner<MidiNote>::contains_object (obj_ref1.id ()));
  EXPECT_FALSE (
    owner->ArrangerObjectOwner<MidiNote>::contains_object (obj_ref2.id ()));
}

TEST_F (ArrangerObjectOwnerTest, Serialization)
{
  auto obj_ref = create_midi_note ();
  owner->ArrangerObjectOwner<MidiNote>::add_object (obj_ref);

  // Serialize (cast to MidiNote base to avoid ambiguity)
  nlohmann::json j;
  to_json (j, static_cast<const ArrangerObjectOwner<MidiNote> &> (*owner));

  // Create new owner
  auto new_owner = std::make_unique<MockArrangerObjectOwner> (
    registry, file_audio_source_registry);
  from_json (j, static_cast<ArrangerObjectOwner<MidiNote> &> (*new_owner));

  // Verify deserialization (use MidiNote base class explicitly)
  EXPECT_EQ (
    new_owner->ArrangerObjectOwner<MidiNote>::get_children_vector ().size (), 1);
  EXPECT_EQ (
    new_owner->ArrangerObjectOwner<MidiNote>::get_children_vector ()[0].id (),
    obj_ref.id ());
}

TEST_F (ArrangerObjectOwnerTest, ObjectCloning)
{
  auto obj_ref = create_midi_note ();
  owner->ArrangerObjectOwner<MidiNote>::add_object (obj_ref);

  // Clone owner with new identities (cast to MidiNote base to avoid ambiguity)
  auto cloned_owner = std::make_unique<MockArrangerObjectOwner> (
    registry, file_audio_source_registry);
  init_from (
    static_cast<ArrangerObjectOwner<MidiNote> &> (*cloned_owner),
    static_cast<const ArrangerObjectOwner<MidiNote> &> (*owner),
    utils::ObjectCloneType::NewIdentity);

  // Verify cloning (use MidiNote base class explicitly)
  EXPECT_EQ (
    cloned_owner->ArrangerObjectOwner<MidiNote>::get_children_vector ().size (),
    1);
  EXPECT_NE (
    cloned_owner->ArrangerObjectOwner<MidiNote>::get_children_vector ()[0].id (),
    obj_ref.id ());
}

} // namespace zrythm::structure::arrangement
