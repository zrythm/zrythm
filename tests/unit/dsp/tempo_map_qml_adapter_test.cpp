// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/tempo_map_qml_adapter.h"

#include <QCoreApplication>
#include <QSignalSpy>

#include "helpers/scoped_qcoreapplication.h"

#include <gtest/gtest.h>

namespace zrythm::dsp
{
class TempoMapWrapperTest
    : public ::testing::Test,
      private test_helpers::ScopedQCoreApplication
{
protected:
  void SetUp () override
  {
    // Create core tempo map and wrapper
    tempo_map_ = std::make_unique<TempoMap> (44100.0);
    wrapper_ = std::make_unique<TempoMapWrapper> (*tempo_map_);
  }

  void TearDown () override
  {
    wrapper_.reset ();
    tempo_map_.reset ();
  }

  std::unique_ptr<TempoMap>        tempo_map_;
  std::unique_ptr<TempoMapWrapper> wrapper_;
};

// Test initial state
TEST_F (TempoMapWrapperTest, InitialState)
{
  // Verify initial tempo event
  auto tempo_events = wrapper_->tempoEvents ();
  ASSERT_EQ (tempo_events.count (&tempo_events), 0);

  auto first_tempo = wrapper_->tempoAtTick (0);
  EXPECT_DOUBLE_EQ (first_tempo, 120.0);

  // Verify initial time signature
  auto time_sig_events = wrapper_->timeSignatureEvents ();
  ASSERT_EQ (time_sig_events.count (&time_sig_events), 0);

  auto first_sig = wrapper_->timeSignatureAtTick (0);
  EXPECT_EQ (first_sig->tick (), 0);
  EXPECT_EQ (first_sig->numerator (), 4);
  EXPECT_EQ (first_sig->denominator (), 4);

  // Verify sample rate
  EXPECT_DOUBLE_EQ (wrapper_->sampleRate (), 44100.0);
}

// Test tempo event signals and wrappers
TEST_F (TempoMapWrapperTest, TempoEventManagement)
{
  QSignalSpy spy (wrapper_.get (), &TempoMapWrapper::tempoEventsChanged);

  // Add tempo event
  wrapper_->addTempoEvent (
    1920, 140.0, static_cast<int> (TempoMap::CurveType::Constant));
  EXPECT_EQ (spy.count (), 1);

  // Verify wrapper count
  auto tempo_events = wrapper_->tempoEvents ();
  EXPECT_EQ (tempo_events.count (&tempo_events), 2);

  // Verify new event properties
  auto new_event = tempo_events.at (&tempo_events, 1);
  EXPECT_EQ (new_event->tick (), 1920);
  EXPECT_DOUBLE_EQ (new_event->bpm (), 140.0);
  EXPECT_EQ (new_event->curve (), TempoMap::CurveType::Constant);
}

// Test time signature signals and wrappers
TEST_F (TempoMapWrapperTest, TimeSignatureManagement)
{
  QSignalSpy spy (wrapper_.get (), &TempoMapWrapper::timeSignatureEventsChanged);

  // Add time signature
  wrapper_->addTimeSignatureEvent (1920, 3, 4);
  EXPECT_EQ (spy.count (), 1);

  // Verify wrapper count
  auto time_sig_events = wrapper_->timeSignatureEvents ();
  EXPECT_EQ (time_sig_events.count (&time_sig_events), 2);

  // Verify new event properties
  auto new_sig = time_sig_events.at (&time_sig_events, 1);
  EXPECT_EQ (new_sig->tick (), 1920);
  EXPECT_EQ (new_sig->numerator (), 3);
  EXPECT_EQ (new_sig->denominator (), 4);
}

// Test sample rate changes
TEST_F (TempoMapWrapperTest, SampleRateChanges)
{
  QSignalSpy spy (wrapper_.get (), &TempoMapWrapper::sampleRateChanged);

  // Change sample rate
  wrapper_->setSampleRate (48000.0);
  EXPECT_EQ (spy.count (), 1);
  EXPECT_DOUBLE_EQ (wrapper_->sampleRate (), 48000.0);
  EXPECT_DOUBLE_EQ (tempo_map_->get_sample_rate (), 48000.0);

  // No signal on same value
  wrapper_->setSampleRate (48000.0);
  EXPECT_EQ (spy.count (), 1);
}

// Test list property access
TEST_F (TempoMapWrapperTest, ListPropertyAccess)
{
  // Add multiple events
  wrapper_->addTimeSignatureEvent (1920, 3, 4);
  wrapper_->addTimeSignatureEvent (3840, 5, 8);
  wrapper_->addTempoEvent (
    1920, 140.0, static_cast<int> (TempoMap::CurveType::Constant));
  wrapper_->addTempoEvent (
    3840, 160.0, static_cast<int> (TempoMap::CurveType::Linear));

  // Verify tempo events
  auto tempo_events = wrapper_->tempoEvents ();
  ASSERT_EQ (tempo_events.count (&tempo_events), 3);

  // Verify order (should be sorted by tick)
  EXPECT_EQ (tempo_events.at (&tempo_events, 0)->tick (), 0);
  EXPECT_EQ (tempo_events.at (&tempo_events, 1)->tick (), 1920);
  EXPECT_EQ (tempo_events.at (&tempo_events, 2)->tick (), 3840);

  // Verify time signature events
  auto time_sig_events = wrapper_->timeSignatureEvents ();
  ASSERT_EQ (time_sig_events.count (&time_sig_events), 3);
  EXPECT_EQ (time_sig_events.at (&time_sig_events, 0)->tick (), 0);
  EXPECT_EQ (time_sig_events.at (&time_sig_events, 1)->tick (), 1920);
  EXPECT_EQ (time_sig_events.at (&time_sig_events, 2)->tick (), 3840);
}

// Test wrapper regeneration on underlying changes
TEST_F (TempoMapWrapperTest, UnderlyingChanges)
{
  // Directly modify underlying map
  tempo_map_->add_time_signature_event (1920, 3, 4);
  tempo_map_->add_tempo_event (1920, 140.0, TempoMap::CurveType::Constant);

  // Manually trigger rebuild (normally done via signals in real app)
  wrapper_->rebuildWrappers ();

  // Verify updates
  auto tempo_events = wrapper_->tempoEvents ();
  EXPECT_EQ (tempo_events.count (&tempo_events), 2);

  auto time_sig_events = wrapper_->timeSignatureEvents ();
  EXPECT_EQ (time_sig_events.count (&time_sig_events), 2);
}

// Test musical position conversion
TEST_F (TempoMapWrapperTest, MusicalPositionConversion)
{
  // Add test data to underlying map
  tempo_map_->add_time_signature_event (1920, 3, 4);
  wrapper_->rebuildWrappers ();

  // Test tick to musical position
  const int64_t testTick1 = 960;
  auto          posWrapper = wrapper_->getMusicalPosition (testTick1);
  auto          posDirect = tempo_map_->tick_to_musical_position (testTick1);
  EXPECT_EQ (posWrapper->bar (), posDirect.bar);
  EXPECT_EQ (posWrapper->beat (), posDirect.beat);
  EXPECT_EQ (posWrapper->sixteenth (), posDirect.sixteenth);
  EXPECT_EQ (posWrapper->tick (), posDirect.tick);
  delete posWrapper;

  // Test position after time signature change
  const int64_t testTick2 = 1920;
  posWrapper = wrapper_->getMusicalPosition (testTick2);
  posDirect = tempo_map_->tick_to_musical_position (testTick2);
  EXPECT_EQ (posWrapper->bar (), posDirect.bar);
  EXPECT_EQ (posWrapper->beat (), posDirect.beat);
  EXPECT_EQ (posWrapper->sixteenth (), posDirect.sixteenth);
  EXPECT_EQ (posWrapper->tick (), posDirect.tick);
  delete posWrapper;
}

// Test musical position string representation
TEST_F (TempoMapWrapperTest, MusicalPositionString)
{
  // Test basic position
  const int64_t testTick1 = 0;
  auto          strWrapper = wrapper_->getMusicalPositionString (testTick1);
  auto          posDirect = tempo_map_->tick_to_musical_position (testTick1);
  auto          expectedStr = fmt::format (
    "{}.{}.{}.000", posDirect.bar, posDirect.beat, posDirect.sixteenth);
  EXPECT_EQ (strWrapper.toStdString (), expectedStr);

  // Test position after some ticks
  const int64_t testTick2 = 481;
  strWrapper = wrapper_->getMusicalPositionString (testTick2);
  posDirect = tempo_map_->tick_to_musical_position (testTick2);
  expectedStr = fmt::format (
    "{}.{}.{}.001", posDirect.bar, posDirect.beat, posDirect.sixteenth);
  EXPECT_EQ (strWrapper.toStdString (), expectedStr);
}

// Test tick from musical position conversion
TEST_F (TempoMapWrapperTest, TickFromMusicalPosition)
{
  // Test basic position
  TempoMap::MusicalPosition pos1{ 1, 1, 1, 0 };
  auto                      tickWrapper = wrapper_->getTickFromMusicalPosition (
    pos1.bar, pos1.beat, pos1.sixteenth, pos1.tick);
  auto tickDirect = tempo_map_->musical_position_to_tick (pos1);
  EXPECT_EQ (tickWrapper, tickDirect);

  // Test beat position
  TempoMap::MusicalPosition pos2{ 1, 2, 1, 0 };
  tickWrapper = wrapper_->getTickFromMusicalPosition (
    pos2.bar, pos2.beat, pos2.sixteenth, pos2.tick);
  tickDirect = tempo_map_->musical_position_to_tick (pos2);
  EXPECT_EQ (tickWrapper, tickDirect);

  // Add time signature change and test
  tempo_map_->add_time_signature_event (1920, 3, 4);
  TempoMap::MusicalPosition pos3{ 2, 1, 1, 0 };
  tickWrapper = wrapper_->getTickFromMusicalPosition (
    pos3.bar, pos3.beat, pos3.sixteenth, pos3.tick);
  tickDirect = tempo_map_->musical_position_to_tick (pos3);
  EXPECT_EQ (tickWrapper, tickDirect);
}

} // namespace zrythm::dsp
