// SPDX-FileCopyrightText: © 2019-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#include "common/dsp/engine.h"
#include "common/dsp/engine_jack.h"
#include "common/dsp/engine_rtaudio.h"
#include "common/dsp/engine_rtmidi.h"
#include "common/dsp/ext_port.h"
#include "common/dsp/rtaudio_device.h"
#include "common/dsp/rtmidi_device.h"
#include "common/utils/dsp.h"
#include "common/utils/string.h"
#include "gui/backend/backend/project.h"

#if HAVE_JACK
#  include "weakjack/weak_libjack.h"
#endif

bool
ExtPort::is_in_active_project () const
{
  return hw_processor_ && hw_processor_->is_in_active_project ();
}

float *
ExtPort::get_buffer (nframes_t nframes) const
{
  switch (type_)
    {
#if HAVE_JACK
    case Type::JACK:
      return static_cast<float *> (jack_port_get_buffer (jport_, nframes));
#endif
#ifdef HAVE_ALSA
    case Type::ALSA:
#endif
#if HAVE_RTMIDI
    case Type::RtMidi:
#endif
    default:
      z_return_val_if_reached (nullptr);
    }
}

std::string
ExtPort::get_id () const
{
  static const std::array<std::string, 5> ext_port_type_strings{
    "JACK", "ALSA", "Windows MME", "RtMidi", "RtAudio",
  };
  return fmt::format (
    "{}/{}", ext_port_type_strings[ENUM_VALUE_TO_INT (type_)], full_name_);
}

std::string
ExtPort::get_friendly_name () const
{
  if (num_aliases_ == 2)
    return alias2_;
  else if (num_aliases_ == 1)
    return alias1_;
  else if (!short_name_.empty ())
    return short_name_;
  else
    return full_name_;
}

void
ExtPort::clear_buffer (nframes_t nframes)
{
  auto buf = get_buffer (nframes);
  if (!buf)
    return;

  z_debug ("clearing buffer of {}", fmt::ptr (this));

  dsp_fill (buf, DENORMAL_PREVENTION_VAL (AUDIO_ENGINE), nframes);
}

bool
ExtPort::activate (Port * port, bool activate)
{
  z_info (
    "attempting to {}activate ext port {}", activate ? "" : "de", full_name_);

  if (activate)
    {
      if (is_midi_)
        {
          switch (AUDIO_ENGINE->midi_backend_)
            {
#if HAVE_JACK
            case MidiBackend::MIDI_BACKEND_JACK:
              {
                if (type_ != Type::JACK)
                  {
                    z_info ("skipping {} (not JACK)", full_name_);
                    return false;
                  }

                port_ = port;

                /* expose the port and connect to JACK port */
                if (!jport_)
                  {
                    jport_ = jack_port_by_name (
                      AUDIO_ENGINE->client_, full_name_.c_str ());
                  }
                if (!jport_)
                  {
                    z_warning (
                      "Could not find external JACK port '{}', skipping...",
                      full_name_);
                    return false;
                  }
                port_->set_expose_to_backend (true);

                z_info (
                  "attempting to connect jack port {} to jack port {}",
                  jack_port_name (jport_),
                  jack_port_name (static_cast<jack_port_t *> (port_->data_)));

                int ret = jack_connect (
                  AUDIO_ENGINE->client_, jack_port_name (jport_),
                  jack_port_name (static_cast<jack_port_t *> (port_->data_)));
                if (ret != 0 && ret != EEXIST)
                  {
                    std::string msg = engine_jack_get_error_message (
                      static_cast<jack_status_t> (ret));
                    z_warning (
                      "Failed connecting {} to {}:\n{}", jack_port_name (jport_),
                      jack_port_name (static_cast<jack_port_t *> (port_->data_)),
                      msg);
                    return false;
                  }
              }
              break;
#endif
#if HAVE_RTMIDI
            case MidiBackend::MIDI_BACKEND_ALSA_RTMIDI:
            case MidiBackend::MIDI_BACKEND_JACK_RTMIDI:
            case MidiBackend::MIDI_BACKEND_WINDOWS_MME_RTMIDI:
            case MidiBackend::MIDI_BACKEND_COREMIDI_RTMIDI:
#  if HAVE_RTMIDI_6
            case MidiBackend::MIDI_BACKEND_WINDOWS_UWP_RTMIDI:
#  endif
              {
                if (type_ != Type::RtMidi)
                  {
                    z_info ("skipping {} (not RtMidi)", full_name_);
                    return false;
                  }
                auto midi_port = dynamic_cast<MidiPort *> (port);
                z_return_val_if_fail (midi_port, false);
                port_ = midi_port;
                rtmidi_dev_ = std::make_shared<RtMidiDevice> (
                  true, 0, midi_port,
                  full_name_); // use rtmidi_id_ instead of 0?
                if (!rtmidi_dev_)
                  {
                    z_warning (
                      "Failed creating RtMidi device for {}", full_name_);
                    return false;
                  }
                rtmidi_dev_->open (true);
                midi_port->rtmidi_ins_.clear ();
                midi_port->rtmidi_ins_.push_back (rtmidi_dev_);
              }
              break;
#endif
            default:
              break;
            }
        }
      /* else if not midi */
      else
        {
          switch (AUDIO_ENGINE->audio_backend_)
            {
#if HAVE_JACK
            case AudioBackend::AUDIO_BACKEND_JACK:
              {
                if (type_ != Type::JACK)
                  {
                    z_info ("skipping {} (not JACK)", full_name_);
                    return false;
                  }
                port_ = port;

                /* expose the port and connect to JACK port */
                if (!jport_)
                  {
                    jport_ = jack_port_by_name (
                      AUDIO_ENGINE->client_, full_name_.c_str ());
                  }
                if (!jport_)
                  {
                    z_warning (
                      "Could not find external JACK port '{}', skipping...",
                      full_name_);
                    return false;
                  }
                port_->set_expose_to_backend (true);

                z_info (
                  "attempting to connect jack port {} to jack port {}",
                  jack_port_name (jport_),
                  jack_port_name (static_cast<jack_port_t *> (port_->data_)));

                int ret = jack_connect (
                  AUDIO_ENGINE->client_, jack_port_name (jport_),
                  jack_port_name (static_cast<jack_port_t *> (port_->data_)));
                if (ret != 0 && ret != EEXIST)
                  return false;
              }
              break;
#endif
#if HAVE_RTAUDIO
            case AudioBackend::AUDIO_BACKEND_ALSA_RTAUDIO:
            case AudioBackend::AUDIO_BACKEND_JACK_RTAUDIO:
            case AudioBackend::AUDIO_BACKEND_PULSEAUDIO_RTAUDIO:
            case AudioBackend::AUDIO_BACKEND_COREAUDIO_RTAUDIO:
            case AudioBackend::AUDIO_BACKEND_WASAPI_RTAUDIO:
            case AudioBackend::AUDIO_BACKEND_ASIO_RTAUDIO:
              {
                if (type_ != Type::RtAudio)
                  {
                    z_info ("skipping {} (not RtAudio)", full_name_);
                    return false;
                  }
                auto audio_port = dynamic_cast<AudioPort *> (port);
                z_return_val_if_fail (audio_port, false);
                port_ = port;
                rtaudio_dev_ = std::make_shared<RtAudioDevice> (
                  true, 0, rtaudio_channel_idx_, audio_port,
                  rtaudio_dev_name_); // use rtaudio_id_ instead of 0?
                rtaudio_dev_->open (true);
                if (!rtaudio_dev_)
                  {
                    return false;
                  }
                audio_port->rtaudio_ins_.clear ();
                audio_port->rtaudio_ins_.push_back (rtaudio_dev_);
              }
              break;
#endif
            default:
              break;
            }
        }
    }

  active_ = activate;

  return true;
}

void
ExtPort::disconnect (Port * port, int src)
{
  /* TODO */
}

bool
ExtPort::matches_backend () const
{
  if (!is_midi_)
    {
      switch (AUDIO_ENGINE->audio_backend_)
        {
#if HAVE_JACK
        case AudioBackend::AUDIO_BACKEND_JACK:
          return type_ == Type::JACK;
#endif
#if HAVE_RTAUDIO
        case AudioBackend::AUDIO_BACKEND_ALSA_RTAUDIO:
        case AudioBackend::AUDIO_BACKEND_JACK_RTAUDIO:
        case AudioBackend::AUDIO_BACKEND_PULSEAUDIO_RTAUDIO:
        case AudioBackend::AUDIO_BACKEND_COREAUDIO_RTAUDIO:
        case AudioBackend::AUDIO_BACKEND_WASAPI_RTAUDIO:
        case AudioBackend::AUDIO_BACKEND_ASIO_RTAUDIO:
          return type_ == Type::RtAudio;
#endif
#ifdef HAVE_ALSA
        case AudioBackend::AUDIO_BACKEND_ALSA:
          break;
#endif
        default:
          break;
        }
    }
  else
    {
      switch (AUDIO_ENGINE->midi_backend_)
        {
#if HAVE_JACK
        case MidiBackend::MIDI_BACKEND_JACK:
          return type_ == Type::JACK;
#endif
#ifdef HAVE_ALSA
        case MidiBackend::MIDI_BACKEND_ALSA:
          break;
#endif
#if HAVE_RTMIDI
        case MidiBackend::MIDI_BACKEND_ALSA_RTMIDI:
        case MidiBackend::MIDI_BACKEND_JACK_RTMIDI:
        case MidiBackend::MIDI_BACKEND_WINDOWS_MME_RTMIDI:
        case MidiBackend::MIDI_BACKEND_COREMIDI_RTMIDI:
#  if HAVE_RTMIDI_6
        case MidiBackend::MIDI_BACKEND_WINDOWS_UWP_RTMIDI:
#  endif
          return type_ == Type::RtMidi;
#endif
        default:
          break;
        }
    }
  return false;
}

#if HAVE_JACK
ExtPort::ExtPort (jack_port_t * jport)
    : jport_ (jport), full_name_ (jack_port_name (jport)),
      short_name_ (jack_port_short_name (jport)), type_ (Type::JACK)
{
  std::array<char *, 2> aliases;
  aliases[0] = static_cast<char *> (g_malloc0 (jack_port_name_size ()));
  aliases[1] = static_cast<char *> (g_malloc0 (jack_port_name_size ()));
  num_aliases_ = jack_port_get_aliases (jport, aliases.data ());

  if (num_aliases_ == 2)
    {
      alias2_ = aliases[1];
      alias1_ = aliases[0];
    }
  else if (num_aliases_ == 1)
    {
      /* jack (or pipewire) behaves weird when num_aliases is
       * 1 (it only puts the alias in the 2nd string) */
      if (strlen (aliases[0]) > 0)
        {
          alias1_ = aliases[0];
        }
      else if (strlen (aliases[1]) > 0)
        {
          alias1_ = aliases[1];
        }
      else
        {
          num_aliases_ = 0;
        }
    }

  g_free (aliases[0]);
  g_free (aliases[1]);
}

static void
get_ext_ports_from_jack (
  PortType              type,
  PortFlow              flow,
  bool                  hw,
  std::vector<ExtPort> &ports,
  AudioEngine          &engine)
{
  unsigned long flags = 0;
  if (hw)
    flags |= JackPortIsPhysical;
  if (flow == PortFlow::Input)
    flags |= JackPortIsInput;
  else if (flow == PortFlow::Output)
    flags |= JackPortIsOutput;
  const char * jtype = engine_jack_get_jack_type (type);
  if (!jtype)
    return;

  if (!engine.client_)
    {
      z_error (
        "JACK client is NULL. make sure to call engine_pre_setup() before calling this");
      return;
    }

  const char ** jports = jack_get_ports (engine.client_, nullptr, jtype, flags);

  if (!jports)
    return;

  for (size_t i = 0; jports[i] != nullptr; ++i)
    {
      jack_port_t * jport = jack_port_by_name (engine.client_, jports[i]);

      ports.emplace_back (jport);
    }

  jack_free (jports);
}
#endif

#if HAVE_RTMIDI
/**
 * Creates an ExtPort from a RtMidi port.
 */
static std::unique_ptr<ExtPort>
ext_port_from_rtmidi (unsigned int id)
{
  auto self = std::make_unique<ExtPort> ();

  auto dev = std::make_unique<RtMidiDevice> (true, id, nullptr);
  self->rtmidi_id_ = id;
  int buf_len;
  rtmidi_get_port_name (dev->in_handle_, id, nullptr, &buf_len);
  std::string buf (buf_len + 1, '\0');
  rtmidi_get_port_name (dev->in_handle_, id, buf.data (), &buf_len);
  self->full_name_ = buf;
  self->type_ = ExtPort::Type::RtMidi;

  return self;
}

static void
get_ext_ports_from_rtmidi (
  PortFlow              flow,
  std::vector<ExtPort> &ports,
  AudioEngine          &engine)
{
  if (flow == PortFlow::Output)
    {
      unsigned int num_ports = engine_rtmidi_get_num_in_ports (&engine);
      for (unsigned int i = 0; i < num_ports; i++)
        {
          ports.push_back (*ext_port_from_rtmidi (i));
        }
    }
  else if (flow == PortFlow::Input)
    {
      /* MIDI out devices not handled yet */
    }
}
#endif

#if HAVE_RTAUDIO
/**
 * Creates an ExtPort from a RtAudio port.
 *
 * @param device_name Device name (from RtAudio).
 */
static std::unique_ptr<ExtPort>
ext_port_from_rtaudio (
  unsigned int       id,
  unsigned int       channel_idx,
  const std::string &device_name,
  bool               is_input,
  bool               is_duplex)
{
  auto self = std::make_unique<ExtPort> ();

  self->rtaudio_id_ = id;
  self->rtaudio_channel_idx_ = channel_idx;
  self->rtaudio_is_input_ = is_input;
  self->rtaudio_is_duplex_ = is_duplex;
  self->rtaudio_dev_name_ = device_name;
  self->full_name_ =
    fmt::format ("{} (in {})", self->rtaudio_dev_name_, channel_idx);
  self->type_ = ExtPort::Type::RtAudio;

  return self;
}

static void
get_ext_ports_from_rtaudio (
  PortFlow              flow,
  std::vector<ExtPort> &ports,
  AudioEngine          &engine)
{
  /* note: this is an output port from the graph side that will be used as an
   * input port on the zrythm side */
  if (flow == PortFlow::Output || flow == PortFlow::Input)
    {
      bool      reuse_rtaudio = true;
      rtaudio_t rtaudio = engine.rtaudio_;
      if (!rtaudio)
        {
          reuse_rtaudio = false;
          rtaudio =
            engine_rtaudio_create_rtaudio (&engine, engine.audio_backend_);
        }
      if (!rtaudio)
        {
          z_warn_if_reached ();
          return;
        }
      int num_devs = rtaudio_device_count (rtaudio);
      z_debug ("RtAudio devices found: {}", num_devs);
      for (int i = 0; i < num_devs; i++)
        {
          unsigned int          dev_id = rtaudio_get_device_id (rtaudio, i);
          rtaudio_device_info_t dev_nfo =
            rtaudio_get_device_info (rtaudio, dev_id);

          unsigned int channels =
            (flow == PortFlow::Output)
              ? dev_nfo.input_channels
              : dev_nfo.output_channels;

          if (channels > 0)
            {
              for (unsigned int j = 0; j < channels; j++)
                {
                  ports.push_back (*ext_port_from_rtaudio (
                    dev_id, j, dev_nfo.name, flow == PortFlow::Output, false));
                }
              /* TODO? duplex channels */
            }
        }
      if (!reuse_rtaudio)
        {
          rtaudio_destroy (rtaudio);
        }
    }
}
#endif

void
ExtPort::ext_ports_get (
  PortType              type,
  PortFlow              flow,
  bool                  hw,
  std::vector<ExtPort> &ports,
  AudioEngine          &engine)
{
  if (type == PortType::Audio)
    {
      switch (engine.audio_backend_)
        {
#if HAVE_JACK
        case AudioBackend::AUDIO_BACKEND_JACK:
          get_ext_ports_from_jack (type, flow, hw, ports, engine);
          break;
#endif
#if HAVE_RTAUDIO
        case AudioBackend::AUDIO_BACKEND_ALSA_RTAUDIO:
        case AudioBackend::AUDIO_BACKEND_JACK_RTAUDIO:
        case AudioBackend::AUDIO_BACKEND_PULSEAUDIO_RTAUDIO:
        case AudioBackend::AUDIO_BACKEND_COREAUDIO_RTAUDIO:
        case AudioBackend::AUDIO_BACKEND_WASAPI_RTAUDIO:
        case AudioBackend::AUDIO_BACKEND_ASIO_RTAUDIO:
          get_ext_ports_from_rtaudio (flow, ports, engine);
          break;
#endif
#ifdef HAVE_ALSA
        case AudioBackend::AUDIO_BACKEND_ALSA:
          break;
#endif
        default:
          break;
        }
    }
  else if (type == PortType::Event)
    {
      switch (engine.midi_backend_)
        {
#if HAVE_JACK
        case MidiBackend::MIDI_BACKEND_JACK:
          get_ext_ports_from_jack (type, flow, hw, ports, engine);
          break;
#endif
#ifdef HAVE_ALSA
        case MidiBackend::MIDI_BACKEND_ALSA:
          break;
#endif
#if HAVE_RTMIDI
        case MidiBackend::MIDI_BACKEND_ALSA_RTMIDI:
        case MidiBackend::MIDI_BACKEND_JACK_RTMIDI:
        case MidiBackend::MIDI_BACKEND_WINDOWS_MME_RTMIDI:
        case MidiBackend::MIDI_BACKEND_COREMIDI_RTMIDI:
#  if HAVE_RTMIDI_6
        case MidiBackend::MIDI_BACKEND_WINDOWS_UWP_RTMIDI:
#  endif
          get_ext_ports_from_rtmidi (flow, ports, engine);
          break;
#endif
        default:
          break;
        } /* end switch MIDI backend */

      for (auto &port : ports)
        {
          port.is_midi_ = true;
        }

    } /* endif MIDI */
}

void
ExtPort::print () const
{
  z_info ("Ext port:\nfull name: {}", full_name_);
}

bool
ExtPort::get_enabled () const
{
  /* TODO */
  return true;
}