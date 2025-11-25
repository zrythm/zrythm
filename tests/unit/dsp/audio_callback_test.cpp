// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/audio_callback.h"

#include "helpers/mock_audio_io_device.h"
#include "helpers/scoped_juce_qapplication.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace testing;
using namespace zrythm::dsp;
using namespace zrythm::test_helpers;

class AudioCallbackTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    process_cb_called_ = false;
    device_about_to_start_called_ = false;
    device_stopped_called_ = false;
    last_device_ = nullptr;
    process_call_count_ = 0;
  }

  // Mock callbacks for testing
  AudioCallback::EngineProcessCallback create_mock_process_callback ()
  {
    return
      [this] (
        const float * const * inputChannelData, int numInputChannels,
        float * const * outputChannelData, int numOutputChannels,
        int numSamples) {
        process_cb_called_ = true;
        process_call_count_++;
        last_num_input_channels_ = numInputChannels;
        last_num_output_channels_ = numOutputChannels;
        last_num_samples_ = numSamples;

        // Store the input/output pointers for verification
        last_input_channel_data_ = inputChannelData;
        last_output_channel_data_ = outputChannelData;
      };
  }

  AudioCallback::DeviceAboutToStartCallback
  create_mock_device_about_to_start_callback ()
  {
    return [this] (juce::AudioIODevice * device) {
      device_about_to_start_called_ = true;
      last_device_ = device;
    };
  }

  AudioCallback::DeviceStoppedCallback create_mock_device_stopped_callback ()
  {
    return [this] () { device_stopped_called_ = true; };
  }

  static auto create_empty_process_cb ()
  {
    return
      [] (
        const float * const * inputChannelData, int numInputChannels,
        float * const * outputChannelData, int numOutputChannels,
        int numSamples) { };
  }
  static auto create_empty_device_about_to_start_cb ()
  {
    return [] (juce::AudioIODevice *) { };
  }
  static auto create_empty_device_stopped_cb ()
  {
    return [] () { };
  }

  // Test data members
  bool                  process_cb_called_ = false;
  bool                  device_about_to_start_called_ = false;
  bool                  device_stopped_called_ = false;
  juce::AudioIODevice * last_device_ = nullptr;
  int                   process_call_count_ = 0;
  int                   last_num_input_channels_ = 0;
  int                   last_num_output_channels_ = 0;
  int                   last_num_samples_ = 0;
  const float * const * last_input_channel_data_ = nullptr;
  float * const *       last_output_channel_data_ = nullptr;

  ScopedJuceQApplication scoped_app_;
};

TEST_F (AudioCallbackTest, ConstructorWithAllCallbacks)
{
  auto process_cb = create_mock_process_callback ();
  auto device_about_to_start_cb = create_mock_device_about_to_start_callback ();
  auto device_stopped_cb = create_mock_device_stopped_callback ();

  AudioCallback audio_callback (
    process_cb, device_about_to_start_cb, device_stopped_cb);

  // Constructor should store the callbacks without calling them
  EXPECT_FALSE (process_cb_called_);
  EXPECT_FALSE (device_about_to_start_called_);
  EXPECT_FALSE (device_stopped_called_);
}

TEST_F (AudioCallbackTest, ConstructorWithOnlyProcessCallback)
{
  auto process_cb = create_mock_process_callback ();
  auto empty_device_about_to_start_cb = [] (juce::AudioIODevice *) { };
  auto empty_device_stopped_cb = [] () { };

  AudioCallback audio_callback (
    process_cb, empty_device_about_to_start_cb, empty_device_stopped_cb);

  // Should work with empty callbacks
  EXPECT_FALSE (process_cb_called_);
}

TEST_F (AudioCallbackTest, AudioDeviceIOCallbackWithContext)
{
  auto process_cb = create_mock_process_callback ();

  AudioCallback audio_callback (
    process_cb, create_empty_device_about_to_start_cb (),
    create_empty_device_stopped_cb ());

  // Setup test audio data
  constexpr int numInputChannels = 2;
  constexpr int numOutputChannels = 2;
  constexpr int numSamples = 256;

  std::array<float, numSamples> inputData1{};
  std::array<float, numSamples> inputData2{};
  std::array<float, numSamples> outputData1{};
  std::array<float, numSamples> outputData2{};

  // Fill input with test data
  std::fill (inputData1.begin (), inputData1.end (), 0.5f);
  std::fill (inputData2.begin (), inputData2.end (), -0.5f);

  const float * inputChannels[] = { inputData1.data (), inputData2.data () };
  float *       outputChannels[] = { outputData1.data (), outputData2.data () };

  // Create a mock context
  juce::AudioIODeviceCallbackContext context{};

  // Call the audio callback
  audio_callback.audioDeviceIOCallbackWithContext (
    inputChannels, numInputChannels, outputChannels, numOutputChannels,
    numSamples, context);

  // Verify process callback was called with correct parameters
  EXPECT_TRUE (process_cb_called_);
  EXPECT_EQ (process_call_count_, 1);
  EXPECT_EQ (last_num_input_channels_, numInputChannels);
  EXPECT_EQ (last_num_output_channels_, numOutputChannels);
  EXPECT_EQ (last_num_samples_, numSamples);
  EXPECT_EQ (last_input_channel_data_, inputChannels);
  EXPECT_EQ (last_output_channel_data_, outputChannels);

  // Verify output buffers were cleared to zero
  for (const auto &sample : outputData1)
    {
      EXPECT_FLOAT_EQ (sample, 0.0f);
    }
  for (const auto &sample : outputData2)
    {
      EXPECT_FLOAT_EQ (sample, 0.0f);
    }
}

TEST_F (AudioCallbackTest, AudioDeviceIOCallbackWithContextZeroChannels)
{
  auto process_cb = create_mock_process_callback ();

  AudioCallback audio_callback (
    process_cb, create_empty_device_about_to_start_cb (),
    create_empty_device_stopped_cb ());

  constexpr int numInputChannels = 0;
  constexpr int numOutputChannels = 0;
  constexpr int numSamples = 128;

  const float * inputChannels[] = { nullptr };
  float *       outputChannels[] = { nullptr };

  juce::AudioIODeviceCallbackContext context{};

  // Should handle zero channels gracefully
  audio_callback.audioDeviceIOCallbackWithContext (
    inputChannels, numInputChannels, outputChannels, numOutputChannels,
    numSamples, context);

  EXPECT_TRUE (process_cb_called_);
  EXPECT_EQ (last_num_input_channels_, 0);
  EXPECT_EQ (last_num_output_channels_, 0);
  EXPECT_EQ (last_num_samples_, numSamples);
}

TEST_F (AudioCallbackTest, AudioDeviceIOCallbackWithContextMultipleCalls)
{
  auto process_cb = create_mock_process_callback ();

  AudioCallback audio_callback (
    process_cb, create_empty_device_about_to_start_cb (),
    create_empty_device_stopped_cb ());

  constexpr int numInputChannels = 1;
  constexpr int numOutputChannels = 1;
  constexpr int numSamples = 64;

  std::array<float, numSamples> outputData{};
  float *                       outputChannels[] = { outputData.data () };
  const float *                 inputChannels[] = { nullptr };

  juce::AudioIODeviceCallbackContext context{};

  // Call multiple times
  for (int i = 0; i < 3; ++i)
    {
      audio_callback.audioDeviceIOCallbackWithContext (
        inputChannels, numInputChannels, outputChannels, numOutputChannels,
        numSamples, context);
    }

  EXPECT_EQ (process_call_count_, 3);
  EXPECT_TRUE (process_cb_called_);

  // Output should still be zero after each call
  for (const auto &sample : outputData)
    {
      EXPECT_FLOAT_EQ (sample, 0.0f);
    }
}

TEST_F (AudioCallbackTest, AudioDeviceAboutToStartWithCallback)
{
  auto device_about_to_start_cb = create_mock_device_about_to_start_callback ();
  AudioCallback audio_callback (
    create_empty_process_cb (), device_about_to_start_cb,
    create_empty_device_stopped_cb ());

  MockAudioIODevice mock_device;

  audio_callback.audioDeviceAboutToStart (&mock_device);

  EXPECT_TRUE (device_about_to_start_called_);
  EXPECT_EQ (last_device_, &mock_device);
}

TEST_F (AudioCallbackTest, AudioDeviceAboutToStartWithEmptyCallback)
{
  AudioCallback audio_callback (
    create_empty_process_cb (), create_empty_device_about_to_start_cb (),
    create_empty_device_stopped_cb ());

  MockAudioIODevice mock_device;

  // Should not crash with empty callback
  audio_callback.audioDeviceAboutToStart (&mock_device);

  EXPECT_FALSE (device_about_to_start_called_);
}

TEST_F (AudioCallbackTest, AudioDeviceStoppedWithCallback)
{
  auto device_stopped_cb = create_mock_device_stopped_callback ();

  AudioCallback audio_callback (
    create_empty_process_cb (), create_empty_device_about_to_start_cb (),
    device_stopped_cb);

  audio_callback.audioDeviceStopped ();

  EXPECT_TRUE (device_stopped_called_);
}

TEST_F (AudioCallbackTest, AudioDeviceStoppedWithEmptyCallback)
{
  AudioCallback audio_callback (
    create_empty_process_cb (), create_empty_device_about_to_start_cb (),
    create_empty_device_stopped_cb ());

  // Should not crash with empty callback
  audio_callback.audioDeviceStopped ();

  EXPECT_FALSE (device_stopped_called_);
}

TEST_F (AudioCallbackTest, AudioDeviceError)
{
  AudioCallback audio_callback (
    create_empty_process_cb (), create_empty_device_about_to_start_cb (),
    create_empty_device_stopped_cb ());

  juce::String error_message = "Test error message";

  // Should not crash and should handle the error gracefully
  // Note: We can't easily test the logging output without capturing it
  audio_callback.audioDeviceError (error_message);
}

TEST_F (AudioCallbackTest, ProcessCallbackModifiesOutput)
{
  // Create a process callback that modifies the output
  AudioCallback::EngineProcessCallback modifying_process_cb =
    [] (
      const float * const * inputChannelData, int numInputChannels,
      float * const * outputChannelData, int numOutputChannels, int numSamples) {
      for (int ch = 0; ch < numOutputChannels; ++ch)
        {
          auto * ch_data = outputChannelData[ch];
          for (int i = 0; i < numSamples; ++i)
            {
              ch_data[i] = 0.75f; // Set to a known value
            }
        }
    };

  AudioCallback audio_callback (
    modifying_process_cb, create_empty_device_about_to_start_cb (),
    create_empty_device_stopped_cb ());

  constexpr int numInputChannels = 0;
  constexpr int numOutputChannels = 2;
  constexpr int numSamples = 128;

  std::array<float, numSamples> outputData1{};
  std::array<float, numSamples> outputData2{};
  float *       outputChannels[] = { outputData1.data (), outputData2.data () };
  const float * inputChannels[] = { nullptr };

  juce::AudioIODeviceCallbackContext context{};

  // First call - AudioCallback clears to 0, then our callback sets to 0.75
  audio_callback.audioDeviceIOCallbackWithContext (
    inputChannels, numInputChannels, outputChannels, numOutputChannels,
    numSamples, context);

  // Verify our process callback was able to modify the output
  for (const auto &sample : outputData1)
    {
      EXPECT_FLOAT_EQ (sample, 0.75f);
    }
  for (const auto &sample : outputData2)
    {
      EXPECT_FLOAT_EQ (sample, 0.75f);
    }
}

TEST_F (AudioCallbackTest, CallbackOrderInRealScenario)
{
  auto process_cb = create_mock_process_callback ();
  auto device_about_to_start_cb = create_mock_device_about_to_start_callback ();
  auto device_stopped_cb = create_mock_device_stopped_callback ();

  AudioCallback audio_callback (
    process_cb, device_about_to_start_cb, device_stopped_cb);

  MockAudioIODevice mock_device;

  // Simulate typical audio device lifecycle
  audio_callback.audioDeviceAboutToStart (&mock_device);
  EXPECT_TRUE (device_about_to_start_called_);
  EXPECT_FALSE (process_cb_called_);
  EXPECT_FALSE (device_stopped_called_);

  // Simulate audio processing
  constexpr int                 numSamples = 256;
  std::array<float, numSamples> outputData{};
  float *                       outputChannels[] = { outputData.data () };
  const float *                 inputChannels[] = { nullptr };

  juce::AudioIODeviceCallbackContext context{};

  audio_callback.audioDeviceIOCallbackWithContext (
    inputChannels, 0, outputChannels, 1, numSamples, context);
  EXPECT_TRUE (process_cb_called_);
  EXPECT_FALSE (device_stopped_called_);

  // Simulate device stop
  audio_callback.audioDeviceStopped ();
  EXPECT_TRUE (device_stopped_called_);
}
