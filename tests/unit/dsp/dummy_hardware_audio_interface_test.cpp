// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <span>
#include <stdexcept>
#include <string_view>

#include "dsp/dummy_hardware_audio_interface.h"

#include <gtest/gtest.h>

namespace zrythm::dsp
{

namespace
{

using Clock = std::chrono::steady_clock;
constexpr auto                  kWaitLimit = std::chrono::seconds (10);
constexpr std::size_t           kBlockCount = 5;
const std::chrono::microseconds kClockTolerance (1);

/** Duration of one block of @p device_info. */
[[nodiscard]] std::chrono::duration<double>
block_duration_of (const AudioDeviceInfo &device_info)
{
  return au::as_chrono_duration (
    au::rep_cast<double> (device_info.block_length) / device_info.sample_rate);
}

/**
 * @brief Callback that records what the interface passes to it.
 */
class RecordingCallback final : public IAudioCallback
{
public:
  void process_audio (
    std::span<const float * const> input_channels,
    std::span<float * const>       output_channels,
    units::sample_u32_t            num_samples) noexcept override
  {
    const std::scoped_lock lock (mutex_);
    const auto             samples_per_channel =
      static_cast<std::size_t> (num_samples.in (units::samples));
    auto inputs_are_silent = true;
    for (const auto * channel : input_channels)
      {
        inputs_are_silent =
          inputs_are_silent
          && std::ranges::all_of (
            std::span (channel, samples_per_channel),
            [] (const float sample) { return sample == 0.f; });
      }
    last_input_channel_count_ = input_channels.size ();
    last_output_channel_count_ = output_channels.size ();
    last_num_samples_ = num_samples;
    last_inputs_were_silent_ = inputs_are_silent;
    ++process_count_;
    condition_.notify_all ();
  }

  void about_to_start () override
  {
    ++about_to_start_count_;
    if (about_to_start_count_ == fail_on_about_to_start_)
      {
        throw std::runtime_error ("device failed to start");
      }
  }

  void stopped () override { ++stopped_count_; }

  void error (std::string_view) override
  {
    const std::scoped_lock lock (mutex_);
    ++error_count_;
  }

  /** Makes the n-th about_to_start() call throw. */
  void fail_on_about_to_start (int n) { fail_on_about_to_start_ = n; }

  /** Waits until at least @p count blocks were processed. */
  bool wait_for_blocks (std::size_t count, Clock::duration timeout)
  {
    std::unique_lock lock (mutex_);
    return condition_.wait_for (lock, timeout, [this, count] {
      return process_count_ >= count;
    });
  }

  [[nodiscard]] std::size_t process_count () const
  {
    const std::scoped_lock lock (mutex_);
    return process_count_;
  }
  [[nodiscard]] std::size_t last_input_channel_count () const
  {
    const std::scoped_lock lock (mutex_);
    return last_input_channel_count_;
  }
  [[nodiscard]] std::size_t last_output_channel_count () const
  {
    const std::scoped_lock lock (mutex_);
    return last_output_channel_count_;
  }
  [[nodiscard]] units::sample_u32_t last_num_samples () const
  {
    const std::scoped_lock lock (mutex_);
    return last_num_samples_;
  }
  [[nodiscard]] bool last_inputs_were_silent () const
  {
    const std::scoped_lock lock (mutex_);
    return last_inputs_were_silent_;
  }
  [[nodiscard]] int about_to_start_count () const
  {
    return about_to_start_count_;
  }
  [[nodiscard]] int         stopped_count () const { return stopped_count_; }
  [[nodiscard]] std::size_t error_count () const
  {
    const std::scoped_lock lock (mutex_);
    return error_count_;
  }

private:
  mutable std::mutex      mutex_;
  std::condition_variable condition_;
  std::size_t             process_count_{};
  std::size_t             last_input_channel_count_{};
  std::size_t             last_output_channel_count_{};
  units::sample_u32_t     last_num_samples_{};
  bool                    last_inputs_were_silent_{};
  std::size_t             error_count_{};
  int                     about_to_start_count_{};
  int                     stopped_count_{};
  int                     fail_on_about_to_start_{};
};

} // namespace

TEST (DummyHardwareAudioInterfaceTest, ReportsTheDefaultDeviceInfo)
{
  const DummyHardwareAudioInterface interface;
  const auto                        info = interface.get_device_info ();
  EXPECT_EQ (info.device_name.view (), std::string_view ("Dummy"));
  EXPECT_EQ (info.sample_rate, units::sample_rate (48000));
  EXPECT_EQ (info.block_length, units::samples (256));
  EXPECT_EQ (info.input_channel_count, units::channels (2));
  EXPECT_EQ (info.output_channel_count, units::channels (2));
}

TEST (DummyHardwareAudioInterfaceTest, ReportsTheDeviceInfoGivenAtConstruction)
{
  auto info = DummyHardwareAudioInterface::make_default_device_info ();
  info.sample_rate = units::sample_rate (96000);
  info.block_length = units::samples (512);
  info.input_channel_count = units::channels (0);
  info.output_channel_count = units::channels (1);
  const DummyHardwareAudioInterface interface (info);
  const auto                        reported = interface.get_device_info ();
  EXPECT_EQ (reported.sample_rate, units::sample_rate (96000));
  EXPECT_EQ (reported.block_length, units::samples (512));
  EXPECT_EQ (reported.input_channel_count, units::channels (0));
  EXPECT_EQ (reported.output_channel_count, units::channels (1));
}

TEST (DummyHardwareAudioInterfaceTest, PassesOneSilentBlockPerBlockDuration)
{
  // The interface is destroyed first, so its thread stops before the
  // callback is destroyed
  RecordingCallback           callback;
  DummyHardwareAudioInterface interface;

  const auto start_time = Clock::now ();
  interface.add_audio_callback (&callback);
  EXPECT_EQ (callback.about_to_start_count (), 1);

  ASSERT_TRUE (callback.wait_for_blocks (kBlockCount, kWaitLimit));

  const auto info = interface.get_device_info ();
  EXPECT_EQ (callback.last_num_samples (), info.block_length);
  EXPECT_EQ (callback.last_input_channel_count (), 2);
  EXPECT_EQ (callback.last_output_channel_count (), 2);
  EXPECT_TRUE (callback.last_inputs_were_silent ());

  // kBlockCount blocks span kBlockCount - 1 block durations
  const auto expected_span =
    block_duration_of (info) * static_cast<double> (kBlockCount - 1);
  EXPECT_GE (Clock::now () - start_time, expected_span - kClockTolerance);

  interface.remove_audio_callback (&callback);
  EXPECT_EQ (callback.stopped_count (), 1);
  EXPECT_EQ (callback.error_count (), 0);
  EXPECT_EQ (interface.processed_block_count (), callback.process_count ());

  // Nothing is processed once the callback is removed
  const auto quiet_period = std::chrono::duration_cast<Clock::duration> (
    3 * block_duration_of (info) + kClockTolerance);
  EXPECT_FALSE (
    callback.wait_for_blocks (callback.process_count () + 1, quiet_period));
}

TEST (DummyHardwareAudioInterfaceTest, ProcessesEveryRegisteredCallback)
{
  RecordingCallback           first;
  RecordingCallback           second;
  DummyHardwareAudioInterface interface;

  interface.add_audio_callback (&first);
  interface.add_audio_callback (&second);
  ASSERT_TRUE (first.wait_for_blocks (1, kWaitLimit));
  ASSERT_TRUE (second.wait_for_blocks (1, kWaitLimit));

  // The removed callback alone is notified about the stop
  interface.remove_audio_callback (&first);
  EXPECT_EQ (first.stopped_count (), 1);
  EXPECT_EQ (second.stopped_count (), 0);
  EXPECT_EQ (second.about_to_start_count (), 1);
  EXPECT_TRUE (second.wait_for_blocks (2, kWaitLimit));
}

TEST (DummyHardwareAudioInterfaceTest, ForgetsACallbackThatFailsToStart)
{
  RecordingCallback           failing;
  RecordingCallback           working;
  DummyHardwareAudioInterface interface;
  failing.fail_on_about_to_start (1);

  EXPECT_THROW (interface.add_audio_callback (&failing), std::runtime_error);
  EXPECT_EQ (interface.processed_block_count (), 0);

  // The failed callback receives no block and no stop notification
  interface.add_audio_callback (&working);
  ASSERT_TRUE (working.wait_for_blocks (2, kWaitLimit));
  EXPECT_EQ (failing.process_count (), 0);
  EXPECT_EQ (failing.stopped_count (), 0);
}

TEST (DummyHardwareAudioInterfaceTest, SetDeviceInfoResumesAtTheNewConfiguration)
{
  RecordingCallback           callback;
  DummyHardwareAudioInterface interface;
  interface.add_audio_callback (&callback);
  ASSERT_TRUE (callback.wait_for_blocks (1, kWaitLimit));

  auto info = DummyHardwareAudioInterface::make_default_device_info ();
  info.sample_rate = units::sample_rate (44100);
  info.block_length = units::samples (512);
  info.input_channel_count = units::channels (1);
  info.output_channel_count = units::channels (3);
  interface.set_device_info (info);

  EXPECT_EQ (callback.stopped_count (), 1);
  EXPECT_EQ (callback.about_to_start_count (), 2);
  ASSERT_TRUE (
    callback.wait_for_blocks (callback.process_count () + 1, kWaitLimit));
  EXPECT_EQ (callback.last_num_samples (), units::samples (512));
  EXPECT_EQ (callback.last_input_channel_count (), 1);
  EXPECT_EQ (callback.last_output_channel_count (), 3);
  EXPECT_EQ (
    interface.get_device_info ().sample_rate, units::sample_rate (44100));
}

TEST (
  DummyHardwareAudioInterfaceTest,
  SetDeviceInfoAppliesToCallbacksAddedAfterwards)
{
  RecordingCallback           callback;
  DummyHardwareAudioInterface interface;

  auto info = DummyHardwareAudioInterface::make_default_device_info ();
  info.block_length = units::samples (1024);
  interface.set_device_info (info);
  EXPECT_EQ (callback.stopped_count (), 0);

  interface.add_audio_callback (&callback);
  ASSERT_TRUE (callback.wait_for_blocks (1, kWaitLimit));
  EXPECT_EQ (callback.last_num_samples (), units::samples (1024));
}

TEST (
  DummyHardwareAudioInterfaceTest,
  ReconfigurationFailureKeepsEveryCallbackStoppedUntilTheNextReconfiguration)
{
  RecordingCallback           failing;
  RecordingCallback           added_after_failure;
  DummyHardwareAudioInterface interface;
  failing.fail_on_about_to_start (2);
  interface.add_audio_callback (&failing);
  ASSERT_TRUE (failing.wait_for_blocks (1, kWaitLimit));

  auto info = DummyHardwareAudioInterface::make_default_device_info ();
  info.block_length = units::samples (512);
  EXPECT_THROW (interface.set_device_info (info), std::runtime_error);

  // A callback added afterwards is started and receives no block
  interface.add_audio_callback (&added_after_failure);
  EXPECT_EQ (added_after_failure.about_to_start_count (), 1);
  const auto quiet_period = std::chrono::duration_cast<Clock::duration> (
    3 * block_duration_of (info) + kClockTolerance);
  EXPECT_FALSE (added_after_failure.wait_for_blocks (1, quiet_period));

  // The next reconfiguration starts every registered callback
  failing.fail_on_about_to_start (0);
  interface.set_device_info (info);
  ASSERT_TRUE (added_after_failure.wait_for_blocks (1, kWaitLimit));
  ASSERT_TRUE (
    failing.wait_for_blocks (failing.process_count () + 1, kWaitLimit));
  EXPECT_EQ (failing.last_num_samples (), units::samples (512));
}

TEST (DummyHardwareAudioInterfaceTest, FailedAddLeavesTheOtherCallbacksRunning)
{
  RecordingCallback           working;
  RecordingCallback           failing;
  DummyHardwareAudioInterface interface;
  failing.fail_on_about_to_start (1);
  interface.add_audio_callback (&working);
  ASSERT_TRUE (working.wait_for_blocks (1, kWaitLimit));

  EXPECT_THROW (interface.add_audio_callback (&failing), std::runtime_error);

  // The surviving callback receives blocks again and no notification
  ASSERT_TRUE (
    working.wait_for_blocks (working.process_count () + 2, kWaitLimit));
  EXPECT_EQ (working.stopped_count (), 0);
  EXPECT_EQ (working.about_to_start_count (), 1);
}

} // namespace zrythm::dsp
