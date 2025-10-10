// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <memory>

#include "dsp/atomic_position.h"
#include "dsp/atomic_position_qml_adapter.h"
#include "structure/arrangement/bounded_object.h"

#include "helpers/mock_qobject.h"

#include "unit/dsp/atomic_position_helpers.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace zrythm::structure::arrangement
{
class ArrangerObjectBoundsTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    time_conversion_funcs = dsp::basic_conversion_providers ();
    start_position =
      std::make_unique<dsp::AtomicPosition> (*time_conversion_funcs);
    parent = std::make_unique<MockQObject> ();
    start_position_adapter = std::make_unique<dsp::AtomicPositionQmlAdapter> (
      *start_position, true, parent.get ());

    // Create bounded object with proper parenting
    obj = std::make_unique<ArrangerObjectBounds> (*start_position_adapter);

    // Set position and length
    start_position_adapter->setSamples (1000);
    obj->length ()->setSamples (2000); // Object spans 1000-3000 samples
  }

  std::unique_ptr<dsp::AtomicPosition::TimeConversionFunctions>
                                                 time_conversion_funcs;
  std::unique_ptr<dsp::AtomicPosition>           start_position;
  std::unique_ptr<MockQObject>                   parent;
  std::unique_ptr<dsp::AtomicPositionQmlAdapter> start_position_adapter;
  std::unique_ptr<ArrangerObjectBounds>          obj;
};

// Test initial state
TEST_F (ArrangerObjectBoundsTest, InitialState)
{
  EXPECT_EQ (obj->length ()->samples (), 2000);
  EXPECT_NE (obj->length (), nullptr);
}

// Test length operations
TEST_F (ArrangerObjectBoundsTest, LengthOperations)
{
  obj->length ()->setSamples (3000);
  EXPECT_EQ (obj->length ()->samples (), 3000);
  EXPECT_EQ (obj->get_end_position_samples (false), units::samples (3999));
  EXPECT_EQ (obj->get_end_position_samples (true), units::samples (4000));
}

// Test is_hit method
TEST_F (ArrangerObjectBoundsTest, IsHit)
{
  // Within object
  EXPECT_TRUE (obj->is_hit (units::samples (1500)));
  EXPECT_TRUE (obj->is_hit (units::samples (1000))); // Start inclusive
  EXPECT_TRUE (obj->is_hit (units::samples (2999))); // End exclusive by default

  // Outside object
  EXPECT_FALSE (obj->is_hit (units::samples (999)));
  EXPECT_FALSE (obj->is_hit (units::samples (3000)));

  // Test inclusive end
  EXPECT_TRUE (obj->is_hit (units::samples (3000), true)); // End inclusive
}

// Test is_hit_by_range method
TEST_F (ArrangerObjectBoundsTest, IsHitByRange)
{
  // Range completely within object
  EXPECT_TRUE (
    obj->is_hit_by_range ({ units::samples (1200), units::samples (1800) }));

  // Range overlapping start
  EXPECT_TRUE (
    obj->is_hit_by_range ({ units::samples (500), units::samples (1500) }));

  // Range overlapping end
  EXPECT_TRUE (
    obj->is_hit_by_range ({ units::samples (2500), units::samples (3500) }));

  // Range covering object
  EXPECT_TRUE (
    obj->is_hit_by_range ({ units::samples (500), units::samples (3500) }));

  // Range before object
  EXPECT_FALSE (
    obj->is_hit_by_range ({ units::samples (500), units::samples (999) }));

  // Range after object
  EXPECT_FALSE (
    obj->is_hit_by_range ({ units::samples (3000), units::samples (3500) }));

  // Range exactly at boundaries
  EXPECT_TRUE (
    obj->is_hit_by_range ({ units::samples (1000), units::samples (1000) }));
  EXPECT_TRUE (
    obj->is_hit_by_range ({ units::samples (2999), units::samples (2999) }));

  // Test exclusive boundaries
  EXPECT_FALSE (obj->is_hit_by_range (
    { units::samples (1000), units::samples (1000) }, false, false, false));
  EXPECT_TRUE (obj->is_hit_by_range (
    { units::samples (999), units::samples (1000) }, false, true, false));

  // Test different boundary combinations
  EXPECT_TRUE (obj->is_hit_by_range (
    { units::samples (1000), units::samples (1000) }, true, true,
    false)); // All inclusive
  EXPECT_FALSE (obj->is_hit_by_range (
    { units::samples (1000), units::samples (1000) }, false, false,
    false)); // All exclusive
}

// Test QML properties
TEST_F (ArrangerObjectBoundsTest, QmlProperties)
{
  EXPECT_EQ (obj->length ()->samples (), 2000);
}

// Test serialization/deserialization
TEST_F (ArrangerObjectBoundsTest, Serialization)
{
  // Set initial state
  obj->length ()->setTicks (1920.0);

  // Serialize
  nlohmann::json j;
  to_json (j, *obj);

  // Create new object from serialized data
  dsp::AtomicPosition           new_start_pos (*time_conversion_funcs);
  dsp::AtomicPositionQmlAdapter new_start_adapter (
    new_start_pos, false, parent.get ());
  ArrangerObjectBounds new_obj (new_start_adapter);
  from_json (j, new_obj);

  // Verify state
  EXPECT_DOUBLE_EQ (new_obj.length ()->ticks (), 1920.0);
}

// Test edge cases
TEST_F (ArrangerObjectBoundsTest, EdgeCases)
{
  // Zero-length object
  obj->length ()->setSamples (0);
  EXPECT_FALSE (obj->is_hit (units::samples (1000)));
  EXPECT_TRUE (obj->is_hit (units::samples (1000), true));
  EXPECT_FALSE (obj->is_hit (units::samples (1001), true));

  // Negative length (should clamp to 0)
  obj->length ()->setSamples (-100);
  EXPECT_EQ (obj->length ()->samples (), 0);
  EXPECT_EQ (
    obj->get_end_position_samples (false),
    units::samples (999)); // Start position remains
  EXPECT_EQ (obj->get_end_position_samples (true), units::samples (1000));

  // Large values
  obj->length ()->setSamples (1e9);
  EXPECT_EQ (
    obj->get_end_position_samples (false),
    units::samples (static_cast<int64_t> (1e9 + 1000 - 1)));
}

} // namespace zrythm::structure::arrangement
