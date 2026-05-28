// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <memory>

#include "utils/units.h"

#include <juce_audio_basics/juce_audio_basics.h>

namespace zrythm::dsp
{

/**
 * @brief Lock-free SPSC buffer for MIDI events between the MIDI device thread
 * and the audio thread.
 *
 * MIDI thread calls push() with juce::MidiMessage objects carrying JUCE
 * timestamps (seconds from Time::getMillisecondCounterHiRes() * 0.001).
 *
 * Audio thread calls drain() to convert timestamps to sample offsets and
 * append events to a juce::MidiBuffer.
 */
class MidiDeviceBuffer
{
public:
  static constexpr size_t kCapacity = 4096;

  MidiDeviceBuffer ();
  ~MidiDeviceBuffer ();

  MidiDeviceBuffer (const MidiDeviceBuffer &) = delete;
  MidiDeviceBuffer &operator= (const MidiDeviceBuffer &) = delete;
  MidiDeviceBuffer (MidiDeviceBuffer &&) = delete;
  MidiDeviceBuffer &operator= (MidiDeviceBuffer &&) = delete;

  /**
   * @brief Push a MIDI message onto the queue.
   *
   * Called from the MIDI device thread. Not necessarily RT-safe.
   *
   * @param message MIDI message with a JUCE timestamp (seconds).
   * @return true if the message was enqueued, false if the queue was full.
   */
  bool push (juce::MidiMessage &&message);

  bool push (const juce::MidiMessage &message)
  {
    return push (juce::MidiMessage (message));
  }

  /**
   * @brief Drain all queued events into @p output, converting timestamps to
   * sample offsets within the current audio block.
   *
   * Called from the audio thread. RT-safe.
   *
   * When @p block_start_time is provided, all events are offset relative to
   * that wall-clock time, giving sample-accurate positioning within the block.
   * When not provided, the first event is placed at sample 0 and subsequent
   * events are offset relative to it.
   *
   * @param output Target MidiBuffer to append events to.
   * @param sample_rate Current sample rate.
   * @param nframes Number of frames in the current audio block.
   * @param block_start_time Wall-clock time at block start, if available.
   */
  void drain (
    juce::MidiBuffer                      &output,
    units::sample_rate_t                   sample_rate,
    units::sample_u32_t                    nframes,
    std::optional<units::precise_second_t> block_start_time =
      std::nullopt) noexcept [[clang::nonblocking]];

  /**
   * @brief Discard all pending events without processing.
   *
   * Called from the audio thread. RT-safe.
   */
  void clear () noexcept [[clang::nonblocking]];

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}
