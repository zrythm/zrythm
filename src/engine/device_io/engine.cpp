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

#include <algorithm>
#include <utility>

#include "dsp/graph_scheduler.h"
#include "dsp/transport.h"
#include "engine/device_io/audio_callback.h"
#include "engine/device_io/engine.h"
#include "engine/session/graph_dispatcher.h"
#include "engine/session/sample_processor.h"
#include "gui/backend/backend/project.h"

namespace zrythm::engine::device_io
{

AudioEngine::AudioEngine (
  dsp::Transport                           &transport,
  const dsp::TempoMap                      &tempo_map,
  dsp::PortRegistry                        &port_registry,
  dsp::ProcessorParameterRegistry          &param_registry,
  dsp::PanLaw                               pan_law,
  dsp::PanAlgorithm                         pan_algorithm,
  std::shared_ptr<juce::AudioDeviceManager> device_mgr,
  QObject *                                 parent)
    : QObject (parent), port_registry_ (port_registry),
      param_registry_ (param_registry), transport_ (transport),
      tempo_map_ (tempo_map), device_manager_ (std::move (device_mgr)),
      monitor_out_ (
        u8"Monitor Out",
        dsp::PortFlow::Output,
        dsp::AudioPort::BusLayout::Stereo,
        2),
      pan_law_ (pan_law), pan_algo_ (pan_algorithm),
      sample_processor_ (std::make_unique<session::SampleProcessor> (this)),
      midi_panic_processor_ (
        utils::make_qobject_unique<dsp::MidiPanicProcessor> (
          dsp::MidiPanicProcessor::ProcessorBaseDependencies{
            .port_registry_ = port_registry_,
            .param_registry_ = param_registry_ })),
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
              this->transport_.playhead ()->playhead ()
            };

            const auto process_status = this->process (numSamples);
            if (process_status == ProcessReturnStatus::ProcessCompleted)
              {
                // Note: the monitor output ports below require the processing
                // graph to be operational. We are guarding against other cases
                // by checking the process() return status
                if (numOutputChannels > 0)
                  {
                    utils::float_ranges::copy (
                      outputChannelData[0],
                      monitor_out_.buffers ()->getReadPointer (0), numSamples);
                  }
                if (numOutputChannels > 1)
                  {
                    utils::float_ranges::copy (
                      outputChannelData[1],
                      monitor_out_.buffers ()->getReadPointer (1), numSamples);
                  }
              }
          },
          [this] (juce::AudioIODevice * dev) {
            router_->recalc_graph (false);
            monitor_out_.prepare_for_processing (
              static_cast<sample_rate_t> (dev->getCurrentSampleRate ()),
              dev->getCurrentBufferSizeSamples ());
          },
          [this] () { monitor_out_.release_resources (); })),
      router_ (std::make_unique<engine::session::DspGraphDispatcher> (this))
{
  z_debug ("Creating audio engine...");

  midi_in_ = std::make_unique<dsp::MidiPort> (u8"MIDI in", dsp::PortFlow::Input);
  midi_in_->set_symbol (u8"midi_in");

  monitor_out_.set_symbol (u8"monitor_out");

  z_debug ("Audio engine created");
}

void
init_from (
  AudioEngine           &obj,
  const AudioEngine     &other,
  utils::ObjectCloneType clone_type)
{
// TODO
#if 0
  obj.midi_in_ = utils::clone_unique (*other.midi_in_);
  obj.sample_processor_ = utils::clone_unique (*other.sample_processor_);
  obj.sample_processor_->audio_engine_ = &obj;
  // obj.midi_clock_out_ = utils::clone_unique (*other.midi_clock_out_);
#endif
}

void
AudioEngine::setup (BeatsPerBarGetter beats_per_bar_getter, BpmGetter bpm_getter)
{
  z_debug ("Setting up...");

  setup_ = true;

  z_debug ("done");
}

void
AudioEngine::init_loaded ()
{
  z_debug ("Initializing...");

  // FIXME this shouldn't be here
  // project->pool_->init_loaded ();

  sample_processor_->init_loaded (this);

  z_debug ("done initializing loaded engine");
}

void
AudioEngine::
  wait_for_pause (EngineState &state, bool force_pause, bool with_fadeout)
{
  z_debug ("waiting for engine to pause...");

  state.running_ = run_.load ();
  state.playing_ = transport_.isRolling ();
  state.looping_ = transport_.loopEnabled ();
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
      transport_.requestPause ();

      if (force_pause)
        {
          transport_.setPlayState (dsp::Transport::PlayState::Paused);
        }
      else
        {
          while (
            transport_.getPlayState ()
            == dsp::Transport::PlayState::PauseRequested)
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

  if (device_manager_->getCurrentAudioDevice () != nullptr)
    {
      /* run 1 more time to flush panic messages */
      SemaphoreRAII                sem (port_operation_lock_, true);
      dsp::PlayheadProcessingGuard playhead_processing_guard{
        transport_.playhead ()->playhead ()
      };
      auto current_transport_state = transport_.get_snapshot ();
      process_prepare (current_transport_state, 1, &sem);

      EngineProcessTimeInfo time_nfo = {
        .g_start_frame_ = static_cast<unsigned_frame_t> (
          transport_.get_playhead_position_in_audio_thread ().in (units::samples)),
        .g_start_frame_w_offset_ = static_cast<unsigned_frame_t> (
          transport_.get_playhead_position_in_audio_thread ().in (units::samples)),
        .local_offset_ = 0,
        .nframes_ = 1,
      };

      router_->start_cycle (
        current_transport_state, time_nfo, remaining_latency_preroll_, false);
      post_process (current_transport_state, 0, 1);
    }
}

void

AudioEngine::resume (EngineState &state)
{
  z_debug ("resuming engine...");
  if (!state.running_)
    {
      z_debug ("engine was not running - won't resume");
      return;
    }
  transport_.setLoopEnabled (state.looping_);
  if (state.playing_)
    {
      transport_.move_playhead (
        transport_.playhead_ticks_before_pause (), false);
      transport_.requestRoll ();
    }
  else
    {
      transport_.requestPause ();
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

      sample_processor_->load_instrument_if_empty ();
    }
  else
    {
      if (!activated_)
        {
          z_debug ("engine already deactivated");
          return;
        }

      EngineState state{};
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

  get_monitor_out_port ().clear_buffer (0, get_block_length ());
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
  const auto playhead_frames_before =
    transport_.get_playhead_position_in_audio_thread ();
  auto playhead = playhead_frames_before + units::samples (frames_to_add);
  pos_nfo.is_rolling_ = transport_.isRolling ();
  pos_nfo.bpm_ = static_cast<bpm_t> (tempo_map_.tempo_at_tick (
    tempo_map_.samples_to_tick (playhead).as<int64_t> (units::ticks)));
  const auto musical_pos = tempo_map_.samples_to_musical_position (playhead);
  pos_nfo.bar_ = musical_pos.bar;
  pos_nfo.beat_ = musical_pos.beat;
  pos_nfo.sixteenth_ = musical_pos.sixteenth;
  // TODO/delete
#if 0
  pos_nfo.sixteenth_within_bar_ =
    pos_nfo.sixteenth_
    + ((pos_nfo.beat_ - 1) * transport_->sixteenths_per_beat_);
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
#endif
}

bool
AudioEngine::process_prepare (
  dsp::Transport::TransportSnapshot                &transport_snapshot,
  nframes_t                                         nframes,
  SemaphoreRAII<moodycamel::LightweightSemaphore> * sem)
{
  const auto block_length = get_block_length ();

  const auto update_transport_play_state =
    [&] (dsp::ITransport::PlayState play_state) {
      transport_.set_play_state_rt_safe (play_state);
      transport_snapshot.set_play_state (play_state);
    };

  if (
    transport_snapshot.get_play_state ()
    == dsp::Transport::PlayState::PauseRequested)
    {
      update_transport_play_state (dsp::Transport::PlayState::Paused);
    }
  else if (
    transport_snapshot.get_play_state ()
      == dsp::Transport::PlayState::RollRequested
    && transport_snapshot.metronome_countin_frames_remaining ()
         == units::samples (0))
    {
      update_transport_play_state (dsp::Transport::PlayState::Rolling);
      remaining_latency_preroll_ = router_->get_max_route_playback_latency ();
    }

  clear_output_buffers (nframes);

  if (!exporting_ && !sem->is_acquired ())
    {
      return true;
    }

  update_position_info (pos_nfo_current_, 0);
  {
    nframes_t frames_to_add = 0;
    if (
      transport_snapshot.get_play_state ()
      == dsp::ITransport::PlayState::Rolling)
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

auto
AudioEngine::process (const nframes_t total_frames_to_process)
  -> ProcessReturnStatus
{
  /* RAIIs */
  AtomicBoolRAII cycle_running (cycle_running_);
  SemaphoreRAII  port_operation_sem (port_operation_lock_);

  z_return_val_if_fail (
    total_frames_to_process > 0, ProcessReturnStatus::ProcessFailed);

  if (!run_.load ()) [[unlikely]]
    {
      // z_info ("skipping processing...");
      clear_output_buffers (total_frames_to_process);
      return ProcessReturnStatus::ProcessSkipped;
    }

  // We create a temporary ITransport snapshot and inject it here (graph
  // nodes will use this instead of the main Transport instance, thus
  // avoiding the need to synchronize access to it)
  auto current_transport_state = transport_.get_snapshot ();

  const auto consume_metronome_countin_samples =
    [&] (const units::sample_t samples) {
      transport_.consume_metronome_countin_samples (samples);
      current_transport_state.consume_metronome_countin_samples (samples);
    };

  const auto consume_recording_preroll_samples =
    [&] (const units::sample_t samples) {
      transport_.consume_recording_preroll_samples (samples);
      current_transport_state.consume_recording_preroll_samples (samples);
    };

  /* run pre-process code */
  bool skip_cycle = process_prepare (
    current_transport_state, total_frames_to_process, &port_operation_sem);

  if (skip_cycle) [[unlikely]]
    {
      // z_info ("skip cycle");
      clear_output_buffers (total_frames_to_process);
      return ProcessReturnStatus::ProcessSkipped;
    }

  /* puts MIDI in events in the MIDI in port */
  receive_midi_events (total_frames_to_process);

  /* process HW processor to get audio/MIDI data from hardware */
  // hw_in_processor_->process (total_frames_to_process);

  nframes_t total_frames_remaining = total_frames_to_process;

  /* --- handle preroll --- */

  EngineProcessTimeInfo split_time_nfo = {
    .g_start_frame_ =
      (unsigned_frame_t) current_transport_state
        .get_playhead_position_in_audio_thread ()
        .in (units::samples),
    .g_start_frame_w_offset_ =
      (unsigned_frame_t) current_transport_state
        .get_playhead_position_in_audio_thread ()
        .in (units::samples),
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
      assert (preroll_offset + num_preroll_frames <= total_frames_to_process);

      split_time_nfo.g_start_frame_w_offset_ =
        split_time_nfo.g_start_frame_ + preroll_offset;
      split_time_nfo.local_offset_ = preroll_offset;
      split_time_nfo.nframes_ = num_preroll_frames;
      router_->start_cycle (
        current_transport_state, split_time_nfo, remaining_latency_preroll_,
        true);

      remaining_latency_preroll_ -= num_preroll_frames;
      total_frames_remaining -= num_preroll_frames;

      if (total_frames_remaining == 0)
        break;

    } /* while latency preroll frames remaining */

  /* if we still have frames to process (i.e., if preroll finished completely
   * and can start processing normally) */
  if (total_frames_remaining > 0)
    {
      [&] () {
        nframes_t cur_offset = total_frames_to_process - total_frames_remaining;

        /* split at countin */
        if (
          current_transport_state.metronome_countin_frames_remaining ()
          > units::samples (0))
          {
            const auto countin_frames = std::min (
              total_frames_remaining,
              static_cast<nframes_t> (
                current_transport_state.metronome_countin_frames_remaining ()
                  .in (units::samples)));

            /* process for countin frames */
            split_time_nfo.g_start_frame_w_offset_ =
              split_time_nfo.g_start_frame_ + cur_offset;
            split_time_nfo.local_offset_ = cur_offset;
            split_time_nfo.nframes_ = countin_frames;
            router_->start_cycle (
              current_transport_state, split_time_nfo,
              remaining_latency_preroll_, true);
            consume_metronome_countin_samples (units::samples (countin_frames));

            /* adjust total frames remaining to process and current offset */
            total_frames_remaining -= countin_frames;
            if (total_frames_remaining == 0)
              return;
            cur_offset += countin_frames;
          }

        /* split at preroll */
        if (
          current_transport_state.metronome_countin_frames_remaining ()
            == units::samples (0)
          && current_transport_state.has_recording_preroll_frames_remaining ())
          {
            nframes_t preroll_frames = std::min (
              total_frames_remaining,
              static_cast<nframes_t> (
                current_transport_state.recording_preroll_frames_remaining ()
                  .in (units::samples)));

            /* process for preroll frames */
            split_time_nfo.g_start_frame_w_offset_ =
              split_time_nfo.g_start_frame_ + cur_offset;
            split_time_nfo.local_offset_ = cur_offset;
            split_time_nfo.nframes_ = preroll_frames;
            router_->start_cycle (
              current_transport_state, split_time_nfo,
              remaining_latency_preroll_, true);
            consume_recording_preroll_samples (units::samples (preroll_frames));

            /* process for remaining frames */
            cur_offset += preroll_frames;
            const auto remaining_frames =
              total_frames_remaining - preroll_frames;
            if (remaining_frames > 0)
              {
                split_time_nfo.g_start_frame_w_offset_ =
                  split_time_nfo.g_start_frame_ + cur_offset;
                split_time_nfo.local_offset_ = cur_offset;
                split_time_nfo.nframes_ = remaining_frames;
                router_->start_cycle (
                  current_transport_state, split_time_nfo,
                  remaining_latency_preroll_, true);
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
            router_->start_cycle (
              current_transport_state, split_time_nfo,
              remaining_latency_preroll_, true);
          }
      }();
    }

  /* run post-process code for the number of frames remaining after handling
   * preroll (if any) */
  post_process (
    current_transport_state, total_frames_remaining, total_frames_to_process);

  cycle_.fetch_add (1);

  return ProcessReturnStatus::ProcessCompleted;
}

void
AudioEngine::post_process (
  dsp::Transport::TransportSnapshot &transport_snapshot,
  const nframes_t                    roll_nframes,
  const nframes_t                    nframes)
{
  /* remember current position info */
  update_position_info (pos_nfo_before_, 0);

  /* move the playhead if rolling and not pre-rolling */
  if (
    transport_snapshot.get_play_state () == dsp::ITransport::PlayState::Rolling
    && remaining_latency_preroll_ == 0)
    {
      transport_.add_to_playhead_in_audio_thread (units::samples (roll_nframes));
      transport_snapshot.set_position (
        transport_.get_playhead_position_in_audio_thread ());
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

  // TODO
  // TRACKLIST->mark_all_tracks_for_bounce (false);
}

AudioEngine::~AudioEngine ()
{
  z_debug ("freeing engine...");
  destroying_ = true;

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
to_json (nlohmann::json &j, const AudioEngine &engine)
{
  j = nlohmann::json{
    { AudioEngine::kMidiInKey,          engine.midi_in_          },
    { AudioEngine::kSampleProcessorKey, engine.sample_processor_ },
  };
}

void
from_json (const nlohmann::json &j, AudioEngine &engine)
{
// TODO
#if 0
  j.at (AudioEngine::kMidiInKey).get_to (engine.midi_in_);
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
