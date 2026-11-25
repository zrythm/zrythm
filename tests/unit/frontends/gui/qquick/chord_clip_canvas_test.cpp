// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/chord_descriptor.h"
#include "dsp/tempo_map.h"
#include "dsp/tick_types.h"
#include "frontends/gui/qquick/chord_clip_canvas_renderer.h"
#include "structure/arrangement/arranger_object_factory.h"
#include "structure/arrangement/chord_clip.h"
#include "utils/app_settings.h"
#include "utils/object_registry.h"

#include "helpers/in_memory_settings_backend.h"
#include "helpers/scoped_qcoreapplication.h"

#include <gtest/gtest.h>

namespace zrythm::frontends::gui::qquick
{

class ChordClipCanvasTest : public ::testing::Test
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
      std::make_unique<test_helpers::InMemorySettingsBackend> (),
      utils::UserDirectories{});
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
  }

  /// Build a chord clip spanning [0, clipEndTicks].
  structure::arrangement::ChordClip * make_clip (double clip_end_ticks)
  {
    auto clip_ref =
      factory_->get_builder<structure::arrangement::ChordClip> ()
        .with_start_ticks (units::ticks (0))
        .with_end_ticks (units::ticks (clip_end_ticks))
        .build_in_registry ();
    auto * clip = clip_ref.get_object_as<structure::arrangement::ChordClip> ();
    clips_.push_back (std::move (clip_ref));
    return clip;
  }

  /// Add a chord object at @p ticks with the given root note and type.
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

  /// Set a custom (non-trackBounds) loop range. All values in ticks.
  void set_loop_range (
    structure::arrangement::ChordClip &clip,
    double                             clip_start,
    double                             loop_start,
    double                             loop_end)
  {
    clip.setTrackBounds (false);
    clip.clipStartPosition ()->setTicks (clip_start);
    clip.loopStartPosition ()->setTicks (loop_start);
    clip.loopEndPosition ()->setTicks (loop_end);
  }

  /// Convenience: compute cells for the clip up to its own length.
  std::vector<ComputedChordCell>
  compute (structure::arrangement::ChordClip &clip)
  {
    return compute_chord_cells (clip, clip.length ()->asTick ());
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

  std::vector<structure::arrangement::ArrangerObjectUuidReference> clips_;
  std::vector<structure::arrangement::ArrangerObjectUuidReference> chord_refs_;
};

// ===========================================================================
// Basic cell count tests
// ===========================================================================

TEST_F (ChordClipCanvasTest, EmptyClipHasNoCells)
{
  auto * clip = make_clip (1920.0);
  auto   cells = compute (*clip);
  EXPECT_TRUE (cells.empty ());
}

TEST_F (ChordClipCanvasTest, NonLoopedOneCellPerChord)
{
  auto * clip = make_clip (1920.0);
  add_chord (*clip, 0.0, dsp::MusicalNote::C, dsp::ChordType::Major);
  add_chord (*clip, 480.0, dsp::MusicalNote::D, dsp::ChordType::Minor);
  add_chord (*clip, 960.0, dsp::MusicalNote::E, dsp::ChordType::Minor);

  auto cells = compute (*clip);
  EXPECT_EQ (cells.size (), 3);
}

// ===========================================================================
// Non-looped clip: cell position/width correctness
// ===========================================================================

TEST_F (ChordClipCanvasTest, NonLoopedCellSpansToNextChord)
{
  auto * clip = make_clip (1920.0);
  add_chord (*clip, 0.0, dsp::MusicalNote::C, dsp::ChordType::Major);
  add_chord (*clip, 960.0, dsp::MusicalNote::G, dsp::ChordType::Major);

  auto cells = compute (*clip);
  ASSERT_EQ (cells.size (), 2);

  EXPECT_DOUBLE_EQ (cells[0].abs_start.asDouble (), 0.0);
  EXPECT_DOUBLE_EQ (cells[0].abs_end.asDouble (), 960.0);
  EXPECT_DOUBLE_EQ (cells[1].abs_start.asDouble (), 960.0);
  EXPECT_DOUBLE_EQ (cells[1].abs_end.asDouble (), 1920.0);
}

TEST_F (ChordClipCanvasTest, NonLoopedSingleChordSpansFullClip)
{
  auto * clip = make_clip (1920.0);
  add_chord (*clip, 0.0, dsp::MusicalNote::C, dsp::ChordType::Major);

  auto cells = compute (*clip);
  ASSERT_EQ (cells.size (), 1);
  EXPECT_DOUBLE_EQ (cells[0].abs_start.asDouble (), 0.0);
  EXPECT_DOUBLE_EQ (cells[0].abs_end.asDouble (), 1920.0);
}

// ===========================================================================
// Loop expansion
// ===========================================================================

TEST_F (ChordClipCanvasTest, LoopDoublesCells)
{
  auto * clip = make_clip (3840.0);
  add_chord (*clip, 0.0, dsp::MusicalNote::C, dsp::ChordType::Major);
  add_chord (*clip, 960.0, dsp::MusicalNote::G, dsp::ChordType::Major);
  set_loop_range (*clip, 0.0, 0.0, 1920.0);

  auto cells = compute (*clip);
  EXPECT_EQ (cells.size (), 4);
}

TEST_F (ChordClipCanvasTest, LoopCellPositions)
{
  auto * clip = make_clip (3840.0);
  add_chord (*clip, 480.0, dsp::MusicalNote::C, dsp::ChordType::Major);
  set_loop_range (*clip, 0.0, 0.0, 1920.0);

  auto cells = compute (*clip);
  ASSERT_EQ (cells.size (), 2);

  // Iteration 0: [480, 1920)
  EXPECT_DOUBLE_EQ (cells[0].abs_start.asDouble (), 480.0);
  EXPECT_DOUBLE_EQ (cells[0].abs_end.asDouble (), 1920.0);
  // Iteration 1: [2400, 3840)
  EXPECT_DOUBLE_EQ (cells[1].abs_start.asDouble (), 2400.0);
  EXPECT_DOUBLE_EQ (cells[1].abs_end.asDouble (), 3840.0);
}

// ===========================================================================
// Clip start
// ===========================================================================

TEST_F (ChordClipCanvasTest, DeadContentBeforeLoopStartNeverVisible)
{
  auto * clip = make_clip (1920.0);
  add_chord (*clip, 0.0, dsp::MusicalNote::C, dsp::ChordType::Major);
  add_chord (*clip, 960.0, dsp::MusicalNote::G, dsp::ChordType::Major);
  set_loop_range (*clip, 480.0, 480.0, 1920.0);

  auto cells = compute (*clip);
  ASSERT_EQ (cells.size (), 1);
  // Chord at 960 maps to abs: 0 + (960 - 480) = 480
  EXPECT_DOUBLE_EQ (cells[0].abs_start.asDouble (), 480.0);
}

TEST_F (ChordClipCanvasTest, ClipStartStillLoopsBackToLoopStart)
{
  auto * clip = make_clip (3840.0);
  add_chord (*clip, 0.0, dsp::MusicalNote::C, dsp::ChordType::Major);
  set_loop_range (*clip, 480.0, 0.0, 1920.0);

  auto cells = compute (*clip);
  ASSERT_EQ (cells.size (), 2);
  EXPECT_DOUBLE_EQ (cells[0].abs_start.asDouble (), 1440.0);
  EXPECT_DOUBLE_EQ (cells[1].abs_start.asDouble (), 3360.0);
}

// ===========================================================================
// Half-open boundary at loop end
// ===========================================================================

TEST_F (ChordClipCanvasTest, HalfOpenBoundaryAtLoopEnd)
{
  auto * clip = make_clip (1920.0);
  add_chord (*clip, 959.0, dsp::MusicalNote::C, dsp::ChordType::Major);
  set_loop_range (*clip, 0.0, 0.0, 960.0);

  auto cells = compute (*clip);
  ASSERT_EQ (cells.size (), 2);
  EXPECT_DOUBLE_EQ (cells[0].abs_start.asDouble (), 959.0);
  EXPECT_DOUBLE_EQ (cells[1].abs_start.asDouble (), 1919.0);
}

TEST_F (ChordClipCanvasTest, ChordExactlyAtLoopEndIsNeverVisible)
{
  auto * clip = make_clip (1920.0);
  add_chord (*clip, 960.0, dsp::MusicalNote::C, dsp::ChordType::Major);
  set_loop_range (*clip, 0.0, 0.0, 960.0);

  auto cells = compute (*clip);
  EXPECT_TRUE (cells.empty ());
}

// ===========================================================================
// Clip shorter than one loop
// ===========================================================================

TEST_F (ChordClipCanvasTest, ClipShorterThanOneLoop)
{
  auto * clip = make_clip (500.0);
  add_chord (*clip, 0.0, dsp::MusicalNote::C, dsp::ChordType::Major);
  set_loop_range (*clip, 0.0, 0.0, 1920.0);

  auto cells = compute (*clip);
  ASSERT_EQ (cells.size (), 1);
  EXPECT_DOUBLE_EQ (cells[0].abs_start.asDouble (), 0.0);
  EXPECT_DOUBLE_EQ (cells[0].abs_end.asDouble (), 500.0);
}

// ===========================================================================
// Edge cases
// ===========================================================================

TEST_F (ChordClipCanvasTest, ZeroLengthLoopDoesNotHang)
{
  auto * clip = make_clip (1920.0);
  add_chord (*clip, 0.0, dsp::MusicalNote::C, dsp::ChordType::Major);
  set_loop_range (*clip, 0.0, 960.0, 960.0);

  auto cells = compute (*clip);
  EXPECT_EQ (cells.size (), 1);
}

TEST_F (ChordClipCanvasTest, ThreeIterationsProduceThreeCells)
{
  auto * clip = make_clip (5760.0);
  add_chord (*clip, 0.0, dsp::MusicalNote::C, dsp::ChordType::Major);
  set_loop_range (*clip, 0.0, 0.0, 1920.0);

  auto cells = compute (*clip);
  ASSERT_EQ (cells.size (), 3);
  EXPECT_DOUBLE_EQ (cells[0].abs_start.asDouble (), 0.0);
  EXPECT_DOUBLE_EQ (cells[1].abs_start.asDouble (), 1920.0);
  EXPECT_DOUBLE_EQ (cells[2].abs_start.asDouble (), 3840.0);
}

TEST_F (ChordClipCanvasTest, LoopedMultiChordPerIterationEndsAtNextChord)
{
  auto * clip = make_clip (3840.0);
  add_chord (*clip, 0.0, dsp::MusicalNote::C, dsp::ChordType::Major);
  add_chord (*clip, 480.0, dsp::MusicalNote::G, dsp::ChordType::Major);
  set_loop_range (*clip, 0.0, 0.0, 1920.0);

  auto cells = compute (*clip);
  ASSERT_EQ (cells.size (), 4);

  // Iteration 0
  EXPECT_DOUBLE_EQ (cells[0].abs_start.asDouble (), 0.0);
  EXPECT_DOUBLE_EQ (cells[0].abs_end.asDouble (), 480.0);
  EXPECT_DOUBLE_EQ (cells[1].abs_start.asDouble (), 480.0);
  EXPECT_DOUBLE_EQ (cells[1].abs_end.asDouble (), 1920.0);
  // Iteration 1 (shifted by 1920)
  EXPECT_DOUBLE_EQ (cells[2].abs_start.asDouble (), 1920.0);
  EXPECT_DOUBLE_EQ (cells[2].abs_end.asDouble (), 2400.0);
  EXPECT_DOUBLE_EQ (cells[3].abs_start.asDouble (), 2400.0);
  EXPECT_DOUBLE_EQ (cells[3].abs_end.asDouble (), 3840.0);
}

// ===========================================================================
// Cell fields populated
// ===========================================================================

TEST_F (ChordClipCanvasTest, ChordObjectPointerPopulated)
{
  auto * clip = make_clip (1920.0);
  auto * chord_c =
    add_chord (*clip, 0.0, dsp::MusicalNote::C, dsp::ChordType::Major);

  auto cells = compute (*clip);
  ASSERT_EQ (cells.size (), 1);
  EXPECT_EQ (cells[0].chord_object, chord_c);
}

TEST_F (ChordClipCanvasTest, ChordIndexPopulated)
{
  auto * clip = make_clip (1920.0);
  add_chord (*clip, 0.0, dsp::MusicalNote::C, dsp::ChordType::Major);
  add_chord (*clip, 960.0, dsp::MusicalNote::G, dsp::ChordType::Major);

  auto cells = compute (*clip);
  ASSERT_EQ (cells.size (), 2);
  EXPECT_EQ (cells[0].chord_index, 0);
  EXPECT_EQ (cells[1].chord_index, 1);
}

// ===========================================================================
// Chord order (sorted by position)
// ===========================================================================

TEST_F (ChordClipCanvasTest, ChordsSortedByPosition)
{
  auto * clip = make_clip (1920.0);
  add_chord (*clip, 960.0, dsp::MusicalNote::G, dsp::ChordType::Major);
  add_chord (*clip, 0.0, dsp::MusicalNote::C, dsp::ChordType::Major);

  auto cells = compute (*clip);
  ASSERT_EQ (cells.size (), 2);
  EXPECT_DOUBLE_EQ (cells[0].abs_start.asDouble (), 0.0);
  EXPECT_DOUBLE_EQ (cells[1].abs_start.asDouble (), 960.0);
}

// ===========================================================================
// display_end_tick extension (drag preview / loop resize)
// ===========================================================================

TEST_F (ChordClipCanvasTest, DisplayEndTickExtendsBeyondClipTicks)
{
  // Clip is 1920 ticks, but display extends to 3840 (drag preview).
  // Looped clip with 1 chord at 0 → 2 iterations up to 1920, then more.
  auto * clip = make_clip (1920.0);
  add_chord (*clip, 0.0, dsp::MusicalNote::C, dsp::ChordType::Major);
  set_loop_range (*clip, 0.0, 0.0, 1920.0);

  const auto extended_end = dsp::ContentTick{ units::ticks (3840.0) };
  auto       cells = compute_chord_cells (*clip, extended_end);
  // 2 iterations fit in the extended range
  EXPECT_EQ (cells.size (), 2);
}

} // namespace zrythm::frontends::gui::qquick
