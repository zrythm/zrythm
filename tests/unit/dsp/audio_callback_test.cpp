// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/audio_callback.h"

#include "helpers/scoped_qcoreapplication.h"

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
    about_to_start_called_ = false;
    stopped_called_ = false;
    process_call_count_ = 0;
  }

  AudioCallback::EngineProcessCallback create_mock_process_callback ()
  {
    return
      [this] (
        const float * const * input_channel_data, int num_input_channels,
        float * const * output_channel_data, int num_output_channels,
        int num_samples) {
        process_cb_called_ = true;
        process_call_count_++;
        last_num_input_channels_ = num_input_channels;
        last_num_output_channels_ = num_output_channels;
        last_num_samples_ = num_samples;
        last_input_channel_data_ = input_channel_data;
        last_output_channel_data_ = output_channel_data;
      };
  }

  AudioCallback::DeviceAboutToStartCallback
  create_mock_about_to_start_callback ()
  {
    return [this] () { about_to_start_called_ = true; };
  }

  AudioCallback::DeviceStoppedCallback create_mock_stopped_callback ()
  {
    return [this] () { stopped_called_ = true; };
  }

  static auto create_empty_process_cb ()
  {
    return [] (const float * const *, int, float * const *, int, int) { };
  }
  static auto create_empty_about_to_start_cb ()
  {
    return [] () { };
  }
  static auto create_empty_stopped_cb ()
  {
    return [] () { };
  }

  // Test data members
  bool                  process_cb_called_ = false;
  bool                  about_to_start_called_ = false;
  bool                  stopped_called_ = false;
  int                   process_call_count_ = 0;
  int                   last_num_input_channels_ = 0;
  int                   last_num_output_channels_ = 0;
  int                   last_num_samples_ = 0;
  const float * const * last_input_channel_data_ = nullptr;
  float * const *       last_output_channel_data_ = nullptr;

  ScopedQCoreApplication scoped_app_;
};

TEST_F (AudioCallbackTest, ConstructorWithAllCallbacks)
{
  auto process_cb = create_mock_process_callback ();
  auto about_to_start_cb = create_mock_about_to_start_callback ();
  auto stopped_cb = create_mock_stopped_callback ();

  AudioCallback audio_callback (process_cb, about_to_start_cb, stopped_cb);

  EXPECT_FALSE (process_cb_called_);
  EXPECT_FALSE (about_to_start_called_);
  EXPECT_FALSE (stopped_called_);
}

TEST_F (AudioCallbackTest, ConstructorWithOnlyProcessCallback)
{
  auto process_cb = create_mock_process_callback ();

  AudioCallback audio_callback (
    process_cb, create_empty_about_to_start_cb (), create_empty_stopped_cb ());

  EXPECT_FALSE (process_cb_called_);
}

TEST_F (AudioCallbackTest, ProcessAudio)
{
  auto process_cb = create_mock_process_callback ();

  AudioCallback audio_callback (
    process_cb, create_empty_about_to_start_cb (), create_empty_stopped_cb ());

  constexpr int num_channels = 2;
  constexpr int num_samples = 256;

  std::array<float, num_samples> input_data1{};
  std::array<float, num_samples> input_data2{};
  std::array<float, num_samples> output_data1{};
  std::array<float, num_samples> output_data2{};

  std::fill (input_data1.begin (), input_data1.end (), 0.5f);
  std::fill (input_data2.begin (), input_data2.end (), -0.5f);

  const float * input_channels[] = { input_data1.data (), input_data2.data () };
  float * output_channels[] = { output_data1.data (), output_data2.data () };

  audio_callback.process_audio (
    input_channels, num_channels, output_channels, num_channels, num_samples);

  EXPECT_TRUE (process_cb_called_);
  EXPECT_EQ (process_call_count_, 1);
  EXPECT_EQ (last_num_input_channels_, num_channels);
  EXPECT_EQ (last_num_output_channels_, num_channels);
  EXPECT_EQ (last_num_samples_, num_samples);
  EXPECT_EQ (last_input_channel_data_, input_channels);
  EXPECT_EQ (last_output_channel_data_, output_channels);

  // Verify output buffers were cleared to zero by AudioCallback before calling
  // the process callback
  for (const auto &sample : output_data1)
    {
      EXPECT_FLOAT_EQ (sample, 0.0f);
    }
  for (const auto &sample : output_data2)
    {
      EXPECT_FLOAT_EQ (sample, 0.0f);
    }
}

TEST_F (AudioCallbackTest, ProcessAudioZeroChannels)
{
  auto process_cb = create_mock_process_callback ();

  AudioCallback audio_callback (
    process_cb, create_empty_about_to_start_cb (), create_empty_stopped_cb ());

  const float * input_channels[] = { nullptr };
  float *       output_channels[] = { nullptr };

  audio_callback.process_audio (input_channels, 0, output_channels, 0, 128);

  EXPECT_TRUE (process_cb_called_);
  EXPECT_EQ (last_num_input_channels_, 0);
  EXPECT_EQ (last_num_output_channels_, 0);
}

TEST_F (AudioCallbackTest, ProcessAudioMultipleCalls)
{
  auto process_cb = create_mock_process_callback ();

  AudioCallback audio_callback (
    process_cb, create_empty_about_to_start_cb (), create_empty_stopped_cb ());

  constexpr int num_channels = 1;
  constexpr int num_samples = 64;

  std::array<float, num_samples> output_data{};
  float *                        output_channels[] = { output_data.data () };
  const float *                  input_channels[] = { nullptr };

  for (int i = 0; i < 3; ++i)
    {
      audio_callback.process_audio (
        input_channels, num_channels, output_channels, num_channels,
        num_samples);
    }

  EXPECT_EQ (process_call_count_, 3);
  EXPECT_TRUE (process_cb_called_);
}

TEST_F (AudioCallbackTest, AboutToStartWithCallback)
{
  auto          about_to_start_cb = create_mock_about_to_start_callback ();
  AudioCallback audio_callback (
    create_empty_process_cb (), about_to_start_cb, create_empty_stopped_cb ());

  audio_callback.about_to_start ();

  EXPECT_TRUE (about_to_start_called_);
}

TEST_F (AudioCallbackTest, AboutToStartWithEmptyCallback)
{
  AudioCallback audio_callback (
    create_empty_process_cb (), create_empty_about_to_start_cb (),
    create_empty_stopped_cb ());

  // Should not crash with empty callback
  audio_callback.about_to_start ();

  EXPECT_FALSE (about_to_start_called_);
}

TEST_F (AudioCallbackTest, StoppedWithCallback)
{
  auto stopped_cb = create_mock_stopped_callback ();

  AudioCallback audio_callback (
    create_empty_process_cb (), create_empty_about_to_start_cb (), stopped_cb);

  audio_callback.stopped ();

  EXPECT_TRUE (stopped_called_);
}

TEST_F (AudioCallbackTest, StoppedWithEmptyCallback)
{
  AudioCallback audio_callback (
    create_empty_process_cb (), create_empty_about_to_start_cb (),
    create_empty_stopped_cb ());

  audio_callback.stopped ();

  EXPECT_FALSE (stopped_called_);
}

TEST_F (AudioCallbackTest, Error)
{
  AudioCallback audio_callback (
    create_empty_process_cb (), create_empty_about_to_start_cb (),
    create_empty_stopped_cb ());

  // Should not crash and should handle the error gracefully
  audio_callback.error ("Test error message");
}

TEST_F (AudioCallbackTest, ProcessCallbackModifiesOutput)
{
  AudioCallback::EngineProcessCallback modifying_process_cb =
    [] (
      const float * const *, int, float * const * output_channel_data,
      int num_output_channels, int num_samples) {
      for (int ch = 0; ch < num_output_channels; ++ch)
        {
          auto * ch_data = output_channel_data[ch];
          for (int i = 0; i < num_samples; ++i)
            {
              ch_data[i] = 0.75f;
            }
        }
    };

  AudioCallback audio_callback (
    modifying_process_cb, create_empty_about_to_start_cb (),
    create_empty_stopped_cb ());

  constexpr int num_samples = 128;

  std::array<float, num_samples> output_data1{};
  std::array<float, num_samples> output_data2{};
  float * output_channels[] = { output_data1.data (), output_data2.data () };
  const float * input_channels[] = { nullptr };

  // AudioCallback clears to 0, then our callback sets to 0.75
  audio_callback.process_audio (
    input_channels, 0, output_channels, 2, num_samples);

  for (const auto &sample : output_data1)
    {
      EXPECT_FLOAT_EQ (sample, 0.75f);
    }
  for (const auto &sample : output_data2)
    {
      EXPECT_FLOAT_EQ (sample, 0.75f);
    }
}

TEST_F (AudioCallbackTest, CallbackOrderInRealScenario)
{
  auto process_cb = create_mock_process_callback ();
  auto about_to_start_cb = create_mock_about_to_start_callback ();
  auto stopped_cb = create_mock_stopped_callback ();

  AudioCallback audio_callback (process_cb, about_to_start_cb, stopped_cb);

  // Simulate typical audio device lifecycle
  audio_callback.about_to_start ();
  EXPECT_TRUE (about_to_start_called_);
  EXPECT_FALSE (process_cb_called_);
  EXPECT_FALSE (stopped_called_);

  // Simulate audio processing
  constexpr int                  num_samples = 256;
  std::array<float, num_samples> output_data{};
  float *                        output_channels[] = { output_data.data () };
  const float *                  input_channels[] = { nullptr };

  audio_callback.process_audio (
    input_channels, 0, output_channels, 1, num_samples);
  EXPECT_TRUE (process_cb_called_);
  EXPECT_FALSE (stopped_called_);

  // Simulate device stop
  audio_callback.stopped ();
  EXPECT_TRUE (stopped_called_);
}
