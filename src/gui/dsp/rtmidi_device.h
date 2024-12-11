// SPDX-FileCopyrightText: © 2020, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __AUDIO_RTMIDI_DEVICE_H__
#define __AUDIO_RTMIDI_DEVICE_H__

#include "zrythm-config.h"

#include "gui/dsp/midi_event.h"

#include "utils/ring_buffer.h"

#if HAVE_RTMIDI

#  include <rtmidi_c.h>

class MidiPort;

/**
 * @addtogroup dsp
 *
 * @{
 */

constexpr size_t MIDI_BUFFER_SIZE = 32768;

/**
 * For readability when passing 0/1 for input
 * and output.
 */
enum RtMidiDeviceFlow
{
  RTMIDI_DEVICE_FLOW_OUTPUT,
  RTMIDI_DEVICE_FLOW_INPUT,
};

class RtMidiDevice
{
public:
  using TimestampGenerator = std::function<utils::MonotonicTime ()>;
  using DeviceNameProvider = std::function<std::string ()>;

  /**
   * @param name If non-empty, search by name instead of by @ref device_id.
   *
   * @throw ZrythmException on error.
   */
  RtMidiDevice (
    bool               is_input,
    unsigned int       device_id,
    DeviceNameProvider device_name_provider,
    std::string        name = "");

  ~RtMidiDevice ()
  {
    if (started_)
      {
        stop ();
      }
    if (opened_)
      {
        close ();
      }
    if (is_input_)
      {
        rtmidi_in_free (in_handle_);
      }
    else
      {
        rtmidi_out_free (out_handle_);
      }
  }

  /**
   * Opens the device.
   *
   * @throw ZrythmException on error.
   */
  void open ();

  /**
   * Close the device.
   */
  void close ();

  /**
   * @brief Start the device.
   *
   * @throw ZrythmException on error.
   */
  void start (TimestampGenerator timestamp_generator);

  /**
   * @brief Stop the device
   */
  void stop ();

public:
  /** 1 for input, 0 for output. */
  bool is_input_{};

  /** Index (device index from RtMidi). */
  unsigned int id_{};

  // std::string name_;

  /** Whether opened. */
  bool opened_{};

  /** Whether started (running). */
  bool started_{};

  /* ---- INPUT ---- */
  RtMidiInPtr in_handle_{};

  /* ---- OUTPUT ---- */
  RtMidiOutPtr out_handle_{};

  /** Associated port. */
  // MidiPort * port_{};

  /** MIDI event ring buffer. */
  RingBuffer<MidiEvent> midi_ring_{ 1024 };

  /** Events enqueued at the beginning of each processing cycle from the ring.
   */
  MidiEventVector events_;

  /** Semaphore for blocking writing events while events are being read. */
  std::binary_semaphore midi_ring_sem_{ 1 };

  TimestampGenerator timestamp_generator_;

  DeviceNameProvider device_name_provider_;
};

/**
 * @}
 */

#endif // HAVE_RTMIDI
#endif // __AUDIO_RTMIDI_DEVICE_H__
