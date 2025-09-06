// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <memory>

#include "dsp/tempo_map.h"
#include "structure/arrangement/arranger_object_all.h"
#include "structure/arrangement/arranger_object_list_model.h"

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
    tempo_map = std::make_unique<dsp::TempoMap> (44100.0);
    parent = std::make_unique<MockQObject> ();

    // Create test MIDI notes
    for (int i = 0; i < 5; i++)
      {
        auto note_ref =
          registry_.create_object<MidiNote> (*tempo_map, parent.get ());
        auto * note = note_ref.get_object_as<MidiNote> ();
        note->setPitch (60 + i);
        note->setVelocity (90 + i);
        objects_.emplace_back (std::move (note_ref));
      }

    // Create model
    model_ = std::make_unique<ArrangerObjectListModel> (objects_, parent.get ());
  }

  std::unique_ptr<dsp::TempoMap>           tempo_map;
  std::unique_ptr<MockQObject>             parent;
  ArrangerObjectRegistry                   registry_;
  std::vector<ArrangerObjectUuidReference> objects_;
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
  std::vector<ArrangerObjectUuidReference> empty_objects;
  ArrangerObjectListModel empty_model (empty_objects, parent.get ());

  EXPECT_EQ (empty_model.rowCount (), 0);
  EXPECT_FALSE (empty_model.data (empty_model.index (0, 0)).isValid ());
}

} // namespace zrythm::structure::arrangement
