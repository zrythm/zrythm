// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <memory>

#include "dsp/tempo_map.h"
#include "structure/arrangement/tempo_object.h"
#include "structure/arrangement/tempo_object_manager.h"
#include "structure/arrangement/time_signature_object.h"

#include <QSignalSpy>

#include "unit/dsp/graph_helpers.h"
#include <gtest/gtest.h>

namespace zrythm::structure::arrangement
{
class TempoObjectManagerTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    // Create test dependencies
    tempo_map = std::make_unique<dsp::TempoMap> (units::sample_rate (44100.0));
    tempo_map_wrapper = std::make_unique<dsp::TempoMapWrapper> (*tempo_map);

    // Create tempo object manager
    tempo_manager = std::make_unique<TempoObjectManager> (
      obj_registry, file_audio_source_registry);
  }

  // Helper to create a tempo object
  ArrangerObjectUuidReference create_tempo_object (double bpm = 120.0)
  {
    return obj_registry.create_object<TempoObject> (*tempo_map);
  }

  // Helper to create a time signature object
  ArrangerObjectUuidReference
  create_time_signature_object (int numerator = 4, int denominator = 4)
  {
    return obj_registry.create_object<TimeSignatureObject> (*tempo_map);
  }

  std::unique_ptr<dsp::TempoMap>                 tempo_map;
  std::unique_ptr<dsp::TempoMapWrapper>          tempo_map_wrapper;
  structure::arrangement::ArrangerObjectRegistry obj_registry;
  dsp::FileAudioSourceRegistry                   file_audio_source_registry;
  dsp::graph_test::MockTransport                 transport;
  std::unique_ptr<TempoObjectManager>            tempo_manager;
};

// Test initial state
TEST_F (TempoObjectManagerTest, InitialState)
{
  EXPECT_EQ (tempo_manager->tempoObjects ()->rowCount (), 0);
  EXPECT_EQ (tempo_manager->timeSignatureObjects ()->rowCount (), 0);
}

// Test adding tempo objects
TEST_F (TempoObjectManagerTest, AddTempoObjects)
{
  auto tempo_obj1 = create_tempo_object (120.0);
  auto tempo_obj2 = create_tempo_object (140.0);

  tempo_manager->ArrangerObjectOwner<TempoObject>::add_object (tempo_obj1);
  tempo_manager->ArrangerObjectOwner<TempoObject>::add_object (tempo_obj2);

  EXPECT_EQ (tempo_manager->tempoObjects ()->rowCount (), 2);

  // Verify the objects are accessible
  auto tempo_obj1_ptr = tempo_obj1.get_object_as<TempoObject> ();
  auto tempo_obj2_ptr = tempo_obj2.get_object_as<TempoObject> ();
  EXPECT_NE (tempo_obj1_ptr, nullptr);
  EXPECT_NE (tempo_obj2_ptr, nullptr);
}

// Test adding time signature objects
TEST_F (TempoObjectManagerTest, AddTimeSignatureObjects)
{
  auto time_sig_obj1 = create_time_signature_object (4, 4);
  auto time_sig_obj2 = create_time_signature_object (3, 4);

  tempo_manager->ArrangerObjectOwner<TimeSignatureObject>::add_object (
    time_sig_obj1);
  tempo_manager->ArrangerObjectOwner<TimeSignatureObject>::add_object (
    time_sig_obj2);

  EXPECT_EQ (tempo_manager->timeSignatureObjects ()->rowCount (), 2);

  // Verify the objects are accessible
  auto time_sig_obj1_ptr = time_sig_obj1.get_object_as<TimeSignatureObject> ();
  auto time_sig_obj2_ptr = time_sig_obj2.get_object_as<TimeSignatureObject> ();
  EXPECT_NE (time_sig_obj1_ptr, nullptr);
  EXPECT_NE (time_sig_obj2_ptr, nullptr);
}

// Test removing tempo objects
TEST_F (TempoObjectManagerTest, RemoveTempoObjects)
{
  auto tempo_obj1 = create_tempo_object (120.0);
  auto tempo_obj2 = create_tempo_object (140.0);

  tempo_manager->ArrangerObjectOwner<TempoObject>::add_object (tempo_obj1);
  tempo_manager->ArrangerObjectOwner<TempoObject>::add_object (tempo_obj2);

  EXPECT_EQ (tempo_manager->tempoObjects ()->rowCount (), 2);

  // Remove first tempo object
  tempo_manager->ArrangerObjectOwner<TempoObject>::remove_object (
    tempo_obj1.id ());
  EXPECT_EQ (tempo_manager->tempoObjects ()->rowCount (), 1);

  // Remove second tempo object
  tempo_manager->ArrangerObjectOwner<TempoObject>::remove_object (
    tempo_obj2.id ());
  EXPECT_EQ (tempo_manager->tempoObjects ()->rowCount (), 0);
}

// Test removing time signature objects
TEST_F (TempoObjectManagerTest, RemoveTimeSignatureObjects)
{
  auto time_sig_obj1 = create_time_signature_object (4, 4);
  auto time_sig_obj2 = create_time_signature_object (3, 4);

  tempo_manager->ArrangerObjectOwner<TimeSignatureObject>::add_object (
    time_sig_obj1);
  tempo_manager->ArrangerObjectOwner<TimeSignatureObject>::add_object (
    time_sig_obj2);

  EXPECT_EQ (tempo_manager->timeSignatureObjects ()->rowCount (), 2);

  // Remove first time signature object
  tempo_manager->ArrangerObjectOwner<TimeSignatureObject>::remove_object (
    time_sig_obj1.id ());
  EXPECT_EQ (tempo_manager->timeSignatureObjects ()->rowCount (), 1);

  // Remove second time signature object
  tempo_manager->ArrangerObjectOwner<TimeSignatureObject>::remove_object (
    time_sig_obj2.id ());
  EXPECT_EQ (tempo_manager->timeSignatureObjects ()->rowCount (), 0);
}

// Test tempo object property changes fire contentChanged signal
TEST_F (TempoObjectManagerTest, TempoObjectPropertyChangesFireContentChanged)
{
  auto tempo_obj = create_tempo_object (120.0);
  tempo_manager->ArrangerObjectOwner<TempoObject>::add_object (tempo_obj);

  auto tempo_obj_ptr = tempo_obj.get_object_as<TempoObject> ();
  ASSERT_NE (tempo_obj_ptr, nullptr);

  // Setup signal spy on tempo model
  QSignalSpy tempoSpy (
    tempo_manager->tempoObjects (), &ArrangerObjectListModel::contentChanged);

  // Change tempo property
  tempo_obj_ptr->setTempo (140.0);

  // Verify contentChanged was fired
  EXPECT_GT (tempoSpy.count (), 0);
}

// Test time signature object property changes fire contentChanged signal
TEST_F (
  TempoObjectManagerTest,
  TimeSignatureObjectPropertyChangesFireContentChanged)
{
  auto time_sig_obj = create_time_signature_object (4, 4);
  tempo_manager->ArrangerObjectOwner<TimeSignatureObject>::add_object (
    time_sig_obj);

  auto time_sig_obj_ptr = time_sig_obj.get_object_as<TimeSignatureObject> ();
  ASSERT_NE (time_sig_obj_ptr, nullptr);

  // Setup signal spy on time signature model
  QSignalSpy timeSigSpy (
    tempo_manager->timeSignatureObjects (),
    &ArrangerObjectListModel::contentChanged);

  // Change numerator property
  time_sig_obj_ptr->setNumerator (6);

  // Verify contentChanged was fired (may fail as mentioned in task)
  EXPECT_GT (timeSigSpy.count (), 0);

  // Reset spy and test denominator change
  timeSigSpy.clear ();
  time_sig_obj_ptr->setDenominator (8);

  // Verify contentChanged was fired again (may fail as mentioned in task)
  EXPECT_GT (timeSigSpy.count (), 0);
}

// Test position changes fire contentChanged signal
TEST_F (TempoObjectManagerTest, PositionChangesFireContentChanged)
{
  auto tempo_obj = create_tempo_object (120.0);
  tempo_manager->ArrangerObjectOwner<TempoObject>::add_object (tempo_obj);

  auto tempo_obj_ptr = tempo_obj.get_object_as<TempoObject> ();
  ASSERT_NE (tempo_obj_ptr, nullptr);

  // Setup signal spy on tempo model
  QSignalSpy tempoSpy (
    tempo_manager->tempoObjects (), &ArrangerObjectListModel::contentChanged);

  // Change position
  tempo_obj_ptr->position ()->setSamples (1000);

  // Verify contentChanged was fired
  EXPECT_GT (tempoSpy.count (), 0);
}

// Test mixed object types
TEST_F (TempoObjectManagerTest, MixedObjectTypes)
{
  auto tempo_obj = create_tempo_object (120.0);
  auto time_sig_obj = create_time_signature_object (4, 4);

  tempo_manager->ArrangerObjectOwner<TempoObject>::add_object (tempo_obj);
  tempo_manager->ArrangerObjectOwner<TimeSignatureObject>::add_object (
    time_sig_obj);

  EXPECT_EQ (tempo_manager->tempoObjects ()->rowCount (), 1);
  EXPECT_EQ (tempo_manager->timeSignatureObjects ()->rowCount (), 1);

  // Verify objects are in correct models
  auto tempo_obj_ptr = tempo_obj.get_object_as<TempoObject> ();
  auto time_sig_obj_ptr = time_sig_obj.get_object_as<TimeSignatureObject> ();
  EXPECT_NE (tempo_obj_ptr, nullptr);
  EXPECT_NE (time_sig_obj_ptr, nullptr);
}

// Test clearing objects
TEST_F (TempoObjectManagerTest, ClearObjects)
{
  auto tempo_obj1 = create_tempo_object (120.0);
  auto tempo_obj2 = create_tempo_object (140.0);
  auto time_sig_obj1 = create_time_signature_object (4, 4);
  auto time_sig_obj2 = create_time_signature_object (3, 4);

  tempo_manager->ArrangerObjectOwner<TempoObject>::add_object (tempo_obj1);
  tempo_manager->ArrangerObjectOwner<TempoObject>::add_object (tempo_obj2);
  tempo_manager->ArrangerObjectOwner<TimeSignatureObject>::add_object (
    time_sig_obj1);
  tempo_manager->ArrangerObjectOwner<TimeSignatureObject>::add_object (
    time_sig_obj2);

  EXPECT_EQ (tempo_manager->tempoObjects ()->rowCount (), 2);
  EXPECT_EQ (tempo_manager->timeSignatureObjects ()->rowCount (), 2);

  // Clear tempo objects
  tempo_manager->ArrangerObjectOwner<TempoObject>::clear_objects ();
  EXPECT_EQ (tempo_manager->tempoObjects ()->rowCount (), 0);
  EXPECT_EQ (tempo_manager->timeSignatureObjects ()->rowCount (), 2);

  // Clear time signature objects
  tempo_manager->ArrangerObjectOwner<TimeSignatureObject>::clear_objects ();
  EXPECT_EQ (tempo_manager->tempoObjects ()->rowCount (), 0);
  EXPECT_EQ (tempo_manager->timeSignatureObjects ()->rowCount (), 0);
}

// Test serialization/deserialization
TEST_F (TempoObjectManagerTest, Serialization)
{
  auto tempo_obj = create_tempo_object (140.0);
  auto time_sig_obj = create_time_signature_object (6, 8);

  tempo_manager->ArrangerObjectOwner<TempoObject>::add_object (tempo_obj);
  tempo_manager->ArrangerObjectOwner<TimeSignatureObject>::add_object (
    time_sig_obj);

  // Set properties on objects
  auto tempo_ptr = tempo_obj.get_object_as<TempoObject> ();
  auto time_sig_ptr = time_sig_obj.get_object_as<TimeSignatureObject> ();
  tempo_ptr->position ()->setSamples (1000);
  tempo_ptr->setTempo (160.0);
  time_sig_ptr->position ()->setSamples (2000);
  time_sig_ptr->setNumerator (7);
  time_sig_ptr->setDenominator (16);

  // Serialize
  nlohmann::json j;
  to_json (j, *tempo_manager);

  // Create new manager for deserialization
  auto new_manager = std::make_unique<TempoObjectManager> (
    obj_registry, file_audio_source_registry);
  from_json (j, *new_manager);

  // Verify deserialization
  EXPECT_EQ (new_manager->tempoObjects ()->rowCount (), 1);
  EXPECT_EQ (new_manager->timeSignatureObjects ()->rowCount (), 1);

  // Verify object properties
  auto new_tempo_obj_ref =
    new_manager->ArrangerObjectOwner<TempoObject>::get_children_vector ()[0];
  auto new_time_sig_obj_ref = new_manager->ArrangerObjectOwner<
    TimeSignatureObject>::get_children_vector ()[0];

  auto new_tempo_ptr = new_tempo_obj_ref.get_object_as<TempoObject> ();
  auto new_time_sig_ptr =
    new_time_sig_obj_ref.get_object_as<TimeSignatureObject> ();

  EXPECT_EQ (new_tempo_ptr->position ()->samples (), 1000);
  EXPECT_DOUBLE_EQ (new_tempo_ptr->tempo (), 160.0);
  EXPECT_EQ (new_time_sig_ptr->position ()->samples (), 2000);
  EXPECT_EQ (new_time_sig_ptr->numerator (), 7);
  EXPECT_EQ (new_time_sig_ptr->denominator (), 16);
}

// Test copying with init_from
TEST_F (TempoObjectManagerTest, Copying)
{
  auto tempo_obj = create_tempo_object (140.0);
  auto time_sig_obj = create_time_signature_object (6, 8);

  tempo_manager->ArrangerObjectOwner<TempoObject>::add_object (tempo_obj);
  tempo_manager->ArrangerObjectOwner<TimeSignatureObject>::add_object (
    time_sig_obj);

  // Set properties on objects
  auto tempo_ptr = tempo_obj.get_object_as<TempoObject> ();
  auto time_sig_ptr = time_sig_obj.get_object_as<TimeSignatureObject> ();
  tempo_ptr->position ()->setSamples (1000);
  tempo_ptr->setTempo (160.0);
  time_sig_ptr->position ()->setSamples (2000);
  time_sig_ptr->setNumerator (7);
  time_sig_ptr->setDenominator (16);

  // Create target manager
  auto target_manager = std::make_unique<TempoObjectManager> (
    obj_registry, file_audio_source_registry);

  // Copy
  init_from (
    *target_manager, *tempo_manager, utils::ObjectCloneType::NewIdentity);

  // Verify copy
  EXPECT_EQ (target_manager->tempoObjects ()->rowCount (), 1);
  EXPECT_EQ (target_manager->timeSignatureObjects ()->rowCount (), 1);

  // Verify object properties
  auto copied_tempo_obj_ref =
    target_manager->ArrangerObjectOwner<TempoObject>::get_children_vector ()[0];
  auto copied_time_sig_obj_ref = target_manager->ArrangerObjectOwner<
    TimeSignatureObject>::get_children_vector ()[0];

  auto copied_tempo_ptr = copied_tempo_obj_ref.get_object_as<TempoObject> ();
  auto copied_time_sig_ptr =
    copied_time_sig_obj_ref.get_object_as<TimeSignatureObject> ();

  EXPECT_EQ (copied_tempo_ptr->position ()->samples (), 1000);
  EXPECT_DOUBLE_EQ (copied_tempo_ptr->tempo (), 160.0);
  EXPECT_EQ (copied_time_sig_ptr->position ()->samples (), 2000);
  EXPECT_EQ (copied_time_sig_ptr->numerator (), 7);
  EXPECT_EQ (copied_time_sig_ptr->denominator (), 16);
}

// Test field names for serialization
TEST_F (TempoObjectManagerTest, SerializationFieldNames)
{
  EXPECT_EQ (
    tempo_manager->get_field_name_for_serialization (
      static_cast<TempoObject *> (nullptr)),
    "tempoObjects");
  EXPECT_EQ (
    tempo_manager->get_field_name_for_serialization (
      static_cast<TimeSignatureObject *> (nullptr)),
    "timeSignatureObjects");
}

} // namespace zrythm::structure::arrangement
