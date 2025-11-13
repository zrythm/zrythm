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
    tempo_map_ = std::make_unique<TempoMap> (units::sample_rate (44100.0));
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

  auto first_sig_numerator = wrapper_->timeSignatureNumeratorAtTick (0);
  auto first_sig_denominator = wrapper_->timeSignatureDenominatorAtTick (0);
  EXPECT_EQ (first_sig_numerator, 4);
  EXPECT_EQ (first_sig_denominator, 4);

  // Verify sample rate
  EXPECT_DOUBLE_EQ (wrapper_->sampleRate (), 44100.0);
}

// Test tempo event signals and wrappers
TEST_F (TempoMapWrapperTest, TempoEventManagement)
{
  QSignalSpy spy (wrapper_.get (), &TempoMapWrapper::tempoEventsChanged);

  // Add tempo event
  wrapper_->addTempoEvent (1920, 140.0, TempoEventWrapper::CurveType::Constant);
  EXPECT_EQ (spy.count (), 1);

  // Verify wrapper count
  auto tempo_events = wrapper_->tempoEvents ();
  EXPECT_EQ (tempo_events.count (&tempo_events), 2);

  // Verify new event properties
  auto new_event = tempo_events.at (&tempo_events, 1);
  EXPECT_EQ (new_event->tick (), 1920);
  EXPECT_DOUBLE_EQ (new_event->bpm (), 140.0);
  EXPECT_EQ (new_event->curve (), TempoEventWrapper::CurveType::Constant);
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
  wrapper_->addTempoEvent (1920, 140.0, TempoEventWrapper::CurveType::Constant);
  wrapper_->addTempoEvent (3840, 160.0, TempoEventWrapper::CurveType::Linear);

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
  tempo_map_->add_time_signature_event (units::ticks (1920), 3, 4);
  tempo_map_->add_tempo_event (
    units::ticks (1920), 140.0, TempoMap::CurveType::Constant);

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
  tempo_map_->add_time_signature_event (units::ticks (1920), 3, 4);
  wrapper_->rebuildWrappers ();

  // Test tick to musical position
  const int64_t testTick1 = 960;
  auto          posWrapper = wrapper_->getMusicalPosition (testTick1);
  auto          posDirect =
    tempo_map_->tick_to_musical_position (units::ticks (testTick1));
  EXPECT_EQ (posWrapper->bar (), posDirect.bar);
  EXPECT_EQ (posWrapper->beat (), posDirect.beat);
  EXPECT_EQ (posWrapper->sixteenth (), posDirect.sixteenth);
  EXPECT_EQ (posWrapper->tick (), posDirect.tick);
  delete posWrapper;

  // Test position after time signature change
  const int64_t testTick2 = 1920;
  posWrapper = wrapper_->getMusicalPosition (testTick2);
  posDirect = tempo_map_->tick_to_musical_position (units::ticks (testTick2));
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
  auto          posDirect =
    tempo_map_->tick_to_musical_position (units::ticks (testTick1));
  auto expectedStr = fmt::format (
    "{}.{}.{}.000", posDirect.bar, posDirect.beat, posDirect.sixteenth);
  EXPECT_EQ (strWrapper.toStdString (), expectedStr);

  // Test position after some ticks
  const int64_t testTick2 = 481;
  strWrapper = wrapper_->getMusicalPositionString (testTick2);
  posDirect = tempo_map_->tick_to_musical_position (units::ticks (testTick2));
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
  EXPECT_EQ (tickWrapper, tickDirect.in (units::ticks));

  // Test beat position
  TempoMap::MusicalPosition pos2{ 1, 2, 1, 0 };
  tickWrapper = wrapper_->getTickFromMusicalPosition (
    pos2.bar, pos2.beat, pos2.sixteenth, pos2.tick);
  tickDirect = tempo_map_->musical_position_to_tick (pos2);
  EXPECT_EQ (tickWrapper, tickDirect.in (units::ticks));

  // Add time signature change and test
  tempo_map_->add_time_signature_event (units::ticks (1920), 3, 4);
  TempoMap::MusicalPosition pos3{ 2, 1, 1, 0 };
  tickWrapper = wrapper_->getTickFromMusicalPosition (
    pos3.bar, pos3.beat, pos3.sixteenth, pos3.tick);
  tickDirect = tempo_map_->musical_position_to_tick (pos3);
  EXPECT_EQ (tickWrapper, tickDirect.in (units::ticks));
}

// Test clearing tempo events via wrapper
TEST_F (TempoMapWrapperTest, ClearTempoEvents)
{
  QSignalSpy spy (wrapper_.get (), &TempoMapWrapper::tempoEventsChanged);

  // Add some tempo events first
  wrapper_->addTempoEvent (960, 140.0, TempoEventWrapper::CurveType::Constant);
  wrapper_->addTempoEvent (1920, 160.0, TempoEventWrapper::CurveType::Linear);
  wrapper_->addTempoEvent (3840, 180.0, TempoEventWrapper::CurveType::Constant);

  // Verify events were added
  auto tempo_events = wrapper_->tempoEvents ();
  EXPECT_EQ (tempo_events.count (&tempo_events), 4); // 3 + default

  // Clear tempo events
  wrapper_->clearTempoEvents ();
  EXPECT_EQ (spy.count (), 4); // Signal emitted during rebuild

  // Verify all tempo events are cleared (only default remains)
  tempo_events = wrapper_->tempoEvents ();
  EXPECT_EQ (tempo_events.count (&tempo_events), 0); // No explicit events

  // Verify tempo lookup returns default
  EXPECT_DOUBLE_EQ (wrapper_->tempoAtTick (960), 120.0);
  EXPECT_DOUBLE_EQ (wrapper_->tempoAtTick (1920), 120.0);
  EXPECT_DOUBLE_EQ (wrapper_->tempoAtTick (3840), 120.0);
}

// Test clearing time signature events via wrapper
TEST_F (TempoMapWrapperTest, ClearTimeSignatureEvents)
{
  QSignalSpy spy (wrapper_.get (), &TempoMapWrapper::timeSignatureEventsChanged);

  // Add some time signature events first
  wrapper_->addTimeSignatureEvent (1920, 3, 4);
  wrapper_->addTimeSignatureEvent (3840, 5, 8);
  wrapper_->addTimeSignatureEvent (5760, 7, 16);

  // Verify events were added
  auto time_sig_events = wrapper_->timeSignatureEvents ();
  EXPECT_EQ (time_sig_events.count (&time_sig_events), 4); // 3 + default

  // Clear time signature events
  wrapper_->clearTimeSignatureEvents ();
  EXPECT_EQ (spy.count (), 4); // Signal emitted during rebuild

  // Verify all time signature events are cleared (only default remains)
  time_sig_events = wrapper_->timeSignatureEvents ();
  EXPECT_EQ (time_sig_events.count (&time_sig_events), 0); // No explicit events

  // Verify time signature lookup returns default
  EXPECT_EQ (wrapper_->timeSignatureNumeratorAtTick (1920), 4);
  EXPECT_EQ (wrapper_->timeSignatureDenominatorAtTick (1920), 4);
  EXPECT_EQ (wrapper_->timeSignatureNumeratorAtTick (3840), 4);
  EXPECT_EQ (wrapper_->timeSignatureDenominatorAtTick (3840), 4);
  EXPECT_EQ (wrapper_->timeSignatureNumeratorAtTick (5760), 4);
  EXPECT_EQ (wrapper_->timeSignatureDenominatorAtTick (5760), 4);
}

// Test clearing both types of events
TEST_F (TempoMapWrapperTest, ClearBothEventTypes)
{
  QSignalSpy tempoSpy (wrapper_.get (), &TempoMapWrapper::tempoEventsChanged);
  QSignalSpy timeSigSpy (
    wrapper_.get (), &TempoMapWrapper::timeSignatureEventsChanged);

  // Add events (time signature first)
  wrapper_->addTimeSignatureEvent (1920, 3, 4);
  wrapper_->addTempoEvent (960, 140.0, TempoEventWrapper::CurveType::Constant);

  // Clear both types
  wrapper_->clearTempoEvents ();
  wrapper_->clearTimeSignatureEvents ();

  EXPECT_EQ (tempoSpy.count (), 2);   // Each clear emits 2 signals
  EXPECT_EQ (timeSigSpy.count (), 2); // Each clear emits 2 signals

  // Verify both are cleared
  auto tempoEvents = wrapper_->tempoEvents ();
  auto timeSigEvents = wrapper_->timeSignatureEvents ();
  EXPECT_EQ (tempoEvents.count (&tempoEvents), 0);
  EXPECT_EQ (timeSigEvents.count (&timeSigEvents), 0);

  // Verify musical position still works with defaults
  auto pos = wrapper_->getMusicalPosition (1920);
  EXPECT_EQ (pos->bar (), 1); // Should be bar 1 in default 4/4 time (1920 ticks
                              // = 0.5 bar)
  EXPECT_EQ (pos->beat (), 3); // 1920 ticks = 2 beats = beat 3
  EXPECT_EQ (pos->sixteenth (), 1);
  EXPECT_EQ (pos->tick (), 0);
  delete pos;
}

// Test clearing empty map
TEST_F (TempoMapWrapperTest, ClearEmptyMap)
{
  QSignalSpy tempoSpy (wrapper_.get (), &TempoMapWrapper::tempoEventsChanged);
  QSignalSpy timeSigSpy (
    wrapper_.get (), &TempoMapWrapper::timeSignatureEventsChanged);

  // Clear empty map - should still emit signals
  wrapper_->clearTempoEvents ();
  wrapper_->clearTimeSignatureEvents ();

  EXPECT_EQ (tempoSpy.count (), 1);
  EXPECT_EQ (timeSigSpy.count (), 1);

  // Should still work with defaults
  EXPECT_DOUBLE_EQ (wrapper_->tempoAtTick (0), 120.0);
  EXPECT_EQ (wrapper_->timeSignatureNumeratorAtTick (0), 4);
  EXPECT_EQ (wrapper_->timeSignatureDenominatorAtTick (0), 4);
}

// Test clearing and adding new events
TEST_F (TempoMapWrapperTest, ClearAndAddNewEvents)
{
  // Add initial events (time signature first)
  wrapper_->addTimeSignatureEvent (1920, 3, 4);
  wrapper_->addTempoEvent (960, 140.0, TempoEventWrapper::CurveType::Constant);

  // Clear events
  wrapper_->clearTempoEvents ();
  wrapper_->clearTimeSignatureEvents ();

  // Add new events after clearing (time signature first)
  wrapper_->addTimeSignatureEvent (960, 6, 8);
  wrapper_->addTempoEvent (480, 130.0, TempoEventWrapper::CurveType::Linear);

  // Verify new events work
  EXPECT_DOUBLE_EQ (wrapper_->tempoAtTick (480), 130.0);
  EXPECT_EQ (wrapper_->timeSignatureNumeratorAtTick (960), 6);
  EXPECT_EQ (wrapper_->timeSignatureDenominatorAtTick (960), 8);

  // Verify wrapper counts
  auto tempoEvents = wrapper_->tempoEvents ();
  auto timeSigEvents = wrapper_->timeSignatureEvents ();
  EXPECT_EQ (tempoEvents.count (&tempoEvents), 2);     // 1 + default
  EXPECT_EQ (timeSigEvents.count (&timeSigEvents), 2); // 1 + default
}

// Test musical position after clearing
TEST_F (TempoMapWrapperTest, MusicalPositionAfterClearing)
{
  // Add time signature change
  wrapper_->addTimeSignatureEvent (1920, 3, 4);

  // Verify position before clearing
  auto pos = wrapper_->getMusicalPosition (3840);
  EXPECT_EQ (pos->bar (), 1); // Bar 1 in 4/4 time (3840 ticks = 1 bar)
  delete pos;

  // Clear time signature events
  wrapper_->clearTimeSignatureEvents ();

  // Verify position after clearing (should be in default 4/4 time)
  pos = wrapper_->getMusicalPosition (3840);
  EXPECT_EQ (pos->bar (), 2); // Bar 2 in 4/4 time (3840 ticks = 1 bar)
  EXPECT_EQ (pos->beat (), 1);
  EXPECT_EQ (pos->sixteenth (), 1);
  EXPECT_EQ (pos->tick (), 0);
  delete pos;

  // Test tick from position after clearing
  auto tick = wrapper_->getTickFromMusicalPosition (1, 1, 1, 0);
  EXPECT_EQ (tick, 0);
}

} // namespace zrythm::dsp
