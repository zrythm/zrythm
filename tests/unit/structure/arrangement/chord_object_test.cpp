// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <memory>

#include "dsp/tempo_map.h"
#include "structure/arrangement/chord_object.h"

#include <QSignalSpy>

#include "helpers/mock_qobject.h"

#include <gtest/gtest.h>

namespace zrythm::structure::arrangement
{
class ChordObjectTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    tempo_map = std::make_unique<dsp::TempoMap> (units::sample_rate (44100.0));
    parent = std::make_unique<MockQObject> ();
    chord_obj = std::make_unique<ChordObject> (*tempo_map, parent.get ());
  }

  std::unique_ptr<dsp::TempoMap> tempo_map;
  std::unique_ptr<MockQObject>   parent;
  std::unique_ptr<ChordObject>   chord_obj;
};

// Test initial state
TEST_F (ChordObjectTest, InitialState)
{
  EXPECT_EQ (chord_obj->type (), ArrangerObject::Type::ChordObject);
  EXPECT_EQ (chord_obj->position ()->samples (), 0);
  EXPECT_EQ (chord_obj->chordDescriptorIndex (), 0);
  EXPECT_FALSE (chord_obj->mute ()->muted ());
}

// Test chordDescriptorIndex property
TEST_F (ChordObjectTest, ChordDescriptorIndex)
{
  // Set index
  chord_obj->setChordDescriptorIndex (3);
  EXPECT_EQ (chord_obj->chordDescriptorIndex (), 3);

  // Set same index (should be no-op)
  QSignalSpy spy (chord_obj.get (), &ChordObject::chordDescriptorIndexChanged);
  chord_obj->setChordDescriptorIndex (3);
  EXPECT_EQ (spy.count (), 0);
}

// Test chordDescriptorIndexChanged signal
TEST_F (ChordObjectTest, ChordDescriptorIndexChangedSignal)
{
  // Setup signal spy
  QSignalSpy spy (chord_obj.get (), &ChordObject::chordDescriptorIndexChanged);

  // First set
  chord_obj->setChordDescriptorIndex (2);
  EXPECT_EQ (spy.count (), 1);
  QList<QVariant> arguments = spy.takeFirst ();
  EXPECT_EQ (arguments.at (0).toInt (), 2);

  // Change index
  chord_obj->setChordDescriptorIndex (4);
  EXPECT_EQ (spy.count (), 1);
  arguments = spy.takeFirst ();
  EXPECT_EQ (arguments.at (0).toInt (), 4);
}

// Test mute functionality
TEST_F (ChordObjectTest, MuteFunctionality)
{
  // Mute the object
  chord_obj->mute ()->setMuted (true);
  EXPECT_TRUE (chord_obj->mute ()->muted ());

  // Unmute the object
  chord_obj->mute ()->setMuted (false);
  EXPECT_FALSE (chord_obj->mute ()->muted ());
}

// Test serialization/deserialization
TEST_F (ChordObjectTest, Serialization)
{
  // Set initial state
  chord_obj->position ()->setSamples (1000);
  chord_obj->setChordDescriptorIndex (5);
  chord_obj->mute ()->setMuted (true);

  // Serialize
  nlohmann::json j;
  to_json (j, *chord_obj);

  // Create new object
  auto new_chord_obj = std::make_unique<ChordObject> (*tempo_map, parent.get ());
  from_json (j, *new_chord_obj);

  // Verify
  EXPECT_EQ (new_chord_obj->position ()->samples (), 1000);
  EXPECT_EQ (new_chord_obj->chordDescriptorIndex (), 5);
  EXPECT_TRUE (new_chord_obj->mute ()->muted ());
}

// Test copying with init_from
TEST_F (ChordObjectTest, Copying)
{
  // Set initial state
  chord_obj->position ()->setSamples (2000);
  chord_obj->setChordDescriptorIndex (7);
  chord_obj->mute ()->setMuted (false);

  // Create target
  auto target = std::make_unique<ChordObject> (*tempo_map, parent.get ());

  // Copy
  init_from (*target, *chord_obj, utils::ObjectCloneType::Snapshot);

  // Verify
  EXPECT_EQ (target->position ()->samples (), 2000);
  EXPECT_EQ (target->chordDescriptorIndex (), 7);
  EXPECT_FALSE (target->mute ()->muted ());
}

// Test edge cases
TEST_F (ChordObjectTest, EdgeCases)
{
  // Negative index
  chord_obj->setChordDescriptorIndex (-1);
  EXPECT_EQ (chord_obj->chordDescriptorIndex (), -1);

  // Large index
  chord_obj->setChordDescriptorIndex (10000);
  EXPECT_EQ (chord_obj->chordDescriptorIndex (), 10000);

  // Position at max value
  chord_obj->position ()->setSamples (std::numeric_limits<int>::max ());
  EXPECT_EQ (
    chord_obj->position ()->samples (), std::numeric_limits<int>::max ());
}

} // namespace zrythm::structure::arrangement
