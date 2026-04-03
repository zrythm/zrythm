// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <cassert>
#include <vector>

#include "helpers/mock_audio_io_device.h"

namespace zrythm::test_helpers
{

// ============================================================================
// MockAudioIODevice
// ============================================================================

MockAudioIODevice::MockAudioIODevice ()
    : AudioIODevice ("Mock Audio Device", "Mock Audio"), sample_rate_ (44100.0),
      buffer_size_ (512), is_running_ (false)
{
}

juce::String
MockAudioIODevice::open (
  const juce::BigInteger & /*inputChannels*/,
  const juce::BigInteger & /*outputChannels*/,
  double requestedSampleRate,
  int    requestedBufferSize)
{
  sample_rate_ = requestedSampleRate;
  buffer_size_ = requestedBufferSize;
  return {};
}

void
MockAudioIODevice::start (juce::AudioIODeviceCallback * callback)
{
  stop ();
  if (callback != nullptr)
    {
      is_running_ = true;
      callback_ = callback;
      callback->audioDeviceAboutToStart (this);

      // Simulate a real audio device by firing callbacks on a background thread
      callback_thread_ = std::jthread ([this] (std::stop_token stoken) {
        constexpr int        num_channels = 2;
        std::vector<float>   output_buf (buffer_size_ * num_channels, 0.f);
        std::vector<float>   input_buf (buffer_size_ * num_channels, 0.f);
        std::vector<float *> output_ptrs (num_channels);
        std::vector<const float *> input_ptrs (num_channels);
        for (int ch = 0; ch < num_channels; ++ch)
          {
            output_ptrs[ch] = output_buf.data () + ch * buffer_size_;
            input_ptrs[ch] = input_buf.data () + ch * buffer_size_;
          }

        while (!stoken.stop_requested () && is_running_.load ())
          {
            if (callback_)
              {
                callback_->audioDeviceIOCallbackWithContext (
                  input_ptrs.data (), num_channels, output_ptrs.data (),
                  num_channels, buffer_size_,
                  juce::AudioIODeviceCallbackContext ());
              }
            auto sleep_us = static_cast<int64_t> (
              static_cast<double> (buffer_size_) * 1'000'000.0 / sample_rate_);
            std::this_thread::sleep_for (std::chrono::microseconds (sleep_us));
          }
      });
    }
}

void
MockAudioIODevice::stop ()
{
  is_running_ = false;
  if (callback_thread_.joinable ())
    {
      callback_thread_.request_stop ();
      callback_thread_.join ();
    }
  if (callback_ != nullptr)
    {
      callback_->audioDeviceStopped ();
      callback_ = nullptr;
    }
}

MockAudioIODevice::~MockAudioIODevice ()
{
  stop ();
}

juce::BigInteger
MockAudioIODevice::getActiveOutputChannels () const
{
  juce::BigInteger channels;
  channels.setRange (0, 2, true);
  return channels;
}

juce::BigInteger
MockAudioIODevice::getActiveInputChannels () const
{
  juce::BigInteger channels;
  channels.setRange (0, 2, true);
  return channels;
}

// ============================================================================
// MockAudioIODeviceType
// ============================================================================

juce::AudioIODevice *
MockAudioIODeviceType::createDevice (
  const juce::String & /*outputDeviceName*/,
  const juce::String & /*inputDeviceName*/)
{
  return new MockAudioIODevice ();
}

// ============================================================================
// Factory function
// ============================================================================

std::unique_ptr<juce::AudioDeviceManager>
create_audio_device_manager_with_dummy_device ()
{
  auto audio_device_manager = std::make_unique<juce::AudioDeviceManager> ();
  audio_device_manager->addAudioDeviceType (
    std::make_unique<MockAudioIODeviceType> ());

  // Initialize with a dummy audio device setup for testing
  juce::AudioDeviceManager::AudioDeviceSetup setup;
  setup.sampleRate = 48000.0;
  setup.bufferSize = 256;
  setup.inputDeviceName = "Mock Audio Device";
  setup.outputDeviceName = "Mock Audio Device";
  const auto error = audio_device_manager->setAudioDeviceSetup (setup, true);
  assert (error.isEmpty ());
  audio_device_manager->initialise (0, 2, nullptr, true, {}, nullptr);
  // Need to call this again for the setup to take effect...
  // https://forum.juce.com/t/setaudiodevicesetup/3275
  audio_device_manager->setAudioDeviceSetup (setup, true);
  return audio_device_manager;
}

}
