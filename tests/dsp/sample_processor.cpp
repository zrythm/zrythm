// SPDX-FileCopyrightText: © 2021-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#include "tests/helpers/plugin_manager.h"
#include "tests/helpers/project_helper.h"

TEST_F (ZrythmFixture, QueueFile)
{
  /* stop engine to process manually */
  test_project_stop_dummy_engine ();

  size_t processed_frames = 0;
  while (processed_frames < FADER_DEFAULT_FADE_FRAMES)
    {
      AUDIO_ENGINE->process (256);
      processed_frames += 256;
    }

  FileDescriptor file (fs::path (TESTS_SRCDIR) / "test.wav");
  SAMPLE_PROCESSOR->queue_file (file);

  ASSERT_EQ (SAMPLE_PROCESSOR->playhead_.frames_, 0);

  /* queue for a few frames */
  AUDIO_ENGINE->process (256);
  FileAudioSource c (file.abs_path_);
  ASSERT_TRUE (audio_frames_equal (
    &SAMPLE_PROCESSOR->fader_->stereo_out_->get_l ()
       .buf_[AUDIO_REGION_BUILTIN_FADE_FRAMES],
    &c.ch_frames_.getReadPointer (0)[AUDIO_REGION_BUILTIN_FADE_FRAMES],
    256 - AUDIO_REGION_BUILTIN_FADE_FRAMES, 0.0000001f));
  ASSERT_TRUE (audio_frames_equal (
    &SAMPLE_PROCESSOR->fader_->stereo_out_->get_l ()
       .buf_[AUDIO_REGION_BUILTIN_FADE_FRAMES],
    &MONITOR_FADER->stereo_out_->get_l ().buf_[AUDIO_REGION_BUILTIN_FADE_FRAMES],
    256 - AUDIO_REGION_BUILTIN_FADE_FRAMES, 0.0000001f));
  AUDIO_ENGINE->process (256);
  ASSERT_TRUE (audio_frames_equal (
    &SAMPLE_PROCESSOR->fader_->stereo_out_->get_l ().buf_[0],
    &c.ch_frames_.getReadPointer (0)[256], 256, 0.0000001f));
  ASSERT_TRUE (audio_frames_equal (
    &SAMPLE_PROCESSOR->fader_->stereo_out_->get_l ().buf_[0],
    &MONITOR_FADER->stereo_out_->get_l ().buf_[0], 256, 0.0000001f));
  AUDIO_ENGINE->process (256);
  ASSERT_TRUE (audio_frames_equal (
    &SAMPLE_PROCESSOR->fader_->stereo_out_->get_l ().buf_[0],
    &c.ch_frames_.getReadPointer (0)[256 * 2], 256, 0.0000001f));
  ASSERT_TRUE (audio_frames_equal (
    &SAMPLE_PROCESSOR->fader_->stereo_out_->get_l ().buf_[0],
    &MONITOR_FADER->stereo_out_->get_l ().buf_[0], 256, 0.0000001f));
}

TEST_F (ZrythmFixture, QueueMidiAndRollTransport)
{
#ifdef HAVE_HELM
  /* stop dummy audio engine processing so we can
   * process manually */
  test_project_stop_dummy_engine ();

  SAMPLE_PROCESSOR->instrument_setting_ = std::make_unique<PluginConfiguration> (
    test_plugin_manager_get_plugin_setting (HELM_BUNDLE, HELM_URI, false));

  FileDescriptor file (fs::path (TESTS_SRCDIR) / "1_track_with_data.mid");

  TRANSPORT->request_roll (true);

  z_info ("=============== queueing file =============");

  SAMPLE_PROCESSOR->queue_file (file);
  ASSERT_SIZE_EQ (SAMPLE_PROCESSOR->tracklist_->tracks_, 3);

  z_info ("============= starting process ===========");

  /* process a few times */
  AUDIO_ENGINE->process (AUDIO_ENGINE->block_length_);
  AUDIO_ENGINE->process (AUDIO_ENGINE->block_length_);
  AUDIO_ENGINE->process (AUDIO_ENGINE->block_length_);

  z_info ("============= done process ===========");
#endif
}
