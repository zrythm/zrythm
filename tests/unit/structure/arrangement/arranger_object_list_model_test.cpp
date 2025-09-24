// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <memory>

#include "dsp/tempo_map.h"
#include "structure/arrangement/arranger_object_all.h"
#include "structure/arrangement/arranger_object_list_model.h"
#include "structure/arrangement/midi_region.h"

#include <QSignalSpy>

#include "helpers/mock_qobject.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace zrythm::structure::arrangement
{
class ArrangerObjectListModelTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    tempo_map = std::make_unique<dsp::TempoMap> (44100.0 * mp_units::si::hertz);
    parent = std::make_unique<MockQObject> ();

    // Create test MIDI notes
    for (int i = 0; i < 5; i++)
      {
        auto note_ref =
          registry_.create_object<MidiNote> (*tempo_map, parent.get ());
        auto * note = note_ref.get_object_as<MidiNote> ();
        note->setPitch (60 + i);
        note->setVelocity (90 + i);
        objects_.get<random_access_index> ().emplace_back (std::move (note_ref));
      }

    // Create model
    model_ = std::make_unique<ArrangerObjectListModel> (objects_, parent.get ());
  }

  std::unique_ptr<dsp::TempoMap>           tempo_map;
  std::unique_ptr<MockQObject>             parent;
  ArrangerObjectRegistry                   registry_;
  dsp::FileAudioSourceRegistry             file_audio_source_registry_;
  ArrangerObjectRefMultiIndexContainer     objects_;
  std::unique_ptr<ArrangerObjectListModel> model_;
};

// Test initial state
TEST_F (ArrangerObjectListModelTest, InitialState)
{
  EXPECT_EQ (model_->rowCount (), 5);
  EXPECT_EQ (model_->roleNames ().size (), 2);
  EXPECT_TRUE (model_->roleNames ().contains (
    ArrangerObjectListModel::ArrangerObjectPtrRole));
  EXPECT_TRUE (model_->roleNames ().contains (
    ArrangerObjectListModel::ArrangerObjectUuidReferenceRole));
}

// Test data access
TEST_F (ArrangerObjectListModelTest, DataAccess)
{
  // Valid index
  QModelIndex idx = model_->index (0, 0);
  QVariant    data =
    model_->data (idx, ArrangerObjectListModel::ArrangerObjectPtrRole);
  ASSERT_TRUE (data.isValid ());

  auto obj = data.value<MidiNote *> ();
  ASSERT_NE (obj, nullptr);
  EXPECT_EQ (obj->pitch (), 60);

  // Test UUID reference role
  QVariant uuid_data = model_->data (
    idx, ArrangerObjectListModel::ArrangerObjectUuidReferenceRole);
  ASSERT_TRUE (uuid_data.isValid ());

  auto uuid_ref = uuid_data.value<ArrangerObjectUuidReference *> ();
  ASSERT_NE (uuid_ref, nullptr);
  EXPECT_EQ (uuid_ref->get_object_as<MidiNote> ()->pitch (), 60);

  // Invalid index
  QModelIndex invalid_idx = model_->index (10, 0);
  EXPECT_FALSE (model_->data (invalid_idx).isValid ());

  // Invalid role
  EXPECT_FALSE (model_->data (idx, Qt::DisplayRole).isValid ());
}

// Test row insertion
TEST_F (ArrangerObjectListModelTest, InsertRows)
{
  // Create new note
  auto new_note_ref =
    registry_.create_object<MidiNote> (*tempo_map, parent.get ());
  auto * note_ptr = new_note_ref.get_object_as<MidiNote> ();
  note_ptr->setPitch (72);

  // Insert at position 2
  model_->insertObject (
    ArrangerObjectUuidReference (note_ptr->get_uuid (), registry_), 2);

  EXPECT_EQ (model_->rowCount (), 6);

  // Verify new position
  QModelIndex idx = model_->index (2, 0);
  QVariant    data =
    model_->data (idx, ArrangerObjectListModel::ArrangerObjectPtrRole);
  auto obj = data.value<MidiNote *> ();
  EXPECT_EQ (obj->pitch (), 72);
}

// Test row removal
TEST_F (ArrangerObjectListModelTest, RemoveRows)
{
  // Remove position 1
  model_->removeRows (1, 1);

  EXPECT_EQ (model_->rowCount (), 4);

  // Verify next item moved up
  QModelIndex idx = model_->index (1, 0);
  QVariant    data =
    model_->data (idx, ArrangerObjectListModel::ArrangerObjectPtrRole);
  auto obj = data.value<MidiNote *> ();
  EXPECT_EQ (obj->pitch (), 62); // Was originally at position 2
}

// Test model reset
TEST_F (ArrangerObjectListModelTest, ResetModel)
{
  model_->clear ();

  EXPECT_EQ (model_->rowCount (), 0);
}

// Test role names
TEST_F (ArrangerObjectListModelTest, RoleNames)
{
  auto roles = model_->roleNames ();
  EXPECT_EQ (roles.size (), 2);
  EXPECT_EQ (
    roles[ArrangerObjectListModel::ArrangerObjectPtrRole],
    QByteArray ("arrangerObject"));
  EXPECT_EQ (
    roles[ArrangerObjectListModel::ArrangerObjectUuidReferenceRole],
    QByteArray ("arrangerObjectReference"));
}

// Test empty model
TEST_F (ArrangerObjectListModelTest, EmptyModel)
{
  ArrangerObjectRefMultiIndexContainer empty_objects;
  ArrangerObjectListModel empty_model (empty_objects, parent.get ());

  EXPECT_EQ (empty_model.rowCount (), 0);
  EXPECT_FALSE (empty_model.data (empty_model.index (0, 0)).isValid ());
}

// Test contentChanged signal on object insertion
TEST_F (ArrangerObjectListModelTest, ContentChangedOnInsert)
{
  QSignalSpy contentChangedSpy (
    model_.get (), &ArrangerObjectListModel::contentChanged);
  QSignalSpy contentChangedForObjectSpy (
    model_.get (), &ArrangerObjectListModel::contentChangedForObject);

  // Create new note
  auto new_note_ref =
    registry_.create_object<MidiNote> (*tempo_map, parent.get ());
  auto * note_ptr = new_note_ref.get_object_as<MidiNote> ();
  note_ptr->setPitch (72);
  note_ptr->position ()->setTicks (100.0);
  note_ptr->bounds ()->length ()->setTicks (50.0);

  // Insert the object
  model_->insertObject (
    ArrangerObjectUuidReference (note_ptr->get_uuid (), registry_), 2);

  // Check that signals were emitted
  EXPECT_EQ (contentChangedSpy.count (), 1);
  EXPECT_EQ (contentChangedForObjectSpy.count (), 1);

  // Verify contentChanged signal contains correct range
  auto rangeArg =
    contentChangedSpy.takeFirst ().at (0).value<utils::ExpandableTickRange> ();
  auto range = rangeArg.range ();
  ASSERT_TRUE (range.has_value ());
  EXPECT_EQ (range->first, 100.0);
  EXPECT_EQ (range->second, 150.0);

  // Verify contentChangedForObject signal contains correct object
  auto objectArg =
    contentChangedForObjectSpy.takeFirst ()
      .at (0)
      .value<const ArrangerObject *> ();
  EXPECT_EQ (objectArg, note_ptr);
}

// Test contentChanged signal on object removal
TEST_F (ArrangerObjectListModelTest, ContentChangedOnRemove)
{
  QSignalSpy contentChangedForObjectSpy (
    model_.get (), &ArrangerObjectListModel::contentChangedForObject);

  // Get the object to be removed
  auto * obj_to_remove =
    objects_.get<random_access_index> ()[1].get_object_as<MidiNote> ();

  // Remove the object
  model_->removeRows (1, 1);

  // Check that contentChangedForObject signal was emitted
  EXPECT_EQ (contentChangedForObjectSpy.count (), 1);

  // Verify signal contains correct object
  auto objectArg =
    contentChangedForObjectSpy.takeFirst ()
      .at (0)
      .value<const ArrangerObject *> ();
  EXPECT_EQ (objectArg, obj_to_remove);
}

// Test contentChangedForObject signal on property changes
TEST_F (ArrangerObjectListModelTest, ContentChangedForObjectOnPropertyChange)
{
  QSignalSpy contentChangedForObjectSpy (
    model_.get (), &ArrangerObjectListModel::contentChangedForObject);

  // Get an existing object
  auto * obj =
    objects_.get<random_access_index> ()[0].get_object_as<MidiNote> ();

  // Change a property - this should trigger the signal
  obj->setPitch (65);

  // Check that contentChangedForObject signal was emitted
  EXPECT_EQ (contentChangedForObjectSpy.count (), 1);

  // Verify signal contains correct object
  auto objectArg =
    contentChangedForObjectSpy.takeFirst ()
      .at (0)
      .value<const ArrangerObject *> ();
  EXPECT_EQ (objectArg, obj);
}

// Test ExpandableTickRange calculation
TEST_F (ArrangerObjectListModelTest, ExpandableTickRangeCalculation)
{
  // Create a test object with specific position and bounds
  auto test_note_ref =
    registry_.create_object<MidiNote> (*tempo_map, parent.get ());
  auto * test_note = test_note_ref.get_object_as<MidiNote> ();
  test_note->position ()->setTicks (200.0);
  test_note->bounds ()->length ()->setTicks (100.0);

  // Test get_object_tick_range function
  auto range = get_object_tick_range (test_note);
  EXPECT_EQ (range.first, 200.0);
  EXPECT_EQ (range.second, 300.0);
}

// Test nested object signal propagation (for ArrangerObjectOwner types)
TEST_F (ArrangerObjectListModelTest, NestedObjectSignalPropagation)
{
  // Create a MIDI region (which is an ArrangerObjectOwner)
  auto region_ref = registry_.create_object<MidiRegion> (
    *tempo_map, registry_, file_audio_source_registry_, parent.get ());
  auto * region = region_ref.get_object_as<MidiRegion> ();

  // Create a vector with the region
  ArrangerObjectRefMultiIndexContainer region_objects;
  region_objects.get<sequenced_index> ().emplace_back (std::move (region_ref));

  // Create model with the region
  ArrangerObjectListModel region_model (region_objects, parent.get ());

  QSignalSpy contentChangedSpy (
    &region_model, &ArrangerObjectListModel::contentChanged);

  // Create a MIDI note to add to the region
  auto note_ref = registry_.create_object<MidiNote> (*tempo_map, parent.get ());
  auto * note = note_ref.get_object_as<MidiNote> ();
  note->position ()->setTicks (50.0);
  note->bounds ()->length ()->setTicks (20.0);

  // Add the note to the region - this should trigger the region's model to emit
  // contentChanged
  region->add_object (note_ref);

  // The region model should emit contentChanged when its children change
  // This verifies the connection is established between the region's internal
  // model and the ArrangerObjectListModel
  EXPECT_GT (contentChangedSpy.count (), 0);
}

// Test signal connections and disconnections
TEST_F (ArrangerObjectListModelTest, SignalConnectionsAndDisconnections)
{
  QSignalSpy contentChangedForObjectSpy (
    model_.get (), &ArrangerObjectListModel::contentChangedForObject);

  // Get an object that will be removed
  auto obj_to_remove = objects_.get<random_access_index> ()[2];

  // Remove the object
  model_->removeRows (2, 1);

  // Clear any signals from the removal
  contentChangedForObjectSpy.clear ();

  // Change a property on the removed object - should NOT trigger signal
  // since the connection should have been disconnected
  obj_to_remove.get_object_as<MidiNote> ()->setPitch (70);

  // Verify no signal was emitted
  EXPECT_EQ (contentChangedForObjectSpy.count (), 0);
}

// Test parent object functionality with ArrangerObjectListModel
TEST_F (ArrangerObjectListModelTest, ParentObjectFunctionality)
{
  // Create a parent object (MIDI region)
  auto parent_region_ref = registry_.create_object<MidiRegion> (
    *tempo_map, registry_, file_audio_source_registry_, parent.get ());
  auto * parent_region = parent_region_ref.get_object_as<MidiRegion> ();

  // Create a new model with the parent arranger object
  ArrangerObjectRefMultiIndexContainer child_objects;
  ArrangerObjectListModel parent_model (child_objects, *parent_region);

  // Create a child note
  auto child_note_ref =
    registry_.create_object<MidiNote> (*tempo_map, parent.get ());
  auto * child_note = child_note_ref.get_object_as<MidiNote> ();
  child_note->setPitch (60);

  // Insert the child into the parent model
  parent_model.insertObject (
    ArrangerObjectUuidReference (child_note->get_uuid (), registry_), 0);

  // Verify that the child's parent object is set to the parent region
  EXPECT_EQ (child_note->parentObject (), parent_region);

  // Remove the child from the parent model
  parent_model.removeRows (0, 1);

  // Verify that the child's parent object is cleared
  EXPECT_EQ (child_note->parentObject (), nullptr);
}

// Test parent object signal emissions
TEST_F (ArrangerObjectListModelTest, ParentObjectSignalEmissions)
{
  // Create a parent object (MIDI region)
  auto parent_region_ref = registry_.create_object<MidiRegion> (
    *tempo_map, registry_, file_audio_source_registry_, parent.get ());
  auto * parent_region = parent_region_ref.get_object_as<MidiRegion> ();

  // Create a new model with the parent arranger object
  ArrangerObjectRefMultiIndexContainer child_objects;
  ArrangerObjectListModel parent_model (child_objects, *parent_region);

  // Create a child note
  auto child_note_ref =
    registry_.create_object<MidiNote> (*tempo_map, parent.get ());
  auto * child_note = child_note_ref.get_object_as<MidiNote> ();
  child_note->setPitch (60);

  // Setup signal spy for parent object changes
  QSignalSpy parentObjectChangedSpy (child_note, &MidiNote::parentObjectChanged);
  QSignalSpy propertiesChangedSpy (child_note, &MidiNote::propertiesChanged);

  // Insert the child into the parent model
  parent_model.insertObject (
    ArrangerObjectUuidReference (child_note->get_uuid (), registry_), 0);

  // Verify that signals were emitted
  EXPECT_EQ (parentObjectChangedSpy.count (), 1);
  EXPECT_EQ (propertiesChangedSpy.count (), 1);

  // Clear spies
  parentObjectChangedSpy.clear ();
  propertiesChangedSpy.clear ();

  // Remove the child from the parent model
  parent_model.removeRows (0, 1);

  // Verify that signals were emitted again
  EXPECT_EQ (parentObjectChangedSpy.count (), 1);
  EXPECT_EQ (propertiesChangedSpy.count (), 1);
}

// Test sorted index functionality - auto-updating when object positions change
TEST_F (ArrangerObjectListModelTest, SortedIndexAutoUpdate)
{
  // Create objects with different positions
  auto note1_ref = registry_.create_object<MidiNote> (*tempo_map, parent.get ());
  auto * note1 = note1_ref.get_object_as<MidiNote> ();
  note1->position ()->setTicks (100.0);
  note1->setPitch (60);

  auto note2_ref = registry_.create_object<MidiNote> (*tempo_map, parent.get ());
  auto * note2 = note2_ref.get_object_as<MidiNote> ();
  note2->position ()->setTicks (50.0);
  note2->setPitch (62);

  auto note3_ref = registry_.create_object<MidiNote> (*tempo_map, parent.get ());
  auto * note3 = note3_ref.get_object_as<MidiNote> ();
  note3->position ()->setTicks (150.0);
  note3->setPitch (64);

  // Add objects to a new container and model
  ArrangerObjectRefMultiIndexContainer sorted_objects;
  sorted_objects.get<sequenced_index> ().emplace_back (std::move (note1_ref));
  sorted_objects.get<sequenced_index> ().emplace_back (std::move (note2_ref));
  sorted_objects.get<sequenced_index> ().emplace_back (std::move (note3_ref));

  ArrangerObjectListModel sorted_model (sorted_objects, parent.get ());

  // Verify initial sorted order: note2 (50), note1 (100), note3 (150)
  const auto &sorted_view = sorted_objects.get<sorted_index> ();
  EXPECT_EQ (sorted_view.size (), 3);

  auto it = sorted_view.begin ();
  EXPECT_DOUBLE_EQ (get_ticks_from_arranger_object_uuid_ref (*it), 50.0);
  EXPECT_EQ ((*it).get_object_as<MidiNote> ()->pitch (), 62);

  ++it;
  EXPECT_DOUBLE_EQ (get_ticks_from_arranger_object_uuid_ref (*it), 100.0);
  EXPECT_EQ ((*it).get_object_as<MidiNote> ()->pitch (), 60);

  ++it;
  EXPECT_DOUBLE_EQ (get_ticks_from_arranger_object_uuid_ref (*it), 150.0);
  EXPECT_EQ ((*it).get_object_as<MidiNote> ()->pitch (), 64);

  // Change note1's position to be after note3 - should trigger auto-resort
  note1->position ()->setTicks (200.0);

  // Verify new sorted order: note2 (50), note3 (150), note1 (200)
  it = sorted_view.begin ();
  EXPECT_DOUBLE_EQ (get_ticks_from_arranger_object_uuid_ref (*it), 50.0);
  EXPECT_EQ ((*it).get_object_as<MidiNote> ()->pitch (), 62);

  ++it;
  EXPECT_DOUBLE_EQ (get_ticks_from_arranger_object_uuid_ref (*it), 150.0);
  EXPECT_EQ ((*it).get_object_as<MidiNote> ()->pitch (), 64);

  ++it;
  EXPECT_DOUBLE_EQ (get_ticks_from_arranger_object_uuid_ref (*it), 200.0);
  EXPECT_EQ ((*it).get_object_as<MidiNote> ()->pitch (), 60);
}

// Test sorted index with multiple objects at same position
TEST_F (ArrangerObjectListModelTest, SortedIndexSamePosition)
{
  // Create objects with same position but different creation order
  auto note1_ref = registry_.create_object<MidiNote> (*tempo_map, parent.get ());
  auto * note1 = note1_ref.get_object_as<MidiNote> ();
  note1->position ()->setTicks (100.0);
  note1->setPitch (60);

  auto note2_ref = registry_.create_object<MidiNote> (*tempo_map, parent.get ());
  auto * note2 = note2_ref.get_object_as<MidiNote> ();
  note2->position ()->setTicks (100.0);
  note2->setPitch (62);

  // Add objects to container
  ArrangerObjectRefMultiIndexContainer same_pos_objects;
  same_pos_objects.get<sequenced_index> ().emplace_back (std::move (note1_ref));
  same_pos_objects.get<sequenced_index> ().emplace_back (std::move (note2_ref));

  // Verify sorted index maintains insertion order for same-position objects
  const auto &sorted_view = same_pos_objects.get<sorted_index> ();
  EXPECT_EQ (sorted_view.size (), 2);

  auto it = sorted_view.begin ();
  EXPECT_EQ ((*it).get_object_as<MidiNote> ()->pitch (), 60); // First inserted

  ++it;
  EXPECT_EQ ((*it).get_object_as<MidiNote> ()->pitch (), 62); // Second inserted
}

} // namespace zrythm::structure::arrangement
