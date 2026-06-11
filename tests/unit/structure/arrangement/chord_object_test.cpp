// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <memory>

#include "dsp/tempo_map.h"
#include "structure/arrangement/chord_object.h"

#include "helpers/mock_qobject.h"

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

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
  EXPECT_NE (chord_obj->chordDescriptor (), nullptr);
  // Default ChordDescriptor is C / None
  EXPECT_EQ (chord_obj->chordDescriptor ()->rootNote (), dsp::MusicalNote::C);
  EXPECT_EQ (chord_obj->chordDescriptor ()->chordType (), dsp::ChordType::None);
  EXPECT_FALSE (chord_obj->mute ()->muted ());
}

// Test chordDescriptor properties
TEST_F (ChordObjectTest, ChordDescriptorProperties)
{
  auto * descr = chord_obj->chordDescriptor ();
  descr->setRootNote (dsp::MusicalNote::A);
  descr->setChordType (dsp::ChordType::Minor);
  EXPECT_EQ (descr->rootNote (), dsp::MusicalNote::A);
  EXPECT_EQ (descr->chordType (), dsp::ChordType::Minor);
}

// Test mute functionality
TEST_F (ChordObjectTest, MuteFunctionality)
{
  chord_obj->mute ()->setMuted (true);
  EXPECT_TRUE (chord_obj->mute ()->muted ());

  chord_obj->mute ()->setMuted (false);
  EXPECT_FALSE (chord_obj->mute ()->muted ());
}

// Test serialization/deserialization
TEST_F (ChordObjectTest, Serialization)
{
  chord_obj->position ()->setSamples (1000);
  chord_obj->chordDescriptor ()->setRootNote (dsp::MusicalNote::D);
  chord_obj->chordDescriptor ()->setChordType (dsp::ChordType::Major);
  chord_obj->chordDescriptor ()->setChordAccent (dsp::ChordAccent::Seventh);
  chord_obj->mute ()->setMuted (true);

  nlohmann::json j;
  to_json (j, *chord_obj);

  auto new_chord_obj = std::make_unique<ChordObject> (*tempo_map, parent.get ());
  from_json (j, *new_chord_obj);

  EXPECT_EQ (new_chord_obj->position ()->samples (), 1000);
  EXPECT_EQ (
    new_chord_obj->chordDescriptor ()->rootNote (), dsp::MusicalNote::D);
  EXPECT_EQ (
    new_chord_obj->chordDescriptor ()->chordType (), dsp::ChordType::Major);
  EXPECT_EQ (
    new_chord_obj->chordDescriptor ()->chordAccent (),
    dsp::ChordAccent::Seventh);
  EXPECT_TRUE (new_chord_obj->mute ()->muted ());
}

// Test copying with init_from
TEST_F (ChordObjectTest, Copying)
{
  chord_obj->position ()->setSamples (2000);
  chord_obj->chordDescriptor ()->setRootNote (dsp::MusicalNote::E);
  chord_obj->chordDescriptor ()->setChordType (dsp::ChordType::Minor);
  chord_obj->chordDescriptor ()->setInversion (2);
  chord_obj->mute ()->setMuted (false);

  auto target = std::make_unique<ChordObject> (*tempo_map, parent.get ());

  init_from (*target, *chord_obj, utils::ObjectCloneType::Snapshot);

  EXPECT_EQ (target->position ()->samples (), 2000);
  EXPECT_EQ (target->chordDescriptor ()->rootNote (), dsp::MusicalNote::E);
  EXPECT_EQ (target->chordDescriptor ()->chordType (), dsp::ChordType::Minor);
  EXPECT_EQ (target->chordDescriptor ()->inversion (), 2);
  EXPECT_FALSE (target->mute ()->muted ());
}

// Test edge cases
TEST_F (ChordObjectTest, EdgeCases)
{
  // Position at max value
  chord_obj->position ()->setSamples (std::numeric_limits<int>::max ());
  EXPECT_EQ (
    chord_obj->position ()->samples (), std::numeric_limits<int>::max ());

  // Chord with bass note
  chord_obj->chordDescriptor ()->setBassNote (dsp::MusicalNote::G);
  EXPECT_TRUE (chord_obj->chordDescriptor ()->hasBass ());
  EXPECT_EQ (chord_obj->chordDescriptor ()->bassNote (), dsp::MusicalNote::G);
}

} // namespace zrythm::structure::arrangement
