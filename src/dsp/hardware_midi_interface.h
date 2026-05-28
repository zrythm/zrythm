// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <functional>
#include <map>

#include "utils/utf8_string.h"

namespace zrythm::dsp
{

class MidiDeviceBuffer;

/**
 * @brief Abstraction for hardware MIDI interface.
 *
 * Decouples the audio engine from specific MIDI backend implementations
 * (JUCE AudioDeviceManager, raw ALSA, CoreMIDI, etc.).
 *
 * The engine registers a notification callback via set_device_change_callback()
 * and pulls the current buffer state via device_buffers().
 *
 * All methods and the callback are main-thread-only.
 */
class IHardwareMidiInterface
{
public:
  using BufferMap =
    std::map<utils::Utf8String, std::shared_ptr<dsp::MidiDeviceBuffer>>;
  using DeviceChangeCallback = std::function<void ()>;

  virtual ~IHardwareMidiInterface () = default;

  /**
   * @brief Registers a notification-only device change callback.
   *
   * The implementation must call @p cb on the main thread whenever the set
   * of active MIDI devices changes. The callback carries no data — the
   * listener should call device_buffers() to pull the current state.
   *
   * Passing nullopt unregisters the callback.
   * Only one callback is supported. Calling again replaces the previous one.
   */
  virtual void
  set_device_change_callback (std::optional<DeviceChangeCallback> cb) = 0;

  /**
   * @brief Returns the current map of device identifiers to buffers.
   *
   * The returned shared_ptrs remain valid as long as the caller holds them,
   * even across device changes (buffers are only destroyed when no longer
   * referenced).
   */
  virtual BufferMap device_buffers () const = 0;
};

}
