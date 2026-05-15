// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <atomic>
#include <thread>

#include "dsp/hardware_audio_interface.h"
#include "dsp/iaudio_callback.h"
#include "utils/units.h"

namespace zrythm::test_helpers
{

/**
 * @brief Threaded mock hardware audio interface for integration tests.
 *
 * Runs audio callbacks in a background thread to simulate real audio device
 * behavior, required for tests that need to process audio asynchronously.
 */
class ThreadedMockHardwareAudioInterface : public dsp::IHardwareAudioInterface
{
public:
  explicit ThreadedMockHardwareAudioInterface (
    units::sample_rate_t   sample_rate = units::sample_rate (48000),
    units::sample_u32_t    block_length = units::samples (256),
    units::channel_count_t input_channels = units::channels (2),
    units::channel_count_t output_channels = units::channels (2))
      : device_info_{
          .device_name = {},
          .sample_rate = sample_rate,
          .block_length = block_length,
          .input_channel_count = input_channels,
          .output_channel_count = output_channels,
        }
  {
  }

  ~ThreadedMockHardwareAudioInterface () override { stop (); }

  [[nodiscard]] dsp::AudioDeviceInfo get_device_info () const override
  {
    return device_info_;
  }

  /**
   * @brief Updates the device info.
   *
   * Must not be called while audio processing is active (between
   * about_to_start() and stopped()).
   */
  void set_device_info (dsp::AudioDeviceInfo info)
  {
    assert (!processing_active_.load (std::memory_order_acquire));
    device_info_ = std::move (info);
  }

  /**
   * @brief Returns the number of times process_audio has been called.
   *
   * Thread-safe: can be called from any thread (e.g., the test main thread)
   * while the audio callback thread is running.
   */
  [[nodiscard]] std::size_t process_call_count () const
  {
    return process_call_count_.load (std::memory_order_acquire);
  }

  void add_audio_callback (dsp::IAudioCallback * callback) override
  {
    stop ();
    auto * old_cb = callback_.load (std::memory_order_acquire);
    if (old_cb != nullptr)
      {
        old_cb->stopped ();
      }
    if (callback != nullptr)
      {
        callback_.store (callback, std::memory_order_release);
        start_callback_thread ();
      }
  }

  void remove_audio_callback (dsp::IAudioCallback * callback) override
  {
    stop ();
    auto * cb = callback_.load (std::memory_order_acquire);
    if (cb != nullptr)
      {
        cb->stopped ();
      }
    callback_.store (nullptr, std::memory_order_release);
    processing_active_.store (false, std::memory_order_release);
  }

  void simulate_device_change (dsp::AudioDeviceInfo new_info)
  {
    stop ();
    auto * cb = callback_.load (std::memory_order_acquire);
    if (cb != nullptr)
      {
        cb->stopped ();
        processing_active_.store (false, std::memory_order_release);
        device_info_ = std::move (new_info);
        start_callback_thread ();
      }
  }

private:
  void start_callback_thread ()
  {
    auto * cb = callback_.load (std::memory_order_acquire);
    cb->about_to_start ();
    processing_active_.store (true, std::memory_order_release);

    is_running_.store (true, std::memory_order_release);
    callback_thread_ = std::jthread ([this] (std::stop_token stoken) {
      const auto num_channels =
        device_info_.input_channel_count.in<int> (units::channels);
      const auto           bl = device_info_.block_length.in (units::samples);
      std::vector<float>   output_buf (bl * num_channels, 0.f);
      std::vector<float>   input_buf (bl * num_channels, 0.f);
      std::vector<float *> output_ptrs (num_channels);
      std::vector<const float *> input_ptrs (num_channels);
      for (int ch = 0; ch < num_channels; ++ch)
        {
          output_ptrs[ch] = output_buf.data () + (static_cast<size_t> (ch) * bl);
          input_ptrs[ch] = input_buf.data () + (static_cast<size_t> (ch) * bl);
        }

      while (
        !stoken.stop_requested ()
        && is_running_.load (std::memory_order_acquire))
        {
          auto * current_cb = callback_.load (std::memory_order_acquire);
          if (current_cb)
            {
              current_cb->process_audio (
                { input_ptrs.data (), static_cast<size_t> (num_channels) },
                { output_ptrs.data (), static_cast<size_t> (num_channels) },
                device_info_.block_length);
              process_call_count_.fetch_add (1, std::memory_order_acq_rel);
            }
          auto sleep_us = static_cast<int64_t> (
            device_info_.block_length.in<double> (units::samples) * 1'000'000.0
            / device_info_.sample_rate.in (units::sample_rate));
          std::this_thread::sleep_for (std::chrono::microseconds (sleep_us));
        }
    });
  }

  void stop ()
  {
    is_running_.store (false, std::memory_order_release);
    if (callback_thread_.joinable ())
      {
        callback_thread_.request_stop ();
        callback_thread_.join ();
      }
  }

  dsp::AudioDeviceInfo               device_info_;
  std::atomic<bool>                  processing_active_{ false };
  std::atomic<dsp::IAudioCallback *> callback_{ nullptr };
  std::atomic<bool>                  is_running_{ false };
  std::atomic<std::size_t>           process_call_count_{ 0 };
  std::jthread                       callback_thread_;
};

} // namespace zrythm::test_helpers
