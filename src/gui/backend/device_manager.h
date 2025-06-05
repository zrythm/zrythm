// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "utils/types.h"

#include <juce_wrapper.h>

namespace zrythm::engine::device_io
{
/**
 * @brief Wrapper over juce::AudioDeviceManager that exposes changes as signals.
 */
class DeviceManager : public QObject, public juce::AudioDeviceManager
{
  Q_OBJECT
  QML_ELEMENT

public:
  using XmlStateGetter = std::function<std::unique_ptr<juce::XmlElement> ()>;
  using XmlStateSetter = std::function<void (const juce::XmlElement &)>;

  DeviceManager (XmlStateGetter state_getter, XmlStateSetter state_setter);

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

private:
  class DeviceSelectorWindow : public juce::DocumentWindow
  {
  public:
    DeviceSelectorWindow (DeviceManager &dev_manager);
    void closeButtonPressed () override
    {
      dev_manager_.save_state ();
      dev_manager_.device_selector_window_.reset ();
    }

  private:
    DeviceManager &dev_manager_;
  };

private:
  XmlStateGetter                        state_getter_;
  XmlStateSetter                        state_setter_;
  std::unique_ptr<DeviceSelectorWindow> device_selector_window_;
};
} // namespace zrythm::engine::device_io
