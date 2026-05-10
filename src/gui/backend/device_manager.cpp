// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <utility>

#include "gui/backend/device_manager.h"
#include "utils/exceptions.h"
#include "utils/logger.h"

#include <QTimer>

#include <juce_audio_utils/juce_audio_utils.h>

namespace zrythm::gui::backend
{

DeviceManager::DeviceManager (
  XmlStateGetter state_getter,
  XmlStateSetter state_setter)
    : state_getter_ (std::move (state_getter)),
      state_setter_ (std::move (state_setter))
{
  addChangeListener (&device_change_listener_);
}

DeviceManager::~DeviceManager ()
{
  removeChangeListener (&device_change_listener_);
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
  if (ret.isNotEmpty ())
    {
      throw utils::exceptions::ZrythmException (
        fmt::format (
          "Audio device initialization failed: {}", ret.toStdString ()));
    }
  Q_EMIT availableAudioInputsChanged ();
}

void
DeviceManager::save_state ()
{
  auto xml = createStateXml ();
  if (xml)
    {
      z_debug ("saving device manager state");
      state_setter_ (*xml);
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

QVector<AudioInputInfo>
DeviceManager::availableAudioInputs () const
{
  QVector<AudioInputInfo> result;
  auto *                  dev = getCurrentAudioDevice ();
  if (dev == nullptr)
    return result;

  const auto active_channels = dev->getActiveInputChannels ();
  const int  num_channels = active_channels.getHighestBit () + 1;
  const auto device_name =
    utils::Utf8String::from_juce_string (dev->getName ()).to_qstring ();

  for (int i = 0; i + 1 < num_channels; i += 2)
    {
      if (!active_channels[i] || !active_channels[i + 1])
        continue;
      result.append (
        {
          .deviceName = device_name,
          .firstChannel = i,
          .stereo = true,
        });
    }
  for (int i = 0; i < num_channels; ++i)
    {
      if (!active_channels[i])
        continue;
      result.append (
        {
          .deviceName = device_name,
          .firstChannel = i,
          .stereo = false,
        });
    }

  return result;
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

void
DeviceManager::DeviceSelectorWindow::closeButtonPressed ()
{
  dev_manager_.save_state ();
  Q_EMIT dev_manager_.availableAudioInputsChanged ();
  auto * dm = &dev_manager_;
  QTimer::singleShot (0, dm, [dm] () { dm->device_selector_window_.reset (); });
}

} // namespace zrythm::engine::device_io
