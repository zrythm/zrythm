// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <juce_wrapper.h>

class MockAudioIODevice : public juce::AudioIODevice
{
public:
  MockAudioIODevice ()
      : AudioIODevice ("Mock Audio Device", "Mock Audio"),
        sample_rate_ (44100.0), buffer_size_ (512), is_running_ (false)
  {
  }

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
    int                     requestedBufferSize) override
  {
    sample_rate_ = requestedSampleRate;
    buffer_size_ = requestedBufferSize;
    return {};
  }

  void close () override { is_running_ = false; }

  bool isOpen () override { return true; }

  void start (juce::AudioIODeviceCallback * callback) override
  {
    if (callback != nullptr)
      {
        is_running_ = true;
        this->callback_ = callback;
        callback->audioDeviceAboutToStart (this);
      }
  }

  void stop () override
  {
    if (callback_ != nullptr)
      {
        callback_->audioDeviceStopped ();
        callback_ = nullptr;
      }
    is_running_ = false;
  }

  bool isPlaying () override { return is_running_; }

  juce::String getLastError () override { return {}; }

  int    getCurrentBufferSizeSamples () override { return buffer_size_; }
  double getCurrentSampleRate () override { return sample_rate_; }
  int    getCurrentBitDepth () override { return 16; }

  juce::BigInteger getActiveOutputChannels () const override
  {
    juce::BigInteger channels;
    channels.setRange (0, 2, true);
    return channels;
  }

  juce::BigInteger getActiveInputChannels () const override
  {
    juce::BigInteger channels;
    channels.setRange (0, 2, true);
    return channels;
  }

  int getOutputLatencyInSamples () override { return 0; }
  int getInputLatencyInSamples () override { return 0; }

  int getDefaultBufferSize () override { return 256; }

private:
  double                        sample_rate_;
  int                           buffer_size_;
  bool                          is_running_;
  juce::AudioIODeviceCallback * callback_ = nullptr;
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
    const juce::String &inputDeviceName) override
  {
    return new MockAudioIODevice ();
  }
};
