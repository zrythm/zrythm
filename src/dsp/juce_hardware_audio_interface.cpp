// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <cassert>

#include "dsp/juce_hardware_audio_interface.h"
#include "utils/format_juce.h"
#include "utils/logger.h"

namespace zrythm::dsp
{

void
JuceHardwareAudioInterface::JuceCallbackAdapter::audioDeviceAboutToStart (
  juce::AudioIODevice * device)
{
  z_info (
    "{} device '{}' about to start", device->getTypeName (), device->getName ());
  callback_.about_to_start ();
}

void
JuceHardwareAudioInterface::JuceCallbackAdapter::audioDeviceStopped ()
{
  callback_.stopped ();
}

void
JuceHardwareAudioInterface::JuceCallbackAdapter::audioDeviceError (
  const juce::String &errorMessage)
{
  callback_.error (errorMessage.toStdString ());
}

JuceHardwareAudioInterface::JuceHardwareAudioInterface (
  std::shared_ptr<juce::AudioDeviceManager> device_manager)
    : device_manager_ (std::move (device_manager))
{
  assert (device_manager_ != nullptr);
}

JuceHardwareAudioInterface::~JuceHardwareAudioInterface ()
{
  // Remove all adapters before destruction
  for (auto &[callback, adapter] : callback_adapters_)
    {
      device_manager_->removeAudioCallback (adapter.get ());
    }
  callback_adapters_.clear ();
}

[[nodiscard]] AudioDeviceInfo
JuceHardwareAudioInterface::get_device_info () const
{
  auto * dev = device_manager_->getCurrentAudioDevice ();
  assert (dev != nullptr);
  return {
    .device_name = utils::Utf8String::from_juce_string (dev->getName ()),
    .sample_rate =
      units::sample_rate (static_cast<int> (dev->getCurrentSampleRate ())),
    .block_length = units::samples (dev->getCurrentBufferSizeSamples ()),
    .input_channel_count =
      units::channels (dev->getActiveInputChannels ().countNumberOfSetBits ()),
    .output_channel_count =
      units::channels (dev->getActiveOutputChannels ().countNumberOfSetBits ()),
  };
}

void
JuceHardwareAudioInterface::add_audio_callback (IAudioCallback * callback)
{
  assert (callback != nullptr);
  assert (callback_adapters_.find (callback) == callback_adapters_.end ());

  auto adapter = std::make_unique<JuceCallbackAdapter> (*callback);
  device_manager_->addAudioCallback (adapter.get ());
  callback_adapters_.emplace (callback, std::move (adapter));
}

void
JuceHardwareAudioInterface::remove_audio_callback (IAudioCallback * callback)
{
  assert (callback != nullptr);

  auto it = callback_adapters_.find (callback);
  if (it == callback_adapters_.end ())
    return;

  device_manager_->removeAudioCallback (it->second.get ());
  callback_adapters_.erase (it);
}

[[nodiscard]] std::optional<juce::AudioWorkgroup>
JuceHardwareAudioInterface::get_device_audio_workgroup () const
{
  return device_manager_->getDeviceAudioWorkgroup ();
}

std::unique_ptr<IHardwareAudioInterface>
JuceHardwareAudioInterface::create (
  std::shared_ptr<juce::AudioDeviceManager> device_manager)
{
  return std::make_unique<JuceHardwareAudioInterface> (
    std::move (device_manager));
}

} // namespace zrythm::dsp
