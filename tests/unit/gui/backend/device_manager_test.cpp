// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <array>
#include <memory>
#include <vector>

#include "gui/backend/device_manager.h"

#include "helpers/scoped_juce_qapplication.h"

#include <gtest/gtest.h>
#include <juce_audio_devices/juce_audio_devices.h>

namespace zrythm::gui::backend
{

namespace
{

constexpr int    fake_num_channels = 2;
constexpr int    fake_block_size = 256;
constexpr double fake_sample_rate = 48000.0;

/** Level the engine callback renders into every output sample. */
constexpr float engine_render_level = 0.25f;

/**
 * @brief Stands in for the audio engine's device callback: clears the
 * buffers it is given, then renders a constant level into them.
 */
class EngineRenderCallback final : public juce::AudioIODeviceCallback
{
public:
  void audioDeviceIOCallbackWithContext (
    const float * const *,
    int,
    float * const * outputChannelData,
    int             numOutputChannels,
    int             numSamples,
    const juce::AudioIODeviceCallbackContext &) override
  {
    for (
      int channel_index = 0; channel_index < numOutputChannels; ++channel_index)
      {
        auto * channel = outputChannelData[channel_index];
        for (int sample_index = 0; sample_index < numSamples; ++sample_index)
          {
            channel[sample_index] = 0.f;
          }
      }
    for (
      int channel_index = 0; channel_index < numOutputChannels; ++channel_index)
      {
        auto * channel = outputChannelData[channel_index];
        for (int sample_index = 0; sample_index < numSamples; ++sample_index)
          {
            channel[sample_index] = engine_render_level;
          }
      }
  }

  void audioDeviceAboutToStart (juce::AudioIODevice *) override { }
  void audioDeviceStopped () override { }
};

class FakeAudioIODeviceType;

/**
 * @brief Audio device driven manually by the test.
 *
 * The device drives the callback lifecycle like a real device, and its
 * period buffers keep their contents between cycles, like real hardware
 * buffers that a device hands to the host without clearing.
 */
class FakeAudioIODevice final : public juce::AudioIODevice
{
public:
  FakeAudioIODevice (juce::String device_name, FakeAudioIODeviceType &owner);

  ~FakeAudioIODevice () override;

  juce::StringArray getOutputChannelNames () override
  {
    return { "Out 1", "Out 2" };
  }

  juce::StringArray getInputChannelNames () override
  {
    return { "In 1", "In 2" };
  }

  juce::Array<double> getAvailableSampleRates () override
  {
    return { 48000.0 };
  }

  juce::Array<int> getAvailableBufferSizes () override
  {
    return { fake_block_size };
  }

  int getDefaultBufferSize () override { return fake_block_size; }

  juce::String open (
    const juce::BigInteger &inputs,
    const juce::BigInteger &outputs,
    double                  sample_rate,
    int                     buffer_size_samples) override
  {
    active_inputs_ = inputs;
    active_outputs_ = outputs;
    sample_rate_ = sample_rate;
    buffer_size_ = buffer_size_samples;
    input_channels_.assign (
      inputs.countNumberOfSetBits (),
      std::vector<float> (buffer_size_samples, 0.f));
    output_channels_.assign (
      outputs.countNumberOfSetBits (),
      std::vector<float> (buffer_size_samples, 0.f));
    open_ = true;
    return {};
  }

  void close () override { open_ = false; }
  bool isOpen () override { return open_; }

  void start (juce::AudioIODeviceCallback * callback) override
  {
    callback_ = callback;
    callback->audioDeviceAboutToStart (this);
    playing_ = true;
  }

  void stop () override
  {
    playing_ = false;
    if (callback_ != nullptr)
      {
        callback_->audioDeviceStopped ();
        callback_ = nullptr;
      }
  }

  bool         isPlaying () override { return playing_; }
  juce::String getLastError () override { return {}; }
  int          getCurrentBufferSizeSamples () override { return buffer_size_; }
  double       getCurrentSampleRate () override { return sample_rate_; }
  int          getCurrentBitDepth () override { return 32; }
  juce::BigInteger getActiveOutputChannels () const override
  {
    return active_outputs_;
  }

  juce::BigInteger getActiveInputChannels () const override
  {
    return active_inputs_;
  }

  int getOutputLatencyInSamples () override { return 0; }
  int getInputLatencyInSamples () override { return 0; }

  /** @brief Runs one device cycle: hands the period buffers to the
   * registered callback. */
  void run_cycle ()
  {
    assert (callback_ != nullptr);
    std::array<const float *, fake_num_channels> input_pointers{
      input_channels_[0].data (), input_channels_[1].data ()
    };
    std::array<float *, fake_num_channels> output_pointers{
      output_channels_[0].data (), output_channels_[1].data ()
    };
    const juce::AudioIODeviceCallbackContext context;
    callback_->audioDeviceIOCallbackWithContext (
      input_pointers.data (), fake_num_channels, output_pointers.data (),
      fake_num_channels, buffer_size_, context);
  }

  const std::vector<std::vector<float>> &output_channels () const
  {
    return output_channels_;
  }

  /** @brief Fills the output period buffers with the given level,
   * standing in for content a previous cycle left behind. */
  void seed_output_channels (float level)
  {
    for (auto &channel : output_channels_)
      {
        std::ranges::fill (channel, level);
      }
  }

private:
  FakeAudioIODeviceType          &owner_;
  juce::AudioIODeviceCallback *   callback_ = nullptr;
  juce::BigInteger                active_inputs_;
  juce::BigInteger                active_outputs_;
  double                          sample_rate_ = fake_sample_rate;
  int                             buffer_size_ = fake_block_size;
  bool                            open_ = false;
  bool                            playing_ = false;
  std::vector<std::vector<float>> input_channels_;
  std::vector<std::vector<float>> output_channels_;
};

class FakeAudioIODeviceType final : public juce::AudioIODeviceType
{
public:
  FakeAudioIODeviceType () : AudioIODeviceType ("Fake") { }

  void scanForDevices () override { }

  juce::StringArray getDeviceNames (bool) const override
  {
    return { "Fake Device" };
  }

  int getDefaultDeviceIndex (bool) const override { return 0; }

  int getIndexOfDevice (juce::AudioIODevice * device, bool) const override
  {
    return device != nullptr && device->getName () == "Fake Device" ? 0 : -1;
  }

  bool hasSeparateInputsAndOutputs () const override { return false; }

  juce::AudioIODevice * createDevice (
    const juce::String &output_device_name,
    const juce::String &input_device_name) override
  {
    if (output_device_name.isEmpty () && input_device_name.isEmpty ())
      return nullptr;
    auto * device = new FakeAudioIODevice ("Fake Device", *this);
    last_created_device = device;
    return device;
  }

  /** Most recent device created by createDevice(); owned by the device
   * manager. */
  FakeAudioIODevice * last_created_device = nullptr;

  void forget_device (const FakeAudioIODevice &device)
  {
    if (last_created_device == &device)
      {
        last_created_device = nullptr;
      }
  }
};

FakeAudioIODevice::FakeAudioIODevice (
  juce::String           device_name,
  FakeAudioIODeviceType &owner)
    : juce::AudioIODevice (std::move (device_name), "Fake"), owner_ (owner)
{
}

FakeAudioIODevice::~FakeAudioIODevice ()
{
  owner_.forget_device (*this);
}

/**
 * @brief DeviceManager with its device types replaced by a single fake
 * type whose device is driven manually by the test.
 */
class TestDeviceManager final : public DeviceManager
{
public:
  TestDeviceManager ()
      : DeviceManager (
          [] () -> std::unique_ptr<juce::XmlElement> { return nullptr; },
          [] (const juce::XmlElement &) { })
  {
  }

  void createAudioDeviceTypes (
    juce::OwnedArray<juce::AudioIODeviceType> &types) override
  {
    auto * type = new FakeAudioIODeviceType ();
    fake_device_type_ = type;
    types.add (type);
  }

  FakeAudioIODeviceType * fake_device_type_ = nullptr;
};

class DeviceManagerTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    app_ = std::make_unique<test_helpers::ScopedJuceQApplication> ();
    engine_callback_ = std::make_unique<EngineRenderCallback> ();
    device_manager_ = std::make_unique<TestDeviceManager> ();
    ASSERT_NO_THROW (device_manager_->initialize (
      fake_num_channels, fake_num_channels, false));
    ASSERT_NE (device_manager_->fake_device_type_->last_created_device, nullptr);
    device_manager_->addAudioCallback (engine_callback_.get ());
  }

  void TearDown () override
  {
    device_manager_->removeAudioCallback (engine_callback_.get ());
    device_manager_.reset ();
    engine_callback_.reset ();
    app_.reset ();
  }

  std::unique_ptr<test_helpers::ScopedJuceQApplication> app_;
  std::unique_ptr<EngineRenderCallback>                 engine_callback_;
  std::unique_ptr<TestDeviceManager>                    device_manager_;
};

TEST_F (DeviceManagerTest, DeviceOutputMatchesEngineRenderAcrossCycles)
{
  auto * device = device_manager_->fake_device_type_->last_created_device;
  device->seed_output_channels (1.f);

  constexpr int num_cycles = 3;
  for (const auto _ : std::views::iota (0, num_cycles))
    {
      device->run_cycle ();
    }

  for (const auto &channel : device->output_channels ())
    {
      for (const auto sample : channel)
        {
          EXPECT_NEAR (sample, engine_render_level, 1e-6f);
        }
    }
}

} // namespace

} // namespace zrythm::gui::backend
