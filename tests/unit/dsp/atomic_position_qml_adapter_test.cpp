// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/atomic_position.h"
#include "dsp/atomic_position_qml_adapter.h"

#include <QSignalSpy>

#include "unit/dsp/atomic_position_helpers.h"
#include <gtest/gtest.h>

namespace zrythm::dsp
{
class AtomicPositionQmlAdapterTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    // Use custom conversion providers that support negative positions
    // 120 BPM = 960 ticks per beat, 0.5 seconds per beat
    time_conversion_funcs = basic_conversion_providers ();
    atomic_pos = std::make_unique<AtomicPosition> (*time_conversion_funcs);
    qml_pos =
      std::make_unique<AtomicPositionQmlAdapter> (*atomic_pos, std::nullopt);
  }

  std::unique_ptr<AtomicPosition::TimeConversionFunctions> time_conversion_funcs;
  std::unique_ptr<AtomicPosition>           atomic_pos;
  std::unique_ptr<AtomicPositionQmlAdapter> qml_pos;
};

// Test initial state
TEST_F (AtomicPositionQmlAdapterTest, InitialState)
{
  EXPECT_EQ (qml_pos->mode (), TimeFormat::Musical);
  EXPECT_DOUBLE_EQ (qml_pos->ticks (), 0.0);
  EXPECT_DOUBLE_EQ (qml_pos->seconds (), 0.0);
  EXPECT_EQ (qml_pos->samples (), 0);
}

// Test property bindings in Musical mode
TEST_F (AtomicPositionQmlAdapterTest, MusicalModePropertyBindings)
{
  QSignalSpy posSpy (qml_pos.get (), &AtomicPositionQmlAdapter::positionChanged);

  qml_pos->setTicks (960.0);
  EXPECT_EQ (posSpy.count (), 1);
  EXPECT_DOUBLE_EQ (qml_pos->ticks (), 960.0);
  EXPECT_DOUBLE_EQ (qml_pos->seconds (), 0.5);
  EXPECT_EQ (qml_pos->samples (), 22050);
}

// Test property bindings in Absolute mode
TEST_F (AtomicPositionQmlAdapterTest, AbsoluteModePropertyBindings)
{
  QSignalSpy posSpy (qml_pos.get (), &AtomicPositionQmlAdapter::positionChanged);

  qml_pos->setMode (TimeFormat::Absolute);
  EXPECT_EQ (posSpy.count (), 1);

  qml_pos->setSeconds (1.0);
  EXPECT_EQ (posSpy.count (), 2);
  EXPECT_DOUBLE_EQ (qml_pos->seconds (), 1.0);
  EXPECT_DOUBLE_EQ (qml_pos->ticks (), 1920.0);
  EXPECT_EQ (qml_pos->samples (), 44100);
}

// Test mode conversion signals
TEST_F (AtomicPositionQmlAdapterTest, ModeConversionSignals)
{
  QSignalSpy posSpy (qml_pos.get (), &AtomicPositionQmlAdapter::positionChanged);

  qml_pos->setMode (TimeFormat::Absolute);
  EXPECT_EQ (posSpy.count (), 1);
  EXPECT_EQ (qml_pos->mode (), TimeFormat::Absolute);

  qml_pos->setMode (TimeFormat::Musical);
  EXPECT_EQ (posSpy.count (), 2);
  EXPECT_EQ (qml_pos->mode (), TimeFormat::Musical);
}

// Test samples property
TEST_F (AtomicPositionQmlAdapterTest, SamplesProperty)
{
  // Test musical mode
  qml_pos->setTicks (480.0);
  EXPECT_EQ (qml_pos->samples (), 22050 / 2);

  // Test absolute mode
  qml_pos->setMode (TimeFormat::Absolute);
  qml_pos->setSeconds (0.25);
  EXPECT_EQ (qml_pos->samples (), 11025);
}

// Test thread safety through signals
TEST_F (AtomicPositionQmlAdapterTest, ThreadSafetySignals)
{
  QSignalSpy posSpy (qml_pos.get (), &AtomicPositionQmlAdapter::positionChanged);

  // Simulate DSP thread update
  atomic_pos->set_ticks (units::ticks (960.0));

  // Should trigger signal emission
  Q_EMIT qml_pos->positionChanged ();
  EXPECT_EQ (posSpy.count (), 1);
  EXPECT_DOUBLE_EQ (qml_pos->ticks (), 960.0);
}

// Test edge cases
TEST_F (AtomicPositionQmlAdapterTest, EdgeCases)
{
  // Negative values
  qml_pos->setTicks (-100.0);
  EXPECT_DOUBLE_EQ (qml_pos->ticks (), -100.0);
  EXPECT_DOUBLE_EQ (qml_pos->seconds (), -100.0 / 960.0 * 0.5);

  qml_pos->setSeconds (-1.0);
  EXPECT_DOUBLE_EQ (qml_pos->seconds (), -1.0);
  EXPECT_DOUBLE_EQ (qml_pos->ticks (), -1.0 / 0.5 * 960.0);

  // Large values
  qml_pos->setTicks (1e9);
  EXPECT_GT (qml_pos->seconds (), 0.0);
}

// Test QML property bindings
TEST_F (AtomicPositionQmlAdapterTest, QmlPropertyBindings)
{
  // Verify property system integration
  QObject dummy;
  QObject::connect (
    qml_pos.get (), &AtomicPositionQmlAdapter::positionChanged, &dummy, [] () {
      // Slot would update QML bindings
    });

  qml_pos->setTicks (960.0);
  EXPECT_DOUBLE_EQ (qml_pos->property ("ticks").toDouble (), 960.0);
}
// Test constraint function - non-negative values
TEST_F (AtomicPositionQmlAdapterTest, NonNegativeConstraint)
{
  // Create a new adapter with non-negative constraint
  auto non_negative_constraint = [] (units::precise_tick_t ticks) {
    return std::max (ticks, units::ticks (0.0));
  };
  AtomicPositionQmlAdapter qml_pos_constrained (
    *atomic_pos, non_negative_constraint);

  // Test that negative values are clamped to 0
  qml_pos_constrained.setTicks (-100.0);
  EXPECT_DOUBLE_EQ (qml_pos_constrained.ticks (), 0.0);
  EXPECT_DOUBLE_EQ (qml_pos_constrained.seconds (), 0.0);
  EXPECT_EQ (qml_pos_constrained.samples (), 0);

  qml_pos_constrained.setSeconds (-1.0);
  EXPECT_DOUBLE_EQ (qml_pos_constrained.seconds (), 0.0);
  EXPECT_DOUBLE_EQ (qml_pos_constrained.ticks (), 0.0);
  EXPECT_EQ (qml_pos_constrained.samples (), 0);

  // Test that positive values work normally
  qml_pos_constrained.setTicks (960.0);
  EXPECT_DOUBLE_EQ (qml_pos_constrained.ticks (), 960.0);
  EXPECT_DOUBLE_EQ (qml_pos_constrained.seconds (), 0.5);
  EXPECT_EQ (qml_pos_constrained.samples (), 22050);

  qml_pos_constrained.setSeconds (1.0);
  EXPECT_DOUBLE_EQ (qml_pos_constrained.seconds (), 1.0);
  EXPECT_DOUBLE_EQ (qml_pos_constrained.ticks (), 1920.0);
  EXPECT_EQ (qml_pos_constrained.samples (), 44100);
}

// Test complex constraint function
TEST_F (AtomicPositionQmlAdapterTest, ComplexConstraint)
{
  // Create a constraint that enforces minimum value of 480 ticks (1/8 note)
  auto min_length_constraint = [] (units::precise_tick_t ticks) {
    return std::max (ticks, units::ticks (480.0));
  };
  AtomicPositionQmlAdapter qml_pos_min_length (
    *atomic_pos, min_length_constraint);

  // Test that values below minimum are clamped
  qml_pos_min_length.setTicks (100.0);
  EXPECT_DOUBLE_EQ (qml_pos_min_length.ticks (), 480.0);

  qml_pos_min_length.setSeconds (
    0.1); // Should be converted to ticks then constrained
  EXPECT_GE (qml_pos_min_length.ticks (), 480.0);

  // Test that values above minimum work normally
  qml_pos_min_length.setTicks (2000.0);
  EXPECT_DOUBLE_EQ (qml_pos_min_length.ticks (), 2000.0);
}

// Test no constraint (optional)
TEST_F (AtomicPositionQmlAdapterTest, NoConstraint)
{
  // Create adapter with no constraint (std::nullopt)
  AtomicPositionQmlAdapter qml_pos_unconstrained (*atomic_pos, std::nullopt);

  // Test that negative values are allowed
  qml_pos_unconstrained.setTicks (-100.0);
  EXPECT_DOUBLE_EQ (qml_pos_unconstrained.ticks (), -100.0);

  qml_pos_unconstrained.setSeconds (-1.0);
  EXPECT_DOUBLE_EQ (qml_pos_unconstrained.seconds (), -1.0);

  // Test that positive values work normally
  qml_pos_unconstrained.setTicks (960.0);
  EXPECT_DOUBLE_EQ (qml_pos_unconstrained.ticks (), 960.0);
}

} // namespace zrythm::dsp
