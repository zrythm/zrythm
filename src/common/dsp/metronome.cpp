// SPDX-FileCopyrightText: © 2019-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#include "common/dsp/engine.h"
#include "common/dsp/metronome.h"
#include "common/dsp/position.h"
#include "common/dsp/transport.h"
#include "common/io/audio_file.h"
#include "common/utils/directory_manager.h"
#include "common/utils/gtest_wrapper.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/settings_manager.h"
#include "gui/backend/backend/zrythm.h"

Metronome::Metronome (AudioEngine &engine)
{
  if (ZRYTHM_TESTING || ZRYTHM_BENCHMARKING)
    {
      auto src_root = Glib::getenv ("G_TEST_SRC_ROOT_DIR");
      z_return_if_fail (!src_root.empty ());
      emphasis_path_ = Glib::build_filename (
        src_root, "data", "samples", "klick", "square_emphasis.wav");
      normal_path_ = Glib::build_filename (
        src_root, "data", "samples", "klick", "square_normal.wav");
    }
  else
    {
      const auto path_from_env =
        QProcessEnvironment::systemEnvironment ().value ("ZRYTHM_SAMPLES_PATH");
      if (!path_from_env.isEmpty ())
        {
          const auto fs_path_from_env = fs::path (path_from_env.toStdString ());
          emphasis_path_ = fs_path_from_env / "klick" / "square_emphasis.wav";
          normal_path_ = fs_path_from_env / "klick" / "square_normal.wav";
        }
      else
        {

          auto * dir_mgr = DirectoryManager::getInstance ();
          auto   samplesdir = dir_mgr->get_dir (
            DirectoryManager::DirectoryType::SYSTEM_SAMPLESDIR);
          emphasis_path_ = samplesdir / "square_emphasis.wav";
          normal_path_ = samplesdir / "square_normal.wav";
        }
    }

  emphasis_ = std::make_shared<juce::AudioSampleBuffer> ();
  normal_ = std::make_unique<juce::AudioSampleBuffer> ();

  AudioFile file (emphasis_path_.string ());
  file.read_full (*emphasis_, engine.sample_rate_);
  file = AudioFile (normal_path_.string ());
  file.read_full (*normal_, engine.sample_rate_);

  /* set volume */
  volume_ =
    ZRYTHM_TESTING || ZRYTHM_BENCHMARKING
      ? 1.f
      : (float) zrythm::gui::SettingsManager::get_instance ()
          ->get_metronome_volume ();
}

void
Metronome::queue_events (
  AudioEngine *   engine,
  const nframes_t loffset,
  const nframes_t nframes)
{
  auto *     transport_ = engine->project_->transport_;
  const auto playhead = transport_->playhead_pos_;
  Position   bar_pos, beat_pos;
  Position   playhead_pos = playhead;
  Position   unlooped_playhead = playhead;
  transport_->position_add_frames (&playhead_pos, nframes);
  unlooped_playhead.add_frames ((long) nframes, engine->ticks_per_frame_);
  bool loop_crossed = unlooped_playhead.frames_ != playhead_pos.frames_;
  if (loop_crossed)
    {
      /* find each bar / beat change until loop end */
      engine->sample_processor_->find_and_queue_metronome (
        playhead, transport_->loop_end_pos_, loffset);

      /* find each bar / beat change after loop start */
      engine->sample_processor_->find_and_queue_metronome (
        transport_->loop_start_pos_, playhead_pos,
        loffset
          + (nframes_t) (transport_->loop_end_pos_.frames_ - playhead.frames_));
    }
  else /* loop not crossed */
    {
      /* find each bar / beat change from start to finish */
      engine->sample_processor_->find_and_queue_metronome (
        playhead, playhead_pos, loffset);
    }
}

void
Metronome::set_volume (float volume)
{
  // TODO: validate
  volume_ = volume;

  // FIXME!!!! separate UI logic
  zrythm::gui::SettingsManager::get_instance ()->set_metronome_volume (volume);
}