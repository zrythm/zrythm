// SPDX-FileCopyrightText: © 2020-2021, 2023-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#if HAVE_RTAUDIO

#  include "gui/backend/backend/project.h"
#  include "gui/backend/backend/zrythm.h"
#  include "gui/dsp/engine.h"
#  include "gui/dsp/engine_rtaudio.h"
#  include "gui/dsp/port.h"
#  include "gui/dsp/rtaudio_device.h"

static void
error_cb (rtaudio_error_t err, const char * msg)
{
  z_error ("RtAudio error: {}", msg);
}

static int
myaudio_cb (
  float *                 out_buf,
  float *                 in_buf,
  unsigned int            nframes,
  double                  stream_time,
  rtaudio_stream_status_t status,
  RtAudioDevice *         self)
{
  /* if input device, receive data */
  if (in_buf)
    {
      SemaphoreRAII<> sem_raii (self->audio_ring_sem_);

      if (status != 0)
        {
          /* xrun */
          z_info ("XRUN in RtAudio");
        }

      self->audio_ring_.force_write_multiple (in_buf, nframes);

#  if 0
      for (unsigned int i = 0; i < nframes; i++)
        {
          if (in_buf[i] > 0.08f)
            {
              z_info (
                "have input {:f}", (double) in_buf[i]);
            }
        }
#  endif
    }

  return 0;
}

RtAudioDevice::RtAudioDevice (
  bool         is_input,
  unsigned int device_id,
  unsigned int channel_idx,
  std::string  device_name)
    : is_input_ (is_input), channel_idx_ (channel_idx), id_ (device_id)
{
  handle_ =
    engine_rtaudio_create_rtaudio (AUDIO_ENGINE, AUDIO_ENGINE->audio_backend_);
  if (!handle_)
    {
      throw ZrythmException ("Failed to create RtAudio handle");
    }

  rtaudio_device_info_t dev_nfo;
  if (!device_name.empty ())
    {
      int dev_count = rtaudio_device_count (handle_);
      if (dev_count < 0)
        {
          throw ZrythmException ("Failed to get device count");
        }
      for (unsigned int i = 0; i < static_cast<unsigned int> (dev_count); i++)
        {
          auto cur_dev_nfo = rtaudio_get_device_info (handle_, i);
          if (cur_dev_nfo.name == device_name)
            {
              dev_nfo = cur_dev_nfo;
              break;
            }
        }
    }
  else
    {
      dev_nfo = rtaudio_get_device_info (handle_, device_id);
    }
  name_ = dev_nfo.name;
}

void
RtAudioDevice::open (bool start)
{
  z_debug ("opening rtaudio device");

  auto dev_nfo = rtaudio_get_device_info (handle_, id_);
  z_debug ("RtAudio device {}: {}", id_, dev_nfo.name);

  /* prepare params */
  rtaudio_stream_parameters stream_params = {
    .device_id = id_,
    .num_channels = 1,
    .first_channel = channel_idx_,
  };
  rtaudio_stream_options stream_opts = {
    .flags = RTAUDIO_FLAGS_SCHEDULE_REALTIME,
    .num_buffers = 2,
    .priority = 99,
    .name = "Zrythm",
  };

  auto samplerate = AUDIO_ENGINE->sample_rate_;
  auto buffer_size = AUDIO_ENGINE->block_length_;
  /* input stream */
  auto ret = rtaudio_open_stream (
    handle_, nullptr, &stream_params, RTAUDIO_FORMAT_FLOAT32, samplerate,
    &buffer_size, (rtaudio_cb_t) myaudio_cb, this, &stream_opts,
    (rtaudio_error_cb_t) error_cb);
  if (ret)
    {
      throw ZrythmException (fmt::format (
        "An error occurred opening the RtAudio stream: {}",
        rtaudio_error (handle_)));
    }
  z_info (
    "Opened {} with samplerate {} and buffer size {}", dev_nfo.name, samplerate,
    buffer_size);
  opened_ = true;

  if (start)
    {
      this->start ();
    }
}

void
RtAudioDevice::start ()
{
  auto ret = rtaudio_start_stream (handle_);
  if (ret)
    {
      throw ZrythmException (fmt::format (
        "An error occurred starting the RtAudio stream: {}",
        rtaudio_error (handle_)));
    }
  auto dev_nfo = rtaudio_get_device_info (handle_, id_);
  z_info ("RtAudio device {} started", dev_nfo.name);
  started_ = true;
}

void
RtAudioDevice::stop ()
{
  auto ret = rtaudio_stop_stream (handle_);
  if (ret)
    {
      throw ZrythmException (fmt::format (
        "An error occurred stopping the RtAudio stream: {}",
        rtaudio_error (handle_)));
    }
  auto dev_nfo = rtaudio_get_device_info (handle_, id_);
  z_info ("RtAudio device {} stopped", dev_nfo.name);
  started_ = false;
}

/**
 * Close the RtAudioDevice.
 */
void
RtAudioDevice::close ()
{
  z_debug ("closing rtaudio device");
  rtaudio_close_stream (handle_);
  opened_ = false;
}

void
RtAudioDevice::print_dev_info (const rtaudio_device_info_t &nfo)
{
  z_info (
    "ID: {}\toutput channels: {}\t input channels: {}\tduplex channels: {}\tis default out: {}\tis default in: {}\tpreferred samplerate: {}\tname: {}",
    nfo.id, nfo.output_channels, nfo.input_channels, nfo.duplex_channels,
    nfo.is_default_output, nfo.is_default_input, nfo.preferred_sample_rate,
    nfo.name);
}

#endif // HAVE_RTAUDIO
