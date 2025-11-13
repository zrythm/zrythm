// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <memory>

#include "dsp/tempo_map.h"
#include "structure/arrangement/tempo_object.h"

#include <QSignalSpy>

#include "helpers/mock_qobject.h"

#include <gtest/gtest.h>

namespace zrythm::structure::arrangement
{
class TempoObjectTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    tempo_map = std::make_unique<dsp::TempoMap> (units::sample_rate (44100.0));
    parent = std::make_unique<MockQObject> ();
    tempo_obj = std::make_unique<TempoObject> (*tempo_map, parent.get ());
  }

  std::unique_ptr<dsp::TempoMap> tempo_map;
  std::unique_ptr<MockQObject>   parent;
  std::unique_ptr<TempoObject>   tempo_obj;
};

// Test initial state
TEST_F (TempoObjectTest, InitialState)
{
  EXPECT_EQ (tempo_obj->type (), ArrangerObject::Type::TempoObject);
  EXPECT_EQ (tempo_obj->position ()->samples (), 0);
  EXPECT_DOUBLE_EQ (tempo_obj->tempo (), TempoObject::DEFAULT_TEMPO);
  EXPECT_EQ (tempo_obj->curve (), TempoObject::CurveType::Constant);
}

// Test tempo property
TEST_F (TempoObjectTest, TempoProperty)
{
  // Set tempo
  tempo_obj->setTempo (140.0);
  EXPECT_DOUBLE_EQ (tempo_obj->tempo (), 140.0);

  // Set same tempo (should be no-op)
  QSignalSpy spy (tempo_obj.get (), &TempoObject::tempoChanged);
  tempo_obj->setTempo (140.0);
  EXPECT_EQ (spy.count (), 0);
}

// Test tempoChanged signal
TEST_F (TempoObjectTest, TempoChangedSignal)
{
  // Setup signal spy
  QSignalSpy spy (tempo_obj.get (), &TempoObject::tempoChanged);

  // First set
  tempo_obj->setTempo (130.0);
  EXPECT_EQ (spy.count (), 1);
  QList<QVariant> arguments = spy.takeFirst ();
  EXPECT_DOUBLE_EQ (arguments.at (0).toDouble (), 130.0);

  // Change tempo
  tempo_obj->setTempo (160.0);
  EXPECT_EQ (spy.count (), 1);
  arguments = spy.takeFirst ();
  EXPECT_DOUBLE_EQ (arguments.at (0).toDouble (), 160.0);
}

// Test curve property
TEST_F (TempoObjectTest, CurveProperty)
{
  // Set curve
  tempo_obj->setCurve (TempoObject::CurveType::Linear);
  EXPECT_EQ (tempo_obj->curve (), TempoObject::CurveType::Linear);

  // Set same curve (should be no-op)
  QSignalSpy spy (tempo_obj.get (), &TempoObject::curveChanged);
  tempo_obj->setCurve (TempoObject::CurveType::Linear);
  EXPECT_EQ (spy.count (), 0);
}

// Test curveChanged signal
TEST_F (TempoObjectTest, CurveChangedSignal)
{
  // Setup signal spy
  QSignalSpy spy (tempo_obj.get (), &TempoObject::curveChanged);

  // First set
  tempo_obj->setCurve (TempoObject::CurveType::Linear);
  EXPECT_EQ (spy.count (), 1);
  QList<QVariant> arguments = spy.takeFirst ();
  EXPECT_EQ (
    arguments.at (0).toInt (),
    static_cast<int> (TempoObject::CurveType::Linear));

  // Change curve
  tempo_obj->setCurve (TempoObject::CurveType::Constant);
  EXPECT_EQ (spy.count (), 1);
  arguments = spy.takeFirst ();
  EXPECT_EQ (
    arguments.at (0).toInt (),
    static_cast<int> (TempoObject::CurveType::Constant));
}

// Test serialization/deserialization
TEST_F (TempoObjectTest, Serialization)
{
  // Set initial state
  tempo_obj->position ()->setSamples (1000);
  tempo_obj->setTempo (150.0);
  tempo_obj->setCurve (TempoObject::CurveType::Linear);

  // Serialize
  nlohmann::json j;
  to_json (j, *tempo_obj);

  // Create new object
  auto new_tempo_obj = std::make_unique<TempoObject> (*tempo_map, parent.get ());
  from_json (j, *new_tempo_obj);

  // Verify
  EXPECT_EQ (new_tempo_obj->position ()->samples (), 1000);
  EXPECT_DOUBLE_EQ (new_tempo_obj->tempo (), 150.0);
  EXPECT_EQ (new_tempo_obj->curve (), TempoObject::CurveType::Linear);
}

// Test copying with init_from
TEST_F (TempoObjectTest, Copying)
{
  // Set initial state
  tempo_obj->position ()->setSamples (2000);
  tempo_obj->setTempo (180.0);
  tempo_obj->setCurve (TempoObject::CurveType::Linear);

  // Create target
  auto target = std::make_unique<TempoObject> (*tempo_map, parent.get ());

  // Copy
  init_from (*target, *tempo_obj, utils::ObjectCloneType::Snapshot);

  // Verify
  EXPECT_EQ (target->position ()->samples (), 2000);
  EXPECT_DOUBLE_EQ (target->tempo (), 180.0);
  EXPECT_EQ (target->curve (), TempoObject::CurveType::Linear);
}

// Test edge cases
TEST_F (TempoObjectTest, EdgeCases)
{
  // Zero tempo
  tempo_obj->setTempo (0.0);
  EXPECT_DOUBLE_EQ (tempo_obj->tempo (), 0.0);

  // Negative tempo
  tempo_obj->setTempo (-60.0);
  EXPECT_DOUBLE_EQ (tempo_obj->tempo (), -60.0);

  // Very high tempo
  tempo_obj->setTempo (1000.0);
  EXPECT_DOUBLE_EQ (tempo_obj->tempo (), 1000.0);

  // Very small tempo
  tempo_obj->setTempo (1.0);
  EXPECT_DOUBLE_EQ (tempo_obj->tempo (), 1.0);

  // Position at max value
  tempo_obj->position ()->setSamples (std::numeric_limits<int>::max ());
  EXPECT_EQ (
    tempo_obj->position ()->samples (), std::numeric_limits<int>::max ());
}

// Test tempo precision
TEST_F (TempoObjectTest, TempoPrecision)
{
  // Test with decimal values
  tempo_obj->setTempo (120.5);
  EXPECT_DOUBLE_EQ (tempo_obj->tempo (), 120.5);

  tempo_obj->setTempo (140.75);
  EXPECT_DOUBLE_EQ (tempo_obj->tempo (), 140.75);

  // Test with very small differences
  tempo_obj->setTempo (120.0001);
  EXPECT_DOUBLE_EQ (tempo_obj->tempo (), 120.0001);
}

// Test curve type enumeration
TEST_F (TempoObjectTest, CurveTypeEnumeration)
{
  // Test all curve types
  tempo_obj->setCurve (TempoObject::CurveType::Constant);
  EXPECT_EQ (tempo_obj->curve (), TempoObject::CurveType::Constant);

  tempo_obj->setCurve (TempoObject::CurveType::Linear);
  EXPECT_EQ (tempo_obj->curve (), TempoObject::CurveType::Linear);
}

} // namespace zrythm::structure::arrangement
