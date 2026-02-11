// SPDX-FileCopyrightText: © 2019-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>

#include "dsp/engine.h"
#include "dsp/midi_event.h"
#include "dsp/port.h"
#include "engine/session/sample_processor.h"
#include "gui/backend/backend/settings/chord_preset.h"
#include "gui/backend/backend/settings/plugin_configuration_manager.h"
#include "gui/backend/backend/settings/settings.h"
#include "gui/backend/backend/zrythm.h"
#include "gui/backend/io/midi_file.h"
#include "gui/backend/plugin_manager.h"
#include "plugins/plugin.h"
#include "structure/arrangement/audio_region.h"
#include "structure/project/project.h"
#include "structure/project/project_graph_builder.h"
#include "structure/tracks/tracklist.h"
#include "utils/debug.h"
#include "utils/dsp.h"
#include "utils/gtest_wrapper.h"
#include "utils/io_utils.h"

using namespace zrythm::structure::tracks;

namespace zrythm::engine::session
{

SampleProcessor::SampleProcessor (device_io::AudioEngine * engine)
    : audio_engine_ (engine)
{
  // TODO
  // fader_ = std::make_unique<dsp::Fader> (
  //   dsp::ProcessorBase::ProcessorBaseDependencies{
  //     .port_registry_ = engine->get_port_registry (),
  //     .param_registry_ = engine->get_param_registry () },
  //   dsp::PortType::Audio, true, false,
  //   [] () -> utils::Utf8String { return u8"Sample Processor"; },
  //   [] (bool fader_solo_status) { return false; });

  init_common ();
}

void
SampleProcessor::load_instrument_if_empty ()
{
// TODO
#if 0
  if (!ZRYTHM_TESTING && !ZRYTHM_BENCHMARKING)
    {
      auto _setting_json = gui::SettingsManager::fileBrowserInstrument ();
      auto setting_json = utils::Utf8String::from_qstring (_setting_json);
      std::unique_ptr<PluginConfiguration> setting;
      bool                                 json_read = false;
      if (!setting_json.empty ())
        {
          try
            {
              setting = std::make_unique<PluginConfiguration> ();
              nlohmann::json j = nlohmann::json::parse (setting_json.view ());
              from_json (j, *setting);
              json_read = true;
            }
          catch (const ZrythmException &e)
            {
              z_warning (
                "failed to get instrument from JSON [{}]:\n{}", e.what (),
                setting_json);
            }
        }

      if (!json_read)
        {
          /* pick first instrument found */
          auto instrument =
            zrythm::gui::old_dsp::plugins::PluginManager::get_active_instance ()
              ->pick_instrument ();
          if (!instrument)
            return;

          auto found_setting = S_PLUGIN_SETTINGS->find (*instrument);
          if (found_setting == nullptr)
            {
              setting = S_PLUGIN_SETTINGS->create_configuration_for_descriptor (
                *instrument);
            }
        }

      z_return_if_fail (setting);
      instrument_setting_ = std::move (setting);
      instrument_setting_->validate ();
    }
#endif
}

void
SampleProcessor::init_common ()
{
  // TODO
  // tracklist_ =
  //   std::make_unique<Tracklist> (audio_engine_->get_track_registry (), this);
  midi_events_ = std::make_unique<dsp::MidiEvents> ();
  current_samples_.reserve (256);
}

void
SampleProcessor::init_loaded (device_io::AudioEngine * engine)
{
  audio_engine_ = engine;

  init_common ();
}

void
SampleProcessor::remove_sample_playback (SamplePlayback &in_sp)
{
  auto it = std::ranges::find_if (current_samples_, [&in_sp] (const auto &sp) {
    return &sp == &in_sp;
  });

  if (it != current_samples_.end ())
    {
      current_samples_.erase (it);
    }
  else
    {
      z_warning ("Sample playback not found for removal");
    }
}

void
SampleProcessor::process_block (
  EngineProcessTimeInfo  time_nfo,
  const dsp::ITransport &transport,
  const dsp::TempoMap   &tempo_map) noexcept
{
#if 0
  const auto    cycle_offset = time_nfo.local_offset_;
  const auto    nframes = time_nfo.nframes_;
  SemaphoreRAII lock (rebuilding_sem_);
  if (!lock.is_acquired ())
    {
      z_warning ("lock not acquired");
      return;
    }

  auto       &fader_stereo_out_port = fader_->get_stereo_out_port ();
  const auto &fader_out_buf = fader_stereo_out_port.buffers ();

  // Process the samples in the queue
  for (auto it = current_samples_.begin (); it != current_samples_.end ();)
    {
      auto &sp = *it;
      z_return_if_fail_cmp (sp.buf_->getNumChannels (), >, 0);

      // If sample starts after this cycle, update offset and skip processing
      if (sp.start_offset_ >= nframes)
        {
          sp.start_offset_ -= nframes;
          ++it;
          continue;
        }

      const auto process_samples =
        [&] (nframes_t fader_buf_offset, nframes_t len) {
          // ensure we don't overflow the fader buffer
          z_return_if_fail_cmp (
            fader_buf_offset + len, <=, AUDIO_ENGINE->get_block_length ());
          // ensure we don't overflow the sample playback buffer
          z_return_if_fail_cmp (
            sp.offset_ + len, <=, (unsigned_frame_t) sp.buf_->getNumSamples ());
          utils::float_ranges::mix_product (
            fader_out_buf->getWritePointer (
              0, static_cast<int> (fader_buf_offset)),
            sp.buf_->getReadPointer (0), sp.volume_, len);
          utils::float_ranges::mix_product (
            fader_out_buf->getWritePointer (
              1, static_cast<int> (fader_buf_offset)),
            sp.buf_->getReadPointer (sp.buf_->getNumChannels () > 1 ? 1 : 0),
            sp.volume_, len);
          sp.offset_ += len;
        };

      // If sample is already playing
      if (sp.offset_ > 0)
        {
          const auto max_frames = std::min (
            (nframes_t) (sp.buf_->getNumSamples () - sp.offset_), nframes);
          process_samples (cycle_offset, max_frames);
        }
      // If we can start playback in this cycle
      else if (sp.start_offset_ >= cycle_offset)
        {
          z_return_if_fail_cmp (sp.offset_, ==, 0);
          z_return_if_fail_cmp ((cycle_offset + nframes), >=, sp.start_offset_);

          const auto max_frames = std::min (
            (nframes_t) sp.buf_->getNumSamples (),
            (cycle_offset + nframes) - sp.start_offset_);
          process_samples (sp.start_offset_, max_frames);
        }

      // If the sample is finished playing, remove it
      if (sp.offset_ >= (unsigned_frame_t) sp.buf_->getNumSamples ())
        {
          it = current_samples_.erase (it);
        }
      else
        {
          ++it;
        }
    }

  if (roll_)
    {
      midi_events_->active_events_.clear ();
      for (
        auto track_var :
        tracklist_->collection ()->get_track_span () | std::views::reverse)
        {
          std::visit (
            [&] (auto &&track) {
              using TrackT = base_type<decltype (track)>;
              if constexpr (ProcessableTrack<TrackT>)
                {
                  EngineProcessTimeInfo inner_time_nfo = {
                    .g_start_frame_ =
                      static_cast<unsigned_frame_t> (playhead_.frames_),
                    .g_start_frame_w_offset_ =
                      static_cast<unsigned_frame_t> (playhead_.frames_)
                      + cycle_offset,
                    .local_offset_ = cycle_offset,
                    .nframes_ = nframes,
                  };

                  const float * audio_data_l = nullptr;
                  const float * audio_data_r = nullptr;
                  if constexpr (std::is_same_v<TrackT, AudioTrack>)
                    {
                      track->get_track_processor ()->process_block (
                        inner_time_nfo);
                      auto &processor_stereo_out =
                        track->get_track_processor ()->get_stereo_out_port ();
                      audio_data_l =
                        processor_stereo_out.buffers ()->getReadPointer (0);
                      audio_data_r =
                        processor_stereo_out.buffers ()->getReadPointer (1);
                    }
                  else if constexpr (std::is_same_v<TrackT, MidiTrack>)
                    {
                      track->get_track_processor ()->process_block (
                        inner_time_nfo);
                      midi_events_->active_events_.append (
                        track->get_track_processor ()
                          ->get_midi_out_port ()
                          .midi_events_.active_events_,
                        cycle_offset, nframes);
                    }
                  else if constexpr (std::is_same_v<TrackT, InstrumentTrack>)
                    {
                    // TODO
#  if 0
                      auto ins_var = track->channel ()->get_instrument ();
                      if (!ins_var)
                        return;
                      std::visit (
                        [&] (auto &&ins) {

                          ins->prepare_process (
                            AUDIO_ENGINE->get_block_length ());
                          ins->midi_in_port_->midi_events_.active_events_.append (
                            midi_events_->active_events_, cycle_offset, nframes);
                          ins->process_block (inner_time_nfo);
                          audio_data_l = ins->l_out_->buf_.data ();
                          audio_data_r = ins->r_out_->buf_.data ();
                        },
                        *ins_var);
#  endif
                    }

                  if (audio_data_l && audio_data_r)
                    {
                      utils::float_ranges::mix_product (
                        fader_out_buf->getWritePointer (
                          0, static_cast<int> (cycle_offset)),
                        &audio_data_l[cycle_offset], fader_->get_current_amp (),
                        nframes);
                      utils::float_ranges::mix_product (
                        fader_out_buf->getWritePointer (
                          1, static_cast<int> (cycle_offset)),
                        &audio_data_r[cycle_offset], fader_->get_current_amp (),
                        nframes);
                    }
                }
            },
            track_var);
        }
    }

  playhead_.add_frames (nframes, AUDIO_ENGINE->ticks_per_frame_);

  // Stop rolling if no more material
  if (playhead_ > file_end_pos_)
    roll_ = false;
#endif
}

void
SampleProcessor::queue_sample_from_file (const char * path)
{
  /* TODO */
}

void
SampleProcessor::queue_file_or_chord_preset (
  const FileDescriptor * file,
  const ChordPreset *    chord_pset)
{
// TODO
#if 0
  SemaphoreRAII<decltype (rebuilding_sem_)> sem_raii (rebuilding_sem_);

  /* clear tracks */
  for (auto track_var : tracklist_->get_track_span () | std::views::reverse)
    {
      std::visit (
        [&] (auto &&tr) {
          using TrackT = base_type<decltype (tr)>;
          /* remove state dir if instrument */
          if constexpr (std::is_same_v<TrackT, InstrumentTrack>)
            {
              auto state_dir =
                zrythm::plugins::Plugin::from_variant (
                  *tr->channel_->get_instrument ())
                  ->get_abs_state_dir (false, true);
              if (!state_dir.empty ())
                {
                  utils::io::rmdir (state_dir, true);
                }
            }

          tracklist_->remove_track (tr->get_uuid ());
        },
        track_var);
    }

  Position start_pos;
  auto     transport = audio_engine_->project_->transport_;
  start_pos.set_to_bar (
    1, transport->ticks_per_bar_, audio_engine_->frames_per_tick_);
  file_end_pos_.set_to_bar (
    1, transport->ticks_per_bar_, audio_engine_->frames_per_tick_);

  /* create master track */
  {
    z_debug ("creating master track...");
    auto track_ref = PROJECT->get_track_registry ().create_object<MasterTrack> (
      PROJECT->get_track_registry (), PROJECT->get_plugin_registry (),
      PROJECT->get_port_registry (), PROJECT->get_arranger_object_registry (),
      true);
    auto * track = std::get<MasterTrack *> (track_ref.get_object ());
    track->set_name (*tracklist_, u8"Sample Processor Master", false);
    tracklist_->master_track_ = track;
    tracklist_->insert_track (
      track_ref, static_cast<int> (tracklist_->track_count ()), *AUDIO_ENGINE,
      false, false);
  }

  if ((file != nullptr) && file->is_audio ())
    {
      z_debug ("creating audio track...");
      auto audio_track_ref =
        PROJECT->get_track_registry ().create_object<AudioTrack> (
          PROJECT->get_track_registry (), PROJECT->get_plugin_registry (),
          PROJECT->get_port_registry (),
          PROJECT->get_arranger_object_registry (), true);
      auto * audio_track =
        std::get<AudioTrack *> (audio_track_ref.get_object ());
      audio_track->set_name (*tracklist_, u8"Sample processor audio", false);
      tracklist_->insert_track (
        audio_track_ref, static_cast<int> (tracklist_->track_count ()),
        *AUDIO_ENGINE, false, false);

      /* create an audio region & add to track */
      try
        {
// TODO
#  if 0
          auto * ar =
            PROJECT->getArrangerObjectFactory ()
              ->addAudioRegionFromFile (
                &audio_track->get_lane_at (0),
                utils::Utf8String::from_path (file->abs_path_).to_qstring (),
                start_pos.ticks_);
          file_end_pos_ = *ar->end_pos_;
#  endif
        }
      catch (const ZrythmException &e)
        {
          e.handle ("Failed to create/add audio region for sample processor");
          return;
        }
    }
  else if (
    (((file != nullptr) && file->is_midi ()) || (chord_pset != nullptr))
    && instrument_setting_)
    {
      /* create an instrument track */
      z_debug ("creating instrument track...");
      auto instrument_track_ref =
        PROJECT->get_track_registry ().create_object<InstrumentTrack> (
          PROJECT->get_track_registry (), PROJECT->get_plugin_registry (),
          PROJECT->get_port_registry (),
          PROJECT->get_arranger_object_registry (), true);
      auto * instrument_track =
        std::get<InstrumentTrack *> (instrument_track_ref.get_object ());
      instrument_track->set_name (
        *tracklist_, u8"Sample processor instrument", false);
      tracklist_->insert_track (
        instrument_track_ref, static_cast<int> (tracklist_->track_count ()),
        *AUDIO_ENGINE, false, false);
      try
        {
          auto pl_ref = PROJECT->getPluginFactory ()->create_plugin_from_setting (
            *instrument_setting_);
          auto * pl = std::get<CarlaNativePlugin *> (pl_ref.get_object ());
          pl->instantiate ();
          pl->activate (true);
          z_return_if_fail (pl->midi_in_port_ && pl->l_out_ && pl->r_out_);

          instrument_track->channel_->add_plugin (
            pl_ref,
            plugins::PluginSlot (zrythm::plugins::PluginSlotType::Instrument),
            false, false, true, false, false);

          int num_tracks =
            (file != nullptr)
              ? MidiFile (file->abs_path_).get_num_tracks (true)
              : 1;
          z_debug ("creating {} MIDI tracks...", num_tracks);
          for (int i = 0; i < num_tracks; i++)
            {
              auto midi_track_ref =
                PROJECT->get_track_registry ().create_object<MidiTrack> (
                  PROJECT->get_track_registry (),
                  PROJECT->get_plugin_registry (), PROJECT->get_port_registry (),
                  PROJECT->get_arranger_object_registry (), true);
              auto * midi_track =
                std::get<MidiTrack *> (midi_track_ref.get_object ());
              midi_track->set_name (
                *tracklist_,
                utils::Utf8String::from_utf8_encoded_string (
                  fmt::format ("Sample processor MIDI {}", i)),
                false);
              tracklist_->insert_track (
                midi_track_ref, static_cast<int> (tracklist_->track_count ()),
                *AUDIO_ENGINE, false, false);

              /* route track to instrument */
              instrument_track->add_child (
                midi_track->get_uuid (), true, false, false);

              if (file != nullptr)
                {
                  /* create a MIDI region from the MIDI file & add to track */
                  try
                    {
                      auto mr =
                        PROJECT->getArrangerObjectFactory ()
                          ->addMidiRegionFromMidiFile (
                            &midi_track->get_lane_at (0),
                            utils::Utf8String::from_path (file->abs_path_)
                              .to_qstring (),
                            start_pos.ticks_, i);
                      file_end_pos_ =
                        std::max (file_end_pos_, mr->get_end_position ());
                    }
                  catch (const ZrythmException &e)
                    {
                      throw ZrythmException (
                        fmt::format (
                          "Failed to create MIDI region from file {}",
                          file->abs_path_));
                    }
                }
              else if (chord_pset != nullptr)
                {
                  try
                    {
                      /* create a MIDI region from the chord preset and add to
                       * track
                       */
                      Position end_pos;
                      end_pos.from_seconds (
                        13.0, audio_engine_->get_sample_rate (),
                        audio_engine_->ticks_per_frame_);
                      auto mr =
                        structure::arrangement::ArrangerObjectFactory::
                          get_instance ()
                            ->addEmptyMidiRegion (
                              &midi_track->get_lane_at (0), start_pos.ticks_);
                      mr->set_end_pos_full_size (
                        end_pos, AUDIO_ENGINE->frames_per_tick_);

                      /* add notes */
                      for (const auto j : std::views::iota (0, 12))
                        {
                          auto descr = chord_pset->descr_.at (j);
                          descr.update_notes ();
                          if (descr.type_ == dsp::ChordType::None)
                            continue;

                          Position cur_pos, cur_end_pos;
                          cur_pos.from_seconds (
                            j * 1.0, audio_engine_->get_sample_rate (),
                            audio_engine_->ticks_per_frame_);
                          cur_end_pos.from_seconds (
                            j * 1.0 + 0.5, audio_engine_->get_sample_rate (),
                            audio_engine_->ticks_per_frame_);
                          for (
                            const auto k : std::views::iota (
                              0_zu, dsp::ChordDescriptor::MAX_NOTES))
                            {
                              if (descr.notes_[k])
                                {
                                  auto * mn =
                                    structure::arrangement::
                                      PROJECT->getArrangerObjectFactory ()
                                        ->addMidiNote (
                                          mr, cur_pos.ticks_, (int) k + 36);
                                  mn->set_end_pos_full_size (
                                    cur_end_pos, AUDIO_ENGINE->frames_per_tick_);
                                }
                            }
                        }

                      file_end_pos_ =
                        std::max (file_end_pos_, mr->get_end_position ());
                    }
                  catch (const ZrythmException &e)
                    {
                      throw ZrythmException ("Failed to add region to track");
                    }
                }
            }
        }
      catch (const ZrythmException &e)
        {
          e.handle ("Failed to setup instrument for sample processor");
          return;
        }
    }

  roll_ = true;
  playhead_.set_to_bar (
    1, transport->ticks_per_bar_, audio_engine_->frames_per_tick_);

  /* add some room to end pos */
  z_debug ("playing until {}", file_end_pos_);
  file_end_pos_.add_bars (
    1, transport->ticks_per_bar_, audio_engine_->frames_per_tick_);

  /*
   * Create a graph so that port buffers are created (FIXME this is hacky,
   * create port buffers elsewhere).
   *
   * WARNING: this must not run while the engine is running because the engine
   * might access the port buffer created here, so we use the port operation
   * lock also used by the engine. The engine will skip its cycle until the port
   * operation lock is released.
   */
  SemaphoreRAII       port_op_raii (AUDIO_ENGINE->port_operation_lock_);
  dsp::graph::Graph   graph;
  ProjectGraphBuilder builder (
    *PROJECT, PROJECT->metronome (), PROJECT->monitor_fader ());
  builder.build_graph (graph);
  tracklist_->get_track_span ().set_caches (ALL_CACHE_TYPES);
#endif
}

void
SampleProcessor::queue_file (const FileDescriptor &file)
{
  queue_file_or_chord_preset (&file, nullptr);
}

void
SampleProcessor::queue_chord_preset (const ChordPreset &chord_pset)
{
  queue_file_or_chord_preset (nullptr, &chord_pset);
}

void
SampleProcessor::stop_file_playback ()
{
#if 0
  roll_ = false;
  auto * transport = audio_engine_->project_->transport_;
  playhead_.set_to_bar (
    1, transport->ticks_per_bar_, audio_engine_->frames_per_tick_);
#endif
}

void
init_from (
  SampleProcessor       &obj,
  const SampleProcessor &other,
  utils::ObjectCloneType clone_type)
{
  // TODO
  // obj.fader_ = utils::clone_unique (*other.fader_);
}

SampleProcessor::~SampleProcessor () = default;
}
