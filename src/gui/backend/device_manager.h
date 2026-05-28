// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <memory>

#include "dsp/hardware_midi_interface.h"
#include "utils/utf8_string.h"

#include <QObject>
#include <QtQmlIntegration/qqmlintegration.h>

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_gui_basics/juce_gui_basics.h>

namespace zrythm::dsp
{
class MidiDeviceBuffer;
}

namespace zrythm::gui::backend
{

/**
 * @brief Describes a single audio input available from the current device.
 */
struct AudioInputInfo
{
  Q_GADGET
  Q_PROPERTY (QString deviceName MEMBER deviceName)
  Q_PROPERTY (int firstChannel MEMBER firstChannel)
  Q_PROPERTY (bool stereo MEMBER stereo)
  QML_VALUE_TYPE (audioInputInfo)
  QML_UNCREATABLE ("")

public:
  QString deviceName;
  int     firstChannel = 0;
  bool    stereo = true;
};

/**
 * @brief Describes a single MIDI input device.
 */
struct MidiInputInfo
{
  Q_GADGET
  Q_PROPERTY (QString deviceName MEMBER deviceName)
  Q_PROPERTY (QString identifier MEMBER identifier)
  QML_VALUE_TYPE (midiInputInfo)
  QML_UNCREATABLE ("")

public:
  QString deviceName;
  QString identifier;
};

/**
 * @brief Wrapper over juce::AudioDeviceManager that exposes changes as signals.
 */
class DeviceManager
    : public QObject,
      public juce::AudioDeviceManager,
      public dsp::IHardwareMidiInterface
{
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE ("")
  Q_DISABLE_COPY_MOVE (DeviceManager)
  Q_PROPERTY (
    QVector<zrythm::gui::backend::AudioInputInfo> availableAudioInputs READ
      availableAudioInputs NOTIFY availableAudioInputsChanged)
  Q_PROPERTY (
    QVector<zrythm::gui::backend::MidiInputInfo> availableMidiInputs READ
      availableMidiInputs NOTIFY availableMidiInputsChanged)

public:
  using XmlStateGetter = std::function<std::unique_ptr<juce::XmlElement> ()>;
  using XmlStateSetter = std::function<void (const juce::XmlElement &)>;

  DeviceManager (XmlStateGetter state_getter, XmlStateSetter state_setter);

  ~DeviceManager () override;

  /**
   * @brief Opens a set of devices ready for use.
   *
   * @param max_input_channels Max number of input channels (channels used might
   * be less than this).
   * @param max_output_channels Max number of output channels (channels used
   * might be less than this).
   * @param fallback_to_default Whether to fallback to the default device if
   * opening the device(s) from the state fails.
   *
   * This calls `juce::AudioDeviceManager::initialise()` internally and passes
   * the state obtained from @ref state_getter_.
   *
   * @exception ZrythmException Error occurred in opening the device(s).
   */
  void initialize (
    int  max_input_channels,
    int  max_output_channels,
    bool fallback_to_default);

  void save_state ();

  // this can be mocked in tests to add a test device
  void createAudioDeviceTypes (
    juce::OwnedArray<juce::AudioIODeviceType> &types) override;

  Q_INVOKABLE void showDeviceSelector ();

  QVector<AudioInputInfo> availableAudioInputs () const;
  Q_SIGNAL void           availableAudioInputsChanged ();

  QVector<MidiInputInfo> availableMidiInputs () const;
  Q_SIGNAL void          availableMidiInputsChanged ();

  dsp::MidiDeviceBuffer *
  midi_buffer_for_device (const utils::Utf8String &identifier) const;

  // IHardwareMidiInterface
  void
  set_device_change_callback (std::optional<DeviceChangeCallback> cb) override;
  BufferMap device_buffers () const override;

private:
  struct MidiImpl;
  friend struct MidiImpl;

  void reconcile_midi_buffers ();

  class DeviceChangeListener final : public juce::ChangeListener
  {
  public:
    explicit DeviceChangeListener (DeviceManager &dev_manager)
        : dev_manager_ (dev_manager)
    {
    }

  private:
    void changeListenerCallback (juce::ChangeBroadcaster *) override
    {
      Q_EMIT dev_manager_.availableAudioInputsChanged ();
      Q_EMIT dev_manager_.availableMidiInputsChanged ();
      dev_manager_.reconcile_midi_buffers ();
    }

    DeviceManager &dev_manager_;
  };

  class DeviceSelectorWindow : public juce::DocumentWindow
  {
  public:
    DeviceSelectorWindow (DeviceManager &dev_manager);
    void closeButtonPressed () override;

  private:
    DeviceManager &dev_manager_;
  };

private:
  XmlStateGetter                        state_getter_;
  XmlStateSetter                        state_setter_;
  std::unique_ptr<DeviceSelectorWindow> device_selector_window_;
  DeviceChangeListener                  device_change_listener_{ *this };
  std::unique_ptr<MidiImpl>             midi_impl_;
};
} // namespace zrythm::gui::backend
