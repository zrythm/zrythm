// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <thread>

#include "dsp/playhead.h"
#include "dsp/tempo_map.h"

#include <gtest/gtest.h>

namespace zrythm::dsp
{

class PlayheadTest : public ::testing::Test
{
protected:
  static constexpr auto SAMPLE_RATE = 44100.0;

  void SetUp () override
  {
    tempo_map_ = std::make_unique<TempoMap> (SAMPLE_RATE);
    playhead_ = std::make_unique<Playhead> (*tempo_map_);
  }

  // Simulate audio thread processing cycle
  void simulateAudioProcessing (
    uint32_t                               nframes,
    std::function<void (uint32_t, double)> sampleCallback = nullptr)
  {
    dsp::PlayheadProcessingGuard guard{ *playhead_ };

    for (uint32_t i = 0; i < nframes; ++i)
      {
        if (sampleCallback)
          sampleCallback (i, playhead_->position_during_processing_precise ());
        playhead_->advance_processing (1);
      }
  }

  std::unique_ptr<TempoMap> tempo_map_;
  std::unique_ptr<Playhead> playhead_;
};

// Test initial state
TEST_F (PlayheadTest, InitialState)
{
  EXPECT_DOUBLE_EQ (playhead_->position_ticks (), 0.0);
  EXPECT_DOUBLE_EQ (
    playhead_->position_samples_FOR_TESTING (),
    0.0); // Assuming we add this getter
}

// Test GUI thread position setting
TEST_F (PlayheadTest, SetPositionFromGUI)
{
  const double testTicks = 1920.0; // 2 beats at 120 BPM
  const double expectedStartSamples = tempo_map_->tick_to_samples (testTicks);
  playhead_->set_position_ticks (testTicks);

  // Verify GUI thread access
  EXPECT_DOUBLE_EQ (playhead_->position_ticks (), testTicks);

  // Verify audio thread sees correct starting position
  uint32_t frameCount = 0;
  simulateAudioProcessing (10, [&] (uint32_t frame, double pos) {
    // Position should start at expected value and increase by 1 per frame
    EXPECT_DOUBLE_EQ (pos, expectedStartSamples + frame);
    frameCount++;
  });
  EXPECT_EQ (frameCount, 10);
}

// Test audio processing advance
TEST_F (PlayheadTest, AudioProcessingAdvance)
{
  const uint32_t blockSize = 512;
  const double   startPos = tempo_map_->tick_to_samples (0);
  double         lastPos = -1.0;

  simulateAudioProcessing (blockSize, [&] (uint32_t frame, double pos) {
    // Position should increment by 1 sample each frame
    if (lastPos >= 0)
      {
        EXPECT_DOUBLE_EQ (pos, lastPos + 1.0);
      }
    else
      {
        EXPECT_DOUBLE_EQ (pos, startPos);
      }
    lastPos = pos;
  });

  // Final position should be start + blockSize
  EXPECT_DOUBLE_EQ (
    playhead_->position_samples_FOR_TESTING (), startPos + blockSize);
}

// Test tick update from samples
TEST_F (PlayheadTest, UpdateTicksFromSamples)
{
  const double testSamples = 22050.0; // 0.5 seconds at 44.1kHz
  playhead_->set_position_ticks (tempo_map_->samples_to_tick (testSamples));

  // Modify samples directly (simulate audio thread advance)
  simulateAudioProcessing (100);

  // Update ticks from samples
  playhead_->update_ticks_from_samples ();

  const double expectedTicks =
    tempo_map_->samples_to_tick (playhead_->position_samples_FOR_TESTING ());
  EXPECT_NEAR (playhead_->position_ticks (), expectedTicks, 1e-6);
}

// Test serialization/deserialization
TEST_F (PlayheadTest, Serialization)
{
  const double testTicks = 3840.0; // 4 beats
  playhead_->set_position_ticks (testTicks);

  // Serialize
  nlohmann::json j;
  j = *playhead_;

  // Create new playhead and deserialize
  Playhead new_playhead (*tempo_map_);
  j.get_to (new_playhead);

  // Verify position
  EXPECT_DOUBLE_EQ (new_playhead.position_ticks (), testTicks);
}

// Test concurrent access (simulated)
TEST_F (PlayheadTest, ConcurrentAccess)
{
  const double guiTicks = 960.0;
  playhead_->set_position_ticks (guiTicks);

  // Simulate GUI thread access during audio processing
  simulateAudioProcessing (1024, [&] (uint32_t frame, double) {
    if (frame == 512)
      {
        // GUI thread operation (would normally lock mutex)
        playhead_->update_ticks_from_samples ();
        EXPECT_GT (playhead_->position_ticks (), 0.0);
      }
  });

  // Should not crash and should maintain reasonable state
  EXPECT_GT (playhead_->position_samples_FOR_TESTING (), 0.0);
  EXPECT_GT (playhead_->position_ticks (), 0.0);
}

// Test large block processing
TEST_F (PlayheadTest, LargeBlockProcessing)
{
  const uint32_t largeBlock = 65536;
  const double   startPos = playhead_->position_samples_FOR_TESTING ();

  simulateAudioProcessing (largeBlock);

  EXPECT_DOUBLE_EQ (
    playhead_->position_samples_FOR_TESTING (), startPos + largeBlock);
}

// Test position consistency during processing
TEST_F (PlayheadTest, PositionConsistency)
{
  const uint32_t      blockSize = 2048;
  std::vector<double> positions;
  positions.reserve (blockSize);

  simulateAudioProcessing (blockSize, [&] (uint32_t, double pos) {
    positions.push_back (pos);
  });

  // Verify all positions were sequential
  for (size_t i = 1; i < positions.size (); ++i)
    {
      EXPECT_DOUBLE_EQ (positions[i], positions[i - 1] + 1.0);
    }
}

// Test zero-length audio block
TEST_F (PlayheadTest, ZeroLengthBlock)
{
  simulateAudioProcessing (0);
  EXPECT_DOUBLE_EQ (playhead_->position_samples_FOR_TESTING (), 0.0);
}

} // namespace zrythm::dsp
