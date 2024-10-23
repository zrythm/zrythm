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

#include <signal.h>

#include "common/dsp/automation_track.h"
#include "common/dsp/channel.h"
#include "common/dsp/channel_track.h"
#include "common/dsp/engine.h"
#include "common/dsp/engine_dummy.h"
#include "common/dsp/engine_jack.h"
#include "common/dsp/engine_pa.h"
#include "common/dsp/engine_pulse.h"
#include "common/dsp/engine_rtaudio.h"
#include "common/dsp/engine_rtmidi.h"
#include "common/dsp/graph.h"
#include "common/dsp/graph_node.h"
#include "common/dsp/hardware_processor.h"
#include "common/dsp/metronome.h"
#include "common/dsp/midi_event.h"
#include "common/dsp/pool.h"
#include "common/dsp/port.h"
#include "common/dsp/recording_manager.h"
#include "common/dsp/router.h"
#include "common/dsp/sample_processor.h"
#include "common/dsp/tempo_track.h"
#include "common/dsp/tracklist.h"
#include "common/dsp/transport.h"
#include "common/plugins/carla_native_plugin.h"
#include "common/plugins/plugin.h"
#include "common/utils/gtest_wrapper.h"
#include "common/utils/mpmc_queue.h"
#include "common/utils/object_pool.h"
#include "common/utils/rt_thread_id.h"
#include "common/utils/ui.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/settings/g_settings_manager.h"
#include "gui/backend/backend/zrythm.h"

#if HAVE_JACK
#  include "weakjack/weak_libjack.h"
#endif

#include <glibmm.h>

void
AudioEngine::init_after_cloning (const AudioEngine &other)
{
  transport_type_ = other.transport_type_;
  sample_rate_ = other.sample_rate_;
  frames_per_tick_ = other.frames_per_tick_;
  monitor_out_ = other.monitor_out_->clone_unique ();
  midi_editor_manual_press_ = other.midi_editor_manual_press_->clone_unique ();
  midi_in_ = other.midi_in_->clone_unique ();
  transport_ = other.transport_->clone_unique ();
  pool_ = other.pool_->clone_unique ();
  pool_->engine_ = this;
  control_room_ = other.control_room_->clone_unique ();
  sample_processor_ = other.sample_processor_->clone_unique ();
  hw_in_processor_ = other.hw_in_processor_->clone_unique ();
  hw_out_processor_ = other.hw_out_processor_->clone_unique ();
  midi_clock_out_ = other.midi_clock_out_->clone_unique ();
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
    && transport_->ticks_per_bar_ > 0);

  z_debug (
    "frames per tick before: {:f} | ticks per frame before: {:f}",
    frames_per_tick_, ticks_per_frame_);

  frames_per_tick_ =
    (static_cast<double> (sample_rate) * 60.0
     * static_cast<double> (beats_per_bar))
    / (static_cast<double> (bpm) * static_cast<double> (transport_->ticks_per_bar_));
  z_return_if_fail (frames_per_tick_ > 1.0);
  ticks_per_frame_ = 1.0 / frames_per_tick_;

  z_debug (
    "frames per tick after: {:f} | ticks per frame after: {:f}",
    frames_per_tick_, ticks_per_frame_);

  /* update positions */
  transport_->update_positions (update_from_ticks);

  for (const auto &track : project_->tracklist_->tracks_)
    {
      std::visit (
        [&] (auto &&tr) {
          tr->update_positions (update_from_ticks, bpm_change);
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
  z_return_val_if_fail (ZRYTHM_IS_QT_THREAD, SourceFuncRemove);

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

  add_port (control_room_->monitor_fader_->amp_.get ());
  add_port (control_room_->monitor_fader_->balance_.get ());
  add_port (control_room_->monitor_fader_->mute_.get ());
  add_port (control_room_->monitor_fader_->solo_.get ());
  add_port (control_room_->monitor_fader_->listen_.get ());
  add_port (control_room_->monitor_fader_->mono_compat_enabled_.get ());
  add_port (&control_room_->monitor_fader_->stereo_in_->get_l ());
  add_port (&control_room_->monitor_fader_->stereo_in_->get_r ());
  add_port (&control_room_->monitor_fader_->stereo_out_->get_l ());
  add_port (&control_room_->monitor_fader_->stereo_out_->get_r ());

  add_port (&monitor_out_->get_l ());
  add_port (&monitor_out_->get_r ());
  add_port (midi_editor_manual_press_.get ());
  add_port (midi_in_.get ());

  add_port (&sample_processor_->fader_->stereo_in_->get_l ());
  add_port (&sample_processor_->fader_->stereo_in_->get_r ());
  add_port (&sample_processor_->fader_->stereo_out_->get_l ());
  add_port (&sample_processor_->fader_->stereo_out_->get_r ());

  for (const auto &tr : sample_processor_->tracklist_->tracks_)
    {
      std::visit (
        [&] (auto &&track) {
          z_warn_if_fail (track->is_auditioner ());
          track->append_ports (ports, true);
        },
        tr);
    }

  add_port (transport_->roll_.get ());
  add_port (transport_->stop_.get ());
  add_port (transport_->backward_.get ());
  add_port (transport_->forward_.get ());
  add_port (transport_->loop_toggle_.get ());
  add_port (transport_->rec_toggle_.get ());

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
          ui_show_message_printf (
            _ ("Backend Initialization Failed"),
            _ ("Failed to initialize the %s audio backend. Will use the dummy backend instead. Please check your backend settings in the Preferences."),
            AudioBackend_to_string (audio_backend_).c_str ());
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
          ui_show_message_printf (
            _ ("Backend Error"),
            _ ("The JACK MIDI backend can only be used with the JACK audio backend (your current audio backend is %s). Will use the dummy MIDI backend instead."),
            AudioBackend_to_string (audio_backend_).c_str ());
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
          ui_show_message_printf (
            _ ("Backend Initialization Failed"),
            _ ("Failed to initialize the %s MIDI backend. Will use the dummy backend instead. Please check your backend settings in the Preferences."),
            MidiBackend_to_string (midi_backend_).c_str ());
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
      ui_show_message_literal (
        _ ("Invalid Backend Combination"),
        _ ("Your selected combination of backends may not work properly. If you want to use JACK, please select JACK as both your audio and MIDI backend."));
    }

  buf_size_set_ = false;

  sample_processor_->fader_->stereo_out_->connect_to (
    *control_room_->monitor_fader_->stereo_in_, true);
  control_room_->monitor_fader_->stereo_out_->connect_to (*monitor_out_, true);

  setup_ = true;

  midi_in_->set_expose_to_backend (true);
  monitor_out_->set_expose_to_backend (true);
  midi_clock_out_->set_expose_to_backend (true);

  z_debug ("processing engine events");
  process_events ();

  z_debug ("done");
}

void
AudioEngine::init_common ()
{
  metronome_ = std::make_unique<Metronome> (*this);
  router_ = std::make_unique<Router> ();

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
        g_settings_get_enum (S_P_GENERAL_ENGINE, "audio-backend"));
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
      g_settings_set_enum (
        S_P_GENERAL_ENGINE, "audio-backend",
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
        g_settings_get_enum (S_P_GENERAL_ENGINE, "midi-backend"));
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
      g_settings_set_enum (
        S_P_GENERAL_ENGINE, "midi-backend",
        ENUM_VALUE_TO_INT (MidiBackend::MIDI_BACKEND_DUMMY));
      backend_reset_to_dummy = true;
      break;
    }

  if (backend_reset_to_dummy && ZRYTHM_HAVE_UI && !ZRYTHM_TESTING)
    {
      ui_show_message_printf (
        _ ("Selected Backend Not Found"),
        _ ("The selected MIDI/audio backend was not "
           "found in the version of %s you have "
           "installed. The audio and MIDI backends "
           "were set to \"Dummy\". Please set your "
           "preferred backend from the "
           "preferences."),
        PROGRAM_NAME);
    }

  pan_law_ =
    ZRYTHM_TESTING || ZRYTHM_BENCHMARKING
      ? PanLaw::PAN_LAW_MINUS_3DB
      : static_cast<PanLaw> (g_settings_get_enum (S_P_DSP_PAN, "pan-law"));
  pan_algo_ =
    ZRYTHM_TESTING || ZRYTHM_BENCHMARKING
      ? PanAlgorithm::PAN_ALGORITHM_SINE_LAW
      : static_cast<PanAlgorithm> (
          g_settings_get_enum (S_P_DSP_PAN, "pan-algorithm"));

  if (block_length_ == 0)
    {
      block_length_ = 8192;
    }
  if (midi_buf_size_ == 0)
    {
      midi_buf_size_ = 8192;
    }

  midi_clock_out_ =
    std::make_unique<MidiPort> ("MIDI Clock Out", PortFlow::Output);
  midi_clock_out_->set_owner (this);
  midi_clock_out_->id_.flags2_ |= PortIdentifier::Flags2::MidiClock;
}

void
AudioEngine::init_loaded (Project * project)
{
  z_debug ("Initializing...");

  project_ = project;

  pool_->init_loaded (this);

  auto * tempo_track = project->tracklist_->tempo_track_;
  if (!tempo_track)
    {
      tempo_track = project->tracklist_->get_track_by_type<TempoTrack> ();
    }
  if (!tempo_track)
    {
      throw ZrythmException ("Tempo track not found");
    }

  transport_->init_loaded (this, tempo_track);
  control_room_->init_loaded (this);
  sample_processor_->init_loaded (this);
  hw_in_processor_->init_loaded (this);
  hw_out_processor_->init_loaded (this);

  init_common ();

  std::vector<Port *> ports;
  append_ports (ports);
  for (auto * port : ports)
    {
      auto &id = port->id_;
      if (id.owner_type_ == PortIdentifier::OwnerType::AudioEngine)
        {
          port->init_loaded (this);
        }
      else if (id.owner_type_ == PortIdentifier::OwnerType::HardwareProcessor)
        {
          if (id.is_output ())
            port->init_loaded (hw_in_processor_.get ());
          else if (id.is_input ())
            port->init_loaded (hw_out_processor_.get ());
        }
      else if (id.owner_type_ == PortIdentifier::OwnerType::Fader)
        {
          if (
            ENUM_BITSET_TEST (
              PortIdentifier::Flags2, id.flags2_,
              PortIdentifier::Flags2::SampleProcessorFader))
            port->init_loaded (sample_processor_->fader_.get ());
          else if (
            ENUM_BITSET_TEST (
              PortIdentifier::Flags2, id.flags2_,
              PortIdentifier::Flags2::MonitorFader))
            port->init_loaded (control_room_->monitor_fader_.get ());
        }
    }

  z_debug ("done initializing loaded engine");
}

AudioEngine::AudioEngine (Project * project)
    : project_ (project), sample_rate_ (44000),
      control_room_ (std::make_unique<ControlRoom> (this)),
      pool_ (std::make_unique<AudioPool> (this)),
      transport_ (std::make_unique<Transport> (this)),
      sample_processor_ (std::make_unique<SampleProcessor> (this))
{
  z_debug ("Creating audio engine...");

  midi_editor_manual_press_ =
    std::make_unique<MidiPort> ("MIDI Editor Manual Press", PortFlow::Input);
  midi_editor_manual_press_->set_owner (this);
  midi_editor_manual_press_->id_.sym_ = "midi_editor_manual_press";
  midi_editor_manual_press_->id_.flags_ |= PortIdentifier::Flags::ManualPress;

  midi_in_ = std::make_unique<MidiPort> ("MIDI in", PortFlow::Input);
  midi_in_->set_owner (this);
  midi_in_->id_.sym_ = "midi_in";

  {
    AudioPort monitor_out_l ("Monitor Out L", PortFlow::Output);
    monitor_out_l.id_.sym_ = "monitor_out_l";
    AudioPort monitor_out_r ("Monitor Out R", PortFlow::Output);
    monitor_out_r.id_.sym_ = "monitor_out_r";
    monitor_out_ = std::make_unique<StereoPorts> (
      std::move (monitor_out_l), std::move (monitor_out_r));
    monitor_out_->set_owner (this);
  }

  hw_in_processor_ = std::make_unique<HardwareProcessor> (true, this);
  hw_out_processor_ = std::make_unique<HardwareProcessor> (false, this);

  init_common ();

  z_debug ("finished creating audio engine");
}

void
AudioEngine::wait_for_pause (State &state, bool force_pause, bool with_fadeout)
{
  z_debug ("waiting for engine to pause...");

  state.running_ = run_.load ();
  state.playing_ = transport_->is_rolling ();
  state.looping_ = transport_->loop_;
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
      transport_->request_pause (true);

      if (force_pause)
        {
          transport_->play_state_ = Transport::PlayState::Paused;
        }
      else
        {
          while (
            transport_->play_state_ == Transport::PlayState::PauseRequested
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
  hw_in_processor_->rescan_ext_ports (hw_in_processor_.get ());

  control_room_->monitor_fader_->fading_out_.store (false);

  if (project_ && project_->loaded_)
    {
      /* run 1 more time to flush panic messages */
      SemaphoreRAII sem (port_operation_lock_, true);
      process_prepare (1, &sem);

      EngineProcessTimeInfo time_nfo = {
        .g_start_frame_ =
          static_cast<unsigned_frame_t> (transport_->playhead_pos_.frames_),
        .g_start_frame_w_offset_ =
          static_cast<unsigned_frame_t> (transport_->playhead_pos_.frames_),
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
  transport_->loop_ = state.looping_;
  if (state.playing_)
    {
      transport_->playhead_before_pause_.update_frames_from_ticks (0.0);
      transport_->move_playhead (
        &transport_->playhead_before_pause_, false, false, false);
      transport_->request_roll (true);
    }
  else
    {
      transport_->request_pause (true);
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
  std::vector<zrythm::plugins::Plugin *> plugins;
  TRACKLIST->get_plugins (plugins);
  for (auto &pl : plugins)
    {
      if (pl && !pl->instantiation_failed_ && pl->setting_->open_with_carla_)
        {
          auto carla = dynamic_cast<zrythm::plugins::CarlaNativePlugin *> (pl);
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
  monitor_out_->clear_buffer (*this);
  midi_clock_out_->clear_buffer (*this);

  /* if not running, do not attempt to access any possibly deleted ports */
  if (!run_.load ()) [[unlikely]]
    return;

  /* clear outputs exposed to the backend */
  for (auto &port : router_->graph_->external_out_ports_)
    {
      port->clear_external_buffer ();
    }
}

void
AudioEngine::update_position_info (
  PositionInfo   &pos_nfo,
  const nframes_t frames_to_add)
{
  Position playhead = transport_->playhead_pos_;
  playhead.add_frames (frames_to_add, ticks_per_frame_);
  pos_nfo.is_rolling_ = transport_->is_rolling ();
  pos_nfo.bpm_ = P_TEMPO_TRACK->get_current_bpm ();
  pos_nfo.bar_ = playhead.get_bars (*transport_, true);
  pos_nfo.beat_ =
    playhead.get_beats (*transport_, *project_->tracklist_->tempo_track_, true);
  pos_nfo.sixteenth_ = playhead.get_sixteenths (
    *transport_, *project_->tracklist_->tempo_track_, true);
  pos_nfo.sixteenth_within_bar_ =
    pos_nfo.sixteenth_ + (pos_nfo.beat_ - 1) * transport_->sixteenths_per_beat_;
  pos_nfo.sixteenth_within_song_ = playhead.get_total_sixteenths (false);
  Position bar_start;
  bar_start.set_to_bar (*transport_, playhead.get_bars (*transport_, true));
  Position beat_start;
  beat_start = bar_start;
  beat_start.add_beats (pos_nfo.beat_ - 1);
  pos_nfo.tick_within_beat_ =
    static_cast<double> (playhead.ticks_ - beat_start.ticks_);
  pos_nfo.tick_within_bar_ =
    static_cast<double> (playhead.ticks_ - bar_start.ticks_);
  pos_nfo.playhead_ticks_ = playhead.ticks_;
  pos_nfo.ninetysixth_notes_ = static_cast<int32_t> (
    std::floor (playhead.ticks_ / TICKS_PER_NINETYSIXTH_NOTE_DBL));
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

  if (transport_->play_state_ == Transport::PlayState::PauseRequested)
    {
      if (ZRYTHM_TESTING)
        {
          z_debug ("pause requested handled");
        }
      transport_->play_state_ = Transport::PlayState::Paused;
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
      transport_->play_state_ = Transport::PlayState::Rolling;
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
    if (transport_->is_rolling ())
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
  for (auto track : project_->tracklist_->tracks_ | type_is<ChannelTrack> ())
    {
      track->channel_->prepare_process (nframes);
    }

  return false;
}

void
AudioEngine::receive_midi_events (uint32_t nframes)
{
  switch (midi_backend_)
    {
#if HAVE_JACK
    case MidiBackend::MIDI_BACKEND_JACK:
      midi_in_->receive_midi_events_from_jack (0, nframes);
      break;
#endif
#ifdef HAVE_ALSA
    case MidiBackend::MIDI_BACKEND_ALSA:
      /*engine_alsa_receive_midi_events (self, print);*/
      break;
#endif
    default:
      break;
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

  /*z_info ("processing...");*/

  /* calculate timestamps (used for synchronizing external events like Windows
   * MME MIDI) */
  timestamp_start_ = g_get_monotonic_time ();
  // timestamp_end_ = timestamp_start_ + (total_frames_to_process * 1000000) /
  // sample_rate_;

  if (!run_.load () || !has_handled_buffer_size_change ()) [[unlikely]]
    {
      /*z_info ("skipping processing...");*/
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
      clear_output_buffers (total_frames_to_process);
      return 0;
    }

  /* puts MIDI in events in the MIDI in port */
  receive_midi_events (total_frames_to_process);

  /* process HW processor to get audio/MIDI data from hardware */
  hw_in_processor_->process (total_frames_to_process);

  nframes_t total_frames_remaining = total_frames_to_process;

  /* --- handle preroll --- */

  EngineProcessTimeInfo split_time_nfo = {
    .g_start_frame_ = (unsigned_frame_t) transport_->playhead_pos_.frames_,
    .g_start_frame_w_offset_ =
      (unsigned_frame_t) transport_->playhead_pos_.frames_,
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
      for (auto start_node : router_->graph_->init_trigger_list_)
        {
          const auto route_latency = start_node->route_playback_latency_;

          if (remaining_latency_preroll_ > route_latency + num_preroll_frames)
            {
              /* this route will no-roll for the complete pre-roll cycle */
            }
          else if (remaining_latency_preroll_ > route_latency)
            {
              /* route may need partial no-roll and partial roll from
               * (transport_sample - remaining_latency_preroll) .. +
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
      if (transport_->metronome_enabled_ && transport_->is_rolling ())
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
  // self->last_timestamp_end = g_get_monotonic_time ();

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
  if (transport_->is_rolling () && remaining_latency_preroll_ == 0)
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
  auto last_time_taken = g_get_monotonic_time () - timestamp_start_;
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

  TRACKLIST->mark_all_tracks_for_bounce (false);
}

bool
AudioEngine::is_port_own (const Port &port) const
{
  const auto &monitor_fader = control_room_->monitor_fader_;
  return (
    &port == &monitor_fader->stereo_in_->get_l ()
    || &port == &monitor_fader->stereo_in_->get_r ()
    || &port == &monitor_fader->stereo_out_->get_l ()
    || &port == &monitor_fader->stereo_out_->get_r ());
}

void
AudioEngine::set_default_backends (bool reset_to_dummy)
{
  bool audio_set = false;
  bool midi_set = false;

  if (reset_to_dummy)
    {
      g_settings_set_enum (
        S_P_GENERAL_ENGINE, "audio-backend",
        ENUM_VALUE_TO_INT (AudioBackend::AUDIO_BACKEND_DUMMY));
      g_settings_set_enum (
        S_P_GENERAL_ENGINE, "midi-backend",
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
#elif defined(__APPLE__)
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
#elif defined(__APPLE__)
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

AudioEngine::~AudioEngine ()
{
  z_debug ("freeing engine...");
  destroying_ = true;

  stop_events ();

  if (router_ && router_->graph_)
    {
      /* terminate graph threads */
      router_->graph_->terminate ();
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

  if (PROJECT && AUDIO_ENGINE && this == AUDIO_ENGINE)
    {
      monitor_out_->disconnect ();
      midi_in_->disconnect_all ();
      midi_editor_manual_press_->disconnect_all ();
    }

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