// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <atomic>
#include <thread>

#include "dsp/midi_device_buffer.h"

#include <gtest/gtest.h>
#include <juce_audio_basics/juce_audio_basics.h>

namespace zrythm::dsp
{

class MidiDeviceBufferTest : public ::testing::Test
{
protected:
  static std::vector<int> sample_positions (const juce::MidiBuffer &buf)
  {
    std::vector<int> positions;
    for (const auto meta : buf)
      positions.push_back (meta.samplePosition);
    return positions;
  }

  MidiDeviceBuffer      buffer_;
  units::sample_rate_t  sample_rate_{ units::sample_rate (48000) };
  static constexpr auto nframes_ = units::samples (256u);
};

TEST_F (MidiDeviceBufferTest, PushAndDrainSingleEvent)
{
  auto msg = juce::MidiMessage::noteOn (1, 60, 0.8f);
  msg.setTimeStamp (1.0);
  buffer_.push (msg);

  juce::MidiBuffer output;
  buffer_.drain (output, sample_rate_, nframes_);

  EXPECT_EQ (output.getNumEvents (), 1);
  EXPECT_EQ (sample_positions (output), (std::vector<int>{ 0 }));
}

TEST_F (MidiDeviceBufferTest, PushAndDrainMultipleEvents)
{
  const int num_events = 100;
  for (int i = 0; i < num_events; ++i)
    {
      auto msg = juce::MidiMessage::noteOn (1, 60 + i, 0.5f);
      msg.setTimeStamp (1.0 + i * 0.0001);
      buffer_.push (msg);
    }

  juce::MidiBuffer output;
  buffer_.drain (output, sample_rate_, nframes_);

  EXPECT_EQ (output.getNumEvents (), num_events);
}

TEST_F (MidiDeviceBufferTest, TimestampsRelativeToFirstEvent)
{
  const double sr = 48000.0;

  auto msg1 = juce::MidiMessage::noteOn (1, 60, 0.5f);
  msg1.setTimeStamp (1.0);
  buffer_.push (msg1);

  auto msg2 = juce::MidiMessage::noteOn (1, 61, 0.5f);
  msg2.setTimeStamp (1.0 + 100.0 / sr);
  buffer_.push (msg2);

  auto msg3 = juce::MidiMessage::noteOn (1, 62, 0.5f);
  msg3.setTimeStamp (1.0 + 200.0 / sr);
  buffer_.push (msg3);

  juce::MidiBuffer output;
  buffer_.drain (output, sample_rate_, nframes_);

  ASSERT_EQ (output.getNumEvents (), 3);
  EXPECT_EQ (sample_positions (output), (std::vector<int>{ 0, 100, 200 }));
}

TEST_F (MidiDeviceBufferTest, TimestampsClampedToBlockRange)
{
  const double sr = 48000.0;

  auto msg1 = juce::MidiMessage::noteOn (1, 60, 0.5f);
  msg1.setTimeStamp (1.0);
  buffer_.push (msg1);

  auto msg2 = juce::MidiMessage::noteOn (1, 61, 0.5f);
  msg2.setTimeStamp (1.0 + 1000.0 / sr);
  buffer_.push (msg2);

  juce::MidiBuffer output;
  buffer_.drain (output, sample_rate_, nframes_);

  ASSERT_EQ (output.getNumEvents (), 2);
  EXPECT_EQ (sample_positions (output), (std::vector<int>{ 0, 255 }));
}

TEST_F (MidiDeviceBufferTest, ClearDiscardsAllEvents)
{
  auto msg = juce::MidiMessage::noteOn (1, 60, 0.5f);
  msg.setTimeStamp (1.0);
  buffer_.push (msg);

  buffer_.clear ();

  juce::MidiBuffer output;
  buffer_.drain (output, sample_rate_, nframes_);

  EXPECT_EQ (output.getNumEvents (), 0);
}

TEST_F (MidiDeviceBufferTest, DrainOnEmptyBufferIsNoOp)
{
  juce::MidiBuffer output;
  buffer_.drain (output, sample_rate_, nframes_);

  EXPECT_EQ (output.getNumEvents (), 0);
}

TEST_F (MidiDeviceBufferTest, SecondDrainReturnsNoEvents)
{
  auto msg = juce::MidiMessage::noteOn (1, 60, 0.5f);
  msg.setTimeStamp (1.0);
  buffer_.push (msg);

  juce::MidiBuffer output1;
  buffer_.drain (output1, sample_rate_, nframes_);
  EXPECT_EQ (output1.getNumEvents (), 1);

  juce::MidiBuffer output2;
  buffer_.drain (output2, sample_rate_, nframes_);
  EXPECT_EQ (output2.getNumEvents (), 0);
}

TEST_F (MidiDeviceBufferTest, ConcurrentPushAndDrainPreservesAllEvents)
{
  const int         num_events = 500;
  std::atomic<bool> done{ false };

  std::jthread producer ([this, &done] (std::stop_token) {
    for (int i = 0; i < num_events; ++i)
      {
        auto msg = juce::MidiMessage::noteOn (
          1, 60 + (i % 12), static_cast<uint8_t> (i % 128));
        msg.setTimeStamp (1.0 + i * 0.001);
        buffer_.push (msg);
      }
    done.store (true, std::memory_order_release);
  });

  int  total_drained = 0;
  bool all_batches_monotonic = true;
  while (!done.load (std::memory_order_acquire))
    {
      juce::MidiBuffer output;
      buffer_.drain (output, sample_rate_, nframes_);
      total_drained += output.getNumEvents ();
      int prev_pos = -1;
      for (const auto meta : output)
        {
          if (meta.samplePosition < prev_pos)
            all_batches_monotonic = false;
          prev_pos = meta.samplePosition;
        }
    }

  juce::MidiBuffer final_output;
  buffer_.drain (final_output, sample_rate_, nframes_);
  total_drained += final_output.getNumEvents ();
  int prev_pos = -1;
  for (const auto meta : final_output)
    {
      if (meta.samplePosition < prev_pos)
        all_batches_monotonic = false;
      prev_pos = meta.samplePosition;
    }

  EXPECT_EQ (total_drained, num_events);
  EXPECT_TRUE (all_batches_monotonic);
}

TEST_F (MidiDeviceBufferTest, PushBeyondCapacityDropsExcessEvents)
{
  for (size_t i = 0; i < MidiDeviceBuffer::kCapacity + 100; ++i)
    {
      auto msg = juce::MidiMessage::noteOn (1, 60, 0.5f);
      msg.setTimeStamp (1.0);
      buffer_.push (msg);
    }

  juce::MidiBuffer output;
  buffer_.drain (output, sample_rate_, nframes_);

  EXPECT_EQ (
    output.getNumEvents (), static_cast<int> (MidiDeviceBuffer::kCapacity))
    << "Only kCapacity events should be retained; excess events are silently dropped";
}

}
