// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/plugin_identifier.h"
#include "utils/gtest_wrapper.h"

using namespace zrythm::dsp;

TEST (PluginIdentifierTest, Construction)
{
  // Default construction
  PluginIdentifier id1;
  EXPECT_EQ (id1.slot_type_, PluginSlotType::Invalid);
  EXPECT_EQ (id1.track_name_hash_, 0);
  EXPECT_EQ (id1.slot_, -1);

  // Construction with values
  PluginIdentifier id2;
  id2.slot_type_ = PluginSlotType::Insert;
  id2.track_name_hash_ = 12345;
  id2.slot_ = 2;
  EXPECT_EQ (id2.slot_type_, PluginSlotType::Insert);
  EXPECT_EQ (id2.track_name_hash_, 12345);
  EXPECT_EQ (id2.slot_, 2);
}

TEST (PluginIdentifierTest, Comparison)
{
  PluginIdentifier id1;
  id1.slot_type_ = PluginSlotType::Insert;
  id1.track_name_hash_ = 100;
  id1.slot_ = 1;

  PluginIdentifier id2;
  id2.slot_type_ = PluginSlotType::Insert;
  id2.track_name_hash_ = 100;
  id2.slot_ = 2;

  PluginIdentifier id3;
  id3.slot_type_ = PluginSlotType::Insert;
  id3.track_name_hash_ = 100;
  id3.slot_ = 1;

  EXPECT_LT (id1, id2);
  EXPECT_GT (id2, id1);
  EXPECT_EQ (id1, id3);
}

TEST (PluginIdentifierTest, SlotValidation)
{
  EXPECT_TRUE (PluginIdentifier::validate_slot_type_slot_combo (
    PluginSlotType::Instrument, -1));
  EXPECT_TRUE (PluginIdentifier::validate_slot_type_slot_combo (
    PluginSlotType::Insert, 0));
  EXPECT_FALSE (PluginIdentifier::validate_slot_type_slot_combo (
    PluginSlotType::Insert, -1));
  EXPECT_FALSE (PluginIdentifier::validate_slot_type_slot_combo (
    PluginSlotType::Instrument, 0));
}

TEST (PluginIdentifierTest, Hashing)
{
  PluginIdentifier id1;
  id1.slot_type_ = PluginSlotType::MidiFx;
  id1.track_name_hash_ = 200;
  id1.slot_ = 3;

  PluginIdentifier id2;
  id2.slot_type_ = PluginSlotType::MidiFx;
  id2.track_name_hash_ = 200;
  id2.slot_ = 3;

  EXPECT_EQ (id1.get_hash (), id2.get_hash ());
}

TEST (PluginIdentifierTest, Serialization)
{
  PluginIdentifier id1;
  id1.slot_type_ = PluginSlotType::Insert;
  id1.track_name_hash_ = 12345;
  id1.slot_ = 2;

  // Serialize to JSON
  auto json_str = id1.serialize_to_json_string ();

  // Create new object and deserialize
  PluginIdentifier id2;
  id2.deserialize_from_json_string (json_str.c_str ());

  // Verify all fields match
  EXPECT_EQ (id1.slot_type_, id2.slot_type_);
  EXPECT_EQ (id1.track_name_hash_, id2.track_name_hash_);
  EXPECT_EQ (id1.slot_, id2.slot_);
}
