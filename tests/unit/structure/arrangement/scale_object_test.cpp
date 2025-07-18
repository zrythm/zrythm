// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <memory>

#include "dsp/musical_scale.h"
#include "dsp/tempo_map.h"
#include "structure/arrangement/scale_object.h"

#include "helpers/mock_qobject.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace zrythm::structure::arrangement
{
class ScaleObjectTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    tempo_map = std::make_unique<dsp::TempoMap> (44100.0);
    parent = std::make_unique<MockQObject> ();
    scale_object = std::make_unique<ScaleObject> (*tempo_map, parent.get ());
  }

  std::unique_ptr<dsp::TempoMap> tempo_map;
  std::unique_ptr<MockQObject>   parent;
  std::unique_ptr<ScaleObject>   scale_object;
};

// Test initial state
TEST_F (ScaleObjectTest, InitialState)
{
  EXPECT_EQ (scale_object->type (), ArrangerObject::Type::ScaleObject);
  EXPECT_NE (scale_object->scale (), nullptr);
  EXPECT_NE (scale_object->mute (), nullptr);
  EXPECT_EQ (scale_object->scale ()->rootKey (), dsp::MusicalNote::A);
  EXPECT_EQ (
    scale_object->scale ()->scaleType (), dsp::MusicalScale::ScaleType::Aeolian);
}

// Test scale property
TEST_F (ScaleObjectTest, ScaleProperty)
{
  // Create new scale
  auto new_scale = new dsp::MusicalScale (
    dsp::MusicalScale::ScaleType::HarmonicMinor, dsp::MusicalNote::D);

  // Set scale
  testing::MockFunction<void (dsp::MusicalScale *)> mockScaleChanged;
  QObject::connect (
    scale_object.get (), &ScaleObject::scaleChanged, parent.get (),
    mockScaleChanged.AsStdFunction ());

  EXPECT_CALL (mockScaleChanged, Call (new_scale)).Times (1);
  scale_object->setScale (new_scale);

  // Verify scale was set
  EXPECT_EQ (scale_object->scale ()->rootKey (), dsp::MusicalNote::D);
  EXPECT_EQ (
    scale_object->scale ()->scaleType (),
    dsp::MusicalScale::ScaleType::HarmonicMinor);

  // Test no signal when same scale
  EXPECT_CALL (mockScaleChanged, Call (new_scale)).Times (0);
  scale_object->setScale (new_scale);
}

// Test mute functionality
TEST_F (ScaleObjectTest, MuteFunctionality)
{
  // Mute the scale object
  scale_object->mute ()->setMuted (true);
  EXPECT_TRUE (scale_object->mute ()->muted ());

  // Unmute the scale object
  scale_object->mute ()->setMuted (false);
  EXPECT_FALSE (scale_object->mute ()->muted ());
}

// Test serialization/deserialization
TEST_F (ScaleObjectTest, Serialization)
{
  // Set initial state
  scale_object->scale ()->setRootKey (dsp::MusicalNote::G);
  scale_object->scale ()->setScaleType (dsp::MusicalScale::ScaleType::Dorian);

  // Serialize
  nlohmann::json j;
  to_json (j, *scale_object);

  // Create new scale object
  auto new_scale_object =
    std::make_unique<ScaleObject> (*tempo_map, parent.get ());
  from_json (j, *new_scale_object);

  // Verify state
  EXPECT_EQ (new_scale_object->scale ()->rootKey (), dsp::MusicalNote::G);
  EXPECT_EQ (
    new_scale_object->scale ()->scaleType (),
    dsp::MusicalScale::ScaleType::Dorian);
}

// Test copying
TEST_F (ScaleObjectTest, Copying)
{
  // Set initial state
  scale_object->scale ()->setRootKey (dsp::MusicalNote::F);
  scale_object->scale ()->setScaleType (dsp::MusicalScale::ScaleType::Phrygian);

  // Create target
  auto target = std::make_unique<ScaleObject> (*tempo_map, parent.get ());

  // Copy
  init_from (*target, *scale_object, utils::ObjectCloneType::Snapshot);

  // Verify state
  EXPECT_EQ (target->scale ()->rootKey (), dsp::MusicalNote::F);
  EXPECT_EQ (
    target->scale ()->scaleType (), dsp::MusicalScale::ScaleType::Phrygian);
}

// Test edge cases
TEST_F (ScaleObjectTest, EdgeCases)
{
  // Test setting invalid scale
  auto * invalid_scale = new dsp::MusicalScale (
    static_cast<dsp::MusicalScale::ScaleType> (100),
    static_cast<dsp::MusicalNote> (100));

  scale_object->setScale (invalid_scale);

  // Should clamp to valid values
  EXPECT_NE (scale_object->scale ()->rootKey (), dsp::MusicalNote (100));
  EXPECT_NE (
    scale_object->scale ()->scaleType (), dsp::MusicalScale::ScaleType (100));

  // Test setting nullptr (should be ignored)
  auto previous_scale = scale_object->scale ();
  scale_object->setScale (nullptr);
  EXPECT_EQ (scale_object->scale (), previous_scale);
}

} // namespace zrythm::structure::arrangement
