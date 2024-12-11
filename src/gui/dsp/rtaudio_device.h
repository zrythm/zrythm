/*
 * SPDX-FileCopyrightText: © 2020 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
 */

#ifndef __AUDIO_RTAUDIO_DEVICE_H__
#define __AUDIO_RTAUDIO_DEVICE_H__

#include "zrythm-config.h"

#include <array>
#include <string>

#include "utils/ring_buffer.h"

#if HAVE_RTAUDIO

constexpr int RTAUDIO_DEVICE_BUFFER_SIZE = 32000;

#  include <semaphore>

#  include <rtaudio_c.h>
#  include <stdint.h>

class AudioPort;

/**
 * @addtogroup dsp
 *
 * @{
 */

/**
 * For readability when passing 0/1 for input
 * and output.
 */
enum RtAudioDeviceFlow
{
  RTAUDIO_DEVICE_FLOW_OUTPUT,
  RTAUDIO_DEVICE_FLOW_INPUT,
};

/**
 * This is actually a single channel on a device.
 */
class RtAudioDevice
{
public:
  /**
   * @brief Construct a new Rt Audio Device object
   *
   * @param is_input
   * @param device_id
   * @param channel_idx
   * @param port
   * @param device_name
   * @throw ZrythmException on error.
   */
  RtAudioDevice (
    bool         is_input,
    unsigned int device_id,
    unsigned int channel_idx,
    std::string  device_name = "");

  ~RtAudioDevice ()
  {
    if (opened_)
      {
        close ();
      }
  }

  /**
   * Opens the device.
   *
   * @param start Also start the device.
   *
   * @throw ZrythmException on error.
   */
  void open (bool start);

  /**
   * Close the device.
   */
  void close ();

  /**
   * @brief Start the device
   *
   * @throw ZrythmException on error.
   */
  void start ();

  /**
   * @brief Stop the device
   *
   * @throw ZrythmException on error.
   */
  void stop ();

  static void print_dev_info (const rtaudio_device_info_t &nfo);

public:
  bool is_input_ = false;

  /** Channel index. */
  unsigned int channel_idx_ = 0;

  /** Index (device index from RtAudio). */
  unsigned int id_ = 0;

  std::string name_;

  /** Whether opened or not. */
  bool opened_ = false;

  /** Whether started (running) or not. */
  bool started_ = false;

  rtaudio_t handle_ = nullptr;

  /**
   * Audio data buffer (see port_prepare_rtaudio_data()).
   */
  std::array<float, 0x4000> audio_buf_{};

  /** Ring buffer for audio data. */
  RingBuffer<float> audio_ring_{ RTAUDIO_DEVICE_BUFFER_SIZE };

  /** Semaphore for blocking writing events while
   * events are being read. */
  std::binary_semaphore audio_ring_sem_{ 1 };
};

/**
 * @}
 */

#endif // HAVE_RTAUDIO
#endif
