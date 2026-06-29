// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/tempo_map.h"
#include "dsp/tempo_map_qml_adapter.h"
#include "structure/arrangement/arranger_object_all.h"
#include "utils/object_registry.h"

#include <gtest/gtest.h>

namespace zrythm::structure::arrangement
{
class ChordClipTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    tempo_map = std::make_unique<dsp::TempoMap> (units::sample_rate (44100.0));
    tempo_map_wrapper = std::make_unique<dsp::TempoMapWrapper> (*tempo_map);
    clip = std::make_unique<ChordClip> (*tempo_map_wrapper, registry, nullptr);

    // Set up clip properties
    clip->position ()->setTicks (100);
    clip->length ()->setTicks (200);
  }

  auto create_chord_object (
    dsp::MusicalNote root,
    dsp::ChordType   type,
    dsp::ChordAccent accent = dsp::ChordAccent::None,
    int              inversion = 0)
  {
    auto chord_ref = utils::create_object<ChordObject> (
      registry, *tempo_map_wrapper, clip.get ());
    auto * descr = chord_ref.get_object_as<ChordObject> ()->chordDescriptor ();
    descr->setRootNote (root);
    descr->setChordType (type);
    descr->setChordAccent (accent);
    descr->setInversion (inversion);
    return chord_ref;
  }

  void add_chord_object (
    dsp::MusicalNote root,
    dsp::ChordType   type,
    double           position_ticks)
  {
    auto chord_ref = create_chord_object (root, type);
    chord_ref.get_object_as<ChordObject> ()->position ()->setTicks (
      position_ticks);
    clip->add_object (chord_ref);
  }

  std::unique_ptr<dsp::TempoMap>        tempo_map;
  std::unique_ptr<dsp::TempoMapWrapper> tempo_map_wrapper;
  utils::ObjectRegistry                 registry;
  std::unique_ptr<ChordClip>            clip;
};

TEST_F (ChordClipTest, InitialState)
{
  EXPECT_EQ (clip->type (), ArrangerObject::Type::ChordClip);
  EXPECT_EQ (clip->position ()->ticks (), 100);
  EXPECT_NE (clip->length (), nullptr);
  EXPECT_NE (clip->loopStartPosition (), nullptr);
  EXPECT_NE (clip->name (), nullptr);
  EXPECT_NE (clip->color (), nullptr);
  EXPECT_NE (clip->mute (), nullptr);
  EXPECT_EQ (clip->get_children_vector ().size (), 0);
}

TEST_F (ChordClipTest, AddChordObject)
{
  // Add a C Major chord object
  add_chord_object (dsp::MusicalNote::C, dsp::ChordType::Major, 100);

  EXPECT_EQ (clip->get_children_vector ().size (), 1);
  auto chord_obj = clip->get_children_view ()[0];
  EXPECT_EQ (chord_obj->chordDescriptor ()->rootNote (), dsp::MusicalNote::C);
  EXPECT_EQ (chord_obj->chordDescriptor ()->chordType (), dsp::ChordType::Major);
  EXPECT_EQ (chord_obj->position ()->ticks (), 100);
}

TEST_F (ChordClipTest, RemoveChordObject)
{
  auto chord_ref =
    create_chord_object (dsp::MusicalNote::D, dsp::ChordType::Minor);
  clip->add_object (chord_ref);

  auto removed_ref = clip->remove_object (chord_ref.id ());

  EXPECT_EQ (clip->get_children_vector ().size (), 0);
  EXPECT_EQ (removed_ref.id (), chord_ref.id ());
}

TEST_F (ChordClipTest, ChordPropertyChange)
{
  // Add a chord object
  add_chord_object (dsp::MusicalNote::C, dsp::ChordType::Major, 100);

  // Change chord properties
  auto * descr = clip->get_children_view ()[0]->chordDescriptor ();
  descr->setRootNote (dsp::MusicalNote::A);
  descr->setChordType (dsp::ChordType::Minor);
  descr->setChordAccent (dsp::ChordAccent::Seventh);

  EXPECT_EQ (descr->rootNote (), dsp::MusicalNote::A);
  EXPECT_EQ (descr->chordType (), dsp::ChordType::Minor);
  EXPECT_EQ (descr->chordAccent (), dsp::ChordAccent::Seventh);
}

TEST_F (ChordClipTest, MuteFunctionality)
{
  add_chord_object (dsp::MusicalNote::F, dsp::ChordType::Major, 100);

  clip->get_children_view ()[0]->mute ()->setMuted (true);

  EXPECT_TRUE (clip->get_children_view ()[0]->mute ()->muted ());
}

TEST_F (ChordClipTest, Serialization)
{
  // Add a D Minor chord
  add_chord_object (dsp::MusicalNote::D, dsp::ChordType::Minor, 150);
  const auto chord_id = clip->get_children_view ()[0]->get_uuid ();

  nlohmann::json j;
  to_json (j, *clip);

  auto new_clip =
    std::make_unique<ChordClip> (*tempo_map_wrapper, registry, nullptr);
  from_json (j, *new_clip);

  EXPECT_EQ (new_clip->get_children_vector ().size (), 1);
  auto chord_obj = new_clip->get_children_view ()[0];
  EXPECT_EQ (chord_obj->chordDescriptor ()->rootNote (), dsp::MusicalNote::D);
  EXPECT_EQ (chord_obj->chordDescriptor ()->chordType (), dsp::ChordType::Minor);
  EXPECT_EQ (chord_obj->position ()->ticks (), 150);
  EXPECT_EQ (chord_obj->get_uuid (), chord_id);
}

TEST_F (ChordClipTest, ObjectCloning)
{
  add_chord_object (dsp::MusicalNote::E, dsp::ChordType::Minor, 200);

  auto cloned_clip =
    std::make_unique<ChordClip> (*tempo_map_wrapper, registry, nullptr);
  init_from (*cloned_clip, *clip, utils::ObjectCloneType::NewIdentity);

  EXPECT_EQ (cloned_clip->get_children_vector ().size (), 1);
  auto cloned_chord = cloned_clip->get_children_view ()[0];
  EXPECT_NE (
    cloned_chord->get_uuid (), clip->get_children_view ()[0]->get_uuid ());
  EXPECT_EQ (cloned_chord->chordDescriptor ()->rootNote (), dsp::MusicalNote::E);
  EXPECT_EQ (
    cloned_chord->chordDescriptor ()->chordType (), dsp::ChordType::Minor);
  EXPECT_EQ (cloned_chord->position ()->ticks (), 200);
}

} // namespace zrythm::structure::arrangement
