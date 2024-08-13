// SPDX-FileCopyrightText: © 2020-2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#include "zrythm.h"

#ifdef HAVE_RTMIDI

#  include "dsp/engine.h"
#  include "dsp/midi_event.h"
#  include "dsp/port.h"
#  include "dsp/rtmidi_device.h"
#  include "project.h"

#  include <gtk/gtk.h>

static enum RtMidiApi
get_api_from_midi_backend (MidiBackend backend)
{
  enum RtMidiApi apis[20];
  int            num_apis = (int) rtmidi_get_compiled_api (apis, 20);
  if (num_apis < 0)
    {
      z_error ("RtMidi: an error occurred fetching compiled APIs");
      return (enum RtMidiApi) - 1;
    }
  for (int i = 0; i < num_apis; i++)
    {
      if (
        backend == MidiBackend::MIDI_BACKEND_ALSA_RTMIDI
        && apis[i] == RTMIDI_API_LINUX_ALSA)
        {
          return apis[i];
        }
      if (
        backend == MidiBackend::MIDI_BACKEND_JACK_RTMIDI
        && apis[i] == RTMIDI_API_UNIX_JACK)
        {
          return apis[i];
        }
      if (
        backend == MidiBackend::MIDI_BACKEND_WINDOWS_MME_RTMIDI
        && apis[i] == RTMIDI_API_WINDOWS_MM)
        {
          return apis[i];
        }
      if (
        backend == MidiBackend::MIDI_BACKEND_COREMIDI_RTMIDI
        && apis[i] == RTMIDI_API_MACOSX_CORE)
        {
          return apis[i];
        }
#  ifdef HAVE_RTMIDI_6
      if (
        backend == MidiBackend::MIDI_BACKEND_WINDOWS_UWP_RTMIDI
        && apis[i] == RTMIDI_API_WINDOWS_UWP)
        {
          return apis[i];
        }
#  endif
    }

  return RTMIDI_API_RTMIDI_DUMMY;
}

/**
 * Midi message callback.
 *
 * @param timestamp The time at which the message
 *   has been received.
 * @param message The midi message.
 */
static void
midi_in_cb (
  double                timestamp,
  const unsigned char * message,
  size_t                message_size,
  RtMidiDevice *        self)
{
  SemaphoreRAII<> sem_raii (self->midi_ring_sem_);

  /* generate timestamp */
  gint64 cur_time = g_get_monotonic_time ();
  gint64 ts = cur_time - self->port_->last_midi_dequeue_;
  z_return_if_fail (ts >= 0);
  if (DEBUGGING)
    {
      z_debug (
        "[{}] message received of size {} at {}",
        self->port_->get_full_designation (), message_size, ts);
    }

  /* add to ring buffer */
  MidiEvent ev;
  std::copy_n (message, message_size, ev.raw_buffer_.data ());
  ev.raw_buffer_sz_ = message_size;
  ev.systime_ = ts;
  self->midi_ring_.write (ev);
}

static bool rtmidi_device_first_run = false;

static int
rtmidi_device_get_id_from_name (bool is_input, std::string name)
{
  if (is_input)
    {
      std::unique_ptr<RtMidiDevice> dev;
      try
        {
          dev = std::make_unique<RtMidiDevice> (true, 0, nullptr, nullptr);
        }
      catch (ZrythmException &e)
        {
          z_error (e.what ());
          return -1;
        }

      unsigned int num_ports = rtmidi_get_port_count (dev->in_handle_);
      for (unsigned int i = 0; i < num_ports; i++)
        {
          int buf_len;
          rtmidi_get_port_name (dev->in_handle_, i, nullptr, &buf_len);
          std::string dev_name (buf_len + 1, '\0');
          rtmidi_get_port_name (dev->in_handle_, i, dev_name.data (), &buf_len);
          if (dev_name == name)
            {
              return (int) i;
            }
        }
    }

  z_warning ("could not find RtMidi device with name {}", name);

  return -1;
}

/**
 * @param name If non-NUL, search by name instead of
 *   by @ref device_id.
 */
RtMidiDevice::RtMidiDevice (
  bool         is_input,
  unsigned int device_id,
  MidiPort *   port,
  std::string  name)
{
  std::array<enum RtMidiApi, 32> apis;
  int num_apis = (int) rtmidi_get_compiled_api (apis.data (), apis.size ());
  if (num_apis < 0)
    {
      throw ZrythmException ("RtMidi: an error occurred fetching compiled APIs");
    }
  if (rtmidi_device_first_run)
    {
      for (int i = 0; i < num_apis; i++)
        {
          z_info ("RtMidi API found: {}", rtmidi_api_name (apis[i]));
        }
      rtmidi_device_first_run = false;
    }

  enum RtMidiApi api = get_api_from_midi_backend (AUDIO_ENGINE->midi_backend_);
  if (api == RTMIDI_API_RTMIDI_DUMMY)
    {
      throw ZrythmException (fmt::format (
        "RtMidi API for {} not enabled",
        ENUM_NAME (AUDIO_ENGINE->midi_backend_)));
    }

  if (!name.empty ())
    {
      int id_from_name = rtmidi_device_get_id_from_name (is_input, name);
      if (id_from_name < 0)
        {
          throw ZrythmException (
            fmt::format ("Could not find RtMidi Device '%s'", name));
        }
      id_ = (unsigned int) id_from_name;
    }
  else
    {
      id_ = device_id;
    }
  is_input_ = is_input;
  port_ = port;
  if (is_input)
    {
      in_handle_ =
        rtmidi_in_create (api, "Zrythm", AUDIO_ENGINE->midi_buf_size_);
      if (!in_handle_->ok)
        {
          throw ZrythmException (fmt::format (
            "An error occurred creating an RtMidi in handle: %s",
            in_handle_->msg));
        }
    }
  else
    {
      /* TODO */
    }
}

void
RtMidiDevice::open (bool start)
{
  z_debug ("opening rtmidi device");
  auto        designation = port_->get_full_designation ();
  std::string lbl = fmt::format ("{} [{}]", designation, id_);
  rtmidi_close_port (in_handle_);
  rtmidi_open_port (in_handle_, id_, lbl.c_str ());
  if (!in_handle_->ok)
    {
      throw ZrythmException (fmt::format (
        "An error occurred opening the RtMidi in port: %s", in_handle_->msg));
    }

  if (start)
    {
      this->start ();
    }
}

void
RtMidiDevice::close ()
{
  z_debug ("closing rtmidi device");
  stop ();

  if (is_input_)
    {
      rtmidi_close_port (in_handle_);
    }
  else
    {
      rtmidi_close_port (out_handle_);
    }
}

void
RtMidiDevice::start ()
{
  z_debug ("starting rtmidi device");
  if (is_input_)
    {
      rtmidi_in_set_callback (in_handle_, (RtMidiCCallback) midi_in_cb, this);
      if (!in_handle_->ok)
        {
          throw ZrythmException (fmt::format (
            "An error occurred starting the RtMidi in port: {}",
            in_handle_->msg));
        }
    }
  started_ = true;
}

void
RtMidiDevice::stop ()
{
  z_debug ("stopping rtmidi device");
  if (is_input_)
    {
      rtmidi_in_cancel_callback (in_handle_);
    }
  started_ = false;
}

#endif // HAVE_RTMIDI
