// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "plugins/plugin_slot.h"
#include "utils/gtest_wrapper.h"

using namespace zrythm::plugins;

TEST (PluginSlotTest, ConstructionAndValidation)
{
  // Default construction (invalid slot type)
  PluginSlot invalid_slot;
  EXPECT_FALSE (invalid_slot.validate_slot_type_slot_combo ());

  // Valid instrument slot (no slot number)
  PluginSlot instrument_slot{ PluginSlotType::Instrument };
  EXPECT_TRUE (instrument_slot.validate_slot_type_slot_combo ());

  // Valid insert slot with slot number
  PluginSlot insert_slot{ PluginSlotType::Insert, 2 };
  EXPECT_TRUE (insert_slot.validate_slot_type_slot_combo ());
}

TEST (PluginSlotTest, SlotValidation)
{
  // Test valid combinations
  EXPECT_TRUE (
    (PluginSlot{ PluginSlotType::Insert, 0 }).validate_slot_type_slot_combo ());
  EXPECT_TRUE (
    (PluginSlot{ PluginSlotType::MidiFx, 3 }).validate_slot_type_slot_combo ());
  EXPECT_TRUE (
    (PluginSlot{ PluginSlotType::Instrument }).validate_slot_type_slot_combo ());

  // Test invalid combinations
  EXPECT_FALSE (
    (PluginSlot{ PluginSlotType::Insert }).validate_slot_type_slot_combo ());
  EXPECT_FALSE (
    (PluginSlot{ PluginSlotType::MidiFx }).validate_slot_type_slot_combo ());
  EXPECT_FALSE (
    (PluginSlot{ PluginSlotType::Instrument, 2 })
      .validate_slot_type_slot_combo ());
}

TEST (PluginSlotTest, Comparison)
{
  PluginSlot insert1{ PluginSlotType::Insert, 1 };
  PluginSlot insert2{ PluginSlotType::Insert, 2 };
  PluginSlot midiFx{ PluginSlotType::MidiFx, 1 };
  PluginSlot instrument{ PluginSlotType::Instrument };

  // Test equality
  EXPECT_EQ (insert1, (PluginSlot{ PluginSlotType::Insert, 1 }));
  EXPECT_NE (insert1, insert2);

  // Test ordering
  EXPECT_LT (insert1, insert2);
  EXPECT_LT (midiFx, insert1);    // Type-based ordering
  EXPECT_GT (instrument, midiFx); // Type-based ordering
}

TEST (PluginSlotTest, Serialization)
{
  PluginSlot original{ PluginSlotType::Insert, 2 };

  // Serialize to JSON
  nlohmann::json j = original;

  // Deserialize
  PluginSlot deserialized;
  from_json (j, deserialized);

  EXPECT_EQ (original, deserialized);
}
