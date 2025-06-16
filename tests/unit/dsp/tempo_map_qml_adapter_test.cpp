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
  ASSERT_EQ (tempo_events.count (&tempo_events), 1);

  auto first_tempo = tempo_events.at (&tempo_events, 0);
  EXPECT_EQ (first_tempo->tick (), 0);
  EXPECT_DOUBLE_EQ (first_tempo->bpm (), 120.0);
  EXPECT_EQ (first_tempo->curve (), TempoMap::CurveType::Constant);

  // Verify initial time signature
  auto time_sig_events = wrapper_->timeSignatureEvents ();
  ASSERT_EQ (time_sig_events.count (&time_sig_events), 1);

  auto first_sig = time_sig_events.at (&time_sig_events, 0);
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

  // Verify underlying map
  EXPECT_EQ (tempo_map_->get_tempo_events ().size (), 2);
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

  // Verify underlying map
  EXPECT_EQ (tempo_map_->get_time_signature_events ().size (), 2);
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
  wrapper_->addTempoEvent (
    1920, 140.0, static_cast<int> (TempoMap::CurveType::Constant));
  wrapper_->addTempoEvent (
    3840, 160.0, static_cast<int> (TempoMap::CurveType::Linear));
  wrapper_->addTimeSignatureEvent (1920, 3, 4);
  wrapper_->addTimeSignatureEvent (3840, 5, 8);

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
  tempo_map_->add_tempo_event (1920, 140.0, TempoMap::CurveType::Constant);
  tempo_map_->add_time_signature_event (1920, 3, 4);

  // Manually trigger rebuild (normally done via signals in real app)
  wrapper_->rebuildWrappers ();

  // Verify updates
  auto tempo_events = wrapper_->tempoEvents ();
  EXPECT_EQ (tempo_events.count (&tempo_events), 2);

  auto time_sig_events = wrapper_->timeSignatureEvents ();
  EXPECT_EQ (time_sig_events.count (&time_sig_events), 2);
}

} // namespace zrythm::dsp
