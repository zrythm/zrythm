// SPDX-FileCopyrightText: © 2018-2025 Alexandros Theodotou <alex@zrythm.org>
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

#include <algorithm>
#include <cmath>
#include <utility>

#include "dsp/graph_scheduler.h"
#include "dsp/midi_event.h"
#include "dsp/port.h"
#include "engine/device_io/audio_callback.h"
#include "engine/device_io/engine.h"
#include "engine/session/graph_dispatcher.h"
#include "engine/session/recording_manager.h"
#include "engine/session/sample_processor.h"
#include "engine/session/transport.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/settings_manager.h"
#include "gui/backend/backend/zrythm.h"
#include "plugins/plugin.h"
#include "structure/tracks/channel.h"
#include "structure/tracks/channel_track.h"
#include "structure/tracks/tracklist.h"
#include "utils/gtest_wrapper.h"

namespace zrythm::engine::device_io
{

AudioEngine::AudioEngine (
  Project *                                 project,
  std::shared_ptr<juce::AudioDeviceManager> device_mgr)
    : QObject (project), port_registry_ (project->get_port_registry ()),
      param_registry_ (project->get_param_registry ()), project_ (project),
      device_manager_ (std::move (device_mgr)),
      control_room_ (
        std::make_unique<session::ControlRoom> (
          project->get_port_registry (),
          project->get_param_registry (),
          this)),
      port_connections_manager_ (project->port_connections_manager_),
      sample_processor_ (std::make_unique<session::SampleProcessor> (this)),
      midi_panic_processor_ (
        utils::make_qobject_unique<dsp::MidiPanicProcessor> (
          dsp::MidiPanicProcessor::ProcessorBaseDependencies{
            .port_registry_ = *port_registry_,
            .param_registry_ = *param_registry_ })),
      audio_callback_ (
        std::make_unique<AudioCallback> (
          [this] (
            const float * const * inputChannelData,
            int                   numInputChannels,
            float * const *       outputChannelData,
            int                   numOutputChannels,
            int                   numSamples) {
            juce::AudioProcessLoadMeasurer::ScopedTimer scoped_timer{
              load_measurer_
            };
            dsp::PlayheadProcessingGuard guard{
              this->project_->transport_->playhead_
            };

            this->process (numSamples);
            if (numOutputChannels > 0)
              {
                utils::float_ranges::copy (
                  outputChannelData[0], monitor_out_left_port_->buf_.data (),
                  numSamples);
              }
            if (numOutputChannels > 1)
              {
                utils::float_ranges::copy (
                  outputChannelData[1], monitor_out_right_port_->buf_.data (),
                  numSamples);
              }
          },
          [this] (juce::AudioIODevice * _) {
            const auto &[monitor_left, monitor_right] = get_monitor_out_ports ();
            monitor_out_left_port_ = &monitor_left;
            monitor_out_right_port_ = &monitor_right;
            router_->recalc_graph (false);
          },
          [] () {}))
{
  z_debug ("Creating audio engine...");

  midi_in_ = std::make_unique<dsp::MidiPort> (u8"MIDI in", dsp::PortFlow::Input);
  midi_in_->set_symbol (u8"midi_in");

  {
    auto monitor_out = dsp::StereoPorts::create_stereo_ports (
      project_->get_port_registry (), false, u8"Monitor Out", u8"monitor_out");
    monitor_out_left_ = monitor_out.first;
    monitor_out_right_ = monitor_out.second;
  }

  init_common ();

  z_debug ("finished creating audio engine");
}

void
init_from (
  AudioEngine           &obj,
  const AudioEngine     &other,
  utils::ObjectCloneType clone_type)
{
  obj.frames_per_tick_ = other.frames_per_tick_;
  obj.monitor_out_left_ = other.monitor_out_left_;
  obj.monitor_out_right_ = other.monitor_out_right_;
// TODO
#if 0
  obj.midi_in_ = utils::clone_unique (*other.midi_in_);
  obj.control_room_ = utils::clone_unique (*other.control_room_);
  obj.sample_processor_ = utils::clone_unique (*other.sample_processor_);
  obj.sample_processor_->audio_engine_ = &obj;
  // obj.midi_clock_out_ = utils::clone_unique (*other.midi_clock_out_);
#endif
}

std::pair<dsp::AudioPort &, dsp::AudioPort &>
AudioEngine::get_monitor_out_ports ()
{
  if (!monitor_out_left_)
    {
      throw std::runtime_error ("No monitor outputs");
    }
  auto * l = std::get<dsp::AudioPort *> (monitor_out_left_->get_object ());
  auto * r = std::get<dsp::AudioPort *> (monitor_out_right_->get_object ());
  return { *l, *r };
}

std::pair<dsp::AudioPort &, dsp::AudioPort &>
AudioEngine::get_dummy_input_ports ()
{
  if (!dummy_left_input_)
    {
      throw std::runtime_error ("No dummy inputs");
    }
  auto * l = dummy_left_input_->get_object_as<dsp::AudioPort> ();
  auto * r = dummy_right_input_->get_object_as<dsp::AudioPort> ();
  return { *l, *r };
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
    type_safe::get (frames_per_tick_), type_safe::get (ticks_per_frame_));

  frames_per_tick_ = dsp::FramesPerTick (
    (static_cast<double> (sample_rate) * 60.0
     * static_cast<double> (beats_per_bar))
    / (static_cast<double> (bpm) * static_cast<double> (project_->transport_->ticks_per_bar_)));
  z_return_if_fail (type_safe::get (frames_per_tick_) > 1.0);
  ticks_per_frame_ = dsp::to_ticks_per_frame (frames_per_tick_);

  z_debug (
    "frames per tick after: {:f} | ticks per frame after: {:f}",
    type_safe::get (frames_per_tick_), type_safe::get (ticks_per_frame_));

  /* update positions */
  project_->transport_->update_positions (update_from_ticks);

  updating_frames_per_tick_ = false;
}

void
AudioEngine::setup (BeatsPerBarGetter beats_per_bar_getter, BpmGetter bpm_getter)
{
  z_debug ("Setting up...");
  buf_size_set_ = false;

  update_frames_per_tick (
    beats_per_bar_getter (), bpm_getter (), get_sample_rate (), true, true,
    false);

  setup_ = true;

  z_debug ("done");
}

void
AudioEngine::init_common ()
{
  router_ = std::make_unique<session::DspGraphDispatcher> (this);

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

// TODO: this should be a separate processor
#if 0
  midi_clock_out_ =
    std::make_unique<MidiPort> (u8"MIDI Clock Out", dsp::PortFlow::Output);
  midi_clock_out_->set_owner (*this);
  midi_clock_out_->id_->flags_ |= dsp::PortIdentifier::Flags::MidiClock;
#endif
}

void
AudioEngine::init_loaded (Project * project)
{
  z_debug ("Initializing...");

  project_ = project;
  port_registry_ = project->get_port_registry ();

  // FIXME this shouldn't be here
  project->pool_->init_loaded ();

  control_room_->init_loaded (*port_registry_, *param_registry_, this);
  sample_processor_->init_loaded (this);

  init_common ();

  // TODO
#if 0
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
#  if 0
          if (id.is_output ())
            port->init_loaded (*hw_in_processor_);
          else if (id.is_input ())
            port->init_loaded (*hw_out_processor_);
#  endif
        }
      else if (id.owner_type_ == dsp::PortIdentifier::OwnerType::Fader)
        {
          if (ENUM_BITSET_TEST (
                id.flags_, dsp::PortIdentifier::Flags::MonitorFader))
            port->init_loaded (*control_room_->monitor_fader_);
        }
      }
#endif

  z_debug ("done initializing loaded engine");
}

structure::tracks::TrackRegistry &
AudioEngine::get_track_registry ()
{
  return project_->get_track_registry ();
}
structure::tracks::TrackRegistry &
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

  if (with_fadeout && state.running_)
    {
// TODO (or use graph cross-fade)
#if 0
      z_debug (
        "setting fade out samples and waiting for remaining samples to become 0");
      control_room_->monitor_fader_->request_fade_out ();
      const auto start_time = SteadyClock::now ();
      const auto max_time_to_wait = std::chrono::seconds (2);
      while (control_room_->monitor_fader_->has_fade_out_samples_left ())
        {
          std::this_thread::sleep_for (std::chrono::microseconds (100));
          if (SteadyClock::now () - start_time > max_time_to_wait)
            {
              /* abort */
              control_room_->monitor_fader_->abort_fade_out ();
              break;
            }
        }
#endif
    }

  if (!destroying_)
    {
      /* send panic */
      panic_all ();
    }

  if (state.playing_)
    {
      project_->transport_->requestPause (true);

      if (force_pause)
        {
          project_->transport_->setPlayState (
            session::Transport::PlayState::Paused);
        }
      else
        {
          while (
            project_->transport_->play_state_
            == session::Transport::PlayState::PauseRequested)
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

  // control_room_->monitor_fader_->abort_fade_out ();

  if ((project_ != nullptr) && project_->loaded_)
    {
      /* run 1 more time to flush panic messages */
      SemaphoreRAII                sem (port_operation_lock_, true);
      dsp::PlayheadProcessingGuard playhead_processing_guard{
        project_->transport_->playhead_
      };
      process_prepare (1, &sem);

      EngineProcessTimeInfo time_nfo = {
        .g_start_frame_ = static_cast<unsigned_frame_t> (
          project_->transport_->get_playhead_position_in_audio_thread ()),
        .g_start_frame_w_offset_ = static_cast<unsigned_frame_t> (
          project_->transport_->get_playhead_position_in_audio_thread ()),
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
      project_->transport_->move_playhead (
        project_->transport_->playhead_before_pause_, false);
      project_->transport_->requestRoll (true);
    }
  else
    {
      project_->transport_->requestPause (true);
    }

  // z_debug ("restarting engine: setting fade in samples");
  // control_room_->monitor_fader_->request_fade_in ();

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

      realloc_port_buffers (get_block_length ());

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

  if (activate)
    {
      load_measurer_.reset (
        get_sample_rate (), static_cast<int> (get_block_length ()));
      device_manager_->addAudioCallback (audio_callback_.get ());
    }
  else
    {
      device_manager_->removeAudioCallback (audio_callback_.get ());
    }

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
  buf_size_set_ = true;
  z_info (
    "Block length changed to {}. reallocating buffers...", get_block_length ());

  /* TODO make function that fetches all plugins in the project */
  std::vector<zrythm::plugins::Plugin *> plugins;
  TRACKLIST->get_track_span ().get_plugins (plugins);
// TODO: needed?
#if 0
  for (auto &pl : plugins)
    {
      if (pl && !pl->instantiation_failed_ && pl->setting_->open_with_carla_)
        {
          auto carla = dynamic_cast<
            zrythm::gui::old_dsp::plugins::CarlaNativePlugin *> (pl);
          carla->update_buffer_size_and_sample_rate ();
        }
      }
#endif
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
    [&] (auto &port) { port.clear_buffer (0, get_block_length ()); },
    get_monitor_out_ports ());
  // midi_clock_out_->clear_buffer (get_block_length ());

  /* if not running, do not attempt to access any possibly deleted ports */
  if (!run_.load ()) [[unlikely]]
    return;
}

void
AudioEngine::update_position_info (
  PositionInfo   &pos_nfo,
  const nframes_t frames_to_add)
{
  auto       transport_ = project_->transport_;
  auto      &tempo_map = project_->get_tempo_map ();
  const auto playhead_frames_before =
    transport_->get_playhead_position_in_audio_thread ();
  auto playhead =
    dsp::Position{ playhead_frames_before, AUDIO_ENGINE->ticks_per_frame_ };
  playhead.add_frames (frames_to_add, ticks_per_frame_);
  pos_nfo.is_rolling_ = transport_->isRolling ();
  pos_nfo.bpm_ = static_cast<bpm_t> (
    tempo_map.tempo_at_tick (static_cast<int64_t> (playhead.ticks_)));
  pos_nfo.bar_ = playhead.get_bars (true, transport_->ticks_per_bar_);
  pos_nfo.beat_ = playhead.get_beats (
    true,
    tempo_map.time_signature_at_tick (static_cast<int64_t> (playhead.ticks_))
      .numerator,
    transport_->ticks_per_beat_);
  pos_nfo.sixteenth_ = playhead.get_sixteenths (
    true,
    tempo_map.time_signature_at_tick (static_cast<int64_t> (playhead.ticks_))
      .numerator,
    transport_->sixteenths_per_beat_, frames_per_tick_);
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
  pos_nfo.tick_within_beat_ = playhead.ticks_ - beat_start.ticks_;
  pos_nfo.tick_within_bar_ = playhead.ticks_ - bar_start.ticks_;
  pos_nfo.playhead_ticks_ = playhead.ticks_;
  pos_nfo.ninetysixth_notes_ = static_cast<int32_t> (std::floor (
    playhead.ticks_ / dsp::Position::TICKS_PER_NINETYSIXTH_NOTE_DBL));
}

bool
AudioEngine::process_prepare (
  nframes_t                                         nframes,
  SemaphoreRAII<moodycamel::LightweightSemaphore> * sem)
{
  const auto block_length = get_block_length ();

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
  if (transport_->play_state_ == session::Transport::PlayState::PauseRequested)
    {
      transport_->set_play_state_rt_safe (session::Transport::PlayState::Paused);
    }
  else if (
    transport_->play_state_ == session::Transport::PlayState::RollRequested
    && transport_->countin_frames_remaining_ == 0)
    {
      transport_->set_play_state_rt_safe (
        session::Transport::PlayState::Rolling);
      remaining_latency_preroll_ = router_->get_max_route_playback_latency ();
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
  // control_room_->monitor_fader_->clear_buffers (block_length);
  midi_in_->clear_buffer (0, block_length);

  return false;
}

void
AudioEngine::receive_midi_events (uint32_t nframes)
{
// TODO
#if 0
  if (midi_in_->backend_ && midi_in_->backend_->is_exposed ())
    {
      midi_in_->backend_->sum_midi_data (
        midi_in_->midi_events_, { .start_frame = 0, .nframes = nframes });
    }
#endif
}

int
AudioEngine::process (const nframes_t total_frames_to_process)
{
  /* RAIIs */
  AtomicBoolRAII cycle_running (cycle_running_);
  SemaphoreRAII  port_operation_sem (port_operation_lock_);

  z_return_val_if_fail (total_frames_to_process > 0, -1);

  /* calculate timestamps (used for synchronizing external events like Windows
   * MME MIDI) */
  timestamp_start_ = Zrythm::getInstance ()->get_monotonic_time_usecs ();
  // timestamp_end_ = timestamp_start_ + (total_frames_to_process * 1000000) /
  // sample_rate_;

  if (!run_.load ()) [[unlikely]]
    {
      // z_info ("skipping processing...");
      clear_output_buffers (total_frames_to_process);
      return 0;
    }

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
  // hw_in_processor_->process (total_frames_to_process);

  nframes_t total_frames_remaining = total_frames_to_process;

  /* --- handle preroll --- */

  auto *                transport_ = project_->transport_;
  EngineProcessTimeInfo split_time_nfo = {
    .g_start_frame_ =
      (unsigned_frame_t) transport_->get_playhead_position_in_audio_thread (),
    .g_start_frame_w_offset_ =
      (unsigned_frame_t) transport_->get_playhead_position_in_audio_thread (),
    .local_offset_ = 0,
    .nframes_ = 0,
  };

  while (remaining_latency_preroll_ > 0)
    {
      nframes_t num_preroll_frames =
        std::min (total_frames_remaining, remaining_latency_preroll_);

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
        && transport_->has_recording_preroll_frames_remaining ())
        {
          nframes_t preroll_frames = std::min (
            total_frames_remaining,
            static_cast<nframes_t> (
              transport_->recording_preroll_frames_remaining ()));

          /* process for preroll frames */
          split_time_nfo.g_start_frame_w_offset_ =
            split_time_nfo.g_start_frame_ + cur_offset;
          split_time_nfo.local_offset_ = cur_offset;
          split_time_nfo.nframes_ = preroll_frames;
          router_->start_cycle (split_time_nfo);
          transport_->recording_preroll_frames_remaining_ -= preroll_frames;

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
  /* remember current position info */
  update_position_info (pos_nfo_before_, 0);

  /* move the playhead if rolling and not pre-rolling */
  auto * transport_ = project_->transport_;
  if (transport_->isRolling () && remaining_latency_preroll_ == 0)
    {
      transport_->add_to_playhead_in_audio_thread (roll_nframes);
    }
}

void
AudioEngine::panic_all ()
{
  z_info ("~ midi panic all ~");
  midi_panic_processor_->request_panic ();
}

void
AudioEngine::reset_bounce_mode ()
{
  bounce_mode_ = BounceMode::Off;

  TRACKLIST->mark_all_tracks_for_bounce (false);
}

AudioEngine::~AudioEngine ()
{
  z_debug ("freeing engine...");
  destroying_ = true;

  // if (router_ && router_->scheduler_)
  // {
  /* terminate graph threads */
  // router_->scheduler_->terminate_threads ();
  // }

  if (activated_)
    {
      activate (false);
    }

  z_debug ("finished freeing engine");
}

AudioEngine *
AudioEngine::get_active_instance ()
{
  auto prj = Project::get_active_instance ();
  if (prj != nullptr)
    {
      return prj->audio_engine_.get ();
    }
  return nullptr;
}

void
from_json (const nlohmann::json &j, AudioEngine &engine)
{
// TODO
#if 0
  j.at (AudioEngine::kFramesPerTickKey).get_to (engine.frames_per_tick_);
  j.at (AudioEngine::kMonitorOutLKey).get_to (engine.monitor_out_left_);
  j.at (AudioEngine::kMonitorOutRKey).get_to (engine.monitor_out_right_);
  j.at (AudioEngine::kMidiInKey).get_to (engine.midi_in_);
  j.at (AudioEngine::kControlRoomKey).get_to (engine.control_room_);
  j.at (AudioEngine::kSampleProcessorKey).get_to (engine.sample_processor_);
#endif
}
}

void
EngineProcessTimeInfo::print () const
{
  z_info (
    "Global start frame: {} (with offset {}) | local offset: {} | num frames: {}",
    g_start_frame_, g_start_frame_w_offset_, local_offset_, nframes_);
}
