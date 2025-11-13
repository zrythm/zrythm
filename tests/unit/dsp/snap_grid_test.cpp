// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/snap_grid.h"
#include "dsp/tempo_map.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace zrythm::dsp
{
class SnapGridTest : public ::testing::Test
{
protected:
  static constexpr auto SAMPLE_RATE = units::sample_rate (44100.0);

  void SetUp () override
  {
    tempo_map = std::make_unique<TempoMap> (SAMPLE_RATE);
    snap_grid = std::make_unique<SnapGrid> (
      *tempo_map, utils::NoteLength ::Note_1_4, [] () { return 0.0; });
  }

  std::unique_ptr<TempoMap> tempo_map;
  std::unique_ptr<SnapGrid> snap_grid;
};

// Test initial state
TEST_F (SnapGridTest, InitialState)
{
  EXPECT_FALSE (snap_grid->snapAdaptive ());
  EXPECT_TRUE (snap_grid->snapToGrid ());
  EXPECT_FALSE (snap_grid->snapToEvents ());
  EXPECT_FALSE (snap_grid->keepOffset ());
  EXPECT_FALSE (snap_grid->sixteenthsVisible ());
  EXPECT_FALSE (snap_grid->beatsVisible ());
}

// Test snapTicks calculation with different note lengths
TEST_F (SnapGridTest, SnapTicksCalculation)
{
  // Test quarter note (1/4) - default setting
  snap_grid->setSnapAdaptive (false);
  snap_grid->setSnapToGrid (true);
  snap_grid->setSnapToEvents (false);

  // Default should be quarter note (960 ticks)
  double snap_ticks = snap_grid->snapTicks (0);
  EXPECT_DOUBLE_EQ (snap_ticks, TempoMap::get_ppq ());

  // Test eighth note (1/8)
  snap_grid->setSnapAdaptive (false);
  snap_grid->setSnapNoteLength (utils::NoteLength::Note_1_8);
  snap_ticks = snap_grid->snapTicks (0);
  EXPECT_DOUBLE_EQ (snap_ticks, TempoMap::get_ppq () / 2.0);

  // Test dotted quarter note
  snap_grid->setSnapAdaptive (false);
  snap_grid->setSnapNoteLength (utils::NoteLength::Note_1_4);
  snap_grid->setSnapNoteType (utils::NoteType::Dotted);
  snap_ticks = snap_grid->snapTicks (0);
  EXPECT_DOUBLE_EQ (snap_ticks, TempoMap::get_ppq () * 1.5);

  // Test triplet quarter note
  snap_grid->setSnapAdaptive (false);
  snap_grid->setSnapNoteLength (utils::NoteLength::Note_1_4);
  snap_grid->setSnapNoteType (utils::NoteType::Triplet);
  snap_ticks = snap_grid->snapTicks (0);
  EXPECT_DOUBLE_EQ (snap_ticks, TempoMap::get_ppq () * 2.0 / 3.0);
}

// Test adaptive snapping
TEST_F (SnapGridTest, AdaptiveSnapping)
{
  snap_grid->setSnapAdaptive (true);

  // Test sixteenths visible
  snap_grid->setSixteenthsVisible (true);
  double snap_ticks = snap_grid->snapTicks (0);
  EXPECT_DOUBLE_EQ (snap_ticks, TempoMap::get_ppq () / 4);

  // Test beats visible
  snap_grid->setSixteenthsVisible (false);
  snap_grid->setBeatsVisible (true);
  snap_ticks = snap_grid->snapTicks (0);
  EXPECT_DOUBLE_EQ (snap_ticks, TempoMap::get_ppq ());

  // Test bars visible (default when neither sixteenths nor beats are visible)
  snap_grid->setSixteenthsVisible (false);
  snap_grid->setBeatsVisible (false);
  snap_ticks = snap_grid->snapTicks (0);
  EXPECT_DOUBLE_EQ (snap_ticks, TempoMap::get_ppq () * 4);
}

// Test next/prev snap point calculation
TEST_F (SnapGridTest, NextPrevSnapPoint)
{
  snap_grid->setSnapAdaptive (false);
  snap_grid->setSnapToGrid (true);

  // Test quarter note grid (default)
  const double quarter_ticks = TempoMap::get_ppq ();

  // Test at tick 0
  EXPECT_DOUBLE_EQ (snap_grid->nextSnapPoint (0), quarter_ticks);
  EXPECT_DOUBLE_EQ (snap_grid->prevSnapPoint (0), 0);

  // Test at tick 480 (halfway through quarter)
  EXPECT_DOUBLE_EQ (snap_grid->nextSnapPoint (480), quarter_ticks);
  EXPECT_DOUBLE_EQ (snap_grid->prevSnapPoint (480), 0);

  // Test at tick 481 (just past halfway)
  EXPECT_DOUBLE_EQ (snap_grid->nextSnapPoint (481), quarter_ticks);
  EXPECT_DOUBLE_EQ (snap_grid->prevSnapPoint (481), 0);

  // Test at tick 960 (exactly on quarter)
  EXPECT_DOUBLE_EQ (snap_grid->nextSnapPoint (960), quarter_ticks * 2);
  EXPECT_DOUBLE_EQ (snap_grid->prevSnapPoint (960), quarter_ticks);
}

// Test closest snap point
TEST_F (SnapGridTest, ClosestSnapPoint)
{
  snap_grid->setSnapAdaptive (false);
  snap_grid->setSnapToGrid (true);

  const double quarter_ticks = TempoMap::get_ppq ();

  // Test exactly on grid
  EXPECT_DOUBLE_EQ (snap_grid->closestSnapPoint (0), 0);
  EXPECT_DOUBLE_EQ (snap_grid->closestSnapPoint (quarter_ticks), quarter_ticks);

  // Test near grid points
  EXPECT_DOUBLE_EQ (snap_grid->closestSnapPoint (quarter_ticks / 2 - 1), 0);
  EXPECT_DOUBLE_EQ (
    snap_grid->closestSnapPoint (quarter_ticks / 2 + 1), quarter_ticks);
}

// Test snap function
TEST_F (SnapGridTest, SnapFunction)
{
  snap_grid->setSnapAdaptive (false);
  snap_grid->setSnapToGrid (true);

  const double quarter_ticks = TempoMap::get_ppq ();

  // Test basic snapping
  EXPECT_DOUBLE_EQ (snap_grid->snap (units::ticks (0)).in (units::ticks), 0.0);
  EXPECT_DOUBLE_EQ (
    snap_grid->snap (units::ticks (quarter_ticks / 2 - 1)).in (units::ticks),
    0.0);
  EXPECT_DOUBLE_EQ (
    snap_grid->snap (units::ticks (quarter_ticks / 2 + 1)).in (units::ticks),
    quarter_ticks);
  EXPECT_DOUBLE_EQ (
    snap_grid->snap (units::ticks (quarter_ticks)).in (units::ticks),
    quarter_ticks);
}

// Test keep offset functionality
TEST_F (SnapGridTest, KeepOffset)
{
  snap_grid->setSnapAdaptive (false);
  snap_grid->setSnapToGrid (true);
  snap_grid->setKeepOffset (true);

  const double quarter_ticks = TempoMap::get_ppq ();

  // Start at 100 ticks offset from grid
  auto start_ticks = units::ticks (100);
  auto current_ticks =
    units::ticks (100 + (quarter_ticks / 2)); // Move to middle of next quarter

  // Should maintain 100 tick offset from grid
  auto snapped = snap_grid->snap (current_ticks, start_ticks);
  EXPECT_DOUBLE_EQ (snapped.in (units::ticks), quarter_ticks + 100);
}

// Test event snapping
TEST_F (SnapGridTest, EventSnapping)
{
  snap_grid->setSnapToGrid (false);
  snap_grid->setSnapToEvents (true);

  // Set up event callback that returns specific snap points
  std::vector<double> test_events = { 100, 300, 500 };
  snap_grid->set_event_callback ([test_events] (double, double) {
    return test_events;
  });

  // Test snapping to events
  EXPECT_DOUBLE_EQ (snap_grid->closestSnapPoint (0), 100);
  EXPECT_DOUBLE_EQ (snap_grid->closestSnapPoint (200), 100);
  EXPECT_DOUBLE_EQ (snap_grid->closestSnapPoint (250), 300);
  EXPECT_DOUBLE_EQ (snap_grid->closestSnapPoint (400), 300);
  EXPECT_DOUBLE_EQ (snap_grid->closestSnapPoint (450), 500);
}

// Test combined grid and event snapping
TEST_F (SnapGridTest, CombinedSnapping)
{
  snap_grid->setSnapToGrid (true);
  snap_grid->setSnapToEvents (true);

  const double quarter_ticks = TempoMap::get_ppq ();

  // Set up event callback
  std::vector<double> test_events = { quarter_ticks / 2, quarter_ticks + 10 };
  snap_grid->set_event_callback ([test_events] (double, double) {
    return test_events;
  });

  // Should snap to closest point (grid or event)
  EXPECT_DOUBLE_EQ (snap_grid->closestSnapPoint (quarter_ticks / 4), 0);
  EXPECT_DOUBLE_EQ (
    snap_grid->closestSnapPoint ((quarter_ticks / 4) + 1), quarter_ticks / 2);
  EXPECT_DOUBLE_EQ (
    snap_grid->closestSnapPoint (quarter_ticks - 5), quarter_ticks);
  EXPECT_DOUBLE_EQ (
    snap_grid->closestSnapPoint (quarter_ticks + 5), quarter_ticks);
  EXPECT_DOUBLE_EQ (
    snap_grid->closestSnapPoint (quarter_ticks + 6), quarter_ticks + 10);
}

// Test default ticks calculation
TEST_F (SnapGridTest, DefaultTicks)
{
  // Test link to snap setting
  snap_grid->setSnapAdaptive (false);
  snap_grid->setLengthType (SnapGrid::NoteLengthType::Link);
  double snap_ticks = snap_grid->snapTicks (0);
  double default_ticks = snap_grid->defaultTicks (0);
  EXPECT_DOUBLE_EQ (default_ticks, snap_ticks);

  // Test last object length
  double last_object_length = 123.0;
  snap_grid->set_last_object_length_callback ([last_object_length] () {
    return last_object_length;
  });
  snap_grid->setLengthType (SnapGrid::NoteLengthType::LastObject);
  default_ticks = snap_grid->defaultTicks (0);
  EXPECT_DOUBLE_EQ (default_ticks, last_object_length);

  // Test custom length
  snap_grid->setSnapAdaptive (false);
  snap_grid->setLengthType (SnapGrid::NoteLengthType::Custom);
  default_ticks = snap_grid->defaultTicks (0);
  EXPECT_DOUBLE_EQ (default_ticks, TempoMap::get_ppq ());
}

// Test time signature changes affect snap calculation
TEST_F (SnapGridTest, TimeSignatureChanges)
{
  // Change to 3/4 time signature
  tempo_map->add_time_signature_event (units::ticks (0), 3, 4);

  snap_grid->setSnapAdaptive (false);
  snap_grid->setSnapNoteLength (utils::NoteLength::Bar);
  snap_grid->setSnapNoteType (utils::NoteType::Normal);

  // Bar length should be 3 * quarter notes = 3 * 960 = 2880 ticks
  double bar_ticks = snap_grid->snapTicks (0);
  EXPECT_DOUBLE_EQ (bar_ticks, TempoMap::get_ppq () * 3);
}

// Test string representation
TEST_F (SnapGridTest, StringRepresentation)
{
  snap_grid->setSnapAdaptive (false);

  QString snap_str = snap_grid->snapString ();
  EXPECT_FALSE (snap_str.isEmpty ());

  // Test adaptive string
  snap_grid->setSnapAdaptive (true);
  snap_str = snap_grid->snapString ();
  EXPECT_EQ (snap_str, "Adaptive");
}

// Test property changes emit signals
TEST_F (SnapGridTest, PropertySignals)
{
  testing::MockFunction<void ()> mock_handler;
  // 18 expected calls: 9 property changes + 9 snapChanged signals
  EXPECT_CALL (mock_handler, Call ()).Times (18);

  QObject::connect (
    snap_grid.get (), &SnapGrid::snapAdaptiveChanged, snap_grid.get (),
    mock_handler.AsStdFunction ());
  QObject::connect (
    snap_grid.get (), &SnapGrid::snapToGridChanged, snap_grid.get (),
    mock_handler.AsStdFunction ());
  QObject::connect (
    snap_grid.get (), &SnapGrid::snapToEventsChanged, snap_grid.get (),
    mock_handler.AsStdFunction ());
  QObject::connect (
    snap_grid.get (), &SnapGrid::keepOffsetChanged, snap_grid.get (),
    mock_handler.AsStdFunction ());
  QObject::connect (
    snap_grid.get (), &SnapGrid::sixteenthsVisibleChanged, snap_grid.get (),
    mock_handler.AsStdFunction ());
  QObject::connect (
    snap_grid.get (), &SnapGrid::beatsVisibleChanged, snap_grid.get (),
    mock_handler.AsStdFunction ());
  QObject::connect (
    snap_grid.get (), &SnapGrid::snapNoteLengthChanged, snap_grid.get (),
    mock_handler.AsStdFunction ());
  QObject::connect (
    snap_grid.get (), &SnapGrid::snapNoteTypeChanged, snap_grid.get (),
    mock_handler.AsStdFunction ());
  QObject::connect (
    snap_grid.get (), &SnapGrid::lengthTypeChanged, snap_grid.get (),
    mock_handler.AsStdFunction ());
  QObject::connect (
    snap_grid.get (), &SnapGrid::snapChanged, snap_grid.get (),
    mock_handler.AsStdFunction ());

  // Each of these should trigger their specific signal + snapChanged
  snap_grid->setSnapAdaptive (true);
  snap_grid->setSnapToGrid (false);
  snap_grid->setSnapToEvents (true);
  snap_grid->setKeepOffset (true);
  snap_grid->setSixteenthsVisible (true);
  snap_grid->setBeatsVisible (true);
  snap_grid->setSnapNoteLength (utils::NoteLength::Note_1_8);
  snap_grid->setSnapNoteType (utils::NoteType::Dotted);
  snap_grid->setLengthType (SnapGrid::NoteLengthType::Custom);
}

// Test edge cases
TEST_F (SnapGridTest, EdgeCases)
{
  // Test negative ticks
  EXPECT_DOUBLE_EQ (
    snap_grid->snap (units::ticks (-100)).in (units::ticks), 0.0);

  // Test very large ticks
  auto large_ticks = units::ticks (1000000.0);
  auto snapped = snap_grid->snap (large_ticks);
  EXPECT_NEAR (snapped.in (units::ticks), 1000000.0, TempoMap::get_ppq ());

  // Test empty event callback
  snap_grid->setSnapToGrid (false);
  snap_grid->setSnapToEvents (true);
  snap_grid->set_event_callback ([] (double, double) {
    return std::vector<double>{};
  });
  EXPECT_DOUBLE_EQ (snap_grid->closestSnapPoint (100), 100);
}

// Test serialization/deserialization
TEST_F (SnapGridTest, Serialization)
{
  // Set various properties
  snap_grid->setSnapAdaptive (true);
  snap_grid->setSnapToGrid (false);
  snap_grid->setSnapToEvents (true);
  snap_grid->setKeepOffset (true);
  snap_grid->setSixteenthsVisible (true);
  snap_grid->setBeatsVisible (true);

  // Serialize to JSON
  nlohmann::json j;
  j = *snap_grid;

  // Create new snap grid and deserialize
  SnapGrid new_snap_grid (*tempo_map, utils::NoteLength::Note_1_4, [] () {
    return 0.0;
  });
  j.get_to (new_snap_grid);

  // Verify properties (note: sixteenthsVisible and beatsVisible are not
  // serialized)
  EXPECT_EQ (new_snap_grid.snapAdaptive (), snap_grid->snapAdaptive ());
  EXPECT_EQ (new_snap_grid.snapToGrid (), snap_grid->snapToGrid ());
  EXPECT_EQ (new_snap_grid.snapToEvents (), snap_grid->snapToEvents ());
  EXPECT_EQ (new_snap_grid.keepOffset (), snap_grid->keepOffset ());
}

// Test note length and type property changes
TEST_F (SnapGridTest, NoteLengthTypeProperties)
{
  // Test initial values
  EXPECT_EQ (snap_grid->snapNoteLength (), utils::NoteLength::Note_1_4);
  EXPECT_EQ (snap_grid->snapNoteType (), utils::NoteType::Normal);

  // Test setting note length
  snap_grid->setSnapNoteLength (utils::NoteLength::Note_1_8);
  EXPECT_EQ (snap_grid->snapNoteLength (), utils::NoteLength::Note_1_8);

  snap_grid->setSnapNoteLength (utils::NoteLength::Note_1_16);
  EXPECT_EQ (snap_grid->snapNoteLength (), utils::NoteLength::Note_1_16);

  snap_grid->setSnapNoteLength (utils::NoteLength::Bar);
  EXPECT_EQ (snap_grid->snapNoteLength (), utils::NoteLength::Bar);

  // Test setting note type
  snap_grid->setSnapNoteType (utils::NoteType::Dotted);
  EXPECT_EQ (snap_grid->snapNoteType (), utils::NoteType::Dotted);

  snap_grid->setSnapNoteType (utils::NoteType::Triplet);
  EXPECT_EQ (snap_grid->snapNoteType (), utils::NoteType::Triplet);

  snap_grid->setSnapNoteType (utils::NoteType::Normal);
  EXPECT_EQ (snap_grid->snapNoteType (), utils::NoteType::Normal);

  // Test that snap ticks calculation respects note length and type
  snap_grid->setSnapNoteLength (utils::NoteLength::Note_1_4);
  snap_grid->setSnapNoteType (utils::NoteType::Normal);
  EXPECT_DOUBLE_EQ (snap_grid->snapTicks (0), TempoMap::get_ppq ());

  snap_grid->setSnapNoteLength (utils::NoteLength::Note_1_8);
  EXPECT_DOUBLE_EQ (snap_grid->snapTicks (0), TempoMap::get_ppq () / 2);

  snap_grid->setSnapNoteLength (utils::NoteLength::Note_1_4);
  snap_grid->setSnapNoteType (utils::NoteType::Dotted);
  EXPECT_DOUBLE_EQ (snap_grid->snapTicks (0), TempoMap::get_ppq () * 1.5);

  snap_grid->setSnapNoteType (utils::NoteType::Triplet);
  EXPECT_DOUBLE_EQ (snap_grid->snapTicks (0), TempoMap::get_ppq () * 2.0 / 3.0);
}

// Test that demonstrates the bug: snap points don't respect time signature
// changes
TEST_F (SnapGridTest, SnapPointsRespectTimeSignatureChanges)
{
  // Set up time signature change: 4/4 for first 2 bars, then 3/4
  // Bar 1: ticks 0-3839 (4/4)
  // Bar 2: ticks 3840-7679 (4/4)
  // Bar 3 onwards: ticks 7680-10559 (3/4)
  tempo_map->add_time_signature_event (units::ticks (0), 4, 4);
  tempo_map->add_time_signature_event (units::ticks (7680), 3, 4);

  snap_grid->setSnapAdaptive (false);
  snap_grid->setSnapToGrid (true);
  snap_grid->setSnapNoteLength (utils::NoteLength::Beat);
  snap_grid->setSnapNoteType (utils::NoteType::Normal);

  // In 4/4 time: each beat is 960 ticks
  // In 3/4 time: each beat is also 960 ticks (same denominator)
  // But bar boundaries are different: 4/4 = 3840 ticks/bar, 3/4 = 2880 ticks/bar

  // Test before time signature change - should snap to beat 0, 960, 1920, 2880,
  // 3840
  EXPECT_DOUBLE_EQ (snap_grid->nextSnapPoint (0), 960);
  EXPECT_DOUBLE_EQ (snap_grid->nextSnapPoint (500), 960);
  EXPECT_DOUBLE_EQ (snap_grid->prevSnapPoint (500), 0);
  EXPECT_DOUBLE_EQ (snap_grid->prevSnapPoint (960), 960);
  EXPECT_DOUBLE_EQ (snap_grid->nextSnapPoint (960), 1920);
  EXPECT_DOUBLE_EQ (snap_grid->prevSnapPoint (1920), 1920);

  // Test at bar boundary (tick 3840) - should snap to beat 4 of bar 2
  EXPECT_DOUBLE_EQ (snap_grid->nextSnapPoint (3840), 4800); // 3840 + 960
  EXPECT_DOUBLE_EQ (snap_grid->prevSnapPoint (3840), 3840); // 3840
  EXPECT_DOUBLE_EQ (snap_grid->prevSnapPoint (3839), 2880); // 3840 - 960

  // Test across time signature change at tick 7680
  // At tick 7680, we're at the start of a 3/4 bar
  // The next beat should be at 8640 (7680 + 960)
  // The previous beat should be at 6720 (7680 - 960)
  EXPECT_DOUBLE_EQ (snap_grid->nextSnapPoint (7680), 8640);
  EXPECT_DOUBLE_EQ (snap_grid->prevSnapPoint (7680), 7680);
  EXPECT_DOUBLE_EQ (snap_grid->prevSnapPoint (7679), 6720);

  // Test within the 3/4 section - should still snap to proper beat boundaries
  EXPECT_DOUBLE_EQ (snap_grid->nextSnapPoint (8000), 8640);
  EXPECT_DOUBLE_EQ (snap_grid->prevSnapPoint (8000), 7680);
  EXPECT_DOUBLE_EQ (snap_grid->nextSnapPoint (8640), 9600);
  EXPECT_DOUBLE_EQ (snap_grid->prevSnapPoint (8640), 8640);
  EXPECT_DOUBLE_EQ (snap_grid->prevSnapPoint (8639), 7680);

  // Test at the end of the 3/4 bar (tick 10560)
  // Bar 3: 7680-10559 (3 beats: 7680, 8640, 9600)
  // Next bar starts at 10560
  EXPECT_DOUBLE_EQ (snap_grid->nextSnapPoint (9600), 10560); // Next bar start
  EXPECT_DOUBLE_EQ (snap_grid->prevSnapPoint (10560), 10560);
  EXPECT_DOUBLE_EQ (snap_grid->prevSnapPoint (10559), 9600); // Previous beat
}

// Test that demonstrates the bug with more complex time signature changes
TEST_F (SnapGridTest, SnapPointsWithComplexTimeSignatureChanges)
{
  // Set up: 4/4 for 1 bar, then 6/8 for 2 bars, then 5/4
  // 4/4: 4 quarter notes per bar = 3840 ticks/bar, 960 ticks/beat
  // 6/8: 6 eighth notes per bar = 2880 ticks/bar, 480 ticks/beat (eighth note
  // gets the beat) 5/4: 5 quarter notes per bar = 4800 ticks/bar, 960 ticks/beat

  tempo_map->add_time_signature_event (units::ticks (0), 4, 4);
  tempo_map->add_time_signature_event (units::ticks (3840), 6, 8);
  tempo_map->add_time_signature_event (
    units::ticks (3840 + 5760), 5, 4); // 3840 + 2*2880

  snap_grid->setSnapAdaptive (false);
  snap_grid->setSnapToGrid (true);
  snap_grid->setSnapNoteLength (utils::NoteLength::Beat);
  snap_grid->setSnapNoteType (utils::NoteType::Normal);

  // Test in 4/4 section - beats at 0, 960, 1920, 2880
  EXPECT_DOUBLE_EQ (snap_grid->nextSnapPoint (0), 960);
  EXPECT_DOUBLE_EQ (snap_grid->prevSnapPoint (1000), 960);

  // Test at 4/4 to 6/8 transition at tick 3840
  // In 6/8, beats are at 3840, 4320, 4800, 5280, 5760, 6240 (every 480 ticks)
  EXPECT_DOUBLE_EQ (snap_grid->nextSnapPoint (3840), 4320);
  EXPECT_DOUBLE_EQ (snap_grid->prevSnapPoint (3840), 3840);
  EXPECT_DOUBLE_EQ (
    snap_grid->prevSnapPoint (3839), 2880); // Last beat of previous 4/4 bar

  // Test within 6/8 section
  EXPECT_DOUBLE_EQ (snap_grid->nextSnapPoint (4000), 4320);
  EXPECT_DOUBLE_EQ (snap_grid->prevSnapPoint (4000), 3840);
  EXPECT_DOUBLE_EQ (snap_grid->nextSnapPoint (4320), 4800);
  EXPECT_DOUBLE_EQ (snap_grid->prevSnapPoint (4320), 4320);
  EXPECT_DOUBLE_EQ (snap_grid->prevSnapPoint (4319), 3840);

  // Test at 6/8 to 5/4 transition
  // 6/8 section: 3840-9599 (2 bars * 2880 ticks/bar)
  // 5/4 section starts at 9600
  // In 5/4, beats are at 9600, 10560, 11520, 12480, 13440 (every 960 ticks)
  EXPECT_DOUBLE_EQ (snap_grid->nextSnapPoint (9600), 10560);
  EXPECT_DOUBLE_EQ (snap_grid->prevSnapPoint (9600), 9600);
  EXPECT_DOUBLE_EQ (
    snap_grid->prevSnapPoint (9599),
    9120); // Last beat of 6/8 section (3840 + 5760 + 480)
}

} // namespace zrythm::dsp
