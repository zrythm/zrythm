// SPDX-FileCopyrightText: © 2019-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "common/dsp/audio_region.h"
#include "common/dsp/engine.h"
#include "common/dsp/metronome.h"
#include "common/dsp/midi_event.h"
#include "common/dsp/port.h"
#include "common/dsp/router.h"
#include "common/dsp/sample_processor.h"
#include "common/dsp/tempo_track.h"
#include "common/dsp/tracklist.h"
#include "common/io/midi_file.h"
#include "common/plugins/plugin.h"
#include "common/plugins/plugin_manager.h"
#include "common/utils/debug.h"
#include "common/utils/dsp.h"
#include "common/utils/gtest_wrapper.h"
#include "common/utils/io.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/settings/chord_preset.h"
#include "gui/backend/backend/settings/g_settings_manager.h"
#include "gui/backend/backend/settings/plugin_settings.h"
#include "gui/backend/backend/settings/settings.h"

#include <glib/gi18n.h>

void
SampleProcessor::load_instrument_if_empty ()
{
  if (!ZRYTHM_TESTING && !ZRYTHM_BENCHMARKING)
    {
      char * _setting_json =
        g_settings_get_string (S_UI_FILE_BROWSER, "instrument");
      std::string setting_json = _setting_json;
      g_free (_setting_json);
      std::unique_ptr<PluginSetting> setting;
      bool                           json_read = false;
      if (!setting_json.empty ())
        {
          try
            {
              setting = std::make_unique<PluginSetting> ();
              setting->deserialize_from_json_string (setting_json.c_str ());
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
            zrythm::plugins::PluginManager::get_active_instance ()
              ->pick_instrument ();
          if (!instrument)
            return;

          auto found_setting = S_PLUGIN_SETTINGS->find (*instrument);
          if (!found_setting)
            {
              setting = std::make_unique<PluginSetting> (*instrument);
            }
        }

      z_return_if_fail (setting);
      instrument_setting_ = std::move (setting);
      instrument_setting_->validate ();
    }
}

void
SampleProcessor::init_common ()
{
  tracklist_ = std::make_unique<Tracklist> (*this);
  midi_events_ = std::make_unique<MidiEvents> ();
  current_samples_.reserve (256);
}

void
SampleProcessor::init_loaded (AudioEngine * engine)
{
  audio_engine_ = engine;
  fader_->init_loaded (nullptr, nullptr, this);

  init_common ();
}

SampleProcessor::SampleProcessor (AudioEngine * engine)
{
  audio_engine_ = engine;

  fader_ = std::make_unique<Fader> (
    Fader::Type::SampleProcessor, false, nullptr, nullptr, this);

  init_common ();
}

void
SampleProcessor::prepare_process (const nframes_t nframes)
{
  fader_->clear_buffers ();
}

void
SampleProcessor::remove_sample_playback (SamplePlayback &in_sp)
{
  auto it = std::find_if (
    current_samples_.begin (), current_samples_.end (),
    [&in_sp] (const SamplePlayback &sp) { return &sp == &in_sp; });

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
SampleProcessor::process (const nframes_t cycle_offset, const nframes_t nframes)
{
  SemaphoreRAII lock (rebuilding_sem_);
  if (!lock.is_acquired ())
    {
      z_warning ("lock not acquired");
      return;
    }

  auto &l = fader_->stereo_out_->get_l ().buf_;
  auto &r = fader_->stereo_out_->get_r ().buf_;

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
            fader_buf_offset + len, <=, AUDIO_ENGINE->block_length_);
          // ensure we don't overflow the sample playback buffer
          z_return_if_fail_cmp (
            sp.offset_ + len, <=, (unsigned_frame_t) sp.buf_->getNumSamples ());
          dsp_mix2 (
            &l[fader_buf_offset], sp.buf_->getReadPointer (0), 1.f, sp.volume_,
            len);
          dsp_mix2 (
            &r[fader_buf_offset],
            sp.buf_->getReadPointer (sp.buf_->getNumChannels () > 1 ? 1 : 0),
            1.f, sp.volume_, len);
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
        auto it = tracklist_->tracks_.rbegin ();
        it != (tracklist_->tracks_.rend () - 1); ++it)
        {
          std::visit([&] (auto && track) {
            using TrackT = base_type<decltype(track)>;
            if constexpr (std::derived_from<TrackT, ProcessableTrack>)
            {

              track->processor_->clear_buffers ();

              EngineProcessTimeInfo time_nfo = {
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
                  track->processor_->process (time_nfo);

                  audio_data_l =
                    track->processor_->stereo_out_->get_l ().buf_.data ();
                  audio_data_r =
                    track->processor_->stereo_out_->get_r ().buf_.data ();
                }
              else if constexpr (std::is_same_v<TrackT, MidiTrack>)
                {
                  track->processor_->process (time_nfo);
                  midi_events_->active_events_.append (
                    track->processor_->midi_out_->midi_events_.active_events_,
                    cycle_offset, nframes);
                }
              else if constexpr (std::is_same_v<TrackT, InstrumentTrack>)
                {
                  auto &ins = track->channel_->instrument_;
                  if (!ins)
                    return;

                  ins->prepare_process ();
                  ins->midi_in_port_->midi_events_.active_events_.append (
                    midi_events_->active_events_, cycle_offset, nframes);
                  ins->process (time_nfo);
                  audio_data_l = ins->l_out_->buf_.data ();
                  audio_data_r = ins->r_out_->buf_.data ();
                }

              if (audio_data_l && audio_data_r)
                {
                  dsp_mix2 (
                    &l[cycle_offset], &audio_data_l[cycle_offset], 1.f,
                    fader_->amp_->control_, nframes);
                  dsp_mix2 (
                    &r[cycle_offset], &audio_data_r[cycle_offset], 1.f,
                    fader_->amp_->control_, nframes);
                }
            }
          }, *it);
       
        }
    }

  playhead_.add_frames (nframes);

  // Stop rolling if no more material
  if (playhead_ > file_end_pos_)
    roll_ = false;
}

void
SampleProcessor::queue_metronome_countin ()
{
  auto bars = ENUM_INT_TO_VALUE (
    PrerollCountBars, g_settings_get_enum (S_TRANSPORT, "metronome-countin"));
  int num_bars = Transport::preroll_count_bars_enum_to_int (bars);
  int beats_per_bar = P_TEMPO_TRACK->get_beats_per_bar ();
  int num_beats = beats_per_bar * num_bars;

  double frames_per_bar =
    AUDIO_ENGINE->frames_per_tick_
    * static_cast<double> (TRANSPORT->ticks_per_bar_);
  for (int i = 0; i < num_bars; i++)
    {
      long offset = static_cast<long> (static_cast<double> (i) * frames_per_bar);
      current_samples_.emplace_back (
        METRONOME->emphasis_, 0.1f * METRONOME->volume_, offset, __FILE__,
        __func__, __LINE__);
    }

  double frames_per_beat =
    AUDIO_ENGINE->frames_per_tick_
    * static_cast<double> (TRANSPORT->ticks_per_beat_);
  for (int i = 0; i < num_beats; i++)
    {
      if (i % beats_per_bar == 0)
        continue;

      long offset =
        static_cast<long> (static_cast<double> (i) * frames_per_beat);
      current_samples_.emplace_back (
        METRONOME->normal_, 0.1f * METRONOME->volume_, offset, __FILE__,
        __func__, __LINE__);
    }
}

void
SampleProcessor::queue_metronome (Metronome::Type type, nframes_t offset)
{
  z_return_if_fail (METRONOME->emphasis_ && METRONOME->normal_);
  z_return_if_fail (offset < AUDIO_ENGINE->block_length_);

  if (type == Metronome::Type::Emphasis)
    {
      current_samples_.emplace_back (
        METRONOME->emphasis_, 0.1f * METRONOME->volume_, offset, __FILE__,
        __func__, __LINE__);
    }
  else if (type == Metronome::Type::Normal)
    {
      current_samples_.emplace_back (
        METRONOME->normal_, 0.1f * METRONOME->volume_, offset, __FILE__,
        __func__, __LINE__);
    }
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
  SemaphoreRAII<std::binary_semaphore> sem_raii (rebuilding_sem_);

  /* clear tracks */
  for (
    auto it = tracklist_->tracks_.rbegin (); it != tracklist_->tracks_.rend ();
    ++it)
    {
      auto &track = *it;
      std::visit (
        [&] (auto &&tr) {
          using TrackT = base_type<decltype (tr)>;
          /* remove state dir if instrument */
          if constexpr (std::is_same_v<TrackT, InstrumentTrack>)
            {
              auto state_dir =
                tr->channel_->instrument_->get_abs_state_dir (false, true);
              if (!state_dir.empty ())
                {
                  io_rmdir (state_dir, true);
                }
            }

          tracklist_->remove_track (*tr, true, true, false, false);
        },
        track);
    }

  Position start_pos;
  start_pos.set_to_bar (*audio_engine_->project_->transport_, 1);
  file_end_pos_.set_to_bar (*audio_engine_->project_->transport_, 1);

  /* create master track */
  {
    z_debug ("creating master track...");
    auto track = *MasterTrack::create_unique (tracklist_->tracks_.size ());
    track->set_name (*tracklist_, "Sample Processor Master", false);
    tracklist_->master_track_ = track.get ();
    tracklist_->insert_track (
      std::move (track), tracklist_->tracks_.size (), false, false);
  }

  if (file && file->is_audio ())
    {
      z_debug ("creating audio track...");
      auto audio_track = *AudioTrack::create_unique (
        "Sample processor audio", tracklist_->tracks_.size (),
        AUDIO_ENGINE->sample_rate_);
      auto audio_track_ptr = tracklist_->insert_track (
        std::move (audio_track), tracklist_->tracks_.size (), false, false);

      /* create an audio region & add to track */
      try
        {
          auto ar = std::make_shared<AudioRegion> (
            -1, file->abs_path_, false, nullptr, 0, std::nullopt, 0,
            BitDepth::BIT_DEPTH_16, start_pos, 0, 0, 0);
          audio_track_ptr->add_region (ar, nullptr, 0, true, false);
          file_end_pos_ = ar->end_pos_;
        }
      catch (const ZrythmException &e)
        {
          e.handle ("Failed to create/add audio region for sample processor");
          return;
        }
    }
  else if (((file && file->is_midi ()) || chord_pset) && instrument_setting_)
    {
      /* create an instrument track */
      z_debug ("creating instrument track...");
      auto instrument_track = *InstrumentTrack::create_unique (
        "Sample processor instrument", tracklist_->tracks_.size ());
      auto * instr_track_ptr = tracklist_->insert_track (
        std::move (instrument_track), tracklist_->tracks_.size (), false, false);
      try
        {
          auto pl = zrythm::plugins::Plugin::create_with_setting (
            *instrument_setting_, instr_track_ptr->get_name_hash (),
            zrythm::plugins::PluginSlotType::Instrument, -1);
          pl->instantiate ();
          pl->activate (true);
          z_return_if_fail (pl->midi_in_port_ && pl->l_out_ && pl->r_out_);

          instr_track_ptr->channel_->add_plugin (
            std::move (pl), zrythm::plugins::PluginSlotType::Instrument,
            pl->id_.slot_, false, false, true, false, false);

          int num_tracks =
            (file != nullptr)
              ? MidiFile (file->abs_path_).get_num_tracks (true)
              : 1;
          z_debug ("creating {} MIDI tracks...", num_tracks);
          for (int i = 0; i < num_tracks; i++)
            {
              auto midi_track = *MidiTrack::create_unique (
                fmt::format ("Sample processor MIDI {}", i),
                tracklist_->tracks_.size ());
              auto * midi_track_ptr = tracklist_->insert_track (
                std::move (midi_track), tracklist_->tracks_.size (), false,
                false);

              /* route track to instrument */
              instr_track_ptr->add_child (
                midi_track_ptr->get_name_hash (), true, false, false);

              if (file)
                {
                  /* create a MIDI region from the MIDI file & add to track */
                  try
                    {
                      auto mr = std::make_shared<MidiRegion> (
                        start_pos, file->abs_path_,
                        midi_track_ptr->get_name_hash (), 0, 0, i);
                      midi_track_ptr->add_region (
                        mr, nullptr, 0, !mr->name_.empty () ? false : true,
                        false);
                      file_end_pos_ = std::max (file_end_pos_, mr->end_pos_);
                    }
                  catch (const ZrythmException &e)
                    {
                      throw ZrythmException (fmt::format (
                        "Failed to create MIDI region from file {}",
                        file->abs_path_));
                    }
                }
              else if (chord_pset)
                {
                  /* create a MIDI region from the chord preset and add to track
                   */
                  Position end_pos;
                  end_pos.from_seconds (13.0);
                  auto mr = std::make_shared<MidiRegion> (
                    start_pos, end_pos, midi_track_ptr->get_name_hash (), 0, 0);

                  /* add notes */
                  for (int j = 0; j < 12; j++)
                    {
                      auto descr = chord_pset->descr_.at (j);
                      descr.update_notes ();
                      if (descr.type_ == ChordType::None)
                        continue;

                      Position cur_pos, cur_end_pos;
                      cur_pos.from_seconds (j * 1.0);
                      cur_end_pos.from_seconds (j * 1.0 + 0.5);
                      for (int k = 0; k < CHORD_DESCRIPTOR_MAX_NOTES; k++)
                        {
                          if (descr.notes_[k])
                            {
                              auto mn = std::make_shared<MidiNote> (
                                mr->id_, cur_pos, cur_end_pos, k + 36,
                                VELOCITY_DEFAULT);
                              mr->append_object (std::move (mn), false);
                            }
                        }
                    }

                  file_end_pos_ = std::max (file_end_pos_, mr->end_pos_);

                  try
                    {
                      midi_track_ptr->add_region (
                        std::move (mr), nullptr, 0,
                        mr->name_.empty () ? true : false, false);
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
  playhead_.set_to_bar (*audio_engine_->project_->transport_, 1);

  /* add some room to end pos */
  z_info ("playing until {}", file_end_pos_.to_string ());
  file_end_pos_.add_bars (1);

  /*
   * Create a graph so that port buffers are created (FIXME this is hacky,
   * create port buffers elsewhere).
   *
   * WARNING: this must not run while the engine is running because the engine
   * might access the port buffer created here, so we use the port operation
   * lock also used by the engine. The engine will skip its cycle until the port
   * operation lock is released.
   */
  SemaphoreRAII<std::counting_semaphore<>> port_op_raii (
    AUDIO_ENGINE->port_operation_lock_);
  graph_ = std::make_unique<Graph> (nullptr, this);
  graph_->setup (true, true);
  graph_.reset ();
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
  roll_ = false;
  playhead_.set_to_bar (*audio_engine_->project_->transport_, 1);
}

void
SampleProcessor::disconnect ()
{
  fader_->disconnect_all ();
}

void
SampleProcessor::init_after_cloning (const SampleProcessor &other)
{
  fader_ = other.fader_->clone_unique ();
}

void
SampleProcessor::find_and_queue_metronome (
  const Position  start_pos,
  const Position  end_pos,
  const nframes_t loffset)
{
  /* special case */
  if (start_pos.frames_ == end_pos.frames_)
    return;

  const auto &audio_engine = *audio_engine_;
  const auto &transport = *audio_engine.project_->transport_;
  const auto &tempo_track = *tracklist_->tempo_track_;

  /* find each bar / beat change from start to finish */
  int num_bars_before = start_pos.get_total_bars (transport, false);
  int num_bars_after = end_pos.get_total_bars (transport, false);
  int bars_diff = num_bars_after - num_bars_before;

#if 0
  char start_pos_str[60];
  char end_pos_str[60];
  position_to_string (start_pos, start_pos_str);
  position_to_string (end_pos, end_pos_str);
  z_info (
    "%s: %s ~ %s <num bars before %d after %d>",
    __func__, start_pos_str, end_pos_str,
    num_bars_before, num_bars_after);
#endif

  /* handle start (not caught below) */
  if (start_pos.frames_ == 0)
    {
      queue_metronome (Metronome::Type::Emphasis, loffset);
    }

  for (int i = 0; i < bars_diff; i++)
    {
      /* get position of bar */
      Position bar_pos;
      bar_pos.add_bars (num_bars_before + i + 1);

      /* offset of bar pos from start pos */
      signed_frame_t bar_offset_long = bar_pos.frames_ - start_pos.frames_;
      if (bar_offset_long < 0)
        {
          z_warning ("bar pos: {} | start pos {}", bar_pos, start_pos);
          z_error ("bar offset long ({}) is < 0", bar_offset_long);
          return;
        }

      /* add local offset */
      signed_frame_t metronome_offset_long =
        bar_offset_long + (signed_frame_t) loffset;
      z_return_if_fail_cmp (metronome_offset_long, >=, 0);
      auto metronome_offset = (nframes_t) metronome_offset_long;
      z_return_if_fail_cmp (metronome_offset, <, audio_engine_->block_length_);
      queue_metronome (Metronome::Type::Emphasis, metronome_offset);
    }

  int num_beats_before =
    start_pos.get_total_beats (transport, tempo_track, false);
  int num_beats_after = end_pos.get_total_beats (transport, tempo_track, false);
  int beats_diff = num_beats_after - num_beats_before;

  for (int i = 0; i < beats_diff; i++)
    {
      /* get position of beat */
      Position beat_pos;
      beat_pos.add_beats (num_beats_before + i + 1);

      /* if not a bar (already handled above) */
      if (beat_pos.get_beats (transport, tempo_track, true) != 1)
        {
          /* adjust position because even though the start and beat pos have the
           * same ticks, their frames differ (the beat position might be before
           * the start position in frames) */
          if (beat_pos.frames_ < start_pos.frames_)
            {
              beat_pos.frames_ = start_pos.frames_;
            }

          /* offset of beat pos from start pos */
          signed_frame_t beat_offset_long = beat_pos.frames_ - start_pos.frames_;
          z_return_if_fail_cmp (beat_offset_long, >=, 0);

          /* add local offset */
          signed_frame_t metronome_offset_long =
            beat_offset_long + (signed_frame_t) loffset;
          z_return_if_fail_cmp (metronome_offset_long, >=, 0);

          nframes_t metronome_offset = (nframes_t) metronome_offset_long;
          z_return_if_fail_cmp (
            metronome_offset, <, AUDIO_ENGINE->block_length_);
          queue_metronome (Metronome::Type::Normal, metronome_offset);
        }
    }
}

bool
SampleProcessor::is_in_active_project () const
{
  return audio_engine_ && audio_engine_->is_in_active_project ();
}

SampleProcessor::~SampleProcessor ()
{
  if (
    PROJECT && AUDIO_ENGINE && SAMPLE_PROCESSOR
    && this == SAMPLE_PROCESSOR.get ())
    disconnect ();
}
