// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>
#include <ranges>

#include "plugins/vst3_channel_mapping.h"
#include "utils/views.h"

#include <gtest/gtest.h>

namespace zrythm::plugins
{

using Steinberg::Vst::AudioBusBuffers;

class Vst3ChannelMappingTest : public ::testing::Test
{
protected:
  static constexpr Steinberg::int32 kMaxBlock = 512;
  static constexpr Steinberg::int32 kNFrames = 128;
  static constexpr Steinberg::int32 kOffset = 32;

  std::vector<AudioBusBuffers>
  make_buses (std::initializer_list<Steinberg::int32> channels_per_bus)
  {
    std::vector<AudioBusBuffers> buses (channels_per_bus.size ());
    for (const auto &[i, channels] : utils::views::enumerate (channels_per_bus))
      {
        auto &bus = buses[i];
        bus.numChannels = channels;
        auto &storage = channel_buffer_storage_.emplace_back (
          std::make_unique<Steinberg::Vst::Sample32 *[]> (
            static_cast<size_t> (channels)));
        bus.channelBuffers32 = storage.get ();
        bus.silenceFlags = 0xDEADBEEF; // must be overwritten by the mapping
      }
    return buses;
  }

private:
  /** Keeps the channel pointer arrays alive for the test's duration. */
  std::vector<std::unique_ptr<Steinberg::Vst::Sample32 *[]>>
    channel_buffer_storage_;
};

// Every channel is backed by a port: buffers point into the port data at
// the chunk offset, and nothing is marked silent
TEST_F (Vst3ChannelMappingTest, FullyMappedBusesPointAtPortBuffers)
{
  auto                       buses = make_buses ({ 2 });
  juce::AudioBuffer<float>   port_buf{ 2, kMaxBlock };
  juce::AudioBuffer<float> * port_buffers[] = { &port_buf };
  auto                       scratch = make_vst3_scratch_buffers (
    buses.data (), static_cast<Steinberg::int32> (buses.size ()), kMaxBlock);

  map_vst3_channel_buffers (
    buses.data (), static_cast<Steinberg::int32> (buses.size ()), port_buffers,
    scratch, kOffset, kNFrames, true);

  EXPECT_EQ (
    buses[0].channelBuffers32[0], port_buf.getReadPointer (0) + kOffset);
  EXPECT_EQ (
    buses[0].channelBuffers32[1], port_buf.getReadPointer (1) + kOffset);
  EXPECT_EQ (buses[0].silenceFlags, 0);
}

// A bus with more channels than the port routes the extra channels to
// scratch; for inputs the scratch is zeroed and the channels flagged silent
TEST_F (Vst3ChannelMappingTest, UnmappedInputChannelsUseScratchAndAreSilent)
{
  auto                       buses = make_buses ({ 2 });
  juce::AudioBuffer<float>   port_buf{ 1, kMaxBlock };
  juce::AudioBuffer<float> * port_buffers[] = { &port_buf };
  auto                       scratch = make_vst3_scratch_buffers (
    buses.data (), static_cast<Steinberg::int32> (buses.size ()), kMaxBlock);
  scratch[0][1][0] = 42.f; // must be cleared by the mapping

  map_vst3_channel_buffers (
    buses.data (), static_cast<Steinberg::int32> (buses.size ()), port_buffers,
    scratch, kOffset, kNFrames, true);

  EXPECT_EQ (
    buses[0].channelBuffers32[0], port_buf.getReadPointer (0) + kOffset);
  EXPECT_EQ (buses[0].channelBuffers32[1], scratch[0][1].data ());
  for (const auto i : std::views::iota (0, kNFrames))
    EXPECT_FLOAT_EQ (scratch[0][1][static_cast<size_t> (i)], 0.f);
  EXPECT_EQ (buses[0].silenceFlags, 0b10);
}

// Buses beyond the port count are entirely scratch-backed and fully silent
// (input direction)
TEST_F (Vst3ChannelMappingTest, UnmappedInputBusIsFullySilent)
{
  auto                       buses = make_buses ({ 2, 2 });
  juce::AudioBuffer<float>   port_buf{ 2, kMaxBlock };
  juce::AudioBuffer<float> * port_buffers[] = { &port_buf };
  auto                       scratch = make_vst3_scratch_buffers (
    buses.data (), static_cast<Steinberg::int32> (buses.size ()), kMaxBlock);

  map_vst3_channel_buffers (
    buses.data (), static_cast<Steinberg::int32> (buses.size ()), port_buffers,
    scratch, kOffset, kNFrames, true);

  EXPECT_EQ (buses[1].channelBuffers32[0], scratch[1][0].data ());
  EXPECT_EQ (buses[1].channelBuffers32[1], scratch[1][1].data ());
  EXPECT_EQ (buses[1].silenceFlags, 0b11);
}

// Output direction: scratch channels absorb plugin writes and are neither
// zeroed nor flagged silent
TEST_F (Vst3ChannelMappingTest, UnmappedOutputChannelsAbsorbWrites)
{
  auto buses = make_buses ({ 1 });
  auto scratch = make_vst3_scratch_buffers (
    buses.data (), static_cast<Steinberg::int32> (buses.size ()), kMaxBlock);
  scratch[0][0][7] = 42.f;

  map_vst3_channel_buffers (
    buses.data (), static_cast<Steinberg::int32> (buses.size ()),
    std::span<juce::AudioBuffer<float> * const>{}, scratch, kOffset, kNFrames,
    false);

  EXPECT_EQ (buses[0].channelBuffers32[0], scratch[0][0].data ());
  EXPECT_EQ (buses[0].silenceFlags, 0);
  EXPECT_FLOAT_EQ (scratch[0][0][7], 42.f);

  // Writes through the mapped pointer land in the scratch storage
  buses[0].channelBuffers32[0][3] = 0.5f;
  EXPECT_FLOAT_EQ (scratch[0][0][3], 0.5f);
}

TEST_F (Vst3ChannelMappingTest, ScratchAllocationMatchesSpeakerArrangement)
{
  auto buses = make_buses ({ 2, 1 });
  auto scratch = make_vst3_scratch_buffers (
    buses.data (), static_cast<Steinberg::int32> (buses.size ()), kMaxBlock);

  ASSERT_EQ (scratch.size (), 2);
  ASSERT_EQ (scratch[0].size (), 2);
  ASSERT_EQ (scratch[1].size (), 1);
  for (const auto &channel : scratch[0])
    {
      ASSERT_EQ (channel.size (), kMaxBlock);
      EXPECT_TRUE (std::ranges::all_of (channel, [] (float s) {
        return s == 0.f;
      }));
    }
}

} // namespace zrythm::plugins
