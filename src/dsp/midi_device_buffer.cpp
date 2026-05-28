// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>

#include "dsp/midi_device_buffer.h"

#include <farbot/fifo.hpp>

namespace zrythm::dsp
{

struct MidiDeviceBuffer::Impl
{
  farbot::fifo<
    juce::MidiMessage,
    farbot::fifo_options::concurrency::single,
    farbot::fifo_options::concurrency::single,
    farbot::fifo_options::full_empty_failure_mode::return_false_on_full_or_empty,
    farbot::fifo_options::full_empty_failure_mode::return_false_on_full_or_empty>
    queue;

  Impl () : queue (static_cast<int> (kCapacity)) { }
};

MidiDeviceBuffer::MidiDeviceBuffer () : impl_ (std::make_unique<Impl> ()) { }

MidiDeviceBuffer::~MidiDeviceBuffer () = default;

bool
MidiDeviceBuffer::push (juce::MidiMessage &&message)
{
  return impl_->queue.push (std::move (message));
}

void
MidiDeviceBuffer::drain (
  juce::MidiBuffer                      &output,
  units::sample_rate_t                   sample_rate,
  units::sample_u32_t                    nframes,
  std::optional<units::precise_second_t> block_start_time) noexcept
{
  std::optional<units::precise_second_t> base_timestamp;
  if (block_start_time)
    {
      base_timestamp = block_start_time;
    }

  juce::MidiMessage msg;
  while (impl_->queue.pop (msg))
    {
      const auto msg_timestamp = units::seconds (msg.getTimeStamp ());

      if (!base_timestamp)
        {
          base_timestamp = msg_timestamp;
        }

      const auto sample_offset = au::round_as<int> (
        units::samples, (msg_timestamp - *base_timestamp) * sample_rate);
      const auto clamped_offset = au::clamp (
        sample_offset, units::samples (0),
        nframes.as<int> (units::samples) - units::samples (1));
      output.addEvent (msg, clamped_offset.in<int> (units::samples));
    }
}

void
MidiDeviceBuffer::clear () noexcept
{
  juce::MidiMessage msg;
  while (impl_->queue.pop (msg))
    {
    }
}

}
