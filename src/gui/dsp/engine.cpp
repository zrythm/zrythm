// SPDX-FileCopyrightText: © 2018-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-FileCopyrightText: © 2020 Ryan Gonzalez <rymg19 at gmail dot com>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
/*
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 * ---
 *
 * Copyright (C) 1999-2002 Paul Davis
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * ---
 */

#include "zrythm-config.h"

#include <cmath>
#include <cstdlib>

#include "dsp/graph_scheduler.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/settings_manager.h"
#include "gui/backend/backend/zrythm.h"
#include "gui/backend/channel.h"
#include "gui/backend/ui.h"
#include "gui/dsp/automation_track.h"
#include "gui/dsp/carla_native_plugin.h"
#include "gui/dsp/channel_track.h"
#include "gui/dsp/engine.h"
#include "gui/dsp/engine_dummy.h"
#include "gui/dsp/engine_jack.h"
#include "gui/dsp/engine_pa.h"
#include "gui/dsp/engine_pulse.h"
#include "gui/dsp/engine_rtaudio.h"
#include "gui/dsp/engine_rtmidi.h"
#include "gui/dsp/hardware_processor.h"
#include "gui/dsp/metronome.h"
#include "gui/dsp/midi_event.h"
#include "gui/dsp/plugin.h"
#include "gui/dsp/pool.h"
#include "gui/dsp/port.h"
#include "gui/dsp/recording_manager.h"
#include "gui/dsp/router.h"
#include "gui/dsp/sample_processor.h"
#include "gui/dsp/tempo_track.h"
#include "gui/dsp/tracklist.h"
#include "gui/dsp/transport.h"
#include "utils/gtest_wrapper.h"
#include "utils/mpmc_queue.h"
#include "utils/object_pool.h"
#include "utils/rt_thread_id.h"

#if HAVE_JACK
#  include "weakjack/weak_libjack.h"
#endif

void
AudioEngine::init_after_cloning (
  const AudioEngine &other,
  ObjectCloneType    clone_type)
{
  transport_type_ = other.transport_type_;
  sample_rate_ = other.sample_rate_;
  frames_per_tick_ = other.frames_per_tick_;
  monitor_out_left_ = other.monitor_out_left_;
  monitor_out_right_ = other.monitor_out_right_;
  midi_editor_manual_press_ = other.midi_editor_manual_press_->clone_unique ();
  midi_in_ = other.midi_in_->clone_unique ();
  pool_ = other.pool_->clone_unique (clone_type, *this);
  control_room_ = other.control_room_->clone_unique ();
  sample_processor_ = other.sample_processor_->clone_unique ();
  sample_processor_->audio_engine_ = this;
  hw_in_processor_ = other.hw_in_processor_->clone_unique ();
  hw_out_processor_ = other.hw_out_processor_->clone_unique ();
  midi_clock_out_ = other.midi_clock_out_->clone_unique ();
}

std::pair<AudioPort &, AudioPort &>
AudioEngine::get_monitor_out_ports ()
{
  if (!monitor_out_left_)
    {
      throw std::runtime_error ("No monitor outputs");
    }
  auto * l = std::get<AudioPort *> (
    port_registry_->find_by_id_or_throw (monitor_out_left_.value ()));
  auto * r = std::get<AudioPort *> (
    port_registry_->find_by_id_or_throw (monitor_out_right_.value ()));
  return { *l, *r };
}

std::pair<AudioPort &, AudioPort &>
AudioEngine::get_dummy_input_ports ()
{
  if (!dummy_left_input_)
    {
      throw std::runtime_error ("No dummy inputs");
    }
  auto * l = std::get<AudioPort *> (
    port_registry_->find_by_id_or_throw (dummy_left_input_.value ()));
  auto * r = std::get<AudioPort *> (
    port_registry_->find_by_id_or_throw (dummy_right_input_.value ()));
  return { *l, *r };
}

void
AudioEngine::set_buffer_size (uint32_t buf_size)
{
  z_return_if_fail (ZRYTHM_IS_QT_THREAD);

  z_debug ("request to set engine buffer size to {}", buf_size);

#if HAVE_JACK
  if (audio_backend_ == AudioBackend::AUDIO_BACKEND_JACK)
    {
      jack_set_buffer_size (client_, buf_size);
      z_debug ("called jack_set_buffer_size");
    }
#endif
}

void
AudioEngine::update_frames_per_tick (
  const int           beats_per_bar,
  const bpm_t         bpm,
  const sample_rate_t sample_rate,
  bool                thread_check,
  bool                update_from_ticks,
  bool                bpm_change)
{
#if 0
  if (ZRYTHM_IS_QT_THREAD)
    {
#endif
  z_debug (
    "updating frames per tick: beats per bar {}, bpm {:f}, sample rate {}",
    beats_per_bar, static_cast<double> (bpm), sample_rate);
#if 0
    }
  else if (thread_check)
    {
      z_error ("Called from non-GTK thread");
      return;
    }
#endif

  updating_frames_per_tick_ = true;

  /* process all recording events */
  RECORDING_MANAGER->process_events ();

  z_return_if_fail (
    beats_per_bar > 0 && bpm > 0 && sample_rate > 0
    && project_->transport_->ticks_per_bar_ > 0);

  z_debug (
    "frames per tick before: {:f} | ticks per frame before: {:f}",
    frames_per_tick_, ticks_per_frame_);

  frames_per_tick_ =
    (static_cast<double> (sample_rate) * 60.0
     * static_cast<double> (beats_per_bar))
    / (static_cast<double> (bpm) * static_cast<double> (project_->transport_->ticks_per_bar_));
  z_return_if_fail (frames_per_tick_ > 1.0);
  ticks_per_frame_ = 1.0 / frames_per_tick_;

  z_debug (
    "frames per tick after: {:f} | ticks per frame after: {:f}",
    frames_per_tick_, ticks_per_frame_);

  /* update positions */
  project_->transport_->update_positions (update_from_ticks);

  for (const auto &track : project_->tracklist_->get_track_span ())
    {
      std::visit (
        [&] (auto &&tr) {
          tr->update_positions (update_from_ticks, bpm_change, frames_per_tick_);
        },
        track);
    }

  updating_frames_per_tick_ = false;
}

int
AudioEngine::clean_duplicate_events_and_copy (std::array<Event *, 100> &ret)
{
  auto &q = ev_queue_;

  int     num_events = 0;
  Event * event = nullptr;
  while (q.pop_front (event))
    {
      bool already_exists = false;

      for (int i = 0; i < num_events; ++i)
        {
          if (
            event->type_ == ret.at (i)->type_ && event->arg_ == ret.at (i)->arg_
            && event->uint_arg_ == ret.at (i)->uint_arg_)
            {
              already_exists = true;
              break;
            }
        }

      if (already_exists)
        {
          ev_pool_.release (event);
        }
      else
        {
          ret[num_events++] = event;
          if (num_events == 100)
            break;
        }
    }

  return num_events;
}

bool
AudioEngine::process_events ()
{
  // we may be creating a project on a different thread
  z_warn_if_fail (ZRYTHM_IS_QT_THREAD);

  if (exporting_)
    {
      return SourceFuncContinue;
    }

  last_events_process_started_ = SteadyClock::now ();

  std::array<Event *, 100> events{};
  int num_events = clean_duplicate_events_and_copy (events);

  State state{};
  bool  need_resume = false;
  if (activated_ && num_events > 0)
    {
      wait_for_pause (state, true, true);
      need_resume = true;
    }

  for (int i = 0; i < num_events; i++)
    {
      if (i > 30)
        {
          z_warning ("more than 30 engine events processed!");
        }
      z_debug ("processing engine event {}", i);

      auto &ev = events[i];

      switch (ev->type_)
        {
        case AudioEngineEventType::AUDIO_ENGINE_EVENT_BUFFER_SIZE_CHANGE:
#if HAVE_JACK
          if (audio_backend_ == AudioBackend::AUDIO_BACKEND_JACK)
            {
              engine_jack_handle_buf_size_change (this, ev->uint_arg_);
            }
#endif
          // EVENTS_PUSH (EventType::ET_ENGINE_BUFFER_SIZE_CHANGED, nullptr);
          break;
        case AudioEngineEventType::AUDIO_ENGINE_EVENT_SAMPLE_RATE_CHANGE:
#if HAVE_JACK
          if (audio_backend_ == AudioBackend::AUDIO_BACKEND_JACK)
            {
              engine_jack_handle_sample_rate_change (this, ev->uint_arg_);
            }
#endif
          // EVENTS_PUSH (EventType::ET_ENGINE_SAMPLE_RATE_CHANGED, nullptr);
          break;
        default:
          z_warning ("event {} not implemented yet", ENUM_NAME (ev->type_));
          break;
        }

      ev_pool_.release (ev);
    }

  if (num_events > 6)
    z_warning ("More than 6 events processed. Optimization needed.");

  if (activated_ && need_resume)
    {
      resume (state);
    }

  last_events_processed_ = SteadyClock::now ();

  return SourceFuncContinue;
}

void
AudioEngine::append_ports (std::vector<Port *> &ports)
{
  auto add_port = [&ports] (Port * port) {
    z_return_if_fail (port);
    ports.push_back (port);
  };

  add_port (&control_room_->monitor_fader_->get_amp_port ());
  add_port (&control_room_->monitor_fader_->get_balance_port ());
  add_port (&control_room_->monitor_fader_->get_mute_port ());
  add_port (&control_room_->monitor_fader_->get_solo_port ());
  add_port (&control_room_->monitor_fader_->get_listen_port ());
  add_port (&control_room_->monitor_fader_->get_mono_compat_enabled_port ());
  add_port (&control_room_->monitor_fader_->get_stereo_in_ports ().first);
  add_port (&control_room_->monitor_fader_->get_stereo_in_ports ().second);
  add_port (&control_room_->monitor_fader_->get_stereo_out_ports ().first);
  add_port (&control_room_->monitor_fader_->get_stereo_out_ports ().second);

  iterate_tuple (
    [&] (auto &port) { add_port (&port); }, get_monitor_out_ports ());
  add_port (midi_editor_manual_press_.get ());
  add_port (midi_in_.get ());

  add_port (&sample_processor_->fader_->get_stereo_in_ports ().first);
  add_port (&sample_processor_->fader_->get_stereo_in_ports ().second);
  add_port (&sample_processor_->fader_->get_stereo_out_ports ().first);
  add_port (&sample_processor_->fader_->get_stereo_out_ports ().second);

  for (const auto &tr : sample_processor_->tracklist_->get_track_span ())
    {
      std::visit (
        [&] (auto &&track) {
          z_warn_if_fail (track->is_auditioner ());
          track->append_ports (ports, true);
        },
        tr);
    }

  add_port (project_->transport_->roll_.get ());
  add_port (project_->transport_->stop_.get ());
  add_port (project_->transport_->backward_.get ());
  add_port (project_->transport_->forward_.get ());
  add_port (project_->transport_->loop_toggle_.get ());
  add_port (project_->transport_->rec_toggle_.get ());

  for (const auto &port : hw_in_processor_->audio_ports_)
    {
      add_port (port.get ());
    }
  for (const auto &port : hw_in_processor_->midi_ports_)
    {
      add_port (port.get ());
    }

  for (const auto &port : hw_out_processor_->audio_ports_)
    {
      add_port (port.get ());
    }
  for (const auto &port : hw_out_processor_->midi_ports_)
    {
      add_port (port.get ());
    }

  add_port (midi_clock_out_.get ());
}

void
AudioEngine::pre_setup ()
{
#if 0
  if (process_source_id_.connected ())
    {
      z_warning ("engine already processing events");
      return;
    }
  z_debug ("starting event timeout");
  process_source_id_ =
    Glib::MainContext::get_default ()->signal_timeout ().connect (
      sigc::mem_fun (*this, &AudioEngine::process_events), 12);
#endif

  z_return_if_fail (!setup_ && !pre_setup_);

  int ret = 0;
  switch (audio_backend_)
    {
    case AudioBackend::AUDIO_BACKEND_DUMMY:
      ret = engine_dummy_setup (this);
      break;
#if HAVE_JACK
    case AudioBackend::AUDIO_BACKEND_JACK:
      ret = engine_jack_setup (this);
      break;
#endif
#if HAVE_PULSEAUDIO
    case AudioBackend::AUDIO_BACKEND_PULSEAUDIO:
      ret = engine_pulse_setup (this);
      break;
#endif
#ifdef HAVE_PORT_AUDIO
    case AudioBackend::AUDIO_BACKEND_PORT_AUDIO:
      ret = engine_pa_setup (this);
      break;
#endif
#if HAVE_RTAUDIO
    case AudioBackend::AUDIO_BACKEND_ALSA_RTAUDIO:
    case AudioBackend::AUDIO_BACKEND_JACK_RTAUDIO:
    case AudioBackend::AUDIO_BACKEND_PULSEAUDIO_RTAUDIO:
    case AudioBackend::AUDIO_BACKEND_COREAUDIO_RTAUDIO:
    case AudioBackend::AUDIO_BACKEND_WASAPI_RTAUDIO:
    case AudioBackend::AUDIO_BACKEND_ASIO_RTAUDIO:
      ret = engine_rtaudio_setup (this);
      break;
#endif
    default:
      z_warning ("Unhandled audio backend");
      break;
    }

  if (ret)
    {
      if (ZRYTHM_HAVE_UI && !ZRYTHM_TESTING)
        {
// TODO
#if 0
          ui_show_message_printf (
            QObject::tr ("Backend Initialization Failed"),
            QObject::tr (
              "Failed to initialize the %s audio backend. Will use the dummy backend instead. Please check your backend settings in the Preferences."),
            AudioBackend_to_string (audio_backend_).c_str ());
#endif
        }

      audio_backend_ = AudioBackend::AUDIO_BACKEND_DUMMY;
      midi_backend_ = MidiBackend::MIDI_BACKEND_DUMMY;
      engine_dummy_setup (this);
    }

  int mret = 0;
  switch (midi_backend_)
    {
    case MidiBackend::MIDI_BACKEND_DUMMY:
      mret = engine_dummy_midi_setup (this);
      break;
#if HAVE_JACK
    case MidiBackend::MIDI_BACKEND_JACK:
      if (client_)
        {
          mret = engine_jack_midi_setup (this);
        }
      else
        {
// TODO
#  if 0
          ui_show_message_printf (
            QObject::tr ("Backend Error"),
            QObject::tr (
              "The JACK MIDI backend can only be used with the JACK audio backend (your current audio backend is %s). Will use the dummy MIDI backend instead."),
            AudioBackend_to_string (audio_backend_).c_str ());
#  endif
          midi_backend_ = MidiBackend::MIDI_BACKEND_DUMMY;
          mret = engine_dummy_midi_setup (this);
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
      mret = engine_rtmidi_setup (this);
      break;
#endif
    default:
      z_warning ("Unhandled MIDI backend");
      break;
    }

  if (mret)
    {
      if (ZRYTHM_HAVE_UI && !ZRYTHM_TESTING)
        {
// TODO
#if 0
          ui_show_message_printf (
            QObject::tr ("Backend Initialization Failed"),
            QObject::tr (
              "Failed to initialize the %s MIDI backend. Will use the dummy backend instead. Please check your backend settings in the Preferences."),
            MidiBackend_to_string (midi_backend_).c_str ());
#endif
        }

      midi_backend_ = MidiBackend::MIDI_BACKEND_DUMMY;
      engine_dummy_midi_setup (this);
    }

  z_debug ("processing engine events");
  process_events ();

  pre_setup_ = true;
}

void
AudioEngine::setup ()
{
  z_debug ("Setting up...");

  z_debug ("processing engine events");
  process_events ();

  hw_in_processor_->setup ();
  hw_out_processor_->setup ();

  if (
    (audio_backend_ == AudioBackend::AUDIO_BACKEND_JACK
     && midi_backend_ != MidiBackend::MIDI_BACKEND_JACK)
    || (audio_backend_ != AudioBackend::AUDIO_BACKEND_JACK && midi_backend_ == MidiBackend::MIDI_BACKEND_JACK))
    {
// TODO
#if 0
      ui_show_message_literal (
        QObject::tr ("Invalid Backend Combination"),
        QObject::tr (
          "Your selected combination of backends may not work properly. If you want to use JACK, please select JACK as both your audio and MIDI backend."));
#endif
    }

  buf_size_set_ = false;

  {
    project_->port_connections_manager_->ensure_connect_default (
      sample_processor_->fader_->get_stereo_out_left_id (),
      control_room_->monitor_fader_->get_stereo_out_left_id (), true);
    project_->port_connections_manager_->ensure_connect_default (
      sample_processor_->fader_->get_stereo_out_right_id (),
      control_room_->monitor_fader_->get_stereo_out_right_id (), true);
  }
  {
    project_->port_connections_manager_->ensure_connect_default (
      control_room_->monitor_fader_->get_stereo_out_left_id (),
      *monitor_out_left_, true);
    project_->port_connections_manager_->ensure_connect_default (
      control_room_->monitor_fader_->get_stereo_out_right_id (),
      *monitor_out_right_, true);
  }
  setup_ = true;

  midi_in_->set_expose_to_backend (*this, true);
  iterate_tuple (
    [&] (auto &port) { port.set_expose_to_backend (*this, true); },
    get_monitor_out_ports ());
  midi_clock_out_->set_expose_to_backend (*this, true);

  z_debug ("processing engine events");
  process_events ();

  z_debug ("done");
}

void
AudioEngine::init_common ()
{
  metronome_ = std::make_unique<Metronome> (*this);
  router_ = std::make_unique<Router> (this);

  auto ab_code = AudioBackend::AUDIO_BACKEND_DUMMY;
  if (ZRYTHM_TESTING || ZRYTHM_BENCHMARKING)
    {
      ab_code =
        gZrythm->use_pipewire_in_tests_
          ? AudioBackend::AUDIO_BACKEND_JACK
          : AudioBackend::AUDIO_BACKEND_DUMMY;
    }
#if 0
  else if (!zrythm_app->audio_backend_.empty ())
    {
      ab_code = AudioBackend_from_string (zrythm_app->audio_backend_);
    }
#endif
  else
    {
      ab_code = static_cast<AudioBackend> (
        zrythm::gui::SettingsManager::get_instance ()->get_audioBackend ());
    }

  bool backend_reset_to_dummy = false;

  switch (ab_code)
    {
    case AudioBackend::AUDIO_BACKEND_DUMMY:
      audio_backend_ = AudioBackend::AUDIO_BACKEND_DUMMY;
      break;
#if HAVE_JACK
    case AudioBackend::AUDIO_BACKEND_JACK:
      audio_backend_ = AudioBackend::AUDIO_BACKEND_JACK;
      break;
#endif
#if HAVE_PULSEAUDIO
    case AudioBackend::AUDIO_BACKEND_PULSEAUDIO:
      audio_backend_ = AudioBackend::AUDIO_BACKEND_PULSEAUDIO;
      break;
#endif
#ifdef HAVE_PORT_AUDIO
    case AudioBackend::AUDIO_BACKEND_PORT_AUDIO:
      audio_backend_ = AudioBackend::AUDIO_BACKEND_PORT_AUDIO;
      break;
#endif
#if HAVE_RTAUDIO
    case AudioBackend::AUDIO_BACKEND_JACK_RTAUDIO:
    case AudioBackend::AUDIO_BACKEND_PULSEAUDIO_RTAUDIO:
    case AudioBackend::AUDIO_BACKEND_COREAUDIO_RTAUDIO:
    case AudioBackend::AUDIO_BACKEND_WASAPI_RTAUDIO:
    case AudioBackend::AUDIO_BACKEND_ASIO_RTAUDIO:
      audio_backend_ = ab_code;
      break;
#endif
    default:
      audio_backend_ = AudioBackend::AUDIO_BACKEND_DUMMY;
      z_warning ("selected audio backend not found. switching to dummy");
      zrythm::gui::SettingsManager::get_instance ()->set_audioBackend (
        ENUM_VALUE_TO_INT (AudioBackend::AUDIO_BACKEND_DUMMY));
      backend_reset_to_dummy = true;
      break;
    }

  auto mb_code = MidiBackend::MIDI_BACKEND_DUMMY;
  if (ZRYTHM_TESTING || ZRYTHM_BENCHMARKING)
    {
      mb_code =
        gZrythm->use_pipewire_in_tests_
          ? MidiBackend::MIDI_BACKEND_JACK
          : MidiBackend::MIDI_BACKEND_DUMMY;
    }
#if 0
  else if (!zrythm_app->midi_backend_.empty ())
    {
      mb_code = MidiBackend_from_string (zrythm_app->midi_backend_);
    }
#endif
  else
    {
      mb_code = static_cast<MidiBackend> (
        zrythm::gui::SettingsManager::get_instance ()->get_midiBackend ());
    }

  switch (mb_code)
    {
    case MidiBackend::MIDI_BACKEND_DUMMY:
      midi_backend_ = MidiBackend::MIDI_BACKEND_DUMMY;
      break;
#if HAVE_JACK
    case MidiBackend::MIDI_BACKEND_JACK:
      midi_backend_ = MidiBackend::MIDI_BACKEND_JACK;
      break;
#endif
#if HAVE_RTMIDI
    case MidiBackend::MIDI_BACKEND_JACK_RTMIDI:
    case MidiBackend::MIDI_BACKEND_WINDOWS_MME_RTMIDI:
    case MidiBackend::MIDI_BACKEND_COREMIDI_RTMIDI:
      midi_backend_ = mb_code;
      break;
#endif
    default:
      midi_backend_ = MidiBackend::MIDI_BACKEND_DUMMY;
      z_warning ("selected midi backend not found. switching to dummy");
      zrythm::gui::SettingsManager::get_instance ()->set_midiBackend (
        ENUM_VALUE_TO_INT (MidiBackend::MIDI_BACKEND_DUMMY));
      backend_reset_to_dummy = true;
      break;
    }

  if (backend_reset_to_dummy && ZRYTHM_HAVE_UI && !ZRYTHM_TESTING)
    {
// TODO
#if 0
      ui_show_message_printf (
        QObject::tr ("Selected Backend Not Found"),
        QObject::tr (
          "The selected MIDI/audio backend was not "
          "found in the version of %s you have "
          "installed. The audio and MIDI backends "
          "were set to \"Dummy\". Please set your "
          "preferred backend from the "
          "preferences."),
        PROGRAM_NAME);
#endif
    }

  pan_law_ =
    ZRYTHM_TESTING || ZRYTHM_BENCHMARKING
      ? zrythm::dsp::PanLaw::Minus3dB
      : static_cast<zrythm::dsp::PanLaw> (
          zrythm::gui::SettingsManager::get_instance ()->get_panLaw ());
  pan_algo_ =
    ZRYTHM_TESTING || ZRYTHM_BENCHMARKING
      ? zrythm::dsp::PanAlgorithm::SineLaw
      : static_cast<zrythm::dsp::PanAlgorithm> (
          zrythm::gui::SettingsManager::get_instance ()->get_panAlgorithm ());

  if (block_length_ == 0)
    {
      block_length_ = 8192;
    }
  if (midi_buf_size_ == 0)
    {
      midi_buf_size_ = 8192;
    }

  midi_clock_out_ =
    std::make_unique<MidiPort> ("MIDI Clock Out", dsp::PortFlow::Output);
  midi_clock_out_->set_owner (*this);
  midi_clock_out_->id_->flags2_ |= dsp::PortIdentifier::Flags2::MidiClock;
}

void
AudioEngine::init_loaded (Project * project)
{
  z_debug ("Initializing...");

  project_ = project;
  port_registry_ = project->get_port_registry ();

  pool_->init_loaded ();

  control_room_->init_loaded (*port_registry_, this);
  sample_processor_->init_loaded (this);
  hw_in_processor_->init_loaded (this);
  hw_out_processor_->init_loaded (this);

  init_common ();

  std::vector<Port *> ports;
  append_ports (ports);
  for (auto * port : ports)
    {
      auto &id = *port->id_;
      if (id.owner_type_ == dsp::PortIdentifier::OwnerType::AudioEngine)
        {
          port->init_loaded (*this);
        }
      else if (
        id.owner_type_ == dsp::PortIdentifier::OwnerType::HardwareProcessor)
        {
// FIXME? this has been either broken or unused for a while
#if 0
          if (id.is_output ())
            port->init_loaded (*hw_in_processor_);
          else if (id.is_input ())
            port->init_loaded (*hw_out_processor_);
#endif
        }
      else if (id.owner_type_ == dsp::PortIdentifier::OwnerType::Fader)
        {
          if (ENUM_BITSET_TEST (
                id.flags2_, dsp::PortIdentifier::Flags2::SampleProcessorFader))
            port->init_loaded (*sample_processor_->fader_);
          else if (
            ENUM_BITSET_TEST (
              id.flags2_, dsp::PortIdentifier::Flags2::MonitorFader))
            port->init_loaded (*control_room_->monitor_fader_);
        }
    }

  z_debug ("done initializing loaded engine");
}

AudioEngine::AudioEngine (Project * project)
    : port_registry_ (project->get_port_registry ()), project_ (project),
      sample_rate_ (44000),
      control_room_ (
        std::make_unique<ControlRoom> (project->get_port_registry (), this)),
      pool_ (std::make_unique<AudioPool> (*this)),
      sample_processor_ (std::make_unique<SampleProcessor> (this))
{
  z_debug ("Creating audio engine...");

  midi_editor_manual_press_ = std::make_unique<MidiPort> (
    "MIDI Editor Manual Press", dsp::PortFlow::Input);
  midi_editor_manual_press_->set_owner (*this);
  midi_editor_manual_press_->id_->sym_ = "midi_editor_manual_press";
  midi_editor_manual_press_->id_->flags_ |=
    dsp::PortIdentifier::Flags::ManualPress;

  midi_in_ = std::make_unique<MidiPort> ("MIDI in", dsp::PortFlow::Input);
  midi_in_->set_owner (*this);
  midi_in_->id_->sym_ = "midi_in";

  {
    auto monitor_out = StereoPorts::create_stereo_ports (
      project_->get_port_registry (), false, "Monitor Out", "monitor_out");
    monitor_out_left_ = monitor_out.first->get_uuid ();
    monitor_out_right_ = monitor_out.second->get_uuid ();
  }

  hw_in_processor_ = std::make_unique<HardwareProcessor> (true, this);
  hw_out_processor_ = std::make_unique<HardwareProcessor> (false, this);

  init_common ();

  z_debug ("finished creating audio engine");
}

TrackRegistry &
AudioEngine::get_track_registry ()
{
  return project_->get_track_registry ();
}
TrackRegistry &
AudioEngine::get_track_registry () const
{
  return project_->get_track_registry ();
}

void
AudioEngine::wait_for_pause (State &state, bool force_pause, bool with_fadeout)
{
  z_debug ("waiting for engine to pause...");

  state.running_ = run_.load ();
  state.playing_ = project_->transport_->isRolling ();
  state.looping_ = project_->transport_->loop_;
  if (!state.running_)
    {
      z_debug ("engine not running - won't wait for pause");
      return;
    }

  if (
    with_fadeout && state.running_
    && (dummy_audio_thread_ == nullptr || !dummy_audio_thread_->threadShouldExit ())
    && has_handled_buffer_size_change ())
    {
      z_debug (
        "setting fade out samples and waiting for remaining samples to become 0");
      control_room_->monitor_fader_->fade_out_samples_.store (
        FADER_DEFAULT_FADE_FRAMES);
      const auto start_time = SteadyClock::now ();
      const auto max_time_to_wait = std::chrono::seconds (2);
      control_room_->monitor_fader_->fading_out_.store (true);
      while (control_room_->monitor_fader_->fade_out_samples_.load () > 0)
        {
          std::this_thread::sleep_for (std::chrono::microseconds (100));
          if (SteadyClock::now () - start_time > max_time_to_wait)
            {
              /* abort */
              control_room_->monitor_fader_->fading_out_.store (false);
              control_room_->monitor_fader_->fade_out_samples_.store (0);
              break;
            }
        }
    }

  if (!destroying_)
    {
      /* send panic */
      midi_in_->midi_events_.panic_all ();
    }

  if (state.playing_)
    {
      project_->transport_->requestPause (true);

      if (force_pause)
        {
          project_->transport_->setPlayState (Transport::PlayState::Paused);
        }
      else
        {
          while (
            project_->transport_->play_state_
              == Transport::PlayState::PauseRequested
            && !dummy_audio_thread_->threadShouldExit ())
            {
              std::this_thread::sleep_for (std::chrono::microseconds (100));
            }
        }
    }

  z_debug ("setting run to false and waiting for cycle to finish...");
  run_.store (false);
  while (cycle_running_.load ())
    {
      std::this_thread::sleep_for (std::chrono::microseconds (100));
    }
  z_debug ("cycle finished");

  /* scan for new ports here for now (why???) (TODO move this to a new thread
   * that runs periodically) */
  hw_in_processor_->rescan_ext_ports ();

  control_room_->monitor_fader_->fading_out_.store (false);

  if (project_ && project_->loaded_)
    {
      /* run 1 more time to flush panic messages */
      SemaphoreRAII sem (port_operation_lock_, true);
      process_prepare (1, &sem);

      EngineProcessTimeInfo time_nfo = {
        .g_start_frame_ = static_cast<unsigned_frame_t> (
          project_->transport_->playhead_pos_->getFrames ()),
        .g_start_frame_w_offset_ = static_cast<unsigned_frame_t> (
          project_->transport_->playhead_pos_->getFrames ()),
        .local_offset_ = 0,
        .nframes_ = 1,
      };

      router_->start_cycle (time_nfo);
      post_process (0, 1);
    }
}

void

AudioEngine::resume (State &state)
{
  z_debug ("resuming engine...");
  if (!state.running_)
    {
      z_debug ("engine was not running - won't resume");
      return;
    }
  project_->transport_->loop_ = state.looping_;
  if (state.playing_)
    {
      project_->transport_->playhead_before_pause_.update_frames_from_ticks (
        0.0);
      project_->transport_->move_playhead (
        &project_->transport_->playhead_before_pause_, false, false, false);
      project_->transport_->requestRoll (true);
    }
  else
    {
      project_->transport_->requestPause (true);
    }

  z_debug ("restarting engine: setting fade in samples");
  control_room_->monitor_fader_->fade_in_samples_.store (
    FADER_DEFAULT_FADE_FRAMES);

  run_.store (state.running_);
}

void
AudioEngine::wait_n_cycles (int n)
{

  auto expected_cycle = cycle_.load () + static_cast<unsigned long> (n);
  while (cycle_.load () < expected_cycle)
    {
      std::this_thread::sleep_for (std::chrono::microseconds (12));
    }
}

void
AudioEngine::activate (bool activate)
{
  z_debug (activate ? "Activating..." : "Deactivating...");
  if (activate)
    {
      if (activated_)
        {
          z_debug ("engine already activated");
          return;
        }

      z_debug ("processing engine events");
      process_events ();

      realloc_port_buffers (block_length_);

      sample_processor_->load_instrument_if_empty ();
    }
  else
    {
      if (!activated_)
        {
          z_debug ("engine already deactivated");
          return;
        }

      State state{};
      wait_for_pause (state, true, true);
      activated_ = false;
    }

  if (!activate)
    {
      hw_in_processor_->activate (false);
    }

#if HAVE_JACK
  if (audio_backend_ == AudioBackend::AUDIO_BACKEND_JACK)
    {
      engine_jack_activate (this, activate);
    }
#endif
#if HAVE_PULSEAUDIO
  if (audio_backend_ == AudioBackend::AUDIO_BACKEND_PULSEAUDIO)
    {
      engine_pulse_activate (this, activate);
    }
#endif
  if (audio_backend_ == AudioBackend::AUDIO_BACKEND_DUMMY)
    {
      engine_dummy_activate (this, activate);
    }
#if HAVE_RTMIDI
  if (midi_backend_is_rtmidi (midi_backend_))
    {
      engine_rtmidi_activate (this, activate);
    }
#endif
#if HAVE_RTAUDIO
  if (audio_backend_is_rtaudio (audio_backend_))
    {
      engine_rtaudio_activate (this, activate);
    }
#endif

  if (activate)
    {
      hw_in_processor_->activate (true);
    }

  z_debug ("processing engine events");
  process_events ();

  activated_ = activate;

  if (ZRYTHM_HAVE_UI && project_->loaded_)
    {
      // EVENTS_PUSH (EventType::ET_ENGINE_ACTIVATE_CHANGED, nullptr);
    }

  z_debug ("done");
}

void
AudioEngine::realloc_port_buffers (nframes_t nframes)
{
  block_length_ = nframes;
  buf_size_set_ = true;
  z_info ("Block length changed to {}. reallocating buffers...", block_length_);

  /* TODO make function that fetches all plugins in the project */
  std::vector<zrythm::gui::old_dsp::plugins::Plugin *> plugins;
  TRACKLIST->get_track_span ().get_plugins (plugins);
  for (auto &pl : plugins)
    {
      if (pl && !pl->instantiation_failed_ && pl->setting_->open_with_carla_)
        {
          auto carla = dynamic_cast<
            zrythm::gui::old_dsp::plugins::CarlaNativePlugin *> (pl);
          carla->update_buffer_size_and_sample_rate ();
        }
    }
  nframes_ = nframes;

  ROUTER->recalc_graph (false);

  z_debug ("done");
}

void
AudioEngine::clear_output_buffers (nframes_t nframes)
{
  /* if graph setup in progress, monitor buffers may be re-allocated so avoid
   * accessing them */
  if (router_->graph_setup_in_progress_.load ()) [[unlikely]]
    return;

  /* clear the monitor output (used by rtaudio) */
  iterate_tuple (
    [&] (auto &port) { port.clear_buffer (*this); }, get_monitor_out_ports ());
  midi_clock_out_->clear_buffer (*this);

  /* if not running, do not attempt to access any possibly deleted ports */
  if (!run_.load ()) [[unlikely]]
    return;

  /* clear outputs exposed to the backend */
  router_->scheduler_->clear_external_output_buffers ();
}

void
AudioEngine::update_position_info (
  PositionInfo   &pos_nfo,
  const nframes_t frames_to_add)
{
  auto transport_ = project_->transport_;
  auto tempo_track = project_->tracklist_->tempo_track_;
  auto playhead = transport_->playhead_pos_->get_position ();
  playhead.add_frames (frames_to_add, ticks_per_frame_);
  pos_nfo.is_rolling_ = transport_->isRolling ();
  pos_nfo.bpm_ = P_TEMPO_TRACK->get_current_bpm ();
  pos_nfo.bar_ = playhead.get_bars (true, transport_->ticks_per_bar_);
  pos_nfo.beat_ = playhead.get_beats (
    true, tempo_track->get_beats_per_bar (), transport_->ticks_per_beat_);
  pos_nfo.sixteenth_ = playhead.get_sixteenths (
    true, tempo_track->get_beats_per_bar (), transport_->sixteenths_per_beat_,
    frames_per_tick_);
  pos_nfo.sixteenth_within_bar_ =
    pos_nfo.sixteenth_ + (pos_nfo.beat_ - 1) * transport_->sixteenths_per_beat_;
  pos_nfo.sixteenth_within_song_ =
    playhead.get_total_sixteenths (false, frames_per_tick_);
  dsp::Position bar_start;
  bar_start.set_to_bar (
    playhead.get_bars (true, transport_->ticks_per_bar_),
    transport_->ticks_per_bar_, frames_per_tick_);
  dsp::Position beat_start;
  beat_start = bar_start;
  beat_start.add_beats (
    pos_nfo.beat_ - 1, transport_->ticks_per_beat_, frames_per_tick_);
  pos_nfo.tick_within_beat_ =
    static_cast<double> (playhead.ticks_ - beat_start.ticks_);
  pos_nfo.tick_within_bar_ =
    static_cast<double> (playhead.ticks_ - bar_start.ticks_);
  pos_nfo.playhead_ticks_ = playhead.ticks_;
  pos_nfo.ninetysixth_notes_ = static_cast<int32_t> (std::floor (
    playhead.ticks_ / dsp::Position::TICKS_PER_NINETYSIXTH_NOTE_DBL));
}

bool
AudioEngine::process_prepare (
  nframes_t                                  nframes,
  SemaphoreRAII<std::counting_semaphore<>> * sem)
{
  if (denormal_prevention_val_positive_)
    {
      denormal_prevention_val_ = -1e-20f;
    }
  else
    {
      denormal_prevention_val_ = 1e-20f;
    }
  denormal_prevention_val_positive_ = !denormal_prevention_val_positive_;

  nframes_ = nframes;

  auto transport_ = project_->transport_;
  if (transport_->play_state_ == Transport::PlayState::PauseRequested)
    {
      if (ZRYTHM_TESTING)
        {
          z_debug ("pause requested handled");
        }
      transport_->set_play_state_rt_safe (Transport::PlayState::Paused);
#if HAVE_JACK
      if (audio_backend_ == AudioBackend::AUDIO_BACKEND_JACK)
        {
          engine_jack_handle_stop (this);
        }
#endif
    }
  else if (
    transport_->play_state_ == Transport::PlayState::RollRequested
    && transport_->countin_frames_remaining_ == 0)
    {
      transport_->set_play_state_rt_safe (Transport::PlayState::Rolling);
      remaining_latency_preroll_ = router_->get_max_route_playback_latency ();
#if HAVE_JACK
      if (audio_backend_ == AudioBackend::AUDIO_BACKEND_JACK)
        {
          engine_jack_handle_start (this);
        }
#endif
    }

  switch (audio_backend_)
    {
#if HAVE_JACK
    case AudioBackend::AUDIO_BACKEND_JACK:
      engine_jack_prepare_process (this);
      break;
#endif
#ifdef HAVE_ALSA
    case AudioBackend::AUDIO_BACKEND_ALSA:
      break;
#endif
    default:
      break;
    }

  clear_output_buffers (nframes);

  if (!exporting_ && !sem->is_acquired ())
    {
      if (ZRYTHM_TESTING)
        z_debug ("port operation lock is busy, skipping cycle...");
      return true;
    }

  update_position_info (pos_nfo_current_, 0);
  {
    nframes_t frames_to_add = 0;
    if (transport_->isRolling ())
      {
        if (remaining_latency_preroll_ < nframes)
          {
            frames_to_add = nframes - remaining_latency_preroll_;
          }
      }
    update_position_info (pos_nfo_at_end_, frames_to_add);
  }

  /* reset all buffers */
  control_room_->monitor_fader_->clear_buffers ();
  midi_in_->clear_buffer (*this);
  midi_editor_manual_press_->clear_buffer (*this);

  sample_processor_->prepare_process (nframes);

  /* prepare channels for this cycle */
  for (
    auto track :
    project_->tracklist_->get_track_span ()
      | std::views::filter (TrackSpan::derived_from_type_projection<ChannelTrack>)
      | std::views::transform (
        TrackSpan::derived_type_transformation<ChannelTrack>))
    {
      track->channel_->prepare_process (nframes);
    }

  return false;
}

void
AudioEngine::receive_midi_events (uint32_t nframes)
{
  if (midi_in_->backend_ && midi_in_->backend_->is_exposed ())
    {
      midi_in_->backend_->sum_data (
        midi_in_->midi_events_, { 0, nframes },
        [] (midi_byte_t) { return true; });
    }
}

int
AudioEngine::process (const nframes_t total_frames_to_process)
{
  /* RAIIs */
  juce::ScopedNoDenormals no_denormals;
  AtomicBoolRAII          cycle_running (cycle_running_);
  SemaphoreRAII           port_operation_sem (port_operation_lock_);

  if (ZRYTHM_TESTING)
    {
      /*z_debug (*/
      /*"engine process started. total frames to "*/
      /*"process: {}", total_frames_to_process);*/
    }

  z_return_val_if_fail (total_frames_to_process > 0, -1);

  // z_info ("processing...");

  /* calculate timestamps (used for synchronizing external events like Windows
   * MME MIDI) */
  timestamp_start_ = Zrythm::getInstance ()->get_monotonic_time_usecs ();
  // timestamp_end_ = timestamp_start_ + (total_frames_to_process * 1000000) /
  // sample_rate_;

  if (!run_.load () || !has_handled_buffer_size_change ()) [[unlikely]]
    {
      // z_info ("skipping processing...");
      clear_output_buffers (total_frames_to_process);
      return 0;
    }

    /* Work around a bug in Pipewire that doesn't inform the host about buffer
     * size (block length) changes */
#if HAVE_JACK
  if (
    audio_backend_ == AudioBackend::AUDIO_BACKEND_JACK && run_.load ()
    && block_length_ != jack_get_buffer_size (client_))
    {
      clear_output_buffers (total_frames_to_process);
      z_warning (
        "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! JACK buffer size changed from {} to {} without notifying us (likely pipewire bug #1591). Attempting workaround...",
        block_length_, jack_get_buffer_size (client_));
      engine_jack_buffer_size_cb (jack_get_buffer_size (client_), this);
      return 0;
    }
#endif

  /* run pre-process code */
  bool skip_cycle =
    process_prepare (total_frames_to_process, &port_operation_sem);

  if (skip_cycle) [[unlikely]]
    {
      // z_info ("skip cycle");
      clear_output_buffers (total_frames_to_process);
      return 0;
    }

  /* puts MIDI in events in the MIDI in port */
  receive_midi_events (total_frames_to_process);

  /* process HW processor to get audio/MIDI data from hardware */
  hw_in_processor_->process (total_frames_to_process);

  nframes_t total_frames_remaining = total_frames_to_process;

  /* --- handle preroll --- */

  auto *                transport_ = project_->transport_;
  EngineProcessTimeInfo split_time_nfo = {
    .g_start_frame_ = (unsigned_frame_t) transport_->playhead_pos_->getFrames (),
    .g_start_frame_w_offset_ =
      (unsigned_frame_t) transport_->playhead_pos_->getFrames (),
    .local_offset_ = 0,
    .nframes_ = 0,
  };

  while (remaining_latency_preroll_ > 0)
    {
      nframes_t num_preroll_frames =
        std::min (total_frames_remaining, remaining_latency_preroll_);
      if (ZRYTHM_TESTING)
        {
          if (num_preroll_frames > 0)
            {
              z_info ("prerolling for {} frames", num_preroll_frames);
            }
        }

      /* loop through each route */
      for (
        const auto start_node : router_->scheduler_->get_nodes ().trigger_nodes_)
        {
          const auto route_latency = start_node.get ().route_playback_latency_;

          if (remaining_latency_preroll_ > route_latency + num_preroll_frames)
            {
              /* this route will no-roll for the complete pre-roll cycle */
            }
          else if (remaining_latency_preroll_ > route_latency)
            {
              /* route may need partial no-roll and partial roll from
               * (transport_sample - remaining_latency_preroll_) .. +
               * num_preroll_frames.
               * shorten and split the process cycle */
              num_preroll_frames = std::min (
                num_preroll_frames, remaining_latency_preroll_ - route_latency);

              /* this route will do a partial roll from num_preroll_frames */
            }
          else
            {
              /* this route will do a normal roll for the complete pre-roll
               * cycle */
            }
        }

      /* offset to start processing at in this cycle */
      nframes_t preroll_offset =
        total_frames_to_process - total_frames_remaining;
      z_warn_if_fail (preroll_offset + num_preroll_frames <= nframes_);

      split_time_nfo.g_start_frame_w_offset_ =
        split_time_nfo.g_start_frame_ + preroll_offset;
      split_time_nfo.local_offset_ = preroll_offset;
      split_time_nfo.nframes_ = num_preroll_frames;
      router_->start_cycle (split_time_nfo);

      remaining_latency_preroll_ -= num_preroll_frames;
      total_frames_remaining -= num_preroll_frames;

      if (total_frames_remaining == 0)
        break;

    } /* while latency preroll frames remaining */

  /* if we still have frames to process (i.e., if
   * preroll finished completely and can start
   * processing normally) */
  if (total_frames_remaining > 0)
    {
      nframes_t cur_offset = total_frames_to_process - total_frames_remaining;

      /* queue metronome if met within this cycle */
      if (transport_->metronome_enabled_ && transport_->isRolling ())
        {
          metronome_->queue_events (this, cur_offset, total_frames_remaining);
        }

      /* split at countin */
      if (transport_->countin_frames_remaining_ > 0)
        {
          const auto countin_frames = std::min (
            total_frames_remaining,
            static_cast<nframes_t> (transport_->countin_frames_remaining_));

          /* process for countin frames */
          split_time_nfo.g_start_frame_w_offset_ =
            split_time_nfo.g_start_frame_ + cur_offset;
          split_time_nfo.local_offset_ = cur_offset;
          split_time_nfo.nframes_ = countin_frames;
          router_->start_cycle (split_time_nfo);
          transport_->countin_frames_remaining_ -= countin_frames;

          /* adjust total frames remaining to process and current offset */
          total_frames_remaining -= countin_frames;
          if (total_frames_remaining == 0)
            goto finalize_processing;
          cur_offset += countin_frames;
        }

      /* split at preroll */
      if (
        transport_->countin_frames_remaining_ == 0
        && transport_->preroll_frames_remaining_ > 0)
        {
          nframes_t preroll_frames = std::min (
            total_frames_remaining,
            static_cast<nframes_t> (transport_->preroll_frames_remaining_));

          /* process for preroll frames */
          split_time_nfo.g_start_frame_w_offset_ =
            split_time_nfo.g_start_frame_ + cur_offset;
          split_time_nfo.local_offset_ = cur_offset;
          split_time_nfo.nframes_ = preroll_frames;
          router_->start_cycle (split_time_nfo);
          transport_->preroll_frames_remaining_ -= preroll_frames;

          /* process for remaining frames */
          cur_offset += preroll_frames;
          const auto remaining_frames = total_frames_remaining - preroll_frames;
          if (remaining_frames > 0)
            {
              split_time_nfo.g_start_frame_w_offset_ =
                split_time_nfo.g_start_frame_ + cur_offset;
              split_time_nfo.local_offset_ = cur_offset;
              split_time_nfo.nframes_ = remaining_frames;
              router_->start_cycle (split_time_nfo);
            }
        }
      else
        {
          /* run the cycle for the remaining frames - this will also play the
           * queued metronome events (if any) */
          split_time_nfo.g_start_frame_w_offset_ =
            split_time_nfo.g_start_frame_ + cur_offset;
          split_time_nfo.local_offset_ = cur_offset;
          split_time_nfo.nframes_ = total_frames_remaining;
          router_->start_cycle (split_time_nfo);
        }
    }

finalize_processing:

  /* run post-process code for the number of frames remaining after handling
   * preroll (if any) */
  post_process (total_frames_remaining, total_frames_to_process);

  cycle_.fetch_add (1);

  if (ZRYTHM_TESTING)
    {
      /*z_debug ("engine process ended...");*/
    }

  last_timestamp_start_ = timestamp_start_;
  // self->last_timestamp_end = Zrythm::getInstance ()->get_monotonic_time_usecs
  // ();

  /*
   * processing finished, return 0 (OK)
   */
  return 0;
}

void
AudioEngine::post_process (const nframes_t roll_nframes, const nframes_t nframes)
{
  if (!exporting_)
    {
      /* fill in the external buffers */
      fill_out_bufs (nframes);
    }

  /* stop panicking */
  panic_.store (false);

  /* remember current position info */
  update_position_info (pos_nfo_before_, 0);

  /* move the playhead if rolling and not pre-rolling */
  auto * transport_ = project_->transport_;
  if (transport_->isRolling () && remaining_latency_preroll_ == 0)
    {
      transport_->add_to_playhead (roll_nframes);
#if HAVE_JACK
      if (audio_backend_ == AudioBackend::AUDIO_BACKEND_JACK)
        {
          engine_jack_handle_position_change (this);
        }
#endif
    }

  /* update max time taken (for calculating DSP %) */
  auto last_time_taken =
    Zrythm::getInstance ()->get_monotonic_time_usecs () - timestamp_start_;
  if (max_time_taken_ < last_time_taken)
    {
      max_time_taken_ = last_time_taken;
    }
}

void
AudioEngine::fill_out_bufs (const nframes_t nframes)
{
  switch (audio_backend_)
    {
    case AudioBackend::AUDIO_BACKEND_DUMMY:
      break;
#ifdef HAVE_ALSA
    case AudioBackend::AUDIO_BACKEND_ALSA:
#  if 0
      engine_alsa_fill_out_bufs (self, nframes);
#  endif
      break;
#endif
#if HAVE_JACK
    case AudioBackend::AUDIO_BACKEND_JACK:
      /*engine_jack_fill_out_bufs (self, nframes);*/
      break;
#endif
#ifdef HAVE_PORT_AUDIO
    case AudioBackend::AUDIO_BACKEND_PORT_AUDIO:
      engine_pa_fill_out_bufs (self, nframes);
      break;
#endif
#if HAVE_RTAUDIO
      /*case AudioBackend::AUDIO_BACKEND_RTAUDIO:*/
      /*break;*/
#endif
    default:
      break;
    }
}

int
AudioEngine::buffer_size_enum_to_int (BufferSize buffer_size)
{
  switch (buffer_size)
    {
    case BufferSize::_16:
      return 16;
    case BufferSize::_32:
      return 32;
    case BufferSize::_64:
      return 64;
    case BufferSize::_128:
      return 128;
    case BufferSize::_256:
      return 256;
    case BufferSize::_512:
      return 512;
    case BufferSize::_1024:
      return 1024;
    case BufferSize::_2048:
      return 2048;
    case BufferSize::_4096:
      return 4096;
    default:
      break;
    }
  z_return_val_if_reached (-1);
}

int
AudioEngine::samplerate_enum_to_int (SampleRate samplerate)
{
  switch (samplerate)
    {
    case SampleRate::SR_22050:
      return 22050;
    case SampleRate::SR_32000:
      return 32000;
    case SampleRate::SR_44100:
      return 44100;
    case SampleRate::SR_48000:
      return 48000;
    case SampleRate::SR_88200:
      return 88200;
    case SampleRate::SR_96000:
      return 96000;
    case SampleRate::SR_192000:
      return 192000;
    default:
      break;
    }
  z_return_val_if_reached (-1);
}

void
AudioEngine::reset_bounce_mode ()
{
  bounce_mode_ = BounceMode::BOUNCE_OFF;

  TRACKLIST->get_track_span ().mark_all_tracks_for_bounce (false);
}

void
AudioEngine::set_default_backends (bool reset_to_dummy)
{
  bool audio_set = false;
  bool midi_set = false;

  if (reset_to_dummy)
    {
      zrythm::gui::SettingsManager::get_instance ()->set_audioBackend (
        ENUM_VALUE_TO_INT (AudioBackend::AUDIO_BACKEND_DUMMY));
      zrythm::gui::SettingsManager::get_instance ()->set_midiBackend (
        ENUM_VALUE_TO_INT (MidiBackend::MIDI_BACKEND_DUMMY));
    }

#if 0
#  if defined(HAVE_JACK) && !defined(_WIN32) && !defined(__APPLE__)
  if (engine_jack_test (nullptr))
    {
      g_settings_set_enum (
        S_P_GENERAL_ENGINE, "audio-backend",
        ENUM_VALUE_TO_INT (AudioBackend::AUDIO_BACKEND_JACK));
      g_settings_set_enum (
        S_P_GENERAL_ENGINE, "midi-backend",
        ENUM_VALUE_TO_INT (MidiBackend::MIDI_BACKEND_JACK));
      audio_set = true;
      midi_set = true;
    }
#  endif

#  if HAVE_PULSEAUDIO
  if (!audio_set && engine_pulse_test (nullptr))
    {
      g_settings_set_enum (
        S_P_GENERAL_ENGINE, "audio-backend",
        ENUM_VALUE_TO_INT (AudioBackend::AUDIO_BACKEND_PULSEAUDIO));
      audio_set = true;
    }
#  endif
#endif

  /* default to RtAudio if above failed */
  if (!audio_set)
    {
#ifdef _WIN32
      g_settings_set_enum (
        S_P_GENERAL_ENGINE, "audio-backend",
        ENUM_VALUE_TO_INT (AudioBackend::AUDIO_BACKEND_WASAPI_RTAUDIO));
      audio_set = true;
#elifdef __APPLE__
      g_settings_set_enum (
        S_P_GENERAL_ENGINE, "audio-backend",
        ENUM_VALUE_TO_INT (AudioBackend::AUDIO_BACKEND_COREAUDIO_RTAUDIO));
      audio_set = true;
#endif
    }

  /* default to RtMidi if above failed */
  if (!midi_set)
    {
#ifdef _WIN32
      g_settings_set_enum (
        S_P_GENERAL_ENGINE, "midi-backend",
        ENUM_VALUE_TO_INT (MidiBackend::MIDI_BACKEND_WINDOWS_MME_RTMIDI));
      audio_set = true;
#elifdef __APPLE__
      g_settings_set_enum (
        S_P_GENERAL_ENGINE, "midi-backend",
        ENUM_VALUE_TO_INT (MidiBackend::MIDI_BACKEND_COREMIDI_RTMIDI));
      audio_set = true;
#endif
    }
}

void
AudioEngine::stop_events ()
{
  // process_source_id_.disconnect ();

  if (activated_)
    {
      /* process any remaining events - clear the queue. */
      process_events ();
    }
}

bool
AudioEngine::is_in_active_project () const
{
  return project_ == PROJECT;
}

void
AudioEngine::set_port_metadata_from_owner (
  dsp::PortIdentifier &id,
  PortRange           &range) const
{
  id.owner_type_ = dsp::PortIdentifier::OwnerType::AudioEngine;
}

std::string
AudioEngine::get_full_designation_for_port (const dsp::PortIdentifier &id) const
{
  return id.get_label ();
}

AudioEngine::~AudioEngine ()
{
  z_debug ("freeing engine...");
  destroying_ = true;

  stop_events ();

  if (router_ && router_->scheduler_)
    {
      /* terminate graph threads */
      router_->scheduler_->terminate_threads ();
    }

  if (activated_)
    {
      activate (false);
    }

  switch (audio_backend_)
    {
#if HAVE_JACK
    case AudioBackend::AUDIO_BACKEND_JACK:
      engine_jack_tear_down (this);
      break;
#endif
#if HAVE_RTAUDIO
    case AudioBackend::AUDIO_BACKEND_ALSA_RTAUDIO:
    case AudioBackend::AUDIO_BACKEND_JACK_RTAUDIO:
    case AudioBackend::AUDIO_BACKEND_PULSEAUDIO_RTAUDIO:
    case AudioBackend::AUDIO_BACKEND_COREAUDIO_RTAUDIO:
    case AudioBackend::AUDIO_BACKEND_WASAPI_RTAUDIO:
    case AudioBackend::AUDIO_BACKEND_ASIO_RTAUDIO:
      engine_rtaudio_tear_down (this);
      break;
#endif
#if HAVE_PULSEAUDIO
    case AudioBackend::AUDIO_BACKEND_PULSEAUDIO:
      engine_pulse_tear_down (this);
      break;
#endif
    case AudioBackend::AUDIO_BACKEND_DUMMY:
      engine_dummy_tear_down (this);
      break;
    default:
      break;
    }

// TODO
#if 0
  if (PROJECT && AUDIO_ENGINE && this == AUDIO_ENGINE)
    {
      iterate_tuple (
        [&] (auto &port) { port.disconnect_all (); }, get_monitor_out_ports ());
      midi_in_->disconnect_all ();
      midi_editor_manual_press_->disconnect_all ();
    }
#endif

  z_debug ("finished freeing engine");
}

void
EngineProcessTimeInfo::print () const
{
  z_info (
    "Global start frame: {} (with offset {}) | local offset: {} | num frames: {}",
    g_start_frame_, g_start_frame_w_offset_, local_offset_, nframes_);
}

AudioEngine *
AudioEngine::get_active_instance ()
{
  auto prj = Project::get_active_instance ();
  if (prj)
    {
      return prj->audio_engine_.get ();
    }
  return nullptr;
}
