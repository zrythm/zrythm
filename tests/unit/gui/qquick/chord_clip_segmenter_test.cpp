// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/chord_descriptor.h"
#include "dsp/tempo_map.h"
#include "gui/qquick/chord_clip_segmenter.h"
#include "structure/arrangement/arranger_object_factory.h"
#include "structure/arrangement/chord_clip.h"
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

class ChordClipSegmenterTest : public ::testing::Test
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

    segmenter_ = std::make_unique<ChordClipSegmenter> ();
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

  std::unique_ptr<test_helpers::ScopedQCoreApplication> app_;
  std::unique_ptr<utils::ObjectRegistry>                registry_;
  std::unique_ptr<dsp::TempoMap>                        tempo_map_;
  std::unique_ptr<dsp::TempoMapWrapper>                 tempo_map_wrapper_;
  structure::arrangement::ArrangerObjectFactory::SampleRateProvider
    sample_rate_provider_;
  structure::arrangement::ArrangerObjectFactory::BpmProvider     bpm_provider_;
  std::unique_ptr<utils::AppSettings>                            app_settings_;
  std::unique_ptr<structure::arrangement::ArrangerObjectFactory> factory_;
  std::unique_ptr<ChordClipSegmenter>                            segmenter_;

  std::vector<structure::arrangement::ArrangerObjectUuidReference> clips_;
  std::vector<structure::arrangement::ArrangerObjectUuidReference> chord_refs_;
};

// ===========================================================================
// Basic segment count tests
// ===========================================================================

TEST_F (ChordClipSegmenterTest, NullClipHasEmptyModel)
{
  // Even with no clip, the segmenter should expose a valid (empty) model
  // so QML Repeaters always have something to bind to.
  auto * segments = segmenter_->segments ();
  ASSERT_NE (segments, nullptr);
  EXPECT_EQ (segments->rowCount (), 0);
}

TEST_F (ChordClipSegmenterTest, EmptyClipHasNoSegments)
{
  auto * clip = make_clip (1920.0);
  segmenter_->setChordClip (clip);
  auto * segments = segmenter_->segments ();
  ASSERT_NE (segments, nullptr);
  EXPECT_EQ (segments->rowCount (), 0);
}

TEST_F (ChordClipSegmenterTest, NonLoopedOneSegmentPerChord)
{
  auto * clip = make_clip (1920.0);
  add_chord (*clip, 0.0, dsp::MusicalNote::C, dsp::ChordType::Major);
  add_chord (*clip, 480.0, dsp::MusicalNote::D, dsp::ChordType::Minor);
  add_chord (*clip, 960.0, dsp::MusicalNote::E, dsp::ChordType::Minor);

  segmenter_->setChordClip (clip);

  auto * segments = segmenter_->segments ();
  ASSERT_NE (segments, nullptr);
  EXPECT_EQ (segments->rowCount (), 3);
}

// ===========================================================================
// Non-looped clip: segment position/width correctness
// ===========================================================================

TEST_F (ChordClipSegmenterTest, NonLoopedSegmentSpansToNextChord)
{
  auto * clip = make_clip (1920.0);
  add_chord (*clip, 0.0, dsp::MusicalNote::C, dsp::ChordType::Major);
  add_chord (*clip, 960.0, dsp::MusicalNote::G, dsp::ChordType::Major);

  segmenter_->setChordClip (clip);

  auto * segments = segmenter_->segments ();
  ASSERT_EQ (segments->rowCount (), 2);

  const int start_role = roleByName (segments, "absStartTicks");
  const int end_role = roleByName (segments, "absEndTicks");
  ASSERT_GE (start_role, 0);
  ASSERT_GE (end_role, 0);

  // First chord: [0, 960) — ends at the next chord's start.
  EXPECT_DOUBLE_EQ (doubleRole (segments, 0, start_role), 0.0);
  EXPECT_DOUBLE_EQ (doubleRole (segments, 0, end_role), 960.0);

  // Second chord: [960, 1920) — ends at clip length.
  EXPECT_DOUBLE_EQ (doubleRole (segments, 1, start_role), 960.0);
  EXPECT_DOUBLE_EQ (doubleRole (segments, 1, end_role), 1920.0);
}

TEST_F (ChordClipSegmenterTest, NonLoopedSingleChordSpansFullClip)
{
  auto * clip = make_clip (1920.0);
  add_chord (*clip, 0.0, dsp::MusicalNote::C, dsp::ChordType::Major);

  segmenter_->setChordClip (clip);

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

TEST_F (ChordClipSegmenterTest, LoopDoublesSegments)
{
  // Clip is 3840 ticks; loop is 1920 ticks with 2 chords at [0, 960].
  // Each chord appears once per iteration → 4 segments.
  auto * clip = make_clip (3840.0);
  add_chord (*clip, 0.0, dsp::MusicalNote::C, dsp::ChordType::Major);
  add_chord (*clip, 960.0, dsp::MusicalNote::G, dsp::ChordType::Major);
  set_loop_range (*clip, 0.0, 0.0, 1920.0);

  segmenter_->setChordClip (clip);

  auto * segments = segmenter_->segments ();
  ASSERT_EQ (segments->rowCount (), 4);
}

TEST_F (ChordClipSegmenterTest, LoopSegmentPositions)
{
  // Clip is 3840 ticks; loop is 1920 ticks; 1 chord at tick 480.
  auto * clip = make_clip (3840.0);
  add_chord (*clip, 480.0, dsp::MusicalNote::C, dsp::ChordType::Major);
  set_loop_range (*clip, 0.0, 0.0, 1920.0);

  segmenter_->setChordClip (clip);

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

TEST_F (ChordClipSegmenterTest, DeadContentBeforeLoopStartNeverVisible)
{
  // clipStart = 480, loopStart = 480: a chord at 0 is "dead" content —
  // it's before clipStart (skipped in iteration 0) AND before loopStart
  // (skipped in all subsequent iterations).
  auto * clip = make_clip (1920.0);
  add_chord (*clip, 0.0, dsp::MusicalNote::C, dsp::ChordType::Major);
  add_chord (*clip, 960.0, dsp::MusicalNote::G, dsp::ChordType::Major);
  set_loop_range (*clip, 480.0, 480.0, 1920.0);

  segmenter_->setChordClip (clip);

  auto * segments = segmenter_->segments ();
  // Only the chord at 960 is visible (in the first iteration).
  // Iteration 0: virt [480, 1920), abs [0, 1440). Chord at 960 is in range.
  // Chord at 0: 0 < virt_start (480) → skipped in all iterations.
  // Iteration 1 would have abs_end > clip_ticks, so it gets clamped to
  // virt [480, 960) — chord at 960 is now >= virt_end (960), so skipped.
  ASSERT_EQ (segments->rowCount (), 1);

  const int start_role = roleByName (segments, "absStartTicks");
  // Chord at 960 maps to abs position: 0 + (960 - 480) = 480.
  EXPECT_DOUBLE_EQ (doubleRole (segments, 0, start_role), 480.0);
}

TEST_F (ChordClipSegmenterTest, ClipStartStillLoopsBackToLoopStart)
{
  // clipStart = 480 but loopStart = 0: a chord at 0 IS visible in
  // subsequent iterations (the loop wraps back to 0). This verifies that
  // clipStart only affects the FIRST iteration, not subsequent ones.
  auto * clip = make_clip (3840.0);
  add_chord (*clip, 0.0, dsp::MusicalNote::C, dsp::ChordType::Major);
  set_loop_range (*clip, 480.0, 0.0, 1920.0);

  segmenter_->setChordClip (clip);

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

TEST_F (ChordClipSegmenterTest, HalfOpenBoundaryAtLoopEnd)
{
  // A chord at exactly loopEnd is never visible — the loop wraps at loopEnd
  // back to loopStart before the chord is reached. A chord just before
  // loopEnd IS visible in the current iteration. This matches MIDI clip
  // rendering semantics (a note starting at loopEnd is also skipped).
  auto * clip = make_clip (1920.0);
  // Chord just before loopEnd — should be visible.
  add_chord (*clip, 959.0, dsp::MusicalNote::C, dsp::ChordType::Major);
  set_loop_range (*clip, 0.0, 0.0, 960.0);

  segmenter_->setChordClip (clip);

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

TEST_F (ChordClipSegmenterTest, ChordExactlyAtLoopEndIsNeverVisible)
{
  // A chord at exactly loopEnd is never visible — it sits on the half-open
  // boundary of every iteration's virtual range.
  auto * clip = make_clip (1920.0);
  add_chord (*clip, 960.0, dsp::MusicalNote::C, dsp::ChordType::Major);
  set_loop_range (*clip, 0.0, 0.0, 960.0);

  segmenter_->setChordClip (clip);

  auto * segments = segmenter_->segments ();
  EXPECT_EQ (segments->rowCount (), 0);
}

// ===========================================================================
// Clip shorter than one loop
// ===========================================================================

TEST_F (ChordClipSegmenterTest, ClipShorterThanOneLoop)
{
  // Clip is 500 ticks, loop is [0, 1920), chord at 0.
  // The single iteration is clamped to [0, 500).
  auto * clip = make_clip (500.0);
  add_chord (*clip, 0.0, dsp::MusicalNote::C, dsp::ChordType::Major);
  set_loop_range (*clip, 0.0, 0.0, 1920.0);

  segmenter_->setChordClip (clip);

  auto * segments = segmenter_->segments ();
  ASSERT_EQ (segments->rowCount (), 1);

  const int start_role = roleByName (segments, "absStartTicks");
  const int end_role = roleByName (segments, "absEndTicks");

  EXPECT_DOUBLE_EQ (doubleRole (segments, 0, start_role), 0.0);
  // End is clamped to clip_ticks.
  EXPECT_DOUBLE_EQ (doubleRole (segments, 0, end_role), 500.0);
}

// ===========================================================================
// Loop expansion: additional edge cases
// ===========================================================================

// A zero-length loop (loop_end == loop_start) carries no looped content. The
// segmenter must terminate promptly (guarded by the current_len <= 0 check)
// and not hang. Only the first (clip) iteration can produce segments.
TEST_F (ChordClipSegmenterTest, ZeroLengthLoopDoesNotHang)
{
  auto * clip = make_clip (1920.0);
  add_chord (*clip, 0.0, dsp::MusicalNote::C, dsp::ChordType::Major);
  set_loop_range (*clip, 0.0, 960.0, 960.0);

  segmenter_->setChordClip (clip);

  auto * segments = segmenter_->segments ();
  // First iteration: virt [0, 960). Chord at 0 is visible.
  // Second iteration: virt [960, 960) is empty → current_len 0 → break.
  EXPECT_EQ (segments->rowCount (), 1);
}

// A clip three loop-lengths long with one chord: the chord appears once
// per iteration → 3 segments. Exercises repeated abs_start accumulation
// across more than two visible iterations.
TEST_F (ChordClipSegmenterTest, ThreeIterationsProduceThreeSegments)
{
  auto * clip = make_clip (5760.0);
  add_chord (*clip, 0.0, dsp::MusicalNote::C, dsp::ChordType::Major);
  set_loop_range (*clip, 0.0, 0.0, 1920.0);

  segmenter_->setChordClip (clip);

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
TEST_F (ChordClipSegmenterTest, LoopedMultiChordPerIterationEndsAtNextChord)
{
  auto * clip = make_clip (3840.0);
  add_chord (*clip, 0.0, dsp::MusicalNote::C, dsp::ChordType::Major);
  add_chord (*clip, 480.0, dsp::MusicalNote::G, dsp::ChordType::Major);
  set_loop_range (*clip, 0.0, 0.0, 1920.0);

  segmenter_->setChordClip (clip);

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

TEST_F (ChordClipSegmenterTest, SegmentsChangedOnSetClip)
{
  QSignalSpy spy (segmenter_.get (), &ChordClipSegmenter::segmentsChanged);
  auto *     clip = make_clip (1920.0);
  segmenter_->setChordClip (clip);
  EXPECT_GE (spy.count (), 1);
}

TEST_F (ChordClipSegmenterTest, SegmentsChangedOnClipResize)
{
  auto * clip = make_clip (1920.0);
  add_chord (*clip, 0.0, dsp::MusicalNote::C, dsp::ChordType::Major);
  segmenter_->setChordClip (clip);

  QSignalSpy spy (segmenter_.get (), &ChordClipSegmenter::segmentsChanged);
  clip->length ()->setTicks (3840.0);
  // Length change routes through several position adapters, each firing
  // recalculate separately — expect >= 1 emission, not exactly 1.
  EXPECT_GE (spy.count (), 1);
}

TEST_F (ChordClipSegmenterTest, SegmentsChangedOnLoopConfigChange)
{
  auto * clip = make_clip (1920.0);
  add_chord (*clip, 0.0, dsp::MusicalNote::C, dsp::ChordType::Major);
  segmenter_->setChordClip (clip);

  QSignalSpy spy (segmenter_.get (), &ChordClipSegmenter::segmentsChanged);
  set_loop_range (*clip, 0.0, 0.0, 960.0);
  // set_loop_range sets 3 positions; each fires recalculate separately.
  EXPECT_GE (spy.count (), 1);
}

// Adding a chord after setChordClip fires contentChanged -> recalculate and
// produces a new segment.
TEST_F (ChordClipSegmenterTest, ReactsToChordAdditionAfterSetClip)
{
  auto * clip = make_clip (1920.0);
  segmenter_->setChordClip (clip);
  ASSERT_EQ (segmenter_->segments ()->rowCount (), 0);

  QSignalSpy spy (segmenter_.get (), &ChordClipSegmenter::segmentsChanged);
  add_chord (*clip, 0.0, dsp::MusicalNote::C, dsp::ChordType::Major);
  EXPECT_GE (spy.count (), 1);
  EXPECT_EQ (segmenter_->segments ()->rowCount (), 1);
}

// ===========================================================================
// Segment fields populated
// ===========================================================================

TEST_F (ChordClipSegmenterTest, ChordObjectPointerPopulated)
{
  auto * clip = make_clip (1920.0);
  auto * chord_c =
    add_chord (*clip, 0.0, dsp::MusicalNote::C, dsp::ChordType::Major);

  segmenter_->setChordClip (clip);

  auto * segments = segmenter_->segments ();
  ASSERT_EQ (segments->rowCount (), 1);

  const int obj_role = roleByName (segments, "chordObject");
  ASSERT_GE (obj_role, 0);
  auto * obj_ptr =
    segments->data (segments->index (0, 0), obj_role)
      .value<structure::arrangement::ChordObject *> ();
  EXPECT_EQ (obj_ptr, chord_c);
}

TEST_F (ChordClipSegmenterTest, ChordIndexPopulated)
{
  auto * clip = make_clip (1920.0);
  add_chord (*clip, 0.0, dsp::MusicalNote::C, dsp::ChordType::Major);
  add_chord (*clip, 960.0, dsp::MusicalNote::G, dsp::ChordType::Major);

  segmenter_->setChordClip (clip);

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

TEST_F (ChordClipSegmenterTest, ChordsSortedByPosition)
{
  // Add chords in non-sorted order; segmenter should iterate sorted.
  auto * clip = make_clip (1920.0);
  add_chord (*clip, 960.0, dsp::MusicalNote::G, dsp::ChordType::Major);
  add_chord (*clip, 0.0, dsp::MusicalNote::C, dsp::ChordType::Major);

  segmenter_->setChordClip (clip);

  auto * segments = segmenter_->segments ();
  ASSERT_EQ (segments->rowCount (), 2);

  const int start_role = roleByName (segments, "absStartTicks");
  // First segment should be the chord at position 0 (sorted ascending).
  EXPECT_DOUBLE_EQ (doubleRole (segments, 0, start_role), 0.0);
  EXPECT_DOUBLE_EQ (doubleRole (segments, 1, start_role), 960.0);
}

// ===========================================================================
// Clip reset to null clears segments
// ===========================================================================

TEST_F (ChordClipSegmenterTest, SettingNullClipClearsSegments)
{
  auto * clip = make_clip (1920.0);
  add_chord (*clip, 0.0, dsp::MusicalNote::C, dsp::ChordType::Major);
  segmenter_->setChordClip (clip);
  ASSERT_NE (segmenter_->segments (), nullptr);
  EXPECT_EQ (segmenter_->segments ()->rowCount (), 1);

  segmenter_->setChordClip (nullptr);
  EXPECT_NE (segmenter_->segments (), nullptr);
  EXPECT_EQ (segmenter_->segments ()->rowCount (), 0);
}

} // namespace zrythm::gui::qquick
