// SPDX-FileCopyrightText: © 2020-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/backend/project.h"
#include "gui/backend/backend/settings/settings.h"
#include "gui/backend/backend/settings_manager.h"
#include "gui/backend/backend/zrythm.h"
#include "gui/dsp/engine.h"
#include "gui/dsp/ext_port.h"
#include "gui/dsp/hardware_processor.h"
#include "gui/dsp/midi_event.h"
#include "gui/dsp/port.h"

#include "utils/dsp.h"
#include "utils/flags.h"
#include "utils/gtest_wrapper.h"
#include "utils/objects.h"
#include "utils/rt_thread_id.h"
#include "utils/string.h"

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
  port->id_->flags_ |= dsp::PortIdentifier::Flags::Hw;
  port->id_->ext_port_id_ = ext_port.get_id ();
  return port;
}

void
HardwareProcessor::rescan_ext_ports ()
{
  z_debug ("rescanning ports...");

  /* get correct flow */
  PortFlow flow =
    /* these are reversed:
     * input here -> port that outputs in backend */
    is_input_ ? PortFlow::Output : PortFlow::Input;

  // Function to collect and add ports
  auto collect_and_add_ports = [&] (dsp::PortType type) {
    std::vector<ExtPort> ports;
    ExtPort::ext_ports_get (type, flow, true, ports, *engine_);

    for (const auto &ext_port : ports)
      {
        if (!find_ext_port (ext_port.get_id ()))
          {
            auto new_port = std::make_unique<ExtPort> (ext_port);
            new_port->hw_processor_ = this;

            if (type == dsp::PortType::Audio)
              {
                ext_audio_ports_.push_back (std::move (new_port));
              }
            else
              {
                ext_midi_ports_.push_back (std::move (new_port));
              }

            z_info (
              "[HW] Added {} port {}",
              type == dsp::PortType::Audio ? "audio" : "MIDI",
              ext_port.get_id ());
          }
      }
  };

  // Collect and add audio ports
  collect_and_add_ports (dsp::PortType::Audio);

  // Collect and add MIDI ports
  collect_and_add_ports (dsp::PortType::Event);

  /* create ports for each ext port */
  audio_ports_.reserve (std::max (size_t (1), ext_audio_ports_.size ()));
  midi_ports_.reserve (std::max (size_t (1), ext_midi_ports_.size ()));

  auto create_ports = [&]<typename PortType> (const auto &ext_ports, auto &ports) {
    for (size_t i = 0; i < ext_ports.size (); ++i)
      {
        const auto &ext_port = ext_ports[i];
        assert (ext_port);

        if (i >= ports.size ())
          {
            ports.emplace_back (
              create_port_for_ext_port<PortType> (*ext_port, PortFlow::Output));
          }

        assert (ports[i]);
      }
  };

  create_ports.template operator()<AudioPort> (ext_audio_ports_, audio_ports_);
  create_ports.template operator()<MidiPort> (ext_midi_ports_, midi_ports_);

  /* TODO deactivate ports that weren't found (stop engine temporarily to
   * remove) */

  z_debug (
    "[{}] have {} audio and {} MIDI ports",
    is_input_ ? "HW processor inputs" : "HW processor outputs",
    ext_audio_ports_.size (), ext_midi_ports_.size ());

  for (size_t i = 0; i < ext_midi_ports_.size (); i++)
    {
      auto &ext_port = ext_midi_ports_[i];

      /* attempt to reconnect the if the port needs reconnect (e.g. if
       * disconnected earlier) */
      if (ext_port->pending_reconnect_)
        {
          auto &port = midi_ports_[i];
          if (ext_port->activate (port.get (), true))
            {
              ext_port->pending_reconnect_ = false;
            }
        }
    }
}

void
HardwareProcessor::setup ()
{
  if (ZRYTHM_TESTING || ZRYTHM_BENCHMARKING || ZRYTHM_GENERATING_PROJECT)
    return;

  z_warn_if_fail (ZRYTHM_IS_QT_THREAD);

  if (is_input_)
    {
      /* cache selections */
      auto tmp =
        zrythm::gui::SettingsManager::get_instance ()->get_midiControllers ();
      selected_midi_ports_ = StringArray (tmp).toStdStringVector ();
      tmp = zrythm::gui::SettingsManager::get_instance ()->get_audioInputs ();
      selected_audio_ports_ = StringArray (tmp).toStdStringVector ();
    }

  /* ---- scan current ports ---- */

  rescan_ext_ports ();

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
      // g_source_remove (rescan_timeout_id_);
      rescan_timeout_id_ = 0;
    }

  activated_ = activate;
}

std::string
HardwareProcessor::get_node_name () const
{
  return is_input_ ? "HW In Processor" : "HW Out Processor";
}

void
HardwareProcessor::process (nframes_t nframes)
{
  auto process_ports = [nframes] (auto &ports, auto &ext_ports) {
    /* go through each selected port and fetch data */
    for (size_t i = 0; i < ports.size (); ++i)
      {
        auto &ext_port = ext_ports[i];
        if (!ext_port->active_)
          continue;

        auto &port = ports[i];

        using PortType = base_type<decltype (port)>;
        if constexpr (std::is_same_v<PortType, AudioPort>)
          {
            port->clear_buffer (*AUDIO_ENGINE);
          }
        else if constexpr (std::is_same_v<PortType, MidiPort>)
          {
            port->midi_events_.active_events_.clear ();
          }

        if (port->backend_ && port->backend_->is_exposed ())
          {
            if constexpr (std::is_same_v<PortType, AudioPort>)
              {
                port->backend_->sum_data (port->buf_.data (), { 0, nframes });
              }
            else if constexpr (std::is_same_v<PortType, MidiPort>)
              {
                port->backend_->sum_data (
                  port->midi_events_, { 0, nframes },
                  [] (midi_byte_t) { return true; });
              }
          }
      }
  };

  process_ports (audio_ports_, ext_audio_ports_);
  process_ports (midi_ports_, ext_midi_ports_);
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
template Port *
HardwareProcessor::find_port (const std::string &);
