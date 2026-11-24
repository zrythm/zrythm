// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <atomic>
#include <mutex>
#include <stop_token>
#include <thread>
#include <vector>

#include "dsp/audio_device_info.h"
#include "dsp/hardware_audio_interface.h"
#include "dsp/iaudio_callback.h"

namespace zrythm::dsp
{

/**
 * @brief Hardware audio interface that runs without an audio device.
 *
 * Reports a fixed device configuration and calls the registered
 * callbacks from an own thread, one block per block duration, so the
 * engine runs as it does with a device. Input and output buffers are
 * silent.
 *
 * A callback receives process_audio() only after it returned normally
 * from about_to_start() for the current device configuration.
 *
 * add_audio_callback(), remove_audio_callback() and set_device_info()
 * must all be called from one thread. get_device_info() and
 * processed_block_count() can be called from any thread. A call to
 * set_device_info() must be ordered against get_device_info() calls
 * from other threads.
 */
class DummyHardwareAudioInterface final : public IHardwareAudioInterface
{
public:
  /**
   * @brief Returns the device configuration of an interface constructed
   * without arguments.
   *
   * The device is named "Dummy". It has 2 input and 2 output channels
   * at 48000 Hz with 256-sample blocks.
   */
  [[nodiscard]] static AudioDeviceInfo make_default_device_info ();

  explicit DummyHardwareAudioInterface (
    AudioDeviceInfo device_info = make_default_device_info ());
  ~DummyHardwareAudioInterface () override;

  [[nodiscard]] AudioDeviceInfo get_device_info () const override;

  /**
   * @brief Registers @p callback and starts it.
   *
   * When about_to_start() throws, the callback is not registered and
   * the exception propagates. The other callbacks are then processed
   * as before the call.
   */
  void add_audio_callback (IAudioCallback * callback) override;
  void remove_audio_callback (IAudioCallback * callback) override;

  /**
   * @brief Replaces the reported device configuration.
   *
   * Processing stops and every registered callback receives stopped().
   * While a callback is registered, each callback then receives
   * about_to_start() and processing resumes with the new configuration.
   *
   * If a callback throws in stopped() or about_to_start(), the
   * exception propagates and processing stays stopped. Blocks pass
   * again once a later call to this function has started every
   * registered callback.
   */
  void set_device_info (AudioDeviceInfo device_info);

  /**
   * @brief Returns the number of blocks passed to the callbacks since
   * construction.
   */
  [[nodiscard]] std::size_t processed_block_count () const;

private:
  /** Both require callbacks_mutex_ to be held. */
  void start_processing ();
  void stop_processing ();

  void processing_loop (std::stop_token stop_token);

  /**
   * Guards callbacks_, callbacks_started_ and thread_. Never held
   * while a callback runs.
   */
  std::mutex                    callbacks_mutex_;
  std::vector<IAudioCallback *> callbacks_;

  /**
   * Whether every callback in callbacks_ returned normally from
   * about_to_start() for device_info_ and received no stopped() since.
   * Requires callbacks_mutex_ to be held.
   */
  bool callbacks_started_{ true };

  /**
   * Changes only while no processing thread exists, so that thread
   * reads it without locking.
   */
  AudioDeviceInfo device_info_;

  std::atomic<std::size_t> processed_block_count_{};
  std::jthread             thread_;
};

} // namespace zrythm::dsp
