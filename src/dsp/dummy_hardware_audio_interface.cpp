// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>
#include <cassert>
#include <chrono>
#include <ranges>

#include "dsp/dummy_hardware_audio_interface.h"
#include "utils/threads.h"

namespace zrythm::dsp
{

AudioDeviceInfo
DummyHardwareAudioInterface::make_default_device_info ()
{
  return {
    .device_name = utils::Utf8String::from_utf8_encoded_string ("Dummy"),
    .sample_rate = units::sample_rate (48000),
    .block_length = units::samples (256),
    .input_channel_count = units::channels (2),
    .output_channel_count = units::channels (2),
  };
}

DummyHardwareAudioInterface::DummyHardwareAudioInterface (
  AudioDeviceInfo device_info)
    : device_info_ (std::move (device_info))
{
}

DummyHardwareAudioInterface::~DummyHardwareAudioInterface ()
{
  const std::scoped_lock lock (callbacks_mutex_);
  stop_processing ();
}

[[nodiscard]] AudioDeviceInfo
DummyHardwareAudioInterface::get_device_info () const
{
  return device_info_;
}

void
DummyHardwareAudioInterface::add_audio_callback (IAudioCallback * callback)
{
  assert (callback != nullptr);
  {
    const std::scoped_lock lock (callbacks_mutex_);
    assert (std::ranges::find (callbacks_, callback) == callbacks_.end ());
    stop_processing ();
    callbacks_.push_back (callback);
  }
  try
    {
      // The lock is released first: the callback may read the device info
      callback->about_to_start ();
    }
  catch (...)
    {
      const std::scoped_lock lock (callbacks_mutex_);
      // A callback that failed to start is not registered
      std::erase (callbacks_, callback);
      start_processing ();
      throw;
    }
  const std::scoped_lock lock (callbacks_mutex_);
  start_processing ();
}

void
DummyHardwareAudioInterface::remove_audio_callback (IAudioCallback * callback)
{
  assert (callback != nullptr);
  {
    const std::scoped_lock lock (callbacks_mutex_);
    const auto             it = std::ranges::find (callbacks_, callback);
    assert (it != callbacks_.end ());
    if (it == callbacks_.end ())
      return;
    stop_processing ();
    callbacks_.erase (it);
  }
  callback->stopped ();
  const std::scoped_lock lock (callbacks_mutex_);
  start_processing ();
}

void
DummyHardwareAudioInterface::set_device_info (AudioDeviceInfo device_info)
{
  std::vector<IAudioCallback *> callbacks;
  {
    const std::scoped_lock lock (callbacks_mutex_);
    stop_processing ();
    // No callback is started for the new configuration yet
    callbacks_started_ = false;
    device_info_ = std::move (device_info);
    callbacks = callbacks_;
  }
  // The lock is released first: the callbacks may read the device info
  for (auto * callback : callbacks)
    {
      callback->stopped ();
    }
  for (auto * callback : callbacks)
    {
      callback->about_to_start ();
    }
  const std::scoped_lock lock (callbacks_mutex_);
  callbacks_started_ = true;
  start_processing ();
}

[[nodiscard]] std::size_t
DummyHardwareAudioInterface::processed_block_count () const
{
  return processed_block_count_.load (std::memory_order_relaxed);
}

void
DummyHardwareAudioInterface::start_processing ()
{
  if (!callbacks_started_ || callbacks_.empty () || thread_.joinable ())
    return;
  thread_ = std::jthread ([this] (std::stop_token stop_token) {
    processing_loop (stop_token);
  });
}

void
DummyHardwareAudioInterface::stop_processing ()
{
  if (thread_.joinable ())
    {
      thread_.request_stop ();
      thread_.join ();
    }
}

void
DummyHardwareAudioInterface::processing_loop (std::stop_token stop_token)
{
  utils::set_current_thread_name ("DummyAudio");

  // callbacks_ and device_info_ change only while this thread does not
  // exist
  const auto callbacks = callbacks_;
  const auto device_info = device_info_;

  const auto samples_per_channel =
    static_cast<std::size_t> (device_info.block_length.in (units::samples));
  const auto input_channel_count =
    device_info.input_channel_count.in<int> (units::channels);
  const auto output_channel_count =
    device_info.output_channel_count.in<int> (units::channels);

  // Each buffer holds one contiguous run of samples per channel
  std::vector<float> input_samples (
    samples_per_channel * static_cast<std::size_t> (input_channel_count), 0.f);
  std::vector<float> output_samples (
    samples_per_channel * static_cast<std::size_t> (output_channel_count), 0.f);
  std::vector<const float *> input_ptrs;
  std::vector<float *>       output_ptrs;
  for (const auto channel : std::views::iota (0, input_channel_count))
    {
      input_ptrs.push_back (
        input_samples.data ()
        + static_cast<std::size_t> (channel) * samples_per_channel);
    }
  for (const auto channel : std::views::iota (0, output_channel_count))
    {
      output_ptrs.push_back (
        output_samples.data ()
        + static_cast<std::size_t> (channel) * samples_per_channel);
    }

  // Rounded up so that each block lasts at least its nominal duration
  const auto block_duration = std::chrono::ceil<
    std::chrono::nanoseconds> (au::as_chrono_duration (
    au::rep_cast<double> (device_info.block_length) / device_info.sample_rate));

  auto next_block_time = std::chrono::steady_clock::now ();
  while (!stop_token.stop_requested ())
    {
      for (auto * callback : callbacks)
        {
          callback->process_audio (
            input_ptrs, output_ptrs, device_info.block_length);
        }
      processed_block_count_.fetch_add (1, std::memory_order_relaxed);

      // A block that overruns its duration leaves nothing to catch up
      next_block_time += block_duration;
      if (next_block_time < std::chrono::steady_clock::now ())
        {
          next_block_time = std::chrono::steady_clock::now ();
        }
      std::this_thread::sleep_until (next_block_time);
    }
}

} // namespace zrythm::dsp
