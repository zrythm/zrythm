// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/tempo_map.h"
#include "structure/arrangement/arranger_object_factory.h"

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

// ============================================================================
// ParentObject Tests - Real Region Types
// ============================================================================
//
// These tests verify that parentObject is correctly set when:
// 1. Children are cloned (copy-move scenario)
// 2. Children are deserialized (project loading scenario)
//
// The mock-based tests above cannot test this because MockArrangerObjectOwner
// doesn't inherit from ArrangerObject.

namespace
{

// Helper trait to get child type for a region type
template <typename RegionT> struct ChildTypeForRegion;

template <> struct ChildTypeForRegion<MidiRegion>
{
  using Type = MidiNote;
};
template <> struct ChildTypeForRegion<AutomationRegion>
{
  using Type = AutomationPoint;
};
template <> struct ChildTypeForRegion<ChordRegion>
{
  using Type = ChordObject;
};

} // namespace

template <typename RegionT>
class ArrangerObjectOwnerParentTest : public ::testing::Test
{
protected:
  using ChildT = typename ChildTypeForRegion<RegionT>::Type;

  void SetUp () override
  {
    tempo_map = std::make_unique<dsp::TempoMap> (units::sample_rate (44100.0));

    factory = std::make_unique<ArrangerObjectFactory> (
      ArrangerObjectFactory::Dependencies{
        .tempo_map_ = *tempo_map,
        .object_registry_ = registry,
        .file_audio_source_registry_ = file_audio_source_registry,
        .musical_mode_getter_ = [] () { return true; },
        .last_timeline_obj_len_provider_ = [] () { return 100.0; },
        .last_editor_obj_len_provider_ = [] () { return 50.0; },
        .automation_curve_algorithm_provider_ =
          [] () { return dsp::CurveOptions::Algorithm::Exponent; } },
      [] () { return units::sample_rate (44100); }, [] () { return 120.0; });

    // Create region
    region_ref = std::make_unique<ArrangerObjectUuidReference> (
      factory->template get_builder<RegionT> ()
        .with_start_ticks (0.0)
        .with_end_ticks (1920.0)
        .build_in_registry ());
  }

  // Helper to create a child object and add it to the region
  ArrangerObjectUuidReference create_and_add_child (double start_ticks = 100.0)
  {
    auto child_ref =
      factory->template get_builder<ChildT> ()
        .with_start_ticks (start_ticks)
        .build_in_registry ();
    region_ref->get_object_as<RegionT> ()->add_object (child_ref);
    return child_ref;
  }

  std::unique_ptr<dsp::TempoMap>               tempo_map;
  ArrangerObjectRegistry                       registry;
  dsp::FileAudioSourceRegistry                 file_audio_source_registry;
  std::unique_ptr<ArrangerObjectFactory>       factory;
  std::unique_ptr<ArrangerObjectUuidReference> region_ref;
};

// Test only the region types that inherit from ArrangerObjectOwner and
// ArrangerObject (AudioRegion excluded - its child AudioSourceObject has
// different construction requirements)
using RegionTypes = ::testing::Types<MidiRegion, AutomationRegion, ChordRegion>;
TYPED_TEST_SUITE (ArrangerObjectOwnerParentTest, RegionTypes);

TYPED_TEST (ArrangerObjectOwnerParentTest, AddObjectSetsParentObject)
{
  // Create and add a child via add_object() - this should set parentObject
  auto child_ref = this->create_and_add_child ();

  auto * region = this->region_ref->template get_object_as<TypeParam> ();
  auto * child =
    child_ref.template get_object_as<typename TestFixture::ChildT> ();

  // Verify parentObject is set when added via add_object()
  EXPECT_EQ (child->parentObject (), region)
    << "Child added via add_object() should have parentObject set to region";
}

TYPED_TEST (ArrangerObjectOwnerParentTest, CloneSetsParentObjectOnChildren)
{
  // Create and add a child via add_object() - this correctly sets parentObject
  auto original_child_ref = this->create_and_add_child ();

  auto * original_region =
    this->region_ref->template get_object_as<TypeParam> ();
  auto * original_child =
    original_child_ref.template get_object_as<typename TestFixture::ChildT> ();

  // Verify original child has parentObject set
  ASSERT_EQ (original_child->parentObject (), original_region)
    << "Original child should have parentObject set";

  // Clone the region using the factory (this is how copy-move works)
  auto cloned_region_ref =
    this->factory->clone_new_object_identity (*original_region);
  auto * cloned_region = cloned_region_ref.template get_object_as<TypeParam> ();

  // Verify the cloned region has children
  ASSERT_EQ (cloned_region->get_children_vector ().size (), 1)
    << "Cloned region should have the same number of children as original";

  // Get the cloned child
  auto * cloned_child = cloned_region->get_children_view ()[0];

  // Verify parentObject is set on cloned children
  EXPECT_EQ (cloned_child->parentObject (), cloned_region)
    << "Cloned child should have parentObject pointing to cloned region";
}

TYPED_TEST (
  ArrangerObjectOwnerParentTest,
  DeserializationSetsParentObjectOnChildren)
{
  // Create and add a child via add_object()
  auto original_child_ref = this->create_and_add_child ();

  auto * original_region =
    this->region_ref->template get_object_as<TypeParam> ();
  auto * original_child =
    original_child_ref.template get_object_as<typename TestFixture::ChildT> ();

  // Verify original child has parentObject set
  ASSERT_EQ (original_child->parentObject (), original_region)
    << "Original child should have parentObject set";

  // Serialize the region
  nlohmann::json j;
  to_json (j, *original_region);

  // Create a new region and deserialize into it
  auto new_region_ref =
    this->factory->template get_builder<TypeParam> ().build_in_registry ();
  auto * new_region = new_region_ref.template get_object_as<TypeParam> ();
  from_json (j, *new_region);

  // Verify the deserialized region has children
  ASSERT_EQ (new_region->get_children_vector ().size (), 1)
    << "Deserialized region should have the same number of children";

  // Get the deserialized child
  auto * deserialized_child = new_region->get_children_view ()[0];

  // Verify parentObject is set on deserialized children
  EXPECT_EQ (deserialized_child->parentObject (), new_region)
    << "Deserialized child should have parentObject pointing to its region";
}

} // namespace zrythm::structure::arrangement
