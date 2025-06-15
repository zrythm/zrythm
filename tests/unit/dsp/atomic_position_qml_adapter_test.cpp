// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/atomic_position.h"
#include "dsp/atomic_position_qml_adapter.h"
#include "dsp/tempo_map.h"

#include <QSignalSpy>

#include <gtest/gtest.h>

namespace zrythm::dsp
{
class AtomicPositionQmlAdapterTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    tempo_map = std::make_unique<TempoMap> (44100.0);
    atomic_pos = std::make_unique<AtomicPosition> (*tempo_map);
    qml_pos = std::make_unique<AtomicPositionQmlAdapter> (*atomic_pos);
  }

  std::unique_ptr<TempoMap>                 tempo_map;
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

// Test string display
TEST_F (AtomicPositionQmlAdapterTest, StringDisplay)
{
  qml_pos->setTicks (241); // 1.1.2.1
  EXPECT_EQ (qml_pos->getStringDisplay ().toStdString (), "1.1.2.001");

  // Test with time signature change
  tempo_map->addTimeSignatureEvent (0, 3, 4);
  qml_pos->setTicks (960); // 1.2.1.0 in 3/4
  EXPECT_EQ (qml_pos->getStringDisplay ().toStdString (), "1.2.1.000");
}

// Test thread safety through signals
TEST_F (AtomicPositionQmlAdapterTest, ThreadSafetySignals)
{
  QSignalSpy posSpy (qml_pos.get (), &AtomicPositionQmlAdapter::positionChanged);

  // Simulate DSP thread update
  atomic_pos->set_ticks (960.0);

  // Should trigger signal emission
  Q_EMIT qml_pos->positionChanged ();
  EXPECT_EQ (posSpy.count (), 1);
  EXPECT_DOUBLE_EQ (qml_pos->ticks (), 960.0);
}

// Test edge cases
TEST_F (AtomicPositionQmlAdapterTest, EdgeCases)
{
  // Negative values (clamped)
  qml_pos->setTicks (-100.0);
  EXPECT_DOUBLE_EQ (qml_pos->ticks (), 0.0);

  qml_pos->setSeconds (-1.0);
  EXPECT_DOUBLE_EQ (qml_pos->seconds (), 0.0);

  // Large values
  qml_pos->setTicks (1e9);
  EXPECT_GT (qml_pos->seconds (), 0.0);
}

// Test tempo map changes
TEST_F (AtomicPositionQmlAdapterTest, TempoMapChanges)
{
  qml_pos->setTicks (960.0);

  // Change tempo (120 -> 140 BPM)
  tempo_map->addEvent (0, 140.0, TempoMap::CurveType::Constant);

  // Verify seconds updated
  EXPECT_DOUBLE_EQ (qml_pos->seconds (), 60.0 / 140.0);
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
} // namespace zrythm::dsp
