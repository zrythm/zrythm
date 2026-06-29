// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/chord_descriptor.h"
#include "dsp/tempo_map.h"
#include "gui/qquick/chord_row_list_model.h"
#include "structure/arrangement/arranger_object_factory.h"
#include "structure/arrangement/chord_clip.h"
#include "utils/app_settings.h"
#include "utils/object_registry.h"

#include <QSignalSpy>

#include "helpers/in_memory_settings_backend.h"
#include "helpers/scoped_qcoreapplication.h"

#include <gtest/gtest.h>

namespace zrythm::gui::qquick
{

class ChordRowListModelTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    app_ = std::make_unique<test_helpers::ScopedQCoreApplication> ();

    registry_ = std::make_unique<utils::ObjectRegistry> ();
    tempo_map_ = std::make_unique<dsp::TempoMap> (units::sample_rate (44100));
    tempo_map_wrapper_ = std::make_unique<dsp::TempoMapWrapper> (*tempo_map_);

    sample_rate_provider_ = [] () { return units::sample_rate (44100); };
    bpm_provider_ = [] () { return units::bpm (120.0); };

    app_settings_ = std::make_unique<utils::AppSettings> (
      std::make_unique<test_helpers::InMemorySettingsBackend> ());

    factory_ = std::make_unique<structure::arrangement::ArrangerObjectFactory> (
      structure::arrangement::ArrangerObjectFactory::Dependencies{
        .tempo_map_ = *tempo_map_wrapper_,
        .registry_ = *registry_,
        .last_timeline_obj_len_provider_ = [] () { return 100.0; },
        .last_editor_obj_len_provider_ = [] () { return 50.0; },
        .automation_curve_algorithm_provider_ =
          [] () { return dsp::CurveOptions::Algorithm::Exponent; },
      },
      sample_rate_provider_, bpm_provider_);

    model_ = std::make_unique<ChordRowListModel> ();
  }

  // Helper: build a chord clip spanning [0, clipEndTicks].
  structure::arrangement::ChordClip * make_clip (double clipEndTicks)
  {
    auto clip_ref =
      factory_->get_builder<structure::arrangement::ChordClip> ()
        .with_start_ticks (units::ticks (0))
        .with_end_ticks (units::ticks (clipEndTicks))
        .build_in_registry ();
    auto * clip = clip_ref.get_object_as<structure::arrangement::ChordClip> ();
    clips_.push_back (std::move (clip_ref));
    return clip;
  }

  // Helper: add a chord object to a clip at the given position.
  structure::arrangement::ChordObject * add_chord (
    structure::arrangement::ChordClip &clip,
    double                             ticks,
    dsp::MusicalNote                   root,
    dsp::ChordType                     type)
  {
    auto chord_ref =
      factory_->get_builder<structure::arrangement::ChordObject> ()
        .with_start_ticks (units::ticks (ticks))
        .with_chord_descriptor (root, type)
        .build_in_registry ();
    auto * obj = chord_ref.get_object_as<structure::arrangement::ChordObject> ();
    clip.add_object (chord_ref);
    chord_refs_.push_back (std::move (chord_ref));
    return obj;
  }

  std::unique_ptr<test_helpers::ScopedQCoreApplication> app_;
  std::unique_ptr<utils::ObjectRegistry>                registry_;
  std::unique_ptr<dsp::TempoMap>                        tempo_map_;
  std::unique_ptr<dsp::TempoMapWrapper>                 tempo_map_wrapper_;
  structure::arrangement::ArrangerObjectFactory::SampleRateProvider
    sample_rate_provider_;
  structure::arrangement::ArrangerObjectFactory::BpmProvider     bpm_provider_;
  std::unique_ptr<utils::AppSettings>                            app_settings_;
  std::unique_ptr<structure::arrangement::ArrangerObjectFactory> factory_;
  std::unique_ptr<ChordRowListModel>                             model_;

  std::vector<structure::arrangement::ArrangerObjectUuidReference> clips_;
  std::vector<structure::arrangement::ArrangerObjectUuidReference> chord_refs_;
};

TEST_F (ChordRowListModelTest, EmptyClipHasNoRows)
{
  auto * clip = make_clip (1920.0);
  model_->setChordClip (clip);
  EXPECT_EQ (model_->rowCount (), 0);
}

TEST_F (ChordRowListModelTest, GroupsEquivalentChords)
{
  auto * clip = make_clip (3840.0);
  add_chord (*clip, 0.0, dsp::MusicalNote::C, dsp::ChordType::Major);
  add_chord (*clip, 960.0, dsp::MusicalNote::C, dsp::ChordType::Major);
  add_chord (*clip, 1920.0, dsp::MusicalNote::D, dsp::ChordType::Minor);

  model_->setChordClip (clip);

  // 2 unique chords: C Major and D Minor.
  EXPECT_EQ (model_->rowCount (), 2);

  // Rows sorted alphabetically: "CMaj" before "Dmin".
  const auto row0 =
    model_->data (model_->index (0, 0), ChordRowListModel::ChordNameRole);
  const auto row1 =
    model_->data (model_->index (1, 0), ChordRowListModel::ChordNameRole);
  EXPECT_EQ (row0.toString ().toStdString (), "CMaj");
  EXPECT_EQ (row1.toString ().toStdString (), "Dmin");

  // The C Major row has 2 chord objects; D Minor has 1.
  EXPECT_EQ (
    model_->data (model_->index (0, 0), ChordRowListModel::ChordObjectCountRole)
      .toInt (),
    2);
  EXPECT_EQ (
    model_->data (model_->index (1, 0), ChordRowListModel::ChordObjectCountRole)
      .toInt (),
    1);
}

TEST_F (ChordRowListModelTest, RowForChordObject)
{
  auto * clip = make_clip (1920.0);
  auto * chord_c =
    add_chord (*clip, 0.0, dsp::MusicalNote::C, dsp::ChordType::Major);
  auto * chord_d =
    add_chord (*clip, 480.0, dsp::MusicalNote::D, dsp::ChordType::Minor);

  model_->setChordClip (clip);

  // "C" sorts before "Dmin", so C is row 0, D is row 1.
  EXPECT_EQ (model_->rowForChordObject (chord_c), 0);
  EXPECT_EQ (model_->rowForChordObject (chord_d), 1);
  EXPECT_EQ (model_->rowForChordObject (nullptr), -1);
}

TEST_F (ChordRowListModelTest, DescriptorAtRow)
{
  auto * clip = make_clip (1920.0);
  add_chord (*clip, 0.0, dsp::MusicalNote::C, dsp::ChordType::Major);
  add_chord (*clip, 480.0, dsp::MusicalNote::D, dsp::ChordType::Minor);

  model_->setChordClip (clip);

  auto * desc0 = model_->descriptorAtRow (0);
  ASSERT_NE (desc0, nullptr);
  EXPECT_EQ (desc0->rootNote (), dsp::MusicalNote::C);
  EXPECT_EQ (desc0->chordType (), dsp::ChordType::Major);

  auto * desc1 = model_->descriptorAtRow (1);
  ASSERT_NE (desc1, nullptr);
  EXPECT_EQ (desc1->rootNote (), dsp::MusicalNote::D);
  EXPECT_EQ (desc1->chordType (), dsp::ChordType::Minor);

  EXPECT_EQ (model_->descriptorAtRow (99), nullptr);
  EXPECT_EQ (model_->descriptorAtRow (-1), nullptr);
}

TEST_F (ChordRowListModelTest, ChordObjectsAtRow)
{
  auto * clip = make_clip (1920.0);
  add_chord (*clip, 0.0, dsp::MusicalNote::C, dsp::ChordType::Major);
  add_chord (*clip, 480.0, dsp::MusicalNote::C, dsp::ChordType::Major);
  add_chord (*clip, 960.0, dsp::MusicalNote::D, dsp::ChordType::Minor);

  model_->setChordClip (clip);

  // C Major row (row 0) should contain 2 chord objects.
  const auto row0_objects = model_->chordObjectsAtRow (0);
  EXPECT_EQ (row0_objects.size (), 2);
  // D Minor row (row 1) should contain 1.
  const auto row1_objects = model_->chordObjectsAtRow (1);
  EXPECT_EQ (row1_objects.size (), 1);
  // Out of range returns empty.
  EXPECT_TRUE (model_->chordObjectsAtRow (99).isEmpty ());
}

TEST_F (ChordRowListModelTest, UpdatesWhenChordAdded)
{
  auto * clip = make_clip (1920.0);
  add_chord (*clip, 0.0, dsp::MusicalNote::C, dsp::ChordType::Major);

  model_->setChordClip (clip);
  EXPECT_EQ (model_->rowCount (), 1);

  // Adding a new chord type triggers a model reset via rowsInserted signal.
  add_chord (*clip, 480.0, dsp::MusicalNote::D, dsp::ChordType::Minor);
  EXPECT_EQ (model_->rowCount (), 2);
}

TEST_F (ChordRowListModelTest, ContentChangedEmittedOnStructuralChange)
{
  auto * clip = make_clip (1920.0);
  add_chord (*clip, 0.0, dsp::MusicalNote::C, dsp::ChordType::Major);

  model_->setChordClip (clip);

  QSignalSpy spy (model_.get (), &ChordRowListModel::contentChanged);
  add_chord (*clip, 480.0, dsp::MusicalNote::D, dsp::ChordType::Minor);
  // rowsInserted + the new object's initial contentChanged may each fire.
  EXPECT_GE (spy.count (), 1);

  const int prev_count = spy.count ();
  add_chord (*clip, 960.0, dsp::MusicalNote::C, dsp::ChordType::Major);
  EXPECT_GT (spy.count (), prev_count);
}

} // namespace zrythm::gui::qquick
