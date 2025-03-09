// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/plugin_identifier.h"
#include "utils/gtest_wrapper.h"

using namespace zrythm::dsp;

TEST (PluginIdentifierTest, Construction)
{
  // Default construction
  PluginIdentifier id1;
  EXPECT_TRUE (id1.slot_.validate_slot_type_slot_combo ());

  // Test with valid slot
  auto track_uuid = PluginIdentifier::TrackUuid{ QUuid::createUuid () };
  PluginIdentifier id2;
  id2.slot_ = PluginSlot{ PluginSlotType::Insert, 2 };
  id2.track_id_ = track_uuid;

  EXPECT_TRUE (id2.slot_.validate_slot_type_slot_combo ());
  auto [type, slot] = id2.slot_.get_slot_with_index ();
  EXPECT_EQ (type, PluginSlotType::Insert);
  EXPECT_EQ (slot, 2);
}

TEST (PluginIdentifierTest, SlotValidation)
{
  // Test instrument plugin (should have no slot)
  PluginSlot instrument_slot{ PluginSlotType::Instrument };
  EXPECT_TRUE (instrument_slot.validate_slot_type_slot_combo ());

  // Test insert plugin (must have slot)
  PluginSlot insert_slot{ PluginSlotType::Insert, 0 };
  EXPECT_TRUE (insert_slot.validate_slot_type_slot_combo ());

  // Invalid combinations would throw during construction
  EXPECT_TRUE (
    PluginSlot{ PluginSlotType::Invalid }.validate_slot_type_slot_combo ());
}

TEST (PluginIdentifierTest, Comparison)
{
  auto track_id = PluginIdentifier::TrackUuid{ QUuid::createUuid () };

  PluginIdentifier id1;
  id1.slot_ = PluginSlot{ PluginSlotType::MidiFx, 1 };
  id1.track_id_ = track_id;

  PluginIdentifier id2;
  id2.slot_ = PluginSlot{ PluginSlotType::MidiFx, 2 };
  id2.track_id_ = track_id;

  PluginIdentifier id3;
  id3.slot_ = PluginSlot{ PluginSlotType::MidiFx, 1 };
  id3.track_id_ = track_id;

  EXPECT_LT (id1, id2);
  EXPECT_GT (id2, id1);
  EXPECT_EQ (id1, id3);
}

TEST (PluginIdentifierTest, Hashing)
{
  auto track_id = PluginIdentifier::TrackUuid{ QUuid::createUuid () };

  PluginIdentifier id1;
  id1.slot_ = PluginSlot{ PluginSlotType::MidiFx, 3 };
  id1.track_id_ = track_id;

  PluginIdentifier id2;
  id2.slot_ = PluginSlot{ PluginSlotType::MidiFx, 3 };
  id2.track_id_ = track_id;

  EXPECT_EQ (id1.get_hash (), id2.get_hash ());
}

TEST (PluginIdentifierTest, Serialization)
{
  PluginIdentifier id1;
  id1.slot_ = PluginSlot{ PluginSlotType::Insert, 2 };
  id1.track_id_ = PluginIdentifier::TrackUuid{ QUuid::createUuid () };

  // Serialize to JSON
  auto json_str = id1.serialize_to_json_string ();

  // Create new object and deserialize
  PluginIdentifier id2;
  id2.deserialize_from_json_string (json_str.c_str ());

  // Verify all fields match
  EXPECT_EQ (id1.slot_, id2.slot_);
  EXPECT_EQ (type_safe::get (id1.track_id_), type_safe::get (id2.track_id_));
}
