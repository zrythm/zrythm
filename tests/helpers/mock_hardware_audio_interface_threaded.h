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
    units::sample_rate_t sample_rate = units::sample_rate (48000),
    units::sample_u32_t  block_length = units::samples (256))
      : sample_rate_ (sample_rate), block_length_ (block_length)
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
        callback->about_to_start ();

        is_running_.store (true, std::memory_order_release);
        callback_thread_ = std::jthread ([this] (std::stop_token stoken) {
          constexpr int      num_channels = 2;
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
              auto * cb = callback_.load (std::memory_order_acquire);
              if (cb)
                {
                  cb->process_audio (
                    input_ptrs.data (), num_channels, output_ptrs.data (),
                    num_channels, block_length_.in<int> (units::samples));
                  process_call_count_.fetch_add (1, std::memory_order_acq_rel);
                }
              auto sleep_us = static_cast<int64_t> (
                block_length_.in<double> (units::samples) * 1'000'000.0
                / sample_rate_.in (units::sample_rate));
              std::this_thread::sleep_for (std::chrono::microseconds (sleep_us));
            }
        });
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

private:
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
  std::atomic<dsp::IAudioCallback *> callback_{ nullptr };
  std::atomic<bool>                  is_running_{ false };
  std::atomic<std::size_t>           process_call_count_{ 0 };
  std::jthread                       callback_thread_;
};

} // namespace zrythm::test_helpers
