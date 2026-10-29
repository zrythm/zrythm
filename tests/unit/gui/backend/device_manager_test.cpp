// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <array>
#include <memory>
#include <string_view>
#include <vector>

#include "gui/backend/device_manager.h"
#include "utils/exceptions.h"

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
    int                     buffer_size_samples) override;

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
  /** Open behavior of a fake device. */
  struct DeviceSpec
  {
    juce::String name;

    /** Every open fails, like a device held by another client. */
    bool busy = false;

    /** Error returned when opening a busy device. */
    juce::String busy_error = "Device or resource busy";

    /** Opens requesting input channels fail, like an ALSA device on a
     * machine with no capture devices. */
    bool no_capture = false;
  };

  FakeAudioIODeviceType () : AudioIODeviceType ("Fake") { }

  void scanForDevices () override { }

  juce::StringArray getDeviceNames (bool) const override
  {
    juce::StringArray names;
    for (const auto &spec : device_specs)
      names.add (spec.name);
    return names;
  }

  int getDefaultDeviceIndex (bool) const override { return 0; }

  int getIndexOfDevice (juce::AudioIODevice * device, bool) const override
  {
    if (device == nullptr)
      return -1;
    return getDeviceNames (false).indexOf (device->getName ());
  }

  bool hasSeparateInputsAndOutputs () const override { return false; }

  juce::AudioIODevice * createDevice (
    const juce::String &output_device_name,
    const juce::String &input_device_name) override
  {
    const auto name =
      output_device_name.isNotEmpty () ? output_device_name : input_device_name;
    if (name.isEmpty () || spec_for (name) == nullptr)
      return nullptr;
    auto * device = new FakeAudioIODevice (name, *this);
    last_created_device = device;
    return device;
  }

  /** Most recent device created by createDevice(); owned by the device
   * manager. */
  FakeAudioIODevice * last_created_device = nullptr;

  /** Devices this type lists; the first one is the default device. */
  std::vector<DeviceSpec> device_specs;

  const DeviceSpec * spec_for (const juce::String &name) const
  {
    for (const auto &spec : device_specs)
      {
        if (spec.name == name)
          return &spec;
      }
    return nullptr;
  }

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

juce::String
FakeAudioIODevice::open (
  const juce::BigInteger &inputs,
  const juce::BigInteger &outputs,
  double                  sample_rate,
  int                     buffer_size_samples)
{
  const auto * spec = owner_.spec_for (getName ());
  assert (spec != nullptr);
  if (spec->busy)
    {
      return spec->busy_error;
    }
  if (spec->no_capture && inputs.countNumberOfSetBits () > 0)
    {
      // Stands in for a machine where no capture device can be opened,
      // e.g. no microphone connected
      return "No such file or directory";
    }
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

FakeAudioIODevice::~FakeAudioIODevice ()
{
  owner_.forget_device (*this);
}

/**
 * @brief DeviceManager with its device types replaced by a single fake
 * type listing the devices in @ref device_specs.
 *
 * @param saved_state Device setup XML served by the state getter.
 */
class TestDeviceManager final : public DeviceManager
{
public:
  explicit TestDeviceManager (
    std::unique_ptr<juce::XmlElement> saved_state = nullptr)
      : DeviceManager (
          [this] () -> std::unique_ptr<juce::XmlElement> {
            return saved_state_
                     ? std::make_unique<juce::XmlElement> (*saved_state_)
                     : nullptr;
          },
          [] (const juce::XmlElement &) { }),
        saved_state_ (std::move (saved_state))
  {
  }

  void createAudioDeviceTypes (
    juce::OwnedArray<juce::AudioIODeviceType> &types) override
  {
    auto * type = new FakeAudioIODeviceType ();
    type->device_specs = device_specs;
    fake_device_type_ = type;
    types.add (type);
  }

  FakeAudioIODeviceType * fake_device_type_ = nullptr;

  /** Devices listed by the fake device type; must be set before
   * initialize(). */
  std::vector<FakeAudioIODeviceType::DeviceSpec> device_specs{
    FakeAudioIODeviceType::DeviceSpec{ .name = juce::String ("Fake Device") }
  };

private:
  std::unique_ptr<juce::XmlElement> saved_state_;
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

// A machine with no capture devices must still get playback: initialize()
// falls back to opening the device with no input channels instead of
// failing altogether
TEST_F (DeviceManagerTest, InitializeFallsBackToOutputOnlyWithoutCapture)
{
  auto manager = std::make_unique<TestDeviceManager> ();
  manager->device_specs = {
    FakeAudioIODeviceType::DeviceSpec{
                                      .name = juce::String ("Fake Device"), .no_capture = true },
  };

  EXPECT_NO_THROW (
    manager->initialize (fake_num_channels, fake_num_channels, true));

  auto * device = manager->fake_device_type_->last_created_device;
  ASSERT_NE (device, nullptr);
  EXPECT_TRUE (device->isPlaying ());
  EXPECT_EQ (device->getActiveInputChannels ().countNumberOfSetBits (), 0);
  EXPECT_GT (device->getActiveOutputChannels ().countNumberOfSetBits (), 0);

  manager.reset ();
}

// The ALSA `default` device can point straight at the sound card and stay
// busy while the audio server owns it, while PipeWire's explicit ALSA
// device opens through the server: initialize() must pick PipeWire instead
// of failing or dropping the input channels
TEST_F (DeviceManagerTest, InitializeFallsBackToPipeWireWhenDefaultIsBusy)
{
  auto manager = std::make_unique<TestDeviceManager> ();
  manager->device_specs = {
    FakeAudioIODeviceType::DeviceSpec{
                                      .name = juce::String ("Default Audio Device"), .busy = true },
    FakeAudioIODeviceType::DeviceSpec{
                                      .name = juce::String ("PipeWire Sound Server") },
  };

  EXPECT_NO_THROW (
    manager->initialize (fake_num_channels, fake_num_channels, true));

  EXPECT_EQ (
    manager->currentAudioDeviceName (),
    QStringLiteral ("PipeWire Sound Server"));
  auto * device = manager->fake_device_type_->last_created_device;
  ASSERT_NE (device, nullptr);
  EXPECT_EQ (device->getName (), juce::String ("PipeWire Sound Server"));
  EXPECT_TRUE (device->isPlaying ());
  EXPECT_EQ (
    device->getActiveInputChannels ().countNumberOfSetBits (),
    fake_num_channels);
  EXPECT_EQ (
    device->getActiveOutputChannels ().countNumberOfSetBits (),
    fake_num_channels);

  manager.reset ();
}

// When no device can be opened, the reported error must lead with the
// initial failure and append the last attempt's error, so the root cause
// is not replaced by a later attempt's error
TEST_F (DeviceManagerTest, InitializeReportsInitialAndLastErrorsWhenAllOpensFail)
{
  auto saved_state = std::make_unique<juce::XmlElement> ("DEVICESETUP");
  saved_state->setAttribute ("deviceType", "Fake");
  saved_state->setAttribute ("audioOutputDeviceName", "Saved Device");
  saved_state->setAttribute ("audioInputDeviceName", "Saved Device");
  auto manager = std::make_unique<TestDeviceManager> (std::move (saved_state));
  // fallback_to_default = false keeps the saved device's error as the
  // initial error: JUCE would otherwise retry the default device and
  // report that error instead
  manager->device_specs = {
    FakeAudioIODeviceType::DeviceSpec{
                                      .name = juce::String ("Default Audio Device"),
                                      .busy = true,
                                      .busy_error = juce::String ("Default device busy") },
    FakeAudioIODeviceType::DeviceSpec{
                                      .name = juce::String ("PipeWire Sound Server"),
                                      .busy = true,
                                      .busy_error = juce::String ("PipeWire busy")       },
    FakeAudioIODeviceType::DeviceSpec{
                                      .name = juce::String ("Saved Device"),
                                      .busy = true,
                                      .busy_error = juce::String ("Saved device busy")   },
  };

  try
    {
      manager->initialize (fake_num_channels, fake_num_channels, false);
      FAIL () << "initialize() was expected to throw";
    }
  catch (const utils::ZrythmException &e)
    {
      const auto message = e.what_string ().str ();
      EXPECT_TRUE (message.starts_with (
        "Audio device initialization failed: Saved device busy"))
        << "message was: " << message;
      EXPECT_NE (
        message.find ("(last attempt: Default device busy)"),
        std::string_view::npos)
        << "message was: " << message;
    }

  manager.reset ();
}

// A saved device selection that opens successfully must be used as-is; the
// fallbacks only engage when the saved selection fails to open
TEST_F (DeviceManagerTest, InitializeUsesSavedDeviceWhenItOpens)
{
  auto saved_state = std::make_unique<juce::XmlElement> ("DEVICESETUP");
  saved_state->setAttribute ("deviceType", "Fake");
  saved_state->setAttribute ("audioOutputDeviceName", "Explicit Device");
  saved_state->setAttribute ("audioInputDeviceName", "Explicit Device");
  auto manager = std::make_unique<TestDeviceManager> (std::move (saved_state));
  manager->device_specs = {
    FakeAudioIODeviceType::DeviceSpec{
                                      .name = juce::String ("Default Audio Device"), .busy = true },
    FakeAudioIODeviceType::DeviceSpec{ .name = juce::String ("Explicit Device") },
  };

  EXPECT_NO_THROW (
    manager->initialize (fake_num_channels, fake_num_channels, true));

  EXPECT_EQ (
    manager->currentAudioDeviceName (), QStringLiteral ("Explicit Device"));
  auto * device = manager->fake_device_type_->last_created_device;
  ASSERT_NE (device, nullptr);
  EXPECT_EQ (device->getName (), juce::String ("Explicit Device"));
  EXPECT_TRUE (device->isPlaying ());
  EXPECT_EQ (
    device->getActiveInputChannels ().countNumberOfSetBits (),
    fake_num_channels);

  manager.reset ();
}

} // namespace

} // namespace zrythm::gui::backend
