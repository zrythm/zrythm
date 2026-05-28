// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <mutex>
#include <shared_mutex>
#include <utility>

#include "dsp/midi_device_buffer.h"
#include "gui/backend/device_manager.h"
#include "utils/exceptions.h"
#include "utils/logger.h"

#include <QTimer>

#include <juce_audio_utils/juce_audio_utils.h>

namespace zrythm::gui::backend
{

struct DeviceManager::MidiImpl
{
  class Callback : public juce::MidiInputCallback
  {
  public:
    explicit Callback (MidiImpl &owner) : owner_ (owner) { }

    void handleIncomingMidiMessage (
      juce::MidiInput *        source,
      const juce::MidiMessage &message) override
    {
      if (source == nullptr)
        return;
      const auto ident =
        utils::Utf8String::from_juce_string (source->getIdentifier ());
      std::shared_lock lock (owner_.buffers_mutex);
      auto             it = owner_.buffers.find (ident);
      if (it != owner_.buffers.end ())
        {
          if (!it->second->push (message))
            {
              ++owner_.overflow_count;
            }
        }
    }

  private:
    MidiImpl &owner_;
  };

  Callback                               callback;
  dsp::IHardwareMidiInterface::BufferMap buffers;
  std::shared_mutex                      buffers_mutex;
  std::atomic<size_t>                    overflow_count{ 0 };
  std::optional<DeviceChangeCallback>    device_change_cb;

  explicit MidiImpl (DeviceManager &owner)
      : callback (*this), owner_ (owner) { }

  void register_with_device_manager ()
  {
    owner_.addMidiInputDeviceCallback ({}, &callback);
  }

  void unregister_from_device_manager ()
  {
    owner_.removeMidiInputDeviceCallback ({}, &callback);
  }

private:
  DeviceManager &owner_;
};

DeviceManager::DeviceManager (
  XmlStateGetter state_getter,
  XmlStateSetter state_setter)
    : state_getter_ (std::move (state_getter)),
      state_setter_ (std::move (state_setter)),
      midi_impl_ (std::make_unique<MidiImpl> (*this))
{
  addChangeListener (&device_change_listener_);
  midi_impl_->register_with_device_manager ();
}

DeviceManager::~DeviceManager ()
{
  midi_impl_->unregister_from_device_manager ();
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
  Q_EMIT availableMidiInputsChanged ();
  reconcile_midi_buffers ();
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

QVector<MidiInputInfo>
DeviceManager::availableMidiInputs () const
{
  QVector<MidiInputInfo> result;
  for (const auto &info : juce::MidiInput::getAvailableDevices ())
    {
      if (isMidiInputDeviceEnabled (info.identifier))
        {
          result.append (
            { .deviceName = QString::fromUtf8 (info.name.toRawUTF8 ()),
              .identifier = QString::fromUtf8 (info.identifier.toRawUTF8 ()) });
        }
    }
  return result;
}

dsp::MidiDeviceBuffer *
DeviceManager::midi_buffer_for_device (const utils::Utf8String &identifier) const
{
  std::shared_lock lock (midi_impl_->buffers_mutex);
  auto             it = midi_impl_->buffers.find (identifier);
  return it != midi_impl_->buffers.end () ? it->second.get () : nullptr;
}

void
DeviceManager::set_device_change_callback (
  std::optional<DeviceChangeCallback> cb)
{
  midi_impl_->device_change_cb = std::move (cb);
}

dsp::IHardwareMidiInterface::BufferMap
DeviceManager::device_buffers () const
{
  std::shared_lock lock (midi_impl_->buffers_mutex);
  return midi_impl_->buffers;
}

void
DeviceManager::reconcile_midi_buffers ()
{
  auto &buffers = midi_impl_->buffers;

  const auto available_devices = juce::MidiInput::getAvailableDevices ();

  {
    std::unique_lock lock (midi_impl_->buffers_mutex);

    std::set<utils::Utf8String> active;
    for (const auto &info : available_devices)
      {
        if (isMidiInputDeviceEnabled (info.identifier))
          {
            const auto ident =
              utils::Utf8String::from_juce_string (info.identifier);
            active.insert (ident);
            if (!buffers.contains (ident))
              {
                buffers.emplace (
                  ident, std::make_shared<dsp::MidiDeviceBuffer> ());
              }
          }
      }

    std::erase_if (buffers, [&active] (const auto &entry) {
      return !active.contains (entry.first);
    });
  }

  if (const auto count = midi_impl_->overflow_count.exchange (0); count > 0)
    {
      z_warning ("{} MIDI events dropped (FIFO full)", count);
    }

  if (midi_impl_->device_change_cb)
    {
      (*midi_impl_->device_change_cb) ();
    }
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
  Q_EMIT dev_manager_.availableMidiInputsChanged ();
  dev_manager_.reconcile_midi_buffers ();
  auto * dm = &dev_manager_;
  QTimer::singleShot (0, dm, [dm] () { dm->device_selector_window_.reset (); });
}

} // namespace zrythm::gui::backend
