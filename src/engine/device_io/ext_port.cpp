// SPDX-FileCopyrightText: © 2019-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#include "engine/device_io/engine.h"
#include "engine/device_io/engine_jack.h"
#include "engine/device_io/ext_port.h"
#include "gui/backend/backend/project.h"
#include "utils/dsp.h"
#include "utils/string.h"

#ifdef HAVE_JACK
#  include "weakjack/weak_libjack.h"
#endif

namespace zrythm::engine::device_io
{

bool
ExtPort::is_in_active_project () const
{
  return hw_processor_ && hw_processor_->is_in_active_project ();
}

void
ExtPort::set_port_metadata_from_owner (dsp::PortIdentifier &id, PortRange &range)
  const
{
  id.owner_type_ = dsp::PortIdentifier::OwnerType::HardwareProcessor;
}

utils::Utf8String
ExtPort::get_full_designation_for_port (const dsp::PortIdentifier &id) const
{
  return utils::Utf8String::from_utf8_encoded_string (
    fmt::format ("HW/{}", id.label_));
}

float *
ExtPort::get_buffer (nframes_t nframes) const
{
  switch (type_)
    {
#ifdef HAVE_JACK
    case Type::JACK:
      return static_cast<float *> (jack_port_get_buffer (jport_, nframes));
#endif
    default:
      z_return_val_if_reached (nullptr);
    }
}

utils::Utf8String
ExtPort::get_id () const
{
  static const std::array<std::string, 5> ext_port_type_strings{
    "JACK", "ALSA", "Windows MME", "RtMidi", "RtAudio",
  };
  return utils::Utf8String::from_utf8_encoded_string (fmt::format (
    "{}/{}", ext_port_type_strings[ENUM_VALUE_TO_INT (type_)], full_name_));
}

utils::Utf8String
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

  utils::float_ranges::fill (
    buf, DENORMAL_PREVENTION_VAL (AUDIO_ENGINE), nframes);
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
#ifdef HAVE_JACK
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
                AUDIO_ENGINE->set_port_exposed_to_backend (*port_, true);

                auto * jack_backend =
                  dynamic_cast<JackPortBackend *> (port_->backend_.get ());
                auto * target_port = jack_backend->get_jack_port ();
                z_info (
                  "attempting to connect jack port {} to jack port {}",
                  jack_port_name (jport_), jack_port_name (target_port));

                int ret = jack_connect (
                  AUDIO_ENGINE->client_, jack_port_name (jport_),
                  jack_port_name (target_port));
                if (ret != 0 && ret != EEXIST)
                  {
                    const auto msg = utils::jack::get_error_message (
                      static_cast<jack_status_t> (ret));
                    z_warning (
                      "Failed connecting {} to {}:\n{}", jack_port_name (jport_),
                      jack_port_name (target_port), msg);
                    return false;
                  }
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
#ifdef HAVE_JACK
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
                AUDIO_ENGINE->set_port_exposed_to_backend (*port_, true);

                auto * jack_backend =
                  dynamic_cast<JackPortBackend *> (port_->backend_.get ());
                auto * target_port = jack_backend->get_jack_port ();
                z_info (
                  "attempting to connect jack port {} to jack port {}",
                  jack_port_name (jport_), jack_port_name (target_port));

                int ret = jack_connect (
                  AUDIO_ENGINE->client_, jack_port_name (jport_),
                  jack_port_name (target_port));
                if (ret != 0 && ret != EEXIST)
                  return false;
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
#ifdef HAVE_JACK
        case AudioBackend::AUDIO_BACKEND_JACK:
          return type_ == Type::JACK;
#endif
        default:
          break;
        }
    }
  else
    {
      switch (AUDIO_ENGINE->midi_backend_)
        {
#ifdef HAVE_JACK
        case MidiBackend::MIDI_BACKEND_JACK:
          return type_ == Type::JACK;
#endif
        default:
          break;
        }
    }
  return false;
}

#ifdef HAVE_JACK
ExtPort::ExtPort (jack_port_t * jport)
    : jport_ (jport),
      full_name_ (
        utils::Utf8String::from_utf8_encoded_string (jack_port_name (jport))),
      short_name_ (utils::Utf8String::from_utf8_encoded_string (
        jack_port_short_name (jport))),
      type_ (Type::JACK)
{
  std::array<char *, 2> aliases{};
  aliases[0] =
    static_cast<char *> (calloc (jack_port_name_size (), sizeof (char)));
  aliases[1] =
    static_cast<char *> (calloc (jack_port_name_size (), sizeof (char)));
  num_aliases_ = jack_port_get_aliases (jport, aliases.data ());

  if (num_aliases_ == 2)
    {
      alias2_ = utils::Utf8String::from_utf8_encoded_string (aliases[1]);
      alias1_ = utils::Utf8String::from_utf8_encoded_string (aliases[0]);
    }
  else if (num_aliases_ == 1)
    {
      /* jack (or pipewire) behaves weird when num_aliases is
       * 1 (it only puts the alias in the 2nd string) */
      if (strlen (aliases[0]) > 0)
        {
          alias1_ = utils::Utf8String::from_utf8_encoded_string (aliases[0]);
        }
      else if (strlen (aliases[1]) > 0)
        {
          alias1_ = utils::Utf8String::from_utf8_encoded_string (aliases[1]);
        }
      else
        {
          num_aliases_ = 0;
        }
    }

  free (aliases[0]);
  free (aliases[1]);
}

static void
get_ext_ports_from_jack (
  dsp::PortType         type,
  dsp::PortFlow         flow,
  bool                  hw,
  std::vector<ExtPort> &ports,
  AudioEngine          &engine)
{
  unsigned long flags = 0;
  if (hw)
    flags |= JackPortIsPhysical;
  if (flow == dsp::PortFlow::Input)
    flags |= JackPortIsInput;
  else if (flow == dsp::PortFlow::Output)
    flags |= JackPortIsOutput;
  const char * jtype = JackPortBackend::get_jack_type (type);
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
#ifdef HAVE_JACK
        case AudioBackend::AUDIO_BACKEND_JACK:
          get_ext_ports_from_jack (type, flow, hw, ports, engine);
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
#ifdef HAVE_JACK
        case MidiBackend::MIDI_BACKEND_JACK:
          get_ext_ports_from_jack (type, flow, hw, ports, engine);
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
}