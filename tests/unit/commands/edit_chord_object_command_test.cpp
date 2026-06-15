// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "commands/edit_chord_object_command.h"
#include "structure/arrangement/arranger_object_factory.h"
#include "utils/object_registry.h"

#include <gtest/gtest.h>

namespace zrythm::commands
{

class EditChordObjectCommandTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    tempo_map_ = std::make_unique<dsp::TempoMap> (units::sample_rate (44100.0));

    factory_ = std::make_unique<structure::arrangement::ArrangerObjectFactory> (
      structure::arrangement::ArrangerObjectFactory::Dependencies{
        .tempo_map_ = *tempo_map_,
        .registry_ = registry_,
        .musical_mode_getter_ = [] () { return true; },
        .last_timeline_obj_len_provider_ = [] () { return 100.0; },
        .last_editor_obj_len_provider_ = [] () { return 50.0; },
        .automation_curve_algorithm_provider_ =
          [] () { return dsp::CurveOptions::Algorithm::Exponent; },
      },
      [] () { return units::sample_rate (44100); }, [] () { return 120.0; });

    // Create a chord object with C Major.
    auto ref =
      factory_->get_builder<structure::arrangement::ChordObject> ()
        .with_start_ticks (units::ticks (0))
        .with_chord_descriptor (dsp::MusicalNote::C, dsp::ChordType::Major)
        .build_in_registry ();
    chord_obj_ = ref.get_object_as<structure::arrangement::ChordObject> ();
    chord_refs_.push_back (std::move (ref));
  }

  static void expect_descriptor (
    const dsp::ChordDescriptor &desc,
    dsp::MusicalNote            root,
    dsp::ChordType              type,
    dsp::ChordAccent            accent = dsp::ChordAccent::None,
    int                         inversion = 0)
  {
    EXPECT_EQ (desc.rootNote (), root);
    EXPECT_EQ (desc.chordType (), type);
    EXPECT_EQ (desc.chordAccent (), accent);
    EXPECT_EQ (desc.inversion (), inversion);
  }

  std::unique_ptr<dsp::TempoMap>                                 tempo_map_;
  utils::ObjectRegistry                                          registry_;
  std::unique_ptr<structure::arrangement::ArrangerObjectFactory> factory_;
  structure::arrangement::ChordObject * chord_obj_ = nullptr;
  std::vector<structure::arrangement::ArrangerObjectUuidReference> chord_refs_;
};

TEST_F (EditChordObjectCommandTest, RedoChangesDescriptor)
{
  dsp::ChordDescriptor target (
    dsp::MusicalNote::D, dsp::ChordType::Minor, dsp::ChordAccent::Seventh, 1);
  EditChordObjectCommand cmd (chord_obj_, target);

  cmd.redo ();
  expect_descriptor (
    *chord_obj_->chordDescriptor (), dsp::MusicalNote::D, dsp::ChordType::Minor,
    dsp::ChordAccent::Seventh, 1);
}

TEST_F (EditChordObjectCommandTest, UndoRestoresDescriptor)
{
  dsp::ChordDescriptor target (dsp::MusicalNote::E, dsp::ChordType::Diminished);
  EditChordObjectCommand cmd (chord_obj_, target);

  cmd.redo ();
  expect_descriptor (
    *chord_obj_->chordDescriptor (), dsp::MusicalNote::E,
    dsp::ChordType::Diminished);

  cmd.undo ();
  expect_descriptor (
    *chord_obj_->chordDescriptor (), dsp::MusicalNote::C, dsp::ChordType::Major);
}

TEST_F (EditChordObjectCommandTest, HandlesBassNote)
{
  // Original: C Major, no bass.
  // Target: A Minor with bass note F.
  dsp::ChordDescriptor target (
    dsp::MusicalNote::A, dsp::ChordType::Minor, dsp::ChordAccent::None, 0,
    dsp::MusicalNote::F);
  EditChordObjectCommand cmd (chord_obj_, target);

  cmd.redo ();
  EXPECT_EQ (chord_obj_->chordDescriptor ()->rootNote (), dsp::MusicalNote::A);
  EXPECT_TRUE (chord_obj_->chordDescriptor ()->hasBass ());
  EXPECT_EQ (chord_obj_->chordDescriptor ()->bassNote (), dsp::MusicalNote::F);

  cmd.undo ();
  EXPECT_FALSE (chord_obj_->chordDescriptor ()->hasBass ());
}

TEST_F (EditChordObjectCommandTest, MergeWithSameObject)
{
  dsp::ChordDescriptor target1 (dsp::MusicalNote::D, dsp::ChordType::Minor);
  dsp::ChordDescriptor target2 (dsp::MusicalNote::E, dsp::ChordType::Diminished);

  EditChordObjectCommand cmd1 (chord_obj_, target1);
  cmd1.redo ();

  EditChordObjectCommand cmd2 (chord_obj_, target2);

  // Should merge: cmd1 takes cmd2's "after" state.
  EXPECT_TRUE (cmd1.mergeWith (&cmd2));

  // Undo the merged command: should restore the ORIGINAL (C Major).
  cmd1.undo ();
  expect_descriptor (
    *chord_obj_->chordDescriptor (), dsp::MusicalNote::C, dsp::ChordType::Major);
}

TEST_F (EditChordObjectCommandTest, DoesNotMergeDifferentObjects)
{
  auto ref2 =
    factory_->get_builder<structure::arrangement::ChordObject> ()
      .with_start_ticks (units::ticks (960))
      .with_chord_descriptor (dsp::MusicalNote::G, dsp::ChordType::Major)
      .build_in_registry ();
  auto * chord_obj2 = ref2.get_object_as<structure::arrangement::ChordObject> ();
  chord_refs_.push_back (std::move (ref2));

  dsp::ChordDescriptor target1 (dsp::MusicalNote::D, dsp::ChordType::Minor);
  dsp::ChordDescriptor target2 (dsp::MusicalNote::E, dsp::ChordType::Minor);

  EditChordObjectCommand cmd1 (chord_obj_, target1);
  EditChordObjectCommand cmd2 (chord_obj2, target2);

  EXPECT_FALSE (cmd1.mergeWith (&cmd2));
}

} // namespace zrythm::commands
