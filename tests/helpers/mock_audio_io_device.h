// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <atomic>
#include <thread>

#include <juce_wrapper.h>

namespace zrythm::test_helpers
{
class MockAudioIODevice : public juce::AudioIODevice
{
public:
  MockAudioIODevice ();

  juce::StringArray getOutputChannelNames () override
  {
    return { "Left", "Right" };
  }

  juce::StringArray getInputChannelNames () override
  {
    return { "Input Left", "Input Right" };
  }

  juce::Array<double> getAvailableSampleRates () override
  {
    return { 44100.0, 48000.0, 96000.0 };
  }

  juce::Array<int> getAvailableBufferSizes () override
  {
    return { 128, 256, 512, 1024 };
  }

  juce::String open (
    const juce::BigInteger &inputChannels,
    const juce::BigInteger &outputChannels,
    double                  requestedSampleRate,
    int                     requestedBufferSize) override;

  void close () override { stop (); }

  bool isOpen () override { return true; }

  void start (juce::AudioIODeviceCallback * callback) override;

  void stop () override;

  bool isPlaying () override { return is_running_; }

  juce::String getLastError () override { return {}; }

  int    getCurrentBufferSizeSamples () override { return buffer_size_; }
  double getCurrentSampleRate () override { return sample_rate_; }
  int    getCurrentBitDepth () override { return 16; }

  juce::BigInteger getActiveOutputChannels () const override;
  juce::BigInteger getActiveInputChannels () const override;

  int getOutputLatencyInSamples () override { return 0; }
  int getInputLatencyInSamples () override { return 0; }

  int getDefaultBufferSize () override { return 256; }

  ~MockAudioIODevice () override;

private:
  double                        sample_rate_;
  int                           buffer_size_;
  std::atomic<bool>             is_running_{ false };
  juce::AudioIODeviceCallback * callback_ = nullptr;
  std::jthread                  callback_thread_;
};

class MockAudioIODeviceType : public juce::AudioIODeviceType
{
public:
  MockAudioIODeviceType () : AudioIODeviceType ("Mock Audio") { }

  void scanForDevices () override
  {
    // No-op for mock
  }

  juce::StringArray getDeviceNames (bool wantInputNames) const override
  {
    return { "Mock Audio Device" };
  }

  int getDefaultDeviceIndex (bool forInput) const override { return 0; }

  int
  getIndexOfDevice (juce::AudioIODevice * device, bool asInput) const override
  {
    return 0;
  }

  bool hasSeparateInputsAndOutputs () const override { return false; }

  juce::AudioIODevice * createDevice (
    const juce::String &outputDeviceName,
    const juce::String &inputDeviceName) override;
};

std::unique_ptr<juce::AudioDeviceManager>
create_audio_device_manager_with_dummy_device ();
}
