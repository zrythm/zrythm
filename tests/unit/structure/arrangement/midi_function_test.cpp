// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <memory>
#include <vector>

#include "dsp/tempo_map.h"
#include "dsp/tempo_map_qml_adapter.h"
#include "structure/arrangement/midi_clip.h"
#include "structure/arrangement/midi_function.h"
#include "structure/arrangement/midi_note.h"
#include "utils/object_registry.h"
#include "utils/registry_utils.h"

#include <gtest/gtest.h>

namespace zrythm::structure::arrangement
{

/// Ticks one millisecond spans at the default tempo (120 BPM, 960 PPQ).
constexpr double kTicksPerMs = 1.92;

/// Content-tick literal for expectations.
constexpr dsp::ContentTick
ticks (double t)
{
  return dsp::ContentTick{ units::ticks (t) };
}

/// One millisecond as a strong duration.
constexpr units::precise_second_t
ms (double milliseconds)
{
  return units::seconds (milliseconds / 1000.0);
}

/// Asserts two content ticks are equal within a tiny tolerance.
void
expect_ticks_near (dsp::ContentTick actual, dsp::ContentTick expected)
{
  EXPECT_LT (dsp::abs (actual - expected), ticks (1e-6));
}

class MidiFunctionTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    tempo_map = std::make_unique<dsp::TempoMap> (units::sample_rate (44100.0));
    tempo_map_wrapper = std::make_unique<dsp::TempoMapWrapper> (*tempo_map);
  }

  /// Creates a note with the given state, kept alive for the test's
  /// duration.
  MidiNote *
  add_note (double position_ticks, double length_ticks, int pitch, int velocity)
  {
    auto note = std::make_unique<MidiNote> (*tempo_map_wrapper);
    note->position ()->set_ticks (ticks (position_ticks));
    note->length ()->set_ticks (ticks (length_ticks));
    note->setPitch (pitch);
    note->setVelocity (velocity);
    notes_.push_back (std::move (note));
    return notes_.back ().get ();
  }

  /// A span over the created notes, in creation order.
  std::span<MidiNote *> all_notes ()
  {
    note_ptrs_.clear ();
    for (const auto &note : notes_)
      {
        note_ptrs_.push_back (note.get ());
      }
    return note_ptrs_;
  }

  /// A crescendo with a linear curve: curviness 0 makes every curve
  /// algorithm a straight line.
  static MidiFunction::Crescendo crescendo (int start_vel, int end_vel)
  {
    MidiFunction::Crescendo params;
    params.start_vel = start_vel;
    params.end_vel = end_vel;
    params.curve_algo = dsp::CurveOptions::Algorithm::Exponent;
    params.curviness = 0.0;
    return params;
  }

  /// A flam with the given grace-note offset and velocity.
  static MidiFunction::Flam flam (double time_ms, int vel)
  {
    MidiFunction::Flam params;
    params.time = ms (time_ms);
    params.vel = vel;
    return params;
  }

  /// A strum with a linear curve.
  static MidiFunction::Strum strum (bool ascending, double time_ms)
  {
    MidiFunction::Strum params;
    params.ascending = ascending;
    params.curve_algo = dsp::CurveOptions::Algorithm::Exponent;
    params.curviness = 0.0;
    params.time = ms (time_ms);
    return params;
  }

  std::unique_ptr<dsp::TempoMap>         tempo_map;
  std::unique_ptr<dsp::TempoMapWrapper>  tempo_map_wrapper;
  std::vector<std::unique_ptr<MidiNote>> notes_;
  std::vector<MidiNote *>                note_ptrs_;
};

TEST_F (MidiFunctionTest, CrescendoInterpolatesVelocitiesLinearly)
{
  add_note (0, 240, 60, 10);
  add_note (960, 240, 64, 10);
  add_note (1920, 240, 67, 10);

  const auto result =
    MidiFunction::Crescendo::compute (all_notes (), crescendo (40, 100));

  ASSERT_EQ (result.new_states_.size (), 3u);
  EXPECT_EQ (result.new_states_[0].velocity_, 40);
  EXPECT_EQ (result.new_states_[1].velocity_, 70);
  EXPECT_EQ (result.new_states_[2].velocity_, 100);
}

TEST_F (MidiFunctionTest, CrescendoSupportsDecrescendo)
{
  add_note (0, 240, 60, 10);
  add_note (960, 240, 64, 10);
  add_note (1920, 240, 67, 10);

  const auto result =
    MidiFunction::Crescendo::compute (all_notes (), crescendo (100, 40));

  EXPECT_EQ (result.new_states_[0].velocity_, 100);
  EXPECT_EQ (result.new_states_[1].velocity_, 70);
  EXPECT_EQ (result.new_states_[2].velocity_, 40);
}

TEST_F (MidiFunctionTest, CrescendoSingleNoteGetsStartVelocity)
{
  add_note (480, 240, 60, 10);

  const auto result =
    MidiFunction::Crescendo::compute (all_notes (), crescendo (33, 99));

  EXPECT_EQ (result.new_states_[0].velocity_, 33);
}

TEST_F (MidiFunctionTest, CrescendoSamePositionNotesAreUnchanged)
{
  add_note (480, 240, 60, 10);
  add_note (480, 240, 64, 20);

  const auto result =
    MidiFunction::Crescendo::compute (all_notes (), crescendo (40, 100));

  EXPECT_EQ (result.new_states_[0].velocity_, 10);
  EXPECT_EQ (result.new_states_[1].velocity_, 20);
}

TEST_F (MidiFunctionTest, FlipVerticalMirrorsPitches)
{
  add_note (0, 240, 55, 10);
  add_note (960, 200, 62, 10);
  add_note (1920, 300, 69, 10);

  const auto result = MidiFunction::FlipVertical::compute (all_notes (), {});

  EXPECT_EQ (result.new_states_[0].pitch_, 69);
  EXPECT_EQ (result.new_states_[1].pitch_, 62);
  EXPECT_EQ (result.new_states_[2].pitch_, 55);
  // Positions and lengths are untouched
  EXPECT_EQ (result.new_states_[1].position_ticks_, ticks (960));
  EXPECT_EQ (result.new_states_[1].length_ticks_, ticks (200));
}

TEST_F (MidiFunctionTest, FlipHorizontalReversesPositions)
{
  add_note (0, 100, 60, 10);
  add_note (960, 200, 64, 10);
  add_note (1920, 300, 67, 10);

  const auto result = MidiFunction::FlipHorizontal::compute (all_notes (), {});

  EXPECT_EQ (result.new_states_[0].position_ticks_, ticks (1920));
  EXPECT_EQ (result.new_states_[1].position_ticks_, ticks (960));
  EXPECT_EQ (result.new_states_[2].position_ticks_, ticks (0));
  // Each note keeps its own length and pitch
  EXPECT_EQ (result.new_states_[0].length_ticks_, ticks (100));
  EXPECT_EQ (result.new_states_[2].length_ticks_, ticks (300));
  EXPECT_EQ (result.new_states_[1].pitch_, 64);
}

TEST_F (MidiFunctionTest, LegatoExtendsNotesToTheNextNote)
{
  add_note (0, 240, 60, 10);
  add_note (480, 240, 64, 10);
  add_note (960, 240, 67, 10);

  const auto result = MidiFunction::Legato::compute (all_notes (), {});

  EXPECT_EQ (result.new_states_[0].length_ticks_, ticks (480));
  EXPECT_EQ (result.new_states_[1].length_ticks_, ticks (480));
  // The last note is untouched
  EXPECT_EQ (result.new_states_[2].length_ticks_, ticks (240));
}

TEST_F (MidiFunctionTest, PortatoLeavesAGapBeforeTheNextNote)
{
  add_note (0, 240, 60, 10);
  add_note (480, 240, 64, 10);
  add_note (960, 240, 67, 10);

  const auto result = MidiFunction::Portato::compute (all_notes (), {});

  EXPECT_EQ (result.new_states_[0].length_ticks_, ticks (432));
  EXPECT_EQ (result.new_states_[1].length_ticks_, ticks (432));
  EXPECT_EQ (result.new_states_[2].length_ticks_, ticks (240));
}

TEST_F (MidiFunctionTest, StaccatoShortensAllNotes)
{
  add_note (0, 240, 60, 10);
  add_note (960, 3840, 64, 10);

  const auto result = MidiFunction::Staccato::compute (all_notes (), {});

  EXPECT_EQ (result.new_states_[0].length_ticks_, ticks (48));
  EXPECT_EQ (result.new_states_[1].length_ticks_, ticks (48));
}

TEST_F (MidiFunctionTest, FlamAddsGraceNoteAfterAndTruncatesNote)
{
  add_note (1920, 960, 60, 60);

  const auto result = MidiFunction::Flam::compute (all_notes (), flam (10, 80));

  ASSERT_EQ (result.added_notes_.size (), 1u);
  EXPECT_EQ (result.added_notes_[0].source_index_, 0u);
  expect_ticks_near (
    result.added_notes_[0].state_.position_ticks_, ticks (1939.2));
  EXPECT_EQ (result.added_notes_[0].state_.length_ticks_, ticks (960));
  EXPECT_EQ (result.added_notes_[0].state_.pitch_, 60);
  EXPECT_EQ (result.added_notes_[0].state_.velocity_, 80);
  // The note is truncated to end where the grace note begins
  expect_ticks_near (result.new_states_[0].length_ticks_, ticks (19.2));
}

TEST_F (MidiFunctionTest, FlamKeepsNoteWhenOffsetExceedsItsLength)
{
  add_note (1920, 960, 60, 60);

  const auto result =
    MidiFunction::Flam::compute (all_notes (), flam (2000, 80));

  EXPECT_EQ (result.new_states_[0].length_ticks_, ticks (960));
  expect_ticks_near (
    result.added_notes_[0].state_.position_ticks_, ticks (1920 + 3840));
}

TEST_F (MidiFunctionTest, FlamEnforcesMinimumNoteLength)
{
  add_note (1920, 960, 60, 60);

  const auto result = MidiFunction::Flam::compute (all_notes (), flam (0.1, 80));

  EXPECT_EQ (result.new_states_[0].length_ticks_, ticks (1.0));
}

TEST_F (MidiFunctionTest, FlamAddsGraceNoteBeforeForNegativeTime)
{
  add_note (1920, 960, 60, 60);

  const auto result = MidiFunction::Flam::compute (all_notes (), flam (-10, 80));

  ASSERT_EQ (result.added_notes_.size (), 1u);
  expect_ticks_near (
    result.added_notes_[0].state_.position_ticks_, ticks (1900.8));
  // The grace note runs up to the note's start
  expect_ticks_near (result.added_notes_[0].state_.length_ticks_, ticks (19.2));
  // The note itself is untouched
  EXPECT_EQ (result.new_states_[0].length_ticks_, ticks (960));
  EXPECT_EQ (result.new_states_[0].position_ticks_, ticks (1920));
}

TEST_F (MidiFunctionTest, StrumSpreadsNotesAscendingByPitch)
{
  add_note (1920, 480, 71, 10);
  add_note (1920, 480, 60, 10);
  add_note (1920, 480, 67, 10);
  add_note (1920, 480, 64, 10);

  const auto result =
    MidiFunction::Strum::compute (all_notes (), strum (true, 40));

  // Creation order: 71, 60, 67, 64
  expect_ticks_near (
    result.new_states_[0].position_ticks_, ticks (1920 + 3 * 10 * kTicksPerMs));
  expect_ticks_near (result.new_states_[1].position_ticks_, ticks (1920));
  expect_ticks_near (
    result.new_states_[2].position_ticks_, ticks (1920 + 2 * 10 * kTicksPerMs));
  expect_ticks_near (
    result.new_states_[3].position_ticks_, ticks (1920 + 10 * kTicksPerMs));
  // Lengths are untouched
  EXPECT_EQ (result.new_states_[0].length_ticks_, ticks (480));
}

TEST_F (MidiFunctionTest, StrumSpreadsNotesDescendingByPitch)
{
  add_note (1920, 480, 71, 10);
  add_note (1920, 480, 60, 10);

  const auto result =
    MidiFunction::Strum::compute (all_notes (), strum (false, 40));

  // The highest pitch starts first
  expect_ticks_near (result.new_states_[0].position_ticks_, ticks (1920));
  expect_ticks_near (
    result.new_states_[1].position_ticks_, ticks (1920 + 20 * kTicksPerMs));
}

TEST_F (MidiFunctionTest, ComputeDoesNotMutateNotes)
{
  add_note (0, 240, 55, 10);
  add_note (960, 200, 69, 10);

  (void) MidiFunction::FlipVertical::compute (all_notes (), {});

  EXPECT_EQ (notes_[0]->pitch (), 55);
  EXPECT_EQ (notes_[1]->pitch (), 69);
}

TEST_F (MidiFunctionTest, ApplyWritesComputedStates)
{
  add_note (0, 240, 55, 10);
  add_note (960, 200, 69, 10);

  MidiFunction::apply (
    all_notes (), MidiFunction::Kind{ MidiFunction::FlipVertical{} });
  MidiFunction::apply (
    all_notes (), MidiFunction::Kind{ MidiFunction::Staccato{} });

  EXPECT_EQ (notes_[0]->pitch (), 69);
  EXPECT_EQ (notes_[1]->pitch (), 55);
  EXPECT_EQ (notes_[0]->length ()->asTick (), ticks (48));
  EXPECT_EQ (notes_[1]->length ()->asTick (), ticks (48));
}

TEST_F (MidiFunctionTest, ComputeOnEmptySelectionIsEmpty)
{
  const auto result =
    MidiFunction::compute ({}, MidiFunction::Kind{ crescendo (40, 100) });
  EXPECT_TRUE (result.new_states_.empty ());
  EXPECT_TRUE (result.added_notes_.empty ());
}

// A grace-note offset is measured as real time on the timeline and mapped
// back into content space, so Flam stays correct in a clip stretched
// against its source tempo
TEST_F (MidiFunctionTest, FlamOffsetAccountsForClipStretch)
{
  utils::ObjectRegistry registry;
  auto clip = std::make_unique<MidiClip> (*tempo_map_wrapper, registry, nullptr);
  // Source BPM 100 under a 120 BPM project: content plays stretched 1.2x
  clip->timebaseProvider ()->setOverride (dsp::Timebase::Absolute);
  clip->set_source_bpm (units::bpm (100.0));

  auto note_ref = utils::create_object<MidiNote> (registry, *tempo_map_wrapper);
  auto * note = note_ref.get_object_as<MidiNote> ();
  note->position ()->set_ticks (ticks (1000.0));
  note->length ()->set_ticks (ticks (960.0));
  clip->ArrangerObjectOwner<MidiNote>::add_object (note_ref);

  std::vector<MidiNote *> notes{ note };
  const auto result = MidiFunction::Flam::compute (notes, flam (96, 100));

  // 96 ms after 0.625 s (content tick 1000 at 100 BPM) is 0.721 s,
  // which is content tick 1153.6 — not the unwarped 1184.32
  ASSERT_EQ (result.added_notes_.size (), 1u);
  expect_ticks_near (
    result.added_notes_[0].state_.position_ticks_, ticks (1153.6));
}

// A negative Flam offset that would reach before the clip start is
// clamped to the clip start
TEST_F (MidiFunctionTest, FlamGraceNoteBeforeClipStartIsClamped)
{
  auto * note = add_note (100.0, 960.0, 60, 100);

  std::vector<MidiNote *> notes{ note };
  const auto result = MidiFunction::Flam::compute (notes, flam (-100, 100));

  // 100 ms at 120 BPM is 192 ticks, so the grace note would start at -92
  ASSERT_EQ (result.added_notes_.size (), 1u);
  EXPECT_EQ (result.added_notes_[0].state_.position_ticks_, ticks (0.0));
}

// Notes sharing an onset form one event: every chord tone sustains until
// the next distinct onset
TEST_F (MidiFunctionTest, LegatoExtendsChordTonesToNextDistinctOnset)
{
  add_note (0, 240, 60, 100);
  add_note (0, 240, 64, 100);
  add_note (960, 240, 67, 100);

  const auto result = MidiFunction::Legato::compute (all_notes (), {});

  EXPECT_EQ (result.new_states_[0].length_ticks_, ticks (960));
  EXPECT_EQ (result.new_states_[1].length_ticks_, ticks (960));
  EXPECT_EQ (result.new_states_[2].length_ticks_, ticks (240));
}

// Portato behaves like legato over chord tones but leaves a gap before
// the next distinct onset
TEST_F (MidiFunctionTest, PortatoLeavesGapAfterChordTones)
{
  add_note (0, 240, 60, 100);
  add_note (0, 240, 64, 100);
  add_note (960, 240, 67, 100);

  const auto result = MidiFunction::Portato::compute (all_notes (), {});

  EXPECT_EQ (result.new_states_[0].length_ticks_, ticks (960 - 48));
  EXPECT_EQ (result.new_states_[1].length_ticks_, ticks (960 - 48));
  EXPECT_EQ (result.new_states_[2].length_ticks_, ticks (240));
}

TEST_F (MidiFunctionTest, KindFromIdFindsEveryKind)
{
  for (
    const auto &[id, expected_label] : {
      std::pair{ MidiFunction::Crescendo::kId,      utils::Utf8String (u8"Crescendo")     },
      { MidiFunction::Flam::kId,           utils::Utf8String (u8"Flam")          },
      { MidiFunction::FlipHorizontal::kId,
                utils::Utf8String (u8"Flip Horizontal")                                   },
      { MidiFunction::FlipVertical::kId,   utils::Utf8String (u8"Flip Vertical") },
      { MidiFunction::Legato::kId,         utils::Utf8String (u8"Legato")        },
      { MidiFunction::Portato::kId,        utils::Utf8String (u8"Portato")       },
      { MidiFunction::Staccato::kId,       utils::Utf8String (u8"Staccato")      },
      { MidiFunction::Strum::kId,          utils::Utf8String (u8"Strum")         }
  })
    {
      const auto kind = MidiFunction::kind_from_id (id);
      ASSERT_TRUE (kind) << id;
      EXPECT_EQ (MidiFunction::label (*kind).view (), expected_label.view ());
    }
}

TEST_F (MidiFunctionTest, KindFromIdRejectsUnknownId)
{
  EXPECT_FALSE (MidiFunction::kind_from_id ("no-such-function"));
}

} // namespace zrythm::structure::arrangement
