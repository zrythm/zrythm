// SPDX-FileCopyrightText: © 2019-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#include "dsp/position.h"
#include "engine/device_io/engine.h"
#include "engine/session/metronome.h"
#include "engine/session/transport.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/settings_manager.h"
#include "gui/backend/backend/zrythm.h"
#include "gui/backend/zrythm_application.h"
#include "utils/audio_file.h"
#include "utils/directory_manager.h"
#include "utils/gtest_wrapper.h"

namespace zrythm::engine::session
{
Metronome::Metronome (device_io::AudioEngine &engine)
{
  QProcessEnvironment env = QProcessEnvironment::systemEnvironment ();
  if (ZRYTHM_TESTING || ZRYTHM_BENCHMARKING)
    {
      auto src_root = env.value (u"G_TEST_SRC_ROOT_DIR"_s);
      z_return_if_fail (!src_root.isEmpty ());
      emphasis_path_ =
        utils::Utf8String::from_qstring (src_root).to_path () / "data"
        / "samples" / "klick" / "square_emphasis.wav";
      normal_path_ =
        utils::Utf8String::from_qstring (src_root).to_path () / "data"
        / "samples" / "klick" / "square_normal.wav";
    }
  else
    {
      const auto path_from_env = env.value (u"ZRYTHM_SAMPLES_PATH"_s);
      if (!path_from_env.isEmpty ())
        {
          const auto fs_path_from_env =
            utils::Utf8String::from_qstring (path_from_env).to_path ();
          emphasis_path_ = fs_path_from_env / "klick" / "square_emphasis.wav";
          normal_path_ = fs_path_from_env / "klick" / "square_normal.wav";
        }
      else
        {
          auto &dir_mgr =
            dynamic_cast<gui::ZrythmApplication *> (qApp)
              ->get_directory_manager ();
          auto samplesdir = dir_mgr.get_dir (
            DirectoryManager::DirectoryType::SYSTEM_SAMPLESDIR);
          emphasis_path_ = samplesdir / "square_emphasis.wav";
          normal_path_ = samplesdir / "square_normal.wav";
        }
    }

  emphasis_ = std::make_shared<zrythm::utils::audio::AudioBuffer> ();
  normal_ = std::make_unique<zrythm::utils::audio::AudioBuffer> ();

  utils::audio::AudioFile file (emphasis_path_);
  file.read_full (*emphasis_, engine.get_sample_rate ());
  file = utils::audio::AudioFile (normal_path_);
  file.read_full (*normal_, engine.get_sample_rate ());

  /* set volume */
  volume_ =
    ZRYTHM_TESTING || ZRYTHM_BENCHMARKING
      ? 1.f
      : (float) zrythm::gui::SettingsManager::metronomeVolume ();
}

void
Metronome::queue_events (
  device_io::AudioEngine * engine,
  const nframes_t          loffset,
  const nframes_t          nframes)
{
  auto *     transport_ = engine->project_->transport_;
  const auto playhead_before =
    transport_->get_playhead_position_in_audio_thread ();
  auto       playhead_after_ignoring_loops = playhead_before;
  const auto playhead_after =
    transport_->get_playhead_position_after_adding_frames_in_audio_thread (
      nframes);
  playhead_after_ignoring_loops += (long) nframes;
  bool loop_crossed = playhead_after_ignoring_loops != playhead_after;
  if (loop_crossed)
    {
      /* find each bar / beat change until loop end */
      engine->sample_processor_->find_and_queue_metronome (
        playhead_before, transport_->loop_end_pos_->get_position ().frames_,
        loffset);

      /* find each bar / beat change after loop start */
      engine->sample_processor_->find_and_queue_metronome (
        transport_->loop_start_pos_->get_position ().frames_, playhead_after,
        loffset
          + (nframes_t) (transport_->loop_end_pos_->getFrames () - playhead_before));
    }
  else /* loop not crossed */
    {
      /* find each bar / beat change from start to finish */
      engine->sample_processor_->find_and_queue_metronome (
        playhead_before, playhead_after, loffset);
    }
}

void
Metronome::set_volume (float volume)
{
  // TODO: validate
  volume_ = volume;

  // FIXME!!!! separate UI logic
  zrythm::gui::SettingsManager::get_instance ()->set_metronomeVolume (volume);
}
}
