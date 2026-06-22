// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/chord_descriptor.h"
#include "dsp/tempo_map.h"
#include "gui/qquick/chord_region_segmenter.h"
#include "structure/arrangement/arranger_object_factory.h"
#include "structure/arrangement/chord_region.h"
#include "utils/app_settings.h"
#include "utils/object_registry.h"

#include <QAbstractItemModel>
#include <QSignalSpy>

#include "helpers/in_memory_settings_backend.h"
#include "helpers/scoped_qcoreapplication.h"

#include <gtest/gtest.h>

namespace zrythm::gui::qquick
{

namespace
{

/// Finds a role id by name in a QAbstractItemModel's roleNames().
int
roleByName (const QAbstractItemModel * model, const char * name)
{
  const auto roles = model->roleNames ();
  for (auto it = roles.begin (); it != roles.end (); ++it)
    {
      if (it.value () == name)
        return it.key ();
    }
  return -1;
}

/// Convenience: reads a double role value at row 0-column 0.
double
doubleRole (const QAbstractItemModel * model, int row, int role)
{
  return model->data (model->index (row, 0), role).toDouble ();
}

} // namespace

class ChordRegionSegmenterTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    app_ = std::make_unique<test_helpers::ScopedQCoreApplication> ();
    registry_ = std::make_unique<utils::ObjectRegistry> ();
    tempo_map_ = std::make_unique<dsp::TempoMap> (units::sample_rate (44100));
    tempo_map_wrapper_ = std::make_unique<dsp::TempoMapWrapper> (*tempo_map_);
    sample_rate_provider_ = [] () { return units::sample_rate (44100); };
    bpm_provider_ = [] () { return 120.0; };
    app_settings_ = std::make_unique<utils::AppSettings> (
      std::make_unique<test_helpers::InMemorySettingsBackend> ());
    factory_ = std::make_unique<structure::arrangement::ArrangerObjectFactory> (
      structure::arrangement::ArrangerObjectFactory::Dependencies{
        .tempo_map_ = *tempo_map_wrapper_,
        .registry_ = *registry_,
        .app_settings_ = *app_settings_,
        .last_timeline_obj_len_provider_ = [] () { return 100.0; },
        .last_editor_obj_len_provider_ = [] () { return 50.0; },
        .automation_curve_algorithm_provider_ =
          [] () { return dsp::CurveOptions::Algorithm::Exponent; },
      },
      sample_rate_provider_, bpm_provider_);

    segmenter_ = std::make_unique<ChordRegionSegmenter> ();
  }

  /// Build a chord region spanning [0, regionEndTicks].
  structure::arrangement::ChordRegion * make_region (double region_end_ticks)
  {
    auto region_ref =
      factory_->get_builder<structure::arrangement::ChordRegion> ()
        .with_start_ticks (units::ticks (0))
        .with_end_ticks (units::ticks (region_end_ticks))
        .build_in_registry ();
    auto * region =
      region_ref.get_object_as<structure::arrangement::ChordRegion> ();
    regions_.push_back (std::move (region_ref));
    return region;
  }

  /// Add a chord object at @p ticks with the given root note and type.
  structure::arrangement::ChordObject * add_chord (
    structure::arrangement::ChordRegion &region,
    double                               ticks,
    dsp::MusicalNote                     root,
    dsp::ChordType                       type)
  {
    auto chord_ref =
      factory_->get_builder<structure::arrangement::ChordObject> ()
        .with_start_ticks (units::ticks (ticks))
        .with_chord_descriptor (root, type)
        .build_in_registry ();
    auto * obj = chord_ref.get_object_as<structure::arrangement::ChordObject> ();
    region.add_object (chord_ref);
    chord_refs_.push_back (std::move (chord_ref));
    return obj;
  }

  /// Set a custom (non-trackBounds) loop range. All values in ticks.
  void set_loop_range (
    structure::arrangement::ChordRegion &region,
    double                               clip_start,
    double                               loop_start,
    double                               loop_end)
  {
    auto * loop_range = region.loopRange ();
    loop_range->setTrackBounds (false);
    loop_range->clipStartPosition ()->setTicks (clip_start);
    loop_range->loopStartPosition ()->setTicks (loop_start);
    loop_range->loopEndPosition ()->setTicks (loop_end);
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
  std::unique_ptr<ChordRegionSegmenter>                          segmenter_;

  std::vector<structure::arrangement::ArrangerObjectUuidReference> regions_;
  std::vector<structure::arrangement::ArrangerObjectUuidReference> chord_refs_;
};

// ===========================================================================
// Basic segment count tests
// ===========================================================================

TEST_F (ChordRegionSegmenterTest, NullRegionHasEmptyModel)
{
  // Even with no region, the segmenter should expose a valid (empty) model
  // so QML Repeaters always have something to bind to.
  auto * segments = segmenter_->segments ();
  ASSERT_NE (segments, nullptr);
  EXPECT_EQ (segments->rowCount (), 0);
}

TEST_F (ChordRegionSegmenterTest, EmptyRegionHasNoSegments)
{
  auto * region = make_region (1920.0);
  segmenter_->setRegion (region);
  auto * segments = segmenter_->segments ();
  ASSERT_NE (segments, nullptr);
  EXPECT_EQ (segments->rowCount (), 0);
}

TEST_F (ChordRegionSegmenterTest, NonLoopedOneSegmentPerChord)
{
  auto * region = make_region (1920.0);
  add_chord (*region, 0.0, dsp::MusicalNote::C, dsp::ChordType::Major);
  add_chord (*region, 480.0, dsp::MusicalNote::D, dsp::ChordType::Minor);
  add_chord (*region, 960.0, dsp::MusicalNote::E, dsp::ChordType::Minor);

  segmenter_->setRegion (region);

  auto * segments = segmenter_->segments ();
  ASSERT_NE (segments, nullptr);
  EXPECT_EQ (segments->rowCount (), 3);
}

// ===========================================================================
// Non-looped region: segment position/width correctness
// ===========================================================================

TEST_F (ChordRegionSegmenterTest, NonLoopedSegmentSpansToNextChord)
{
  auto * region = make_region (1920.0);
  add_chord (*region, 0.0, dsp::MusicalNote::C, dsp::ChordType::Major);
  add_chord (*region, 960.0, dsp::MusicalNote::G, dsp::ChordType::Major);

  segmenter_->setRegion (region);

  auto * segments = segmenter_->segments ();
  ASSERT_EQ (segments->rowCount (), 2);

  const int start_role = roleByName (segments, "absStartTicks");
  const int end_role = roleByName (segments, "absEndTicks");
  ASSERT_GE (start_role, 0);
  ASSERT_GE (end_role, 0);

  // First chord: [0, 960) — ends at the next chord's start.
  EXPECT_DOUBLE_EQ (doubleRole (segments, 0, start_role), 0.0);
  EXPECT_DOUBLE_EQ (doubleRole (segments, 0, end_role), 960.0);

  // Second chord: [960, 1920) — ends at region length.
  EXPECT_DOUBLE_EQ (doubleRole (segments, 1, start_role), 960.0);
  EXPECT_DOUBLE_EQ (doubleRole (segments, 1, end_role), 1920.0);
}

TEST_F (ChordRegionSegmenterTest, NonLoopedSingleChordSpansFullRegion)
{
  auto * region = make_region (1920.0);
  add_chord (*region, 0.0, dsp::MusicalNote::C, dsp::ChordType::Major);

  segmenter_->setRegion (region);

  auto * segments = segmenter_->segments ();
  ASSERT_EQ (segments->rowCount (), 1);

  const int start_role = roleByName (segments, "absStartTicks");
  const int end_role = roleByName (segments, "absEndTicks");

  EXPECT_DOUBLE_EQ (doubleRole (segments, 0, start_role), 0.0);
  EXPECT_DOUBLE_EQ (doubleRole (segments, 0, end_role), 1920.0);
}

// ===========================================================================
// Loop expansion
// ===========================================================================

TEST_F (ChordRegionSegmenterTest, LoopDoublesSegments)
{
  // Region is 3840 ticks; loop is 1920 ticks with 2 chords at [0, 960].
  // Each chord appears once per iteration → 4 segments.
  auto * region = make_region (3840.0);
  add_chord (*region, 0.0, dsp::MusicalNote::C, dsp::ChordType::Major);
  add_chord (*region, 960.0, dsp::MusicalNote::G, dsp::ChordType::Major);
  set_loop_range (*region, 0.0, 0.0, 1920.0);

  segmenter_->setRegion (region);

  auto * segments = segmenter_->segments ();
  ASSERT_EQ (segments->rowCount (), 4);
}

TEST_F (ChordRegionSegmenterTest, LoopSegmentPositions)
{
  // Region is 3840 ticks; loop is 1920 ticks; 1 chord at tick 480.
  auto * region = make_region (3840.0);
  add_chord (*region, 480.0, dsp::MusicalNote::C, dsp::ChordType::Major);
  set_loop_range (*region, 0.0, 0.0, 1920.0);

  segmenter_->setRegion (region);

  auto * segments = segmenter_->segments ();
  ASSERT_EQ (segments->rowCount (), 2);

  const int start_role = roleByName (segments, "absStartTicks");
  const int end_role = roleByName (segments, "absEndTicks");

  // Iteration 0: [480, 1920) — chord at 480, ends at loop boundary.
  EXPECT_DOUBLE_EQ (doubleRole (segments, 0, start_role), 480.0);
  EXPECT_DOUBLE_EQ (doubleRole (segments, 0, end_role), 1920.0);

  // Iteration 1: [480 + 1920, 1920 + 1920) = [2400, 3840).
  EXPECT_DOUBLE_EQ (doubleRole (segments, 1, start_role), 2400.0);
  EXPECT_DOUBLE_EQ (doubleRole (segments, 1, end_role), 3840.0);
}

// ===========================================================================
// Clip start
// ===========================================================================

TEST_F (ChordRegionSegmenterTest, DeadContentBeforeLoopStartNeverVisible)
{
  // clipStart = 480, loopStart = 480: a chord at 0 is "dead" content —
  // it's before clipStart (skipped in iteration 0) AND before loopStart
  // (skipped in all subsequent iterations).
  auto * region = make_region (1920.0);
  add_chord (*region, 0.0, dsp::MusicalNote::C, dsp::ChordType::Major);
  add_chord (*region, 960.0, dsp::MusicalNote::G, dsp::ChordType::Major);
  set_loop_range (*region, 480.0, 480.0, 1920.0);

  segmenter_->setRegion (region);

  auto * segments = segmenter_->segments ();
  // Only the chord at 960 is visible (in the first iteration).
  // Iteration 0: virt [480, 1920), abs [0, 1440). Chord at 960 is in range.
  // Chord at 0: 0 < virt_start (480) → skipped in all iterations.
  // Iteration 1 would have abs_end > region_ticks, so it gets clamped to
  // virt [480, 960) — chord at 960 is now >= virt_end (960), so skipped.
  ASSERT_EQ (segments->rowCount (), 1);

  const int start_role = roleByName (segments, "absStartTicks");
  // Chord at 960 maps to abs position: 0 + (960 - 480) = 480.
  EXPECT_DOUBLE_EQ (doubleRole (segments, 0, start_role), 480.0);
}

TEST_F (ChordRegionSegmenterTest, ClipStartStillLoopsBackToLoopStart)
{
  // clipStart = 480 but loopStart = 0: a chord at 0 IS visible in
  // subsequent iterations (the loop wraps back to 0). This verifies that
  // clipStart only affects the FIRST iteration, not subsequent ones.
  auto * region = make_region (3840.0);
  add_chord (*region, 0.0, dsp::MusicalNote::C, dsp::ChordType::Major);
  set_loop_range (*region, 480.0, 0.0, 1920.0);

  segmenter_->setRegion (region);

  auto * segments = segmenter_->segments ();
  // Iteration 0 (first): virt [480, 1920) — chord at 0 not visible (0 < 480).
  // Iteration 1: virt [0, 1920), abs [1440, 3360). No clamping (3360 < 3840).
  //   Chord at 0 is in [0, 1920) → visible at abs 1440 + (0 - 0) = 1440.
  // Iteration 2: abs [3360, 5280) clamped to [3360, 3840); virt_end = 1920 -
  // 1440 = 480.
  //   virt [0, 480). Chord at 0 is in [0, 480) → visible at abs 3360 + (0 - 0)
  //   = 3360.
  ASSERT_EQ (segments->rowCount (), 2);

  const int start_role = roleByName (segments, "absStartTicks");
  EXPECT_DOUBLE_EQ (doubleRole (segments, 0, start_role), 1440.0);
  EXPECT_DOUBLE_EQ (doubleRole (segments, 1, start_role), 3360.0);
}

// ===========================================================================
// Chord at loop boundary belongs to next iteration (half-open interval)
// ===========================================================================

TEST_F (ChordRegionSegmenterTest, HalfOpenBoundaryAtLoopEnd)
{
  // A chord at exactly loopEnd is never visible — the loop wraps at loopEnd
  // back to loopStart before the chord is reached. A chord just before
  // loopEnd IS visible in the current iteration. This matches MIDI region
  // rendering semantics (a note starting at loopEnd is also skipped).
  auto * region = make_region (1920.0);
  // Chord just before loopEnd — should be visible.
  add_chord (*region, 959.0, dsp::MusicalNote::C, dsp::ChordType::Major);
  set_loop_range (*region, 0.0, 0.0, 960.0);

  segmenter_->setRegion (region);

  auto * segments = segmenter_->segments ();
  // Iteration 0: virt [0, 960). Chord at 959 is in range → visible.
  // Iteration 1: virt [0, 960). Chord at 959 is in range → visible again.
  ASSERT_EQ (segments->rowCount (), 2);

  const int start_role = roleByName (segments, "absStartTicks");
  // Iteration 0: abs 0 + (959 - 0) = 959.
  EXPECT_DOUBLE_EQ (doubleRole (segments, 0, start_role), 959.0);
  // Iteration 1: abs 960 + (959 - 0) = 1919.
  EXPECT_DOUBLE_EQ (doubleRole (segments, 1, start_role), 1919.0);
}

TEST_F (ChordRegionSegmenterTest, ChordExactlyAtLoopEndIsNeverVisible)
{
  // A chord at exactly loopEnd is never visible — it sits on the half-open
  // boundary of every iteration's virtual range.
  auto * region = make_region (1920.0);
  add_chord (*region, 960.0, dsp::MusicalNote::C, dsp::ChordType::Major);
  set_loop_range (*region, 0.0, 0.0, 960.0);

  segmenter_->setRegion (region);

  auto * segments = segmenter_->segments ();
  EXPECT_EQ (segments->rowCount (), 0);
}

// ===========================================================================
// Region shorter than one loop
// ===========================================================================

TEST_F (ChordRegionSegmenterTest, RegionShorterThanOneLoop)
{
  // Region is 500 ticks, loop is [0, 1920), chord at 0.
  // The single iteration is clamped to [0, 500).
  auto * region = make_region (500.0);
  add_chord (*region, 0.0, dsp::MusicalNote::C, dsp::ChordType::Major);
  set_loop_range (*region, 0.0, 0.0, 1920.0);

  segmenter_->setRegion (region);

  auto * segments = segmenter_->segments ();
  ASSERT_EQ (segments->rowCount (), 1);

  const int start_role = roleByName (segments, "absStartTicks");
  const int end_role = roleByName (segments, "absEndTicks");

  EXPECT_DOUBLE_EQ (doubleRole (segments, 0, start_role), 0.0);
  // End is clamped to region_ticks.
  EXPECT_DOUBLE_EQ (doubleRole (segments, 0, end_role), 500.0);
}

// ===========================================================================
// Loop expansion: additional edge cases
// ===========================================================================

// A zero-length loop (loop_end == loop_start) carries no looped content. The
// segmenter must terminate promptly (guarded by the current_len <= 0 check)
// and not hang. Only the first (clip) iteration can produce segments.
TEST_F (ChordRegionSegmenterTest, ZeroLengthLoopDoesNotHang)
{
  auto * region = make_region (1920.0);
  add_chord (*region, 0.0, dsp::MusicalNote::C, dsp::ChordType::Major);
  set_loop_range (*region, 0.0, 960.0, 960.0);

  segmenter_->setRegion (region);

  auto * segments = segmenter_->segments ();
  // First iteration: virt [0, 960). Chord at 0 is visible.
  // Second iteration: virt [960, 960) is empty → current_len 0 → break.
  EXPECT_EQ (segments->rowCount (), 1);
}

// A region three loop-lengths long with one chord: the chord appears once
// per iteration → 3 segments. Exercises repeated abs_start accumulation
// across more than two visible iterations.
TEST_F (ChordRegionSegmenterTest, ThreeIterationsProduceThreeSegments)
{
  auto * region = make_region (5760.0);
  add_chord (*region, 0.0, dsp::MusicalNote::C, dsp::ChordType::Major);
  set_loop_range (*region, 0.0, 0.0, 1920.0);

  segmenter_->setRegion (region);

  auto * segments = segmenter_->segments ();
  ASSERT_EQ (segments->rowCount (), 3);

  const int start_role = roleByName (segments, "absStartTicks");
  EXPECT_DOUBLE_EQ (doubleRole (segments, 0, start_role), 0.0);
  EXPECT_DOUBLE_EQ (doubleRole (segments, 1, start_role), 1920.0);
  EXPECT_DOUBLE_EQ (doubleRole (segments, 2, start_role), 3840.0);
}

// Two chords within one loop iteration: each chord's segment must end at the
// next chord's position, and this relationship must hold across loop
// iterations with the abs_start offset applied.
TEST_F (ChordRegionSegmenterTest, LoopedMultiChordPerIterationEndsAtNextChord)
{
  auto * region = make_region (3840.0);
  add_chord (*region, 0.0, dsp::MusicalNote::C, dsp::ChordType::Major);
  add_chord (*region, 480.0, dsp::MusicalNote::G, dsp::ChordType::Major);
  set_loop_range (*region, 0.0, 0.0, 1920.0);

  segmenter_->setRegion (region);

  auto * segments = segmenter_->segments ();
  ASSERT_EQ (segments->rowCount (), 4);

  const int start_role = roleByName (segments, "absStartTicks");
  const int end_role = roleByName (segments, "absEndTicks");

  // Iteration 0: C ends at G's position; G ends at the loop boundary.
  EXPECT_DOUBLE_EQ (doubleRole (segments, 0, start_role), 0.0);
  EXPECT_DOUBLE_EQ (doubleRole (segments, 0, end_role), 480.0);
  EXPECT_DOUBLE_EQ (doubleRole (segments, 1, start_role), 480.0);
  EXPECT_DOUBLE_EQ (doubleRole (segments, 1, end_role), 1920.0);
  // Iteration 1: shifted by loop_length (1920).
  EXPECT_DOUBLE_EQ (doubleRole (segments, 2, start_role), 1920.0);
  EXPECT_DOUBLE_EQ (doubleRole (segments, 2, end_role), 2400.0);
  EXPECT_DOUBLE_EQ (doubleRole (segments, 3, start_role), 2400.0);
  EXPECT_DOUBLE_EQ (doubleRole (segments, 3, end_role), 3840.0);
}

// ===========================================================================
// Reactivity: segmentsChanged emission
// ===========================================================================

TEST_F (ChordRegionSegmenterTest, SegmentsChangedOnSetRegion)
{
  QSignalSpy spy (segmenter_.get (), &ChordRegionSegmenter::segmentsChanged);
  auto *     region = make_region (1920.0);
  segmenter_->setRegion (region);
  EXPECT_GE (spy.count (), 1);
}

TEST_F (ChordRegionSegmenterTest, SegmentsChangedOnRegionResize)
{
  auto * region = make_region (1920.0);
  add_chord (*region, 0.0, dsp::MusicalNote::C, dsp::ChordType::Major);
  segmenter_->setRegion (region);

  QSignalSpy spy (segmenter_.get (), &ChordRegionSegmenter::segmentsChanged);
  region->bounds ()->length ()->setTicks (3840.0);
  // Length change routes through several position adapters, each firing
  // recalculate separately — expect >= 1 emission, not exactly 1.
  EXPECT_GE (spy.count (), 1);
}

TEST_F (ChordRegionSegmenterTest, SegmentsChangedOnLoopConfigChange)
{
  auto * region = make_region (1920.0);
  add_chord (*region, 0.0, dsp::MusicalNote::C, dsp::ChordType::Major);
  segmenter_->setRegion (region);

  QSignalSpy spy (segmenter_.get (), &ChordRegionSegmenter::segmentsChanged);
  set_loop_range (*region, 0.0, 0.0, 960.0);
  // set_loop_range sets 3 positions; each fires recalculate separately.
  EXPECT_GE (spy.count (), 1);
}

// Adding a chord after setRegion fires contentChanged -> recalculate and
// produces a new segment.
TEST_F (ChordRegionSegmenterTest, ReactsToChordAdditionAfterSetRegion)
{
  auto * region = make_region (1920.0);
  segmenter_->setRegion (region);
  ASSERT_EQ (segmenter_->segments ()->rowCount (), 0);

  QSignalSpy spy (segmenter_.get (), &ChordRegionSegmenter::segmentsChanged);
  add_chord (*region, 0.0, dsp::MusicalNote::C, dsp::ChordType::Major);
  EXPECT_GE (spy.count (), 1);
  EXPECT_EQ (segmenter_->segments ()->rowCount (), 1);
}

// ===========================================================================
// Segment fields populated
// ===========================================================================

TEST_F (ChordRegionSegmenterTest, ChordObjectPointerPopulated)
{
  auto * region = make_region (1920.0);
  auto * chord_c =
    add_chord (*region, 0.0, dsp::MusicalNote::C, dsp::ChordType::Major);

  segmenter_->setRegion (region);

  auto * segments = segmenter_->segments ();
  ASSERT_EQ (segments->rowCount (), 1);

  const int obj_role = roleByName (segments, "chordObject");
  ASSERT_GE (obj_role, 0);
  auto * obj_ptr =
    segments->data (segments->index (0, 0), obj_role)
      .value<structure::arrangement::ChordObject *> ();
  EXPECT_EQ (obj_ptr, chord_c);
}

TEST_F (ChordRegionSegmenterTest, ChordIndexPopulated)
{
  auto * region = make_region (1920.0);
  add_chord (*region, 0.0, dsp::MusicalNote::C, dsp::ChordType::Major);
  add_chord (*region, 960.0, dsp::MusicalNote::G, dsp::ChordType::Major);

  segmenter_->setRegion (region);

  auto * segments = segmenter_->segments ();
  ASSERT_EQ (segments->rowCount (), 2);

  const int idx_role = roleByName (segments, "chordIndex");
  ASSERT_GE (idx_role, 0);
  // Both chords appear in insertion order; indices 0 and 1.
  EXPECT_EQ (segments->data (segments->index (0, 0), idx_role).toInt (), 0);
  EXPECT_EQ (segments->data (segments->index (1, 0), idx_role).toInt (), 1);
}

// ===========================================================================
// Chord order (sorted by position)
// ===========================================================================

TEST_F (ChordRegionSegmenterTest, ChordsSortedByPosition)
{
  // Add chords in non-sorted order; segmenter should iterate sorted.
  auto * region = make_region (1920.0);
  add_chord (*region, 960.0, dsp::MusicalNote::G, dsp::ChordType::Major);
  add_chord (*region, 0.0, dsp::MusicalNote::C, dsp::ChordType::Major);

  segmenter_->setRegion (region);

  auto * segments = segmenter_->segments ();
  ASSERT_EQ (segments->rowCount (), 2);

  const int start_role = roleByName (segments, "absStartTicks");
  // First segment should be the chord at position 0 (sorted ascending).
  EXPECT_DOUBLE_EQ (doubleRole (segments, 0, start_role), 0.0);
  EXPECT_DOUBLE_EQ (doubleRole (segments, 1, start_role), 960.0);
}

// ===========================================================================
// Region reset to null clears segments
// ===========================================================================

TEST_F (ChordRegionSegmenterTest, SettingNullRegionClearsSegments)
{
  auto * region = make_region (1920.0);
  add_chord (*region, 0.0, dsp::MusicalNote::C, dsp::ChordType::Major);
  segmenter_->setRegion (region);
  ASSERT_NE (segmenter_->segments (), nullptr);
  EXPECT_EQ (segmenter_->segments ()->rowCount (), 1);

  segmenter_->setRegion (nullptr);
  EXPECT_NE (segmenter_->segments (), nullptr);
  EXPECT_EQ (segmenter_->segments ()->rowCount (), 0);
}

} // namespace zrythm::gui::qquick
