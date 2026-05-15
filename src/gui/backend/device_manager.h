// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <QObject>
#include <QtQmlIntegration/qqmlintegration.h>

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_gui_basics/juce_gui_basics.h>

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
 * @brief Wrapper over juce::AudioDeviceManager that exposes changes as signals.
 */
class DeviceManager : public QObject, public juce::AudioDeviceManager
{
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE ("")
  Q_DISABLE_COPY_MOVE (DeviceManager)
  Q_PROPERTY (
    QVector<zrythm::gui::backend::AudioInputInfo> availableAudioInputs READ
      availableAudioInputs NOTIFY availableAudioInputsChanged)

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

private:
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
};
} // namespace zrythm::gui::backend
