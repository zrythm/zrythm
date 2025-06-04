// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <utility>

#include "engine/device_io/device_manager.h"

namespace zrythm::engine::device_io
{

DeviceManager::DeviceManager (
  XmlStateGetter state_getter,
  XmlStateSetter state_setter)
    : state_getter_ (std::move (state_getter)),
      state_setter_ (std::move (state_setter))
{
}

void
DeviceManager::initialize (
  int  max_input_channels,
  int  max_output_channels,
  bool fallback_to_default)
{
  auto ret = initialise (
    max_input_channels, max_output_channels, state_getter_ ().get (),
    fallback_to_default);
}

void
DeviceManager::save_state ()
{
  state_setter_ (*createStateXml ());
}

void
DeviceManager::createAudioDeviceTypes (
  juce::OwnedArray<juce::AudioIODeviceType> &types)
{
  const auto add_device = [&types] (auto * dev) {
    if (dev != nullptr)
      {
        types.add (dev);
      }
  };
  add_device (juce::AudioIODeviceType::createAudioIODeviceType_WASAPI (
    juce::WASAPIDeviceMode::shared));
  add_device (juce::AudioIODeviceType::createAudioIODeviceType_WASAPI (
    juce::WASAPIDeviceMode::exclusive));
  add_device (juce::AudioIODeviceType::createAudioIODeviceType_WASAPI (
    juce::WASAPIDeviceMode::sharedLowLatency));
  add_device (juce::AudioIODeviceType::createAudioIODeviceType_DirectSound ());
  // cannot distribute builds with ASIO support for legal reasons
  // add_device (juce::AudioIODeviceType::createAudioIODeviceType_ASIO ());
  add_device (juce::AudioIODeviceType::createAudioIODeviceType_CoreAudio ());
  add_device (juce::AudioIODeviceType::createAudioIODeviceType_iOSAudio ());
  add_device (juce::AudioIODeviceType::createAudioIODeviceType_Bela ());
  add_device (juce::AudioIODeviceType::createAudioIODeviceType_ALSA ());
  add_device (juce::AudioIODeviceType::createAudioIODeviceType_JACK ());
  add_device (juce::AudioIODeviceType::createAudioIODeviceType_Oboe ());
  add_device (juce::AudioIODeviceType::createAudioIODeviceType_OpenSLES ());
  add_device (juce::AudioIODeviceType::createAudioIODeviceType_Android ());
}

} // namespace zrythm::engine::device_io
