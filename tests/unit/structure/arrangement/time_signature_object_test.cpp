// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <memory>

#include "dsp/tempo_map.h"
#include "structure/arrangement/time_signature_object.h"

#include <QSignalSpy>

#include "helpers/mock_qobject.h"

#include <gtest/gtest.h>

namespace zrythm::structure::arrangement
{
class TimeSignatureObjectTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    tempo_map = std::make_unique<dsp::TempoMap> (units::sample_rate (44100.0));
    parent = std::make_unique<MockQObject> ();
    time_sig_obj =
      std::make_unique<TimeSignatureObject> (*tempo_map, parent.get ());
  }

  std::unique_ptr<dsp::TempoMap>       tempo_map;
  std::unique_ptr<MockQObject>         parent;
  std::unique_ptr<TimeSignatureObject> time_sig_obj;
};

// Test initial state
TEST_F (TimeSignatureObjectTest, InitialState)
{
  EXPECT_EQ (time_sig_obj->type (), ArrangerObject::Type::TimeSignatureObject);
  EXPECT_EQ (time_sig_obj->position ()->samples (), 0);
  EXPECT_EQ (time_sig_obj->numerator (), TimeSignatureObject::DEFAULT_NUMERATOR);
  EXPECT_EQ (
    time_sig_obj->denominator (), TimeSignatureObject::DEFAULT_DENOMINATOR);
}

// Test numerator property
TEST_F (TimeSignatureObjectTest, NumeratorProperty)
{
  // Set numerator
  time_sig_obj->setNumerator (6);
  EXPECT_EQ (time_sig_obj->numerator (), 6);

  // Set same numerator (should be no-op)
  QSignalSpy spy (time_sig_obj.get (), &TimeSignatureObject::numeratorChanged);
  time_sig_obj->setNumerator (6);
  EXPECT_EQ (spy.count (), 0);
}

// Test numeratorChanged signal
TEST_F (TimeSignatureObjectTest, NumeratorChangedSignal)
{
  // Setup signal spy
  QSignalSpy spy (time_sig_obj.get (), &TimeSignatureObject::numeratorChanged);

  // First set
  time_sig_obj->setNumerator (3);
  EXPECT_EQ (spy.count (), 1);
  QList<QVariant> arguments = spy.takeFirst ();
  EXPECT_EQ (arguments.at (0).toInt (), 3);

  // Change numerator
  time_sig_obj->setNumerator (7);
  EXPECT_EQ (spy.count (), 1);
  arguments = spy.takeFirst ();
  EXPECT_EQ (arguments.at (0).toInt (), 7);
}

// Test denominator property
TEST_F (TimeSignatureObjectTest, DenominatorProperty)
{
  // Set denominator
  time_sig_obj->setDenominator (8);
  EXPECT_EQ (time_sig_obj->denominator (), 8);

  // Set same denominator (should be no-op)
  QSignalSpy spy (time_sig_obj.get (), &TimeSignatureObject::denominatorChanged);
  time_sig_obj->setDenominator (8);
  EXPECT_EQ (spy.count (), 0);
}

// Test denominatorChanged signal
TEST_F (TimeSignatureObjectTest, DenominatorChangedSignal)
{
  // Setup signal spy
  QSignalSpy spy (time_sig_obj.get (), &TimeSignatureObject::denominatorChanged);

  // First set
  time_sig_obj->setDenominator (8);
  EXPECT_EQ (spy.count (), 1);
  QList<QVariant> arguments = spy.takeFirst ();
  EXPECT_EQ (arguments.at (0).toInt (), 8);

  // Change denominator
  time_sig_obj->setDenominator (16);
  EXPECT_EQ (spy.count (), 1);
  arguments = spy.takeFirst ();
  EXPECT_EQ (arguments.at (0).toInt (), 16);
}

// Test both properties changing
TEST_F (TimeSignatureObjectTest, BothPropertiesChanging)
{
  // Setup signal spies
  QSignalSpy numeratorSpy (
    time_sig_obj.get (), &TimeSignatureObject::numeratorChanged);
  QSignalSpy denominatorSpy (
    time_sig_obj.get (), &TimeSignatureObject::denominatorChanged);

  // Change both properties
  time_sig_obj->setNumerator (5);
  time_sig_obj->setDenominator (8);

  EXPECT_EQ (numeratorSpy.count (), 1);
  EXPECT_EQ (denominatorSpy.count (), 1);

  // Verify final values
  EXPECT_EQ (time_sig_obj->numerator (), 5);
  EXPECT_EQ (time_sig_obj->denominator (), 8);
}

// Test serialization/deserialization
TEST_F (TimeSignatureObjectTest, Serialization)
{
  // Set initial state
  time_sig_obj->position ()->setSamples (1000);
  time_sig_obj->setNumerator (7);
  time_sig_obj->setDenominator (8);

  // Serialize
  nlohmann::json j;
  to_json (j, *time_sig_obj);

  // Create new object
  auto new_time_sig_obj =
    std::make_unique<TimeSignatureObject> (*tempo_map, parent.get ());
  from_json (j, *new_time_sig_obj);

  // Verify
  EXPECT_EQ (new_time_sig_obj->position ()->samples (), 1000);
  EXPECT_EQ (new_time_sig_obj->numerator (), 7);
  EXPECT_EQ (new_time_sig_obj->denominator (), 8);
}

// Test copying with init_from
TEST_F (TimeSignatureObjectTest, Copying)
{
  // Set initial state
  time_sig_obj->position ()->setSamples (2000);
  time_sig_obj->setNumerator (9);
  time_sig_obj->setDenominator (16);

  // Create target
  auto target =
    std::make_unique<TimeSignatureObject> (*tempo_map, parent.get ());

  // Copy
  init_from (*target, *time_sig_obj, utils::ObjectCloneType::Snapshot);

  // Verify
  EXPECT_EQ (target->position ()->samples (), 2000);
  EXPECT_EQ (target->numerator (), 9);
  EXPECT_EQ (target->denominator (), 16);
}

// Test edge cases
TEST_F (TimeSignatureObjectTest, EdgeCases)
{
  // Zero numerator
  time_sig_obj->setNumerator (0);
  EXPECT_EQ (time_sig_obj->numerator (), 0);

  // Negative numerator
  time_sig_obj->setNumerator (-4);
  EXPECT_EQ (time_sig_obj->numerator (), -4);

  // Zero denominator
  time_sig_obj->setDenominator (0);
  EXPECT_EQ (time_sig_obj->denominator (), 0);

  // Negative denominator
  time_sig_obj->setDenominator (-8);
  EXPECT_EQ (time_sig_obj->denominator (), -8);

  // Large values
  time_sig_obj->setNumerator (1000);
  time_sig_obj->setDenominator (2000);
  EXPECT_EQ (time_sig_obj->numerator (), 1000);
  EXPECT_EQ (time_sig_obj->denominator (), 2000);

  // Position at max value
  time_sig_obj->position ()->setSamples (std::numeric_limits<int>::max ());
  EXPECT_EQ (
    time_sig_obj->position ()->samples (), std::numeric_limits<int>::max ());
}

// Test common time signatures
TEST_F (TimeSignatureObjectTest, CommonTimeSignatures)
{
  // 2/4
  time_sig_obj->setNumerator (2);
  time_sig_obj->setDenominator (4);
  EXPECT_EQ (time_sig_obj->numerator (), 2);
  EXPECT_EQ (time_sig_obj->denominator (), 4);

  // 3/4
  time_sig_obj->setNumerator (3);
  time_sig_obj->setDenominator (4);
  EXPECT_EQ (time_sig_obj->numerator (), 3);
  EXPECT_EQ (time_sig_obj->denominator (), 4);

  // 4/4
  time_sig_obj->setNumerator (4);
  time_sig_obj->setDenominator (4);
  EXPECT_EQ (time_sig_obj->numerator (), 4);
  EXPECT_EQ (time_sig_obj->denominator (), 4);

  // 6/8
  time_sig_obj->setNumerator (6);
  time_sig_obj->setDenominator (8);
  EXPECT_EQ (time_sig_obj->numerator (), 6);
  EXPECT_EQ (time_sig_obj->denominator (), 8);

  // 12/8
  time_sig_obj->setNumerator (12);
  time_sig_obj->setDenominator (8);
  EXPECT_EQ (time_sig_obj->numerator (), 12);
  EXPECT_EQ (time_sig_obj->denominator (), 8);

  // 5/4 (odd meter)
  time_sig_obj->setNumerator (5);
  time_sig_obj->setDenominator (4);
  EXPECT_EQ (time_sig_obj->numerator (), 5);
  EXPECT_EQ (time_sig_obj->denominator (), 4);
}

// Test power of 2 denominators (common in music)
TEST_F (TimeSignatureObjectTest, PowerOfTwoDenominators)
{
  const int common_denominators[] = { 1, 2, 4, 8, 16, 32, 64 };

  for (int denominator : common_denominators)
    {
      time_sig_obj->setDenominator (denominator);
      EXPECT_EQ (time_sig_obj->denominator (), denominator);
    }
}

} // namespace zrythm::structure::arrangement
