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
      : sample_rate_ (sample_rate), block_length_ (block_length),
        input_channels_ (input_channels), output_channels_ (output_channels)
  {
  }

  ~ThreadedMockHardwareAudioInterface () override { stop (); }

  [[nodiscard]] units::sample_u32_t get_block_length () const override
  {
    return block_length_;
  }
  [[nodiscard]] units::sample_rate_t get_sample_rate () const override
  {
    return sample_rate_;
  }
  [[nodiscard]] utils::Utf8String get_device_name () const override
  {
    return device_name_;
  }

  void set_device_name (utils::Utf8String name)
  {
    device_name_ = std::move (name);
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
        start_callback_thread (make_device_info ());
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
  }

  void simulate_device_change (const dsp::AudioDeviceInfo &new_info)
  {
    stop ();
    auto * cb = callback_.load (std::memory_order_acquire);
    if (cb != nullptr)
      {
        cb->stopped ();
        // Sync member variables so the thread lambda and public getters
        // (get_sample_rate, get_block_length) reflect the new device.
        // This will be removed when get_sample_rate/get_block_length are
        // removed from IHardwareAudioInterface and cached in Engine
        // instead.
        sample_rate_ = new_info.sample_rate;
        block_length_ = new_info.block_length;
        input_channels_ = new_info.input_channel_count;
        output_channels_ = new_info.output_channel_count;
        start_callback_thread (new_info);
      }
  }

private:
  void start_callback_thread (const dsp::AudioDeviceInfo &info)
  {
    auto * cb = callback_.load (std::memory_order_acquire);
    cb->about_to_start (info);

    is_running_.store (true, std::memory_order_release);
    callback_thread_ = std::jthread ([this] (std::stop_token stoken) {
      const auto num_channels = input_channels_.in<int> (units::channels);
      std::vector<float> output_buf (
        block_length_.in (units::samples) *num_channels, 0.f);
      std::vector<float> input_buf (
        block_length_.in (units::samples) *num_channels, 0.f);
      std::vector<float *>       output_ptrs (num_channels);
      std::vector<const float *> input_ptrs (num_channels);
      for (int ch = 0; ch < num_channels; ++ch)
        {
          output_ptrs[ch] =
            output_buf.data ()
            + (static_cast<size_t> (ch) * block_length_.in (units::samples));
          input_ptrs[ch] =
            input_buf.data ()
            + (static_cast<size_t> (ch) * block_length_.in (units::samples));
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
                block_length_);
              process_call_count_.fetch_add (1, std::memory_order_acq_rel);
            }
          auto sleep_us = static_cast<int64_t> (
            block_length_.in<double> (units::samples) * 1'000'000.0
            / sample_rate_.in (units::sample_rate));
          std::this_thread::sleep_for (std::chrono::microseconds (sleep_us));
        }
    });
  }

  dsp::AudioDeviceInfo make_device_info () const
  {
    return {
      .sample_rate = sample_rate_,
      .block_length = block_length_,
      .input_channel_count = input_channels_,
      .output_channel_count = output_channels_,
    };
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

  units::sample_rate_t               sample_rate_;
  units::sample_u32_t                block_length_;
  units::channel_count_t             input_channels_;
  units::channel_count_t             output_channels_;
  utils::Utf8String                  device_name_;
  std::atomic<dsp::IAudioCallback *> callback_{ nullptr };
  std::atomic<bool>                  is_running_{ false };
  std::atomic<std::size_t>           process_call_count_{ 0 };
  std::jthread                       callback_thread_;
};

} // namespace zrythm::test_helpers
