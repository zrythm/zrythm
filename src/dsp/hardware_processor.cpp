// SPDX-FileCopyrightText: © 2020-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/engine.h"
#include "dsp/ext_port.h"
#include "dsp/hardware_processor.h"
#include "dsp/midi_event.h"
#include "dsp/port.h"
#include "project.h"
#include "settings/g_settings_manager.h"
#include "settings/settings.h"
#include "utils/arrays.h"
#include "utils/dsp.h"
#include "utils/flags.h"
#include "utils/objects.h"
#include "utils/rt_thread_id.h"
#include "utils/string.h"
#include "zrythm.h"
#include "zrythm_app.h"

void
HardwareProcessor::init_loaded (AudioEngine * engine)
{
  engine_ = engine;
}

bool
HardwareProcessor::is_in_active_project () const
{

  return engine_ && engine_->is_in_active_project ();
}

/**
 * Returns a new empty instance.
 */
HardwareProcessor::HardwareProcessor (bool input, AudioEngine * engine)
    : is_input_ (input), engine_ (engine)
{
}

ExtPort *
HardwareProcessor::find_ext_port (const std::string &id)
{
  {
    auto it = std::find_if (
      ext_audio_ports_.begin (), ext_audio_ports_.end (),
      [&id] (auto &port) { return port->get_id () == id; });
    if (it != ext_audio_ports_.end ())
      return (*it).get ();
  }
  {
    auto it = std::find_if (
      ext_midi_ports_.begin (), ext_midi_ports_.end (),
      [&id] (auto &port) { return port->get_id () == id; });
    if (it != ext_midi_ports_.end ())
      return (*it).get ();
  }

  return nullptr;
}

template <typename T>
T *
HardwareProcessor::find_port (const std::string &id)
{
  auto find_port = [&] (const auto &ext_ports, const auto &ports) {
    for (size_t i = 0; i < ext_ports.size (); ++i)
      {
        const auto &ext_port = ext_ports[i];
        const auto  ext_port_id = ext_port->get_id ();
        auto       &port = ports[i];
        z_return_val_if_fail (ext_port && port, static_cast<T *> (nullptr));
        if (ext_port_id == id)
          {
            return dynamic_cast<T *> (port.get ());
          }
      }
    return static_cast<T *> (nullptr);
  };

  if (auto result = find_port (ext_audio_ports_, audio_ports_))
    {
      return result;
    }
  return find_port (ext_midi_ports_, midi_ports_);
}

template <typename T>
std::unique_ptr<T>
HardwareProcessor::create_port_for_ext_port (
  const ExtPort &ext_port,
  PortFlow       flow)
{
  auto port = std::make_unique<T> (ext_port.full_name_, flow);
  port->set_owner (const_cast<ExtPort *> (&ext_port));
  port->id_.flags_ |= PortIdentifier::Flags::Hw;
  port->id_.ext_port_id_ = ext_port.get_id ();
  return port;
}

bool
HardwareProcessor::rescan_ext_ports (HardwareProcessor * self)
{
  z_debug ("rescanning ports...");

  /* get correct flow */
  PortFlow flow =
    /* these are reversed:
     * input here -> port that outputs in backend */
    self->is_input_ ? PortFlow::Output : PortFlow::Input;

  // Function to collect and add ports
  auto collect_and_add_ports = [&] (PortType type) {
    std::vector<ExtPort> ports;
    ExtPort::ext_ports_get (type, flow, true, ports);

    for (const auto &ext_port : ports)
      {
        if (!self->find_ext_port (ext_port.get_id ()))
          {
            auto new_port = std::make_unique<ExtPort> (ext_port);
            new_port->hw_processor_ = self;

            if (type == PortType::Audio)
              {
                self->ext_audio_ports_.push_back (std::move (new_port));
              }
            else
              {
                self->ext_midi_ports_.push_back (std::move (new_port));
              }

            z_info (
              "[HW] Added {} port {}",
              type == PortType::Audio ? "audio" : "MIDI", ext_port.get_id ());
          }
      }
  };

  // Collect and add audio ports
  collect_and_add_ports (PortType::Audio);

  // Collect and add MIDI ports
  collect_and_add_ports (PortType::Event);

  /* create ports for each ext port */
  self->audio_ports_.reserve (
    std::max (size_t (1), self->ext_audio_ports_.size ()));
  self->midi_ports_.reserve (
    std::max (size_t (1), self->ext_midi_ports_.size ()));

  auto create_ports =
    [&]<typename PortType> (const auto &ext_ports, auto &ports) {
      for (size_t i = 0; i < ext_ports.size (); ++i)
        {
          const auto &ext_port = ext_ports[i];
          assert (ext_port);

          if (i >= ports.size ())
            {
              ports.emplace_back (self->create_port_for_ext_port<PortType> (
                *ext_port, PortFlow::Output));
            }

          assert (ports[i]);
        }
    };

  create_ports.template
  operator()<AudioPort> (self->ext_audio_ports_, self->audio_ports_);
  create_ports.template
  operator()<MidiPort> (self->ext_midi_ports_, self->midi_ports_);

  /* TODO deactivate ports that weren't found (stop engine temporarily to
   * remove) */

  z_debug (
    "[{}] have {} audio and {} MIDI ports",
    self->is_input_ ? "HW processor inputs" : "HW processor outputs",
    self->ext_audio_ports_.size (), self->ext_midi_ports_.size ());

  for (size_t i = 0; i < self->ext_midi_ports_.size (); i++)
    {
      auto &ext_port = self->ext_midi_ports_[i];

      /* attempt to reconnect the if the port needs reconnect (e.g. if
       * disconnected earlier) */
      if (ext_port->pending_reconnect_)
        {
          auto &port = self->midi_ports_[i];
          if (ext_port->activate (port.get (), true))
            {
              ext_port->pending_reconnect_ = false;
            }
        }
    }

  return G_SOURCE_CONTINUE;
}

void
HardwareProcessor::setup ()
{
  if (ZRYTHM_TESTING || ZRYTHM_GENERATING_PROJECT)
    return;

  z_return_if_fail (ZRYTHM_APP_IS_GTK_THREAD && S_P_GENERAL_ENGINE);

  if (is_input_)
    {
      /* cache selections */
      auto tmp = g_settings_get_strv (S_P_GENERAL_ENGINE, "midi-controllers");
      selected_midi_ports_ = StringArray (tmp).toStdStringVector ();
      g_strfreev (tmp);
      tmp = g_settings_get_strv (S_P_GENERAL_ENGINE, "audio-inputs");
      selected_audio_ports_ = StringArray (tmp).toStdStringVector ();
      g_strfreev (tmp);
    }

  /* ---- scan current ports ---- */

  rescan_ext_ports (this);

  /* ---- end scan ---- */

  setup_ = true;
}

void
HardwareProcessor::activate (bool activate)
{
  z_info ("hw processor activate: {}", activate);

  /* go through each selected port and activate/deactivate */
  auto activate_ports = [&] (const auto &selected_ports) {
    for (const auto &selected_port : selected_ports)
      {
        auto ext_port = find_ext_port (selected_port);
        auto port = find_port (selected_port);
        if (port && ext_port)
          {
            ext_port->activate (port, activate);
          }
        else
          {
            z_info ("could not find port {}", selected_port);
          }
      }
  };

  activate_ports (selected_midi_ports_);
  activate_ports (selected_audio_ports_);

  if (activate && !rescan_timeout_id_)
    {
      /* TODO: do the following on another thread - this blocks the UI
       * until then, the rescan is done when temporarily pausing the engine (so
       * on a user action) */
#if 0
      /* add timer to keep rescanning */
      self->rescan_timeout_id = g_timeout_add_seconds (
        7, (GSourceFunc) hardware_processor_rescan_ext_ports, self);
#endif
    }
  else if (!activate && rescan_timeout_id_)
    {
      /* remove timeout */
      g_source_remove (rescan_timeout_id_);
      rescan_timeout_id_ = 0;
    }

  activated_ = activate;
}

void
HardwareProcessor::process (nframes_t nframes)
{
  /* go through each selected port and fetch data */
  for (size_t i = 0; i < audio_ports_.size (); ++i)
    {
      auto &ext_port = ext_audio_ports_[i];
      if (!ext_port->active_)
        continue;

      auto &port = audio_ports_[i];

      /* clear the buffer */
      port->clear_buffer (*AUDIO_ENGINE);

      switch (AUDIO_ENGINE->audio_backend_)
        {
#ifdef HAVE_JACK
        case AudioBackend::AUDIO_BACKEND_JACK:
          port->receive_audio_data_from_jack (0, nframes);
          break;
#endif
#ifdef HAVE_RTAUDIO
        case AudioBackend::AUDIO_BACKEND_ALSA_RTAUDIO:
        case AudioBackend::AUDIO_BACKEND_JACK_RTAUDIO:
        case AudioBackend::AUDIO_BACKEND_PULSEAUDIO_RTAUDIO:
        case AudioBackend::AUDIO_BACKEND_COREAUDIO_RTAUDIO:
        case AudioBackend::AUDIO_BACKEND_WASAPI_RTAUDIO:
        case AudioBackend::AUDIO_BACKEND_ASIO_RTAUDIO:
          /* extract audio data from the RtAudio device ring buffer into RtAudio
           * device temp buffer */
          port->prepare_rtaudio_data ();

          /* copy data from RtAudio temp buffer to normal buffer */
          port->sum_data_from_rtaudio (0, nframes);
          break;
#endif
        default:
          break;
        }
    }
  for (size_t i = 0; i < midi_ports_.size (); i++)
    {
      auto &ext_port = ext_midi_ports_[i];
      if (!ext_port->active_)
        continue;

      auto &port = midi_ports_[i];

      /* clear the buffer */
      port->midi_events_.active_events_.clear ();

      switch (AUDIO_ENGINE->midi_backend_)
        {
#ifdef HAVE_JACK
        case MidiBackend::MIDI_BACKEND_JACK:
          port->receive_midi_events_from_jack (0, nframes);
          break;
#endif
#ifdef HAVE_RTMIDI
        case MidiBackend::MIDI_BACKEND_ALSA_RTMIDI:
        case MidiBackend::MIDI_BACKEND_JACK_RTMIDI:
        case MidiBackend::MIDI_BACKEND_WINDOWS_MME_RTMIDI:
        case MidiBackend::MIDI_BACKEND_COREMIDI_RTMIDI:
#  ifdef HAVE_RTMIDI_6
        case MidiBackend::MIDI_BACKEND_WINDOWS_UWP_RTMIDI:
#  endif
          /* extract MIDI events from the RtMidi device ring buffer into RtMidi
           * device */
          port->prepare_rtmidi_events ();

          /* copy data from RtMidi device events to normal events */
          port->sum_data_from_rtmidi (0, nframes);
          break;
#endif
        default:
          break;
        }
    }
}

void
HardwareProcessor::init_after_cloning (const HardwareProcessor &other)
{
  is_input_ = other.is_input_;

  clone_unique_ptr_container (ext_audio_ports_, other.ext_audio_ports_);
  clone_unique_ptr_container (ext_midi_ports_, other.ext_midi_ports_);
  clone_unique_ptr_container (audio_ports_, other.audio_ports_);
  clone_unique_ptr_container (midi_ports_, other.midi_ports_);
}

template std::unique_ptr<MidiPort>
HardwareProcessor::create_port_for_ext_port (const ExtPort &, PortFlow);
template std::unique_ptr<AudioPort>
HardwareProcessor::create_port_for_ext_port (const ExtPort &, PortFlow);