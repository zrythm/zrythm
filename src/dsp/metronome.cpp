// SPDX-FileCopyrightText: © 2019-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#include "dsp/engine.h"
#include "dsp/metronome.h"
#include "dsp/position.h"
#include "dsp/transport.h"
#include "gui/cpp/glue/settings_manager.h"
#include "io/audio_file.h"
#include "project.h"
#include "utils/directory_manager.h"
#include "utils/gtest_wrapper.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include "gtk_wrapper.h"

Metronome::Metronome (AudioEngine &engine)
{
  if (ZRYTHM_TESTING || ZRYTHM_BENCHMARKING)
    {
      auto src_root = Glib::getenv ("G_TEST_SRC_ROOT_DIR");
      z_return_if_fail (!src_root.empty ());
      emphasis_path_ = Glib::build_filename (
        src_root, "data", "samples", "klick", "square_emphasis.wav");
      normal_path_ = Glib::build_filename (
        src_root, "data", "samples", "klick", "/square_normal.wav");
    }
  else
    {
      auto * dir_mgr = DirectoryManager::getInstance ();
      auto   samplesdir =
        dir_mgr->get_dir (DirectoryManager::DirectoryType::SYSTEM_SAMPLESDIR);
      emphasis_path_ = samplesdir / "square_emphasis.wav";
      normal_path_ = samplesdir / "square_normal.wav";
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
      : (float) SettingsManager::get_instance ()->get_metronome_volume ();
}

void
Metronome::queue_events (
  AudioEngine *   self,
  const nframes_t loffset,
  const nframes_t nframes)
{
  const auto playhead = self->transport_->playhead_pos_;
  Position   bar_pos, beat_pos;
  Position   playhead_pos = playhead;
  Position   unlooped_playhead = playhead;
  self->transport_->position_add_frames (&playhead_pos, nframes);
  unlooped_playhead.add_frames ((long) nframes, self->ticks_per_frame_);
  bool loop_crossed = unlooped_playhead.frames_ != playhead_pos.frames_;
  if (loop_crossed)
    {
      /* find each bar / beat change until loop end */
      self->sample_processor_->find_and_queue_metronome (
        playhead, self->transport_->loop_end_pos_, loffset);

      /* find each bar / beat change after loop start */
      self->sample_processor_->find_and_queue_metronome (
        self->transport_->loop_start_pos_, playhead_pos,
        loffset
          + (nframes_t) (self->transport_->loop_end_pos_.frames_
                         - playhead.frames_));
    }
  else /* loop not crossed */
    {
      /* find each bar / beat change from start to finish */
      self->sample_processor_->find_and_queue_metronome (
        playhead, playhead_pos, loffset);
    }
}

void
Metronome::set_volume (float volume)
{
  // TODO: validate
  volume_ = volume;

  SettingsManager::get_instance ()->set_metronome_volume (volume);
}