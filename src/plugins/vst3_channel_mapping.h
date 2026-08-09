// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <algorithm>
#include <cstdint>
#include <ranges>
#include <span>
#include <vector>

#include <juce_audio_basics/juce_audio_basics.h>
#include <pluginterfaces/vst/ivstaudioprocessor.h>

namespace zrythm::plugins
{

/** Scratch buffer storage indexed [bus][channel][sample]. */
using Vst3ScratchBuffers = std::vector<std::vector<std::vector<float>>>;

/**
 * @brief Allocates per-channel scratch storage matching a bus layout.
 *
 * Scratch backs plugin bus channels that have no corresponding Zrythm port
 * buffer, so every channel pointer handed to the plugin stays valid even
 * when the live bus layout disagrees with the port topology (e.g. after a
 * bus count change).
 *
 * @param buses The AudioBusBuffers array from HostProcessData.
 * @param bus_count Number of buses in @p buses.
 * @param max_block_samples Maximum processing block length in samples.
 * @return Storage indexed [bus][channel], each channel buffer sized to
 * @p max_block_samples and zero-initialized.
 *
 * @note Allocates: prepare-time (non-realtime) use only.
 */
inline Vst3ScratchBuffers
make_vst3_scratch_buffers (
  const Steinberg::Vst::AudioBusBuffers * buses,
  Steinberg::int32                        bus_count,
  Steinberg::int32                        max_block_samples)
{
  Vst3ScratchBuffers scratch;
  scratch.reserve (static_cast<size_t> (bus_count));
  for (const auto bus_idx : std::views::iota (0, bus_count))
    {
      auto &bus_scratch = scratch.emplace_back ();
      bus_scratch.reserve (static_cast<size_t> (buses[bus_idx].numChannels));
      for (
        [[maybe_unused]] const auto ch :
        std::views::iota (0, buses[bus_idx].numChannels))
        bus_scratch.emplace_back (static_cast<size_t> (max_block_samples), 0.f);
    }
  return scratch;
}

/**
 * @brief Points each bus channel at its Zrythm port buffer, or at scratch.
 *
 * Mapped channels point directly into the port data at @p local_offset
 * (zero-copy). Channels with no corresponding port buffer (fewer port
 * channels, or fewer ports than plugin buses) point at @p scratch instead;
 * for inputs the scratch region is zeroed over @p nframes and the channel's
 * silence bit is set, for outputs the scratch absorbs the plugin's writes.
 *
 * @param buses The AudioBusBuffers array from HostProcessData.
 * @param bus_count Number of buses in @p buses.
 * @param port_buffers Port buffers by bus index; may be shorter than
 * @p bus_count.
 * @param scratch Scratch from make_vst3_scratch_buffers(), matching the bus
 * layout in @p buses.
 * @param local_offset Sample offset into the port buffers for this chunk.
 * @param nframes Number of samples in this processing chunk.
 * @param is_input True for input buses, false for outputs.
 *
 * @note Realtime-safe: no allocations or locks.
 */
inline void
map_vst3_channel_buffers (
  Steinberg::Vst::AudioBusBuffers *           buses,
  Steinberg::int32                            bus_count,
  std::span<juce::AudioBuffer<float> * const> port_buffers,
  Vst3ScratchBuffers                         &scratch,
  Steinberg::int32                            local_offset,
  Steinberg::int32                            nframes,
  bool                                        is_input) noexcept
{
  for (const auto bus_idx : std::views::iota (0, bus_count))
    {
      auto  &bus = buses[bus_idx];
      auto * port_buf =
        bus_idx < static_cast<Steinberg::int32> (port_buffers.size ())
          ? port_buffers[static_cast<size_t> (bus_idx)]
          : nullptr;
      const auto mapped_channels =
        port_buf != nullptr
          ? std::min (bus.numChannels, port_buf->getNumChannels ())
          : 0;
      auto &bus_scratch = scratch[static_cast<size_t> (bus_idx)];

      Steinberg::uint64 silence_flags = 0;
      for (const auto ch : std::views::iota (0, bus.numChannels))
        {
          if (ch < mapped_channels)
            {
              // getReadPointer() for inputs: getWritePointer() would
              // trigger JUCE's copy-on-write if the buffer were shared
              bus.channelBuffers32[ch] =
                (is_input
                   ? const_cast<float *> (port_buf->getReadPointer (ch))
                   : port_buf->getWritePointer (ch))
                + local_offset;
            }
          else
            {
              auto &channel_scratch = bus_scratch[static_cast<size_t> (ch)];
              if (is_input)
                {
                  std::fill_n (
                    channel_scratch.data (), static_cast<size_t> (nframes), 0.f);
                  if (ch < 64)
                    silence_flags |= Steinberg::uint64{ 1 } << ch;
                }
              bus.channelBuffers32[ch] = channel_scratch.data ();
            }
        }
      bus.silenceFlags = silence_flags;
    }
}

} // namespace zrythm::plugins
