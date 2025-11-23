// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <utility>

#include "gui/backend/device_manager.h"

namespace zrythm::gui::backend
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
  auto xml = createStateXml ();
  if (xml)
    {
      z_debug ("saving device manager state");
      state_setter_ (*createStateXml ());
    }
  else
    {
      z_warning ("Can't save state: state XML was empty");
    }
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
  add_device (
    juce::AudioIODeviceType::createAudioIODeviceType_WASAPI (
      juce::WASAPIDeviceMode::shared));
  add_device (
    juce::AudioIODeviceType::createAudioIODeviceType_WASAPI (
      juce::WASAPIDeviceMode::exclusive));
  add_device (
    juce::AudioIODeviceType::createAudioIODeviceType_WASAPI (
      juce::WASAPIDeviceMode::sharedLowLatency));
  add_device (juce::AudioIODeviceType::createAudioIODeviceType_DirectSound ());
  add_device (juce::AudioIODeviceType::createAudioIODeviceType_ASIO ());
  add_device (juce::AudioIODeviceType::createAudioIODeviceType_CoreAudio ());
  add_device (juce::AudioIODeviceType::createAudioIODeviceType_iOSAudio ());
  add_device (juce::AudioIODeviceType::createAudioIODeviceType_Bela ());
  add_device (juce::AudioIODeviceType::createAudioIODeviceType_ALSA ());
  add_device (juce::AudioIODeviceType::createAudioIODeviceType_JACK ());
  add_device (juce::AudioIODeviceType::createAudioIODeviceType_Oboe ());
  add_device (juce::AudioIODeviceType::createAudioIODeviceType_OpenSLES ());
  add_device (juce::AudioIODeviceType::createAudioIODeviceType_Android ());
}

void
DeviceManager::showDeviceSelector ()
{
  if (device_selector_window_)
    {
      device_selector_window_->toFront (true);
      return;
    }
  device_selector_window_ = std::make_unique<DeviceSelectorWindow> (*this);
}

DeviceManager::DeviceSelectorWindow::DeviceSelectorWindow (
  DeviceManager &dev_manager)
    : juce::DocumentWindow (
        "Device Selector",
        juce::Colours::grey,
        DocumentWindow::allButtons,
        true),
      dev_manager_ (dev_manager)
{
  auto * component = new juce::AudioDeviceSelectorComponent (
    dev_manager, 0, 2, 0, 2, true, true, true, true);
  setContentOwned (component, true);
  setUsingNativeTitleBar (true);
  // for some reason width is 0 otherwise
  centreWithSize (
    std::max (component->getWidth (), 400), component->getHeight ());

  setAlwaysOnTop (true);
  setVisible (true);
  toFront (true);
}

} // namespace zrythm::engine::device_io
