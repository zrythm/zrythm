// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/playhead.h"
#include "dsp/playhead_qml_adapter.h"
#include "dsp/tempo_map.h"

#include <QSignalSpy>
#include <QTest>

#include "helpers/scoped_qcoreapplication.h"

#include <gtest/gtest.h>

namespace zrythm::dsp
{

class PlayheadQmlWrapperTest
    : public ::testing::Test,
      public test_helpers::ScopedQCoreApplication
{
protected:
  static constexpr auto SAMPLE_RATE = 44100.0;

  void SetUp () override
  {
    tempo_map_ = std::make_unique<dsp::TempoMap> (SAMPLE_RATE);
    playhead_ = std::make_unique<dsp::Playhead> (*tempo_map_);
    wrapper_ = std::make_unique<PlayheadQmlWrapper> (*playhead_);
  }

  std::unique_ptr<dsp::TempoMap>      tempo_map_;
  std::unique_ptr<dsp::Playhead>      playhead_;
  std::unique_ptr<PlayheadQmlWrapper> wrapper_;
};

// Test initial state
TEST_F (PlayheadQmlWrapperTest, InitialState)
{
  EXPECT_DOUBLE_EQ (wrapper_->ticks (), 0.0);
}

// Test ticks property
TEST_F (PlayheadQmlWrapperTest, TicksProperty)
{
  QSignalSpy spy (wrapper_.get (), &PlayheadQmlWrapper::ticksChanged);

  const double testTicks = 1920.0; // 2 beats at 120 BPM
  wrapper_->setTicks (testTicks);

  // Verify property value
  EXPECT_DOUBLE_EQ (wrapper_->ticks (), testTicks);

  // Verify signal emission
  EXPECT_EQ (spy.count (), 1);

  // Verify underlying playhead was updated
  EXPECT_DOUBLE_EQ (playhead_->position_ticks (), testTicks);
}

// Test no signal on insignificant change
TEST_F (PlayheadQmlWrapperTest, NoSignalOnInsignificantChange)
{
  QSignalSpy spy (wrapper_.get (), &PlayheadQmlWrapper::ticksChanged);

  // Set position with insignificant change
  wrapper_->setTicks (0.0000001);

  // Should not emit signal
  EXPECT_EQ (spy.count (), 0);
}

// Test periodic updates
TEST_F (PlayheadQmlWrapperTest, PeriodicUpdates)
{
  QSignalSpy spy (wrapper_.get (), &PlayheadQmlWrapper::ticksChanged);

  // Advance playhead
  playhead_->set_position_ticks (960.0);

  // Wait for timer to trigger
  spy.wait (50);

  // Verify property updated
  EXPECT_DOUBLE_EQ (wrapper_->ticks (), 960.0);

  // Verify signal was emitted
  EXPECT_GE (spy.count (), 1);
}

// Test concurrent access safety
TEST_F (PlayheadQmlWrapperTest, ConcurrentAccess)
{
  QSignalSpy spy (wrapper_.get (), &PlayheadQmlWrapper::ticksChanged);

  // Set position from GUI thread
  wrapper_->setTicks (960.0);

  // Simulate audio thread activity
  std::thread audioThread ([this] () {
    // Advance playhead
    playhead_->set_position_ticks (1920.0);
  });

  audioThread.join ();

  // Wait for timer to trigger
  spy.wait (50); // > 33ms interval

  // Verify value updated
  EXPECT_DOUBLE_EQ (wrapper_->ticks (), 1920.0);
}

} // namespace zrythm::dsp
